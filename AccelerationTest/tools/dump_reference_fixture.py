"""One-shot: extract the 47 reference frames from RemoteConfigSender/main.cpp
into tests/fixtures/reference_frames.bin.

That configuration is known to be accepted by the DTN, so it is the only
correctness oracle available without the hardware. tests/test_reference.c
rebuilds every frame from its decoded block structure and compares bytes.

Container format: for each frame, a 2-byte big-endian length followed by the
frame itself. Kept in the repo so the test needs no Python at build time.
"""
import os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC  = os.path.join(HERE, "..", "..", "RemoteConfigSender", "main.cpp")
OUT  = os.path.join(HERE, "..", "tests", "fixtures", "reference_frames.bin")

NAMES = ["end_system_conf", "ptp_conf_0"] + [f"switch_conf_{i}" for i in range(44)] + ["packet"]


def main():
    text = open(SRC).read()
    arrays = {
        m.group(1): bytes(int(x, 0) for x in re.findall(r"0x[0-9a-fA-F]+", m.group(2)))
        for m in re.finditer(r"^unsigned char (\w+)\[\]\s*=\s*\{(.*?)\};", text, re.S | re.M)
    }

    blob = bytearray()
    for name in NAMES:
        frame = arrays[name]
        blob += struct.pack(">H", len(frame)) + frame

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, "wb").write(blob)
    print(f"{len(NAMES)} frames, {len(blob)} bytes -> {os.path.relpath(OUT, HERE)}")


if __name__ == "__main__":
    sys.exit(main())
