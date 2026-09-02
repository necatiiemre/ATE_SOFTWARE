# AccelerationTest

Acceleration test rig for the ATE units. The operator picks a unit and the
application runs that unit's acceleration test. Only the DTN test exists today;
VMC and CMC are registered so their wiring is already in place.

The DTN test configures the switch's VL routing table and reads its health
monitor back. No DPDK, no fibre, no VLAN — the workstation only needs the two
copper links to the DTN's end-system ports.

```
src/main.c                 unit menu
src/UnitManager.c          the unit registry
src/units/DtnTest.c        DTN acceleration test
src/units/VmcTest.c        placeholder
src/units/CmcTest.c        placeholder
src/VlProfile.c            the three DTN configuration rounds
src/AppConfig.c            copper links, DTN port to interface map, timings
src/VlWatch.c              what arrived on copper, by VL
src/RawSocket.c            AF_PACKET access to a copper link
src/HealthMonitor.c        recognising the DTN's health-monitor stream
src/SafeShutdown.c         releases sockets on Ctrl-C or any error path
src/Log.c                  timestamped run log, flushed line by line
include/DtnConfig.h        wire format: VL records, config blocks, frame assembly
src/DtnConfig.c            the encoder
tests/test_reference.c     rebuilds the 47 reference frames byte for byte
tests/fixtures/            those frames, extracted from RemoteConfigSender
tools/                     analysis-side helpers (see below)
```

## What the DTN test does

1. The operator picks one of the three rounds.
2. The profile is expanded into a VL table and validated.
3. The VL table becomes configuration frames.
4. The test waits for the unit to start talking on either copper link, which is
   how it knows the DTN has booted. The power-up broadcast is not routed by the
   VL table and the one place it has been observed is the 1G link, so both are
   watched rather than assumed.
5. The frames go out of the configuration link, followed by a `0x52` status query.
6. It watches both copper links until the operator presses Ctrl-C.

## The live table

Until the health-monitor payloads are decoded, the run answers a simpler and
more useful question: did the configuration take? Every VL the profile routes to
copper is seeded into a table at zero packets, so one that never arrives shows up
as a row rather than as an absence:

```
unit ALIVE   expected VLs seen 3/7   power interruptions 1

  link       DTN  VL-ID   packets      bytes   last   sizes           status
  ---------- ---  -----  --------  ---------  -----   --------------  ------
  eno12409    33    100       284     322340   0.0s   1187,1083       ok
  eno12409    33    101         0          0      -   -               MISSING
  eno12399    32   4485        30      35610   0.0s   1187            ok
  eno12409    33      0         6        564   0.0s   94              extra
```

`MISSING` means the profile routes that VL to that copper port but nothing has
arrived — either it is not being generated or it is not being routed. `extra` is
a VL nobody asked for. Packet contents are ignored.

Power is operated separately; the test only observes a unit that is already
live. What it does watch for is power *dropping* mid-run - on a vibration rig
that is a likely fault and probably the most valuable thing a run can catch.
When the health monitor goes quiet and comes back, the DTN has rebooted and lost
its VL table, so the test re-sends the configuration and records both the loss
and the recovery with timestamps.

Everything lands in `LOGS/DTN/<profile>_<timestamp>.log`, flushed line by line
so a run that ends abruptly still leaves what it saw.

Raw sockets need root or `CAP_NET_RAW`.

## Adding a unit's test

Write `src/units/<Unit>Test.c` exposing a `unit_result_t <unit>_test_run(void)`
and flip its `implemented` flag in the table at the top of `src/UnitManager.c`.
Nothing else in the application needs to change.

## Build and test

```
$ make            # build/acceleration_test
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

## The three rounds

The unit under test on the other side has 12 ports, the DTN has 32 fibre ports,
so the fibre links are covered in three rounds. Each round pairs six low ports
with six high ports in both directions, 10 VLs per direction, plus two
health-monitor VLs out to copper port 33.

| profile | fibre ports | health monitor |
|---------|-------------|----------------|
| config1 | 0-5 <-> 16-21  | ports 15, 31 |
| config2 | 6-11 <-> 22-27 | ports 15, 31 |
| config3 | 10-15 <-> 26-31 | ports 0, 16 |

Round 3 moves the health monitor because ports 15 and 31 carry fibre traffic in
that round. The three rounds together cover fibre ports 0-31. Every round is
122 VL records in 4 frames.

## Two health monitors

They are different things and both reach the workstation over copper:

* **The fibre-side unit's health monitor.** It arrives on a DTN fibre port and
  each profile routes it to copper port 33 — VL 100 and 101 in the rounds above.
  These carry flag nibble `0xD` rather than the `0x9` every other record uses,
  matching VL 4488 in the main ATE software.
* **The DTN's own health monitor.** It comes from the DTN's internal management
  port 34. Port 34 is not physical: it is absent from the device's port table,
  yet it is the source of the PTP Sync broadcast and of the reply to a `0x52`
  status query, and the health data reports it as the last of 35 ports.

Every profile therefore also carries VL 4484-4490, copied byte for byte from the
reference configuration, wiring port 34 to both copper ports in both directions.
The 28 V power-up broadcast reaches copper with no configuration at all, but a
query and its reply plausibly do not — the reference would not define those seven
VLs otherwise. `tests/test_profiles.c` checks the copy stays faithful.

## The VL table is written contiguously

The reference configuration's VL ids run from 3 to 4490 with no gap, every
record enabled, and the vendor XML carries an `ENABLE` attribute. Taken together
that reads like a table the device indexes rather than searches — so a sparse
table would leave every VL above the record count unreachable, which is exactly
what a first run on the hardware looked like: the DTN's own health monitor
stopped after configuration and almost nothing came back over fibre.

Each round is therefore expanded across VL 3 to the highest id it uses, with a
disabled record for every id it does not:

```
VL    3  disabled  00 03 06 02 00 40 15 ee 00 00 00 00 00 00
VL  100  ENABLE    00 64 06 02 00 40 d5 ee 02 0f 00 00 00 00
VL 1024  ENABLE    04 00 06 02 00 40 95 ee 00 00 00 01 00 00
```

A disabled record clears the `ENABLE` bit of the flag nibble and names no source
or destination; the enabled ones are byte for byte what they were.

That makes a round 4488 records in 46 frames, about 65 KB and 190 ms — the same
shape as the reference, which is 4488 records in 47. `--sparse-table` sends only
the profile's own 129 records in 4 frames, for comparing the two on the bench.

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
  to diff the C encoder against. That cross-check has already caught a real bug,
  and confirms all three rounds still encode identically after refactoring.
* `vl_xml.py` — vendor `<VL .../>` XML to VL records.
* `dump_reference_fixture.py` — regenerates `tests/fixtures/reference_frames.bin`.
