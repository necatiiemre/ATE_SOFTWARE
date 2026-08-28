"""Regression test: re-encode the legacy 46-frame DTN configuration from the
decoded block structure and compare it byte-for-byte with the reference blobs
in RemoteConfigSender/main.cpp.

If this passes, the encoder reproduces a configuration the hardware is known to
accept - which is the only evidence we can gather without the DUT.
"""
import re, os, sys, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dtn_config import *

REF = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "..", "RemoteConfigSender", "main.cpp")


def load_arrays():
    src = open(REF).read()
    out = {}
    for m in re.finditer(r'^unsigned char (\w+)\[\]\s*=\s*\{(.*?)\};', src, re.S | re.M):
        out[m.group(1)] = bytes(int(x, 0) for x in re.findall(r'0x[0-9a-fA-F]+', m.group(2)))
    return out


def split_payload(frame):
    off = 18 if frame[12:14] == b"\x81\x00" else 14
    udp_len = int.from_bytes(frame[off + 24:off + 26], "big")
    return frame[off + 28:off + 28 + udp_len - 8], frame[-1]


def parse_blocks(payload):
    """Inverse of build_payload. Returns (blocks, terminated)."""
    blocks, i, first = [], 3, True
    while i < len(payload):
        if not first:
            expected = block_marker(blocks[-1][0])
            if payload[i] != expected:
                raise AssertionError(f"marker 0x{payload[i]:02x} != expected 0x{expected:02x}")
            i += 1
            if i >= len(payload):          # marker was the terminator
                return blocks, True
        addr = payload[i]
        ln = int.from_bytes(payload[i + 1:i + 3], "big")
        blocks.append((addr, payload[i + 3:i + 3 + ln]))
        i += 3 + ln
        first = False
    return blocks, False


def main():
    arrays = load_arrays()
    names = ["end_system_conf", "ptp_conf_0"] + [f"switch_conf_{i}" for i in range(44)]

    total_records = 0
    ok = True
    for seq, name in enumerate(names):
        ref = arrays[name]
        payload, ref_seq = split_payload(ref)
        blocks, terminated = parse_blocks(payload)
        for addr, data in blocks:
            if addr == ADDR_VL_TABLE:
                total_records += len(data) // VL_RECORD_LEN

        rebuilt = build_frame(build_payload(blocks, terminate=terminated),
                              seq=ref_seq, vlan=97)
        status = "OK " if rebuilt == ref else "FAIL"
        if rebuilt != ref:
            ok = False
            for k, (a, b) in enumerate(zip(rebuilt, ref)):
                if a != b:
                    print(f"    first diff at byte {k}: got 0x{a:02x} want 0x{b:02x}")
                    break
            print(f"    len rebuilt={len(rebuilt)} ref={len(ref)}")
        if name in ("end_system_conf", "ptp_conf_0", "switch_conf_0", "switch_conf_43"):
            desc = " + ".join(f"0x{a:02x}({len(d)}B)" for a, d in blocks)
            print(f"[{status}] seq={ref_seq:3d} {name:<16} {len(ref):5d}B  "
                  f"{desc}{'  +TERM' if terminated else ''}")
        assert ref_seq == seq, f"{name}: sequence {ref_seq} != expected {seq}"

    print(f"[ OK ] 44 switch_conf frames all reproduced; VL records = {total_records}")

    # And the trailing 0x52 status query
    ref = arrays["packet"]
    payload, ref_seq = split_payload(ref)
    rebuilt = build_frame(payload, seq=ref_seq, vlan=None)
    print(f"[{'OK ' if rebuilt == ref else 'FAIL'}] seq={ref_seq:3d} status query   "
          f"{len(ref):5d}B  op=0x{payload[2]:02x} (untagged, copper)")
    ok = ok and rebuilt == ref

    print("\n" + ("ALL 47 REFERENCE FRAMES REPRODUCED BYTE-FOR-BYTE"
                  if ok else "MISMATCH - encoder is wrong"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
