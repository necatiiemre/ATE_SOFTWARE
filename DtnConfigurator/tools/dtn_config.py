"""
DTN (LRU 0x2600) configuration encoder.

Wire format, reverse-engineered from and cross-checked against three independent
implementations in this repository:

  * RemoteConfigSender/main.cpp        - the reference config blobs
  * dpdk/src/HealthMonitor/HealthMonitor.c - the 0x52 status query
  * FirmwareUpdater/fpga_firmware_loader.py - LRU_ID / OpType / trailing seq byte

  Ethernet   dst 03:00:00:00:<VL hi>:<VL lo>   src 02:00:00:00:00:<net>
             ethertype 0x0800, or 0x8100 + TCI + 0x0800 when tagged
  IPv4       src 10.1.33.1  dst 224.224.<VL hi>.<VL lo>  TTL 1  proto UDP
  UDP        100 -> 100, checksum disabled
  Payload    LRU_ID(2) OpType(1) | Addr(1) Len(2) Data(Len) | M | Addr Len Data | ...
  Trailer    1-byte AFDX sequence, OUTSIDE the IP total_length

`M` is a one-byte end-of-block marker; see `block_marker`.
"""

import struct

LRU_DTN      = 0x2600
OP_WRITE     = 0x57
OP_READ      = 0x52

NET_A        = 0x20
NET_B        = 0x40

SRC_IP       = bytes([10, 1, 33, 1])
UDP_PORT     = 100
MAX_FRAME    = 1518

# Config-space start addresses (the "CfgType" byte)
ADDR_ES_GLOBAL   = 0x10   # end-system globals (VL count, Lmax)
ADDR_ES_PARAMS   = 0x17   # end-system parameters
ADDR_PTP         = 0x46   # PTP session table
ADDR_SW_BEGIN    = 0x70   # switch table: begin
ADDR_SW_END      = 0x71   # switch table: end
ADDR_VL_TABLE    = 0x72   # switch table: VL records (14 bytes each)
ADDR_PORT_TABLE  = 0x73   # per-port table (4 bytes each)
ADDR_SW_MISC     = 0x74   # switch misc

VL_RECORD_LEN    = 14
DTN_PORT_COUNT   = 35     # ports 0..34 (34 = CPU)

# Largest record count that keeps an untagged frame within 1518 bytes:
#   14 eth + 20 ip + 8 udp + 3 hdr + 3 block + 14*N + 1 seq <= 1518
MAX_RECORDS_PER_BLOCK = 104


def block_marker(addr: int) -> int:
    """One-byte end-of-block marker that follows a block's data.

    Derived empirically; reproduces every marker in the reference blobs:
      0x70..0x7F  ->  (addr & 0x0F) | 0x80      (0x70->0x80, 0x72->0x82, ...)
      otherwise   ->  addr + 2                  (0x10->0x12, 0x17->0x19, 0x46->0x48)
    """
    if 0x70 <= addr <= 0x7F:
        return (addr & 0x0F) | 0x80
    return (addr + 2) & 0xFF


# ---------------------------------------------------------------------------
# VL record
# ---------------------------------------------------------------------------

