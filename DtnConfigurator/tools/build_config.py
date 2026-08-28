"""Turn a profile (JSON port map, or vendor XML) into the DTN configuration frames.

  python3 build_config.py ../profiles/config1.json
  python3 build_config.py round1.xml
"""
import argparse, json, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dtn_config import *
import vl_xml

# Fixed blocks copied verbatim from the known-good reference configuration.
# ES_GLOBAL bytes [2:4] are the VL count and are patched per profile.
REF_ES_GLOBAL = bytes.fromhex("0001118805ee0409c700")
REF_ES_PARAMS = bytes.fromhex("000000000a00000000000001110000000000000000023100000000000000")
REF_SW_BEGIN  = bytes.fromhex("000000")
REF_SW_END    = bytes.fromhex("000000")
REF_SW_MISC   = bytes.fromhex("000000000030")


def records_from_json(spec):
    """Expand a port map into VL records.

    Each link group walks its own contiguous VL range: the first link takes
    `vl_block_size` IDs from `vl_id_base`, the next link continues from there.
    """
    default_size = spec.get("vl_block_size", 10)
    records = []
    for group in spec["link_groups"]:
        vl = group["vl_id_base"]
        size = group.get("vl_block_size", default_size)
        for src, dst in group["links"]:
            for _ in range(size):
                records.append(VlRecord(vl, src, {dst}))
                vl += 1
    for hm in spec.get("health_monitor", []):
        records.append(VlRecord(hm["vl"], hm["src"], {hm["dst"]}))
    records.sort(key=lambda r: r.vl_id)
    return records


def check(records):
    seen = {}
    for r in records:
        if r.vl_id in seen:
            raise SystemExit(f"duplicate VL ID {r.vl_id}")
        if r.vl_id < 3:
            raise SystemExit(f"VL {r.vl_id} is reserved (management VLs are 0-2)")
        if not r.dest:
            raise SystemExit(f"VL {r.vl_id} has no destination port")
        seen[r.vl_id] = r


def build_frames(records, vlan=None, with_ptp_block=None):
    check(records)
    es_global = REF_ES_GLOBAL[:2] + struct.pack(">H", len(records)) + REF_ES_GLOBAL[4:]

    frames, seq = [], 0
    def emit(blocks, terminate, label):
        nonlocal seq
        frames.append((seq, label, build_frame(build_payload(blocks, terminate=terminate),
                                               seq=seq, vlan=vlan)))
        seq = next_seq(seq) if seq else 1

    emit([(ADDR_ES_GLOBAL, es_global), (ADDR_ES_PARAMS, REF_ES_PARAMS)], True, "end system")
    if with_ptp_block is not None:
        emit([(ADDR_PTP, with_ptp_block)], True, "ptp")

    chunks = list(chunk_vl_records(records))
    for i, chunk in enumerate(chunks):
        data = b"".join(r.encode() for r in chunk)
        blocks = []
        if i == 0:
            blocks.append((ADDR_SW_BEGIN, REF_SW_BEGIN))
        blocks.append((ADDR_VL_TABLE, data))
        last = i == len(chunks) - 1
        if last:
            blocks += [(ADDR_PORT_TABLE, encode_port_table()),
                       (ADDR_SW_MISC, REF_SW_MISC),
                       (ADDR_SW_END, REF_SW_END)]
        emit(blocks, last, f"vl table {i + 1}/{len(chunks)} ({len(chunk)} records)")

    # 0x52 status query, byte-identical to the reference in
    # RemoteConfigSender/main.cpp and HealthMonitor.c
    query = bytes.fromhex("26005200000000440000000000000000000000000000")
    frames.append((seq, "status query (0x52)", build_frame(query, seq=seq, vlan=None)))
    return frames


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--vlan", type=int, default=None,
                    help="802.1Q tag (only needed when going through the Cumulus bridge)")
    ap.add_argument("--out", help="directory to write raw frames into")
    ap.add_argument("--list", action="store_true", help="print every VL record")
    args = ap.parse_args()

    if args.profile.lower().endswith(".xml"):
        records = vl_xml.load(args.profile)
        spec = {"name": os.path.splitext(os.path.basename(args.profile))[0]}
    else:
        spec = json.load(open(args.profile))
        records = records_from_json(spec)

    frames = build_frames(records, vlan=args.vlan)

    print(f"profile          : {spec.get('name')}")
    if spec.get("description"):
        print(f"                   {spec['description']}")
    print(f"VL records       : {len(records)}  (VL {records[0].vl_id}..{records[-1].vl_id})")
    print(f"tagging          : {'VLAN ' + str(args.vlan) if args.vlan else 'untagged (copper)'}")
    print(f"frames to send   : {len(frames)}  ({sum(len(f) for _,_,f in frames)} bytes total)\n")

    print("  seq  bytes  contents")
    for seq, label, frame in frames:
        print(f"  {seq:3d}  {len(frame):5d}  {label}")

    ports = sorted({r.src_port for r in records} | {p for r in records for p in r.dest})
    print(f"\nDTN ports used   : {ports}")

    if args.list:
        print()
        for r in records:
            print("  " + repr(r))

    if args.out:
        os.makedirs(args.out, exist_ok=True)
        for seq, label, frame in frames:
            name = os.path.join(args.out, f"{spec.get('name')}_{seq:03d}.bin")
            open(name, "wb").write(frame)
        print(f"\nframes written to {args.out}/")


if __name__ == "__main__":
    main()
