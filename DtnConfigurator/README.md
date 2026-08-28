# DtnConfigurator

Builds DTN (LRU `0x2600`) configuration frames from a VL profile and sends them
over a plain Ethernet raw socket, then listens for the health-monitor stream
coming back. No DPDK, no fibre, no VLAN — the workstation only needs the two
copper links to the DTN's end-system ports.

```
include/DtnConfig.h        wire format: VL records, config blocks, frame assembly
src/DtnConfig.c            the encoder
tests/test_reference.c     rebuilds the 47 reference frames byte for byte
tests/fixtures/            those frames, extracted from RemoteConfigSender
profiles/config1.json      round 1 of 3
tools/                     analysis-side helpers (see below)
```

## Build and test

```
$ make            # build/libdtnconfig.a
$ make test
47 reference frames, 4488 VL records
PASS: every reference frame reproduced byte for byte
```

`RemoteConfigSender/main.cpp` ships a 47-frame configuration as hard-coded hex
that real hardware accepts. The wire format is not documented anywhere — it was
recovered by reading that blob together with `HealthMonitor.c` and
`fpga_firmware_loader.py` — so reproducing those frames exactly is the only
correctness evidence available without the DUT. The test covers the block
chaining and its marker byte, the IP checksum and total length, the trailing
AFDX sequence byte, the 14-byte VL record layout and the VLAN tag position.

It says nothing about whether a *new* profile is correct; only the hardware can
answer that. What it guarantees is that the encoder still emits valid frames.

## Wire format

Every frame is `ETH + IPv4 + UDP(100->100) + AFDX payload + 1 sequence byte`,
where the payload is `LRU_ID(2) OpType(1)` followed by one or more
`Addr(1) Len(2) Data(Len)` blocks separated by a one-byte marker. `OpType` is
`0x57` to write configuration and `0x52` to read device and port status. The
sequence byte sits deliberately outside the IP total length, so the frame on the
wire is one byte longer than IP declares.

The VL table lives at address `0x72`, 14 bytes per VL, mapping one-to-one onto
the vendor XML attributes:

| offset | size | XML attribute |
|--------|------|---------------|
| 0  | 2 | `ID` |
| 2  | 2 | `BAG` / `PRIORITY` word |
| 4  | 1 | `JITTER`, milliseconds |
| 5  | 1 | `LMIN` |
| 6  | 2 | flag nibble \| `LMAX` (12 bit) |
| 8  | 1 | `DESTPORT` bits 34..32 |
| 9  | 1 | `SRCPORT` |
| 10 | 4 | `DESTPORT` bits 31..0 |

`DESTPORT` is a 35-character bit string with port 34 leftmost. Confirmed against
the reference blob: the vendor line for VL 620 encodes to that record exactly.

## Health-monitor VLs

`profiles/config1.json` gives the two HM VLs flag nibble `0xD` rather than the
`0x9` every other record uses. That matches VL 4488 in the main software — the
VL that carries health-monitor data, per `HEALTH_MONITOR_RESPONSE_VL_IDX`. It is
one constant per profile if that turns out to be wrong.

## Not yet pinned down

* `BAG`, `PRIORITY` and `FEEDBACKVL` share the `0x0602` word and the flag nibble.
  Every reference record uses `BAG=1MS PRIORITY=LOW FEEDBACKVL=FALSE`, so their
  encodings cannot be derived from it. The XML reader raises rather than guessing
  when a profile deviates.
* The end-system VL-count field is written as the number of records. The
  reference VL IDs are contiguous, so "count" and "span" are indistinguishable
  there — and our profiles are sparse.
* Whether the device accepts `0x57` writes on a copper end-system port. Reads are
  proven: the health monitor polls over `eno12409`. Writes have only ever gone
  over the tagged fibre path. The device's `eth_wrong_op_cnt`,
  `eth_wrong_type_cnt` and `config_id` fields answer this in one round trip.

## tools/

Python helpers from the reverse-engineering work, kept until the C application
covers the same ground:

* `build_config.py` — expands a JSON profile into frames; a second implementation
  to diff the C encoder against. That cross-check has already caught a real bug.
* `vl_xml.py` — vendor `<VL .../>` XML to VL records.
* `dump_reference_fixture.py` — regenerates `tests/fixtures/reference_frames.bin`.