class VlRecord:
    """One row of the switch VL table - the binary form of an XML <VL .../> line.

      off  size  XML attribute
      [0]   2    ID
      [2]   2    BAG / PRIORITY word          (0x0602 for BAG=1MS PRIORITY=LOW)
      [4]   1    JITTER, milliseconds
      [5]   1    LMIN
      [6]   2    flag nibble | LMAX (12 bit)  (0x9 = ENABLE, LOW priority)
      [8]   1    DESTPORT bits 34..32
      [9]   1    SRCPORT
      [10]  4    DESTPORT bits 31..0
    """

    def __init__(self, vl_id, src_port, dest_ports,
                 lmax=1518, lmin=64, jitter_ms=0,
                 bag_word=0x0602, flags=0x9):
        self.vl_id     = vl_id
        self.src_port  = src_port
        self.dest      = frozenset(dest_ports)
        self.lmax      = lmax
        self.lmin      = lmin
        self.jitter_ms = jitter_ms
        self.bag_word  = bag_word
        self.flags     = flags

    def dest_mask(self) -> int:
        m = 0
        for p in self.dest:
            if not 0 <= p < DTN_PORT_COUNT:
                raise ValueError(f"VL {self.vl_id}: destination port {p} out of range 0..34")
            m |= 1 << p
        return m

    def encode(self) -> bytes:
        if not 0 <= self.src_port < DTN_PORT_COUNT:
            raise ValueError(f"VL {self.vl_id}: source port {self.src_port} out of range 0..34")
        if not 0 < self.lmax <= 0xFFF:
            raise ValueError(f"VL {self.vl_id}: LMAX {self.lmax} does not fit in 12 bits")
        mask = self.dest_mask()
        return struct.pack(
            ">HHBBHBBI",
            self.vl_id,
            self.bag_word,
            self.jitter_ms,
            self.lmin,
            ((self.flags & 0xF) << 12) | (self.lmax & 0xFFF),
            (mask >> 32) & 0xFF,
            self.src_port,
            mask & 0xFFFFFFFF,
        )

    @staticmethod
    def decode(raw: bytes) -> "VlRecord":
        (vl, bag, jit, lmin, fl_lmax, dhi, src, dlo) = struct.unpack(">HHBBHBBI", raw)
        mask = (dhi << 32) | dlo
        return VlRecord(vl, src, {p for p in range(DTN_PORT_COUNT) if mask >> p & 1},
                        lmax=fl_lmax & 0xFFF, lmin=lmin, jitter_ms=jit,
                        bag_word=bag, flags=fl_lmax >> 12)

    def __repr__(self):
        d = sorted(self.dest)
        return (f"VL {self.vl_id:5d}  src={self.src_port:2d} -> dst={d}  "
                f"LMIN={self.lmin} LMAX={self.lmax} JITTER={self.jitter_ms}MS "
                f"bag=0x{self.bag_word:04x} flags=0x{self.flags:x}")


def encode_port_table(value=0x0112, port_count=34) -> bytes:
    """Per-port table (address 0x73): port_id(2 BE) + 2-byte value, one row per port."""
    return b"".join(struct.pack(">HH", p, value) for p in range(port_count))


# ---------------------------------------------------------------------------
# Datagram / frame assembly
# ---------------------------------------------------------------------------

def build_payload(blocks, op=OP_WRITE, lru=LRU_DTN, terminate=False) -> bytes:
    """blocks: list of (addr, data). A marker separates consecutive blocks;
    `terminate` appends the final block's marker (last datagram of a group)."""
    out = bytearray(struct.pack(">HB", lru, op))
    for i, (addr, data) in enumerate(blocks):
        if i:
            out.append(block_marker(blocks[i - 1][0]))
        out += struct.pack(">BH", addr, len(data)) + data
    if terminate:
        out.append(block_marker(blocks[-1][0]))
    return bytes(out)


def ip_checksum(header: bytes) -> int:
    total = 0
    for i in range(0, len(header), 2):
        total += (header[i] << 8) | header[i + 1]
    while total >> 16:
        total = (total & 0xFFFF) + (total >> 16)
    return ~total & 0xFFFF


def build_frame(payload: bytes, seq: int, vl_id=0, vlan=None, net=NET_A) -> bytes:
    vl = struct.pack(">H", vl_id)
    eth = b"\x03\x00\x00\x00" + vl + b"\x02\x00\x00\x00\x00" + bytes([net])
    eth += (b"\x81\x00" + struct.pack(">H", vlan & 0xFFF) if vlan is not None else b"") + b"\x08\x00"

    udp = struct.pack(">HHHH", UDP_PORT, UDP_PORT, 8 + len(payload), 0)
    ip_len = 20 + len(udp) + len(payload)
    ip = (b"\x45\x00" + struct.pack(">H", ip_len) + b"\xd4\x3b\x00\x00\x01\x11"
          + b"\x00\x00" + SRC_IP + b"\xe0\xe0" + vl)
    ip = ip[:10] + struct.pack(">H", ip_checksum(ip)) + ip[12:]

    frame = eth + ip + udp + payload + bytes([seq & 0xFF])
    if len(frame) > MAX_FRAME:
        raise ValueError(f"frame is {len(frame)} bytes, over the {MAX_FRAME} limit")
    return frame


def next_seq(seq: int) -> int:
    """The device's 1-byte AFDX counter: 0 is only ever the first value, then 1..255."""
    return 1 if seq >= 255 else seq + 1


def chunk_vl_records(records, per_block=MAX_RECORDS_PER_BLOCK):
    for i in range(0, len(records), per_block):
        yield records[i:i + per_block]
