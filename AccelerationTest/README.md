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

Every profile therefore also carries **VL 4419-4490** and the block written at
address `0x46`, both copied byte for byte from the reference configuration.

That block looks like a PTP table but its body *enumerates* VL 4420-4487. It is
sent as its own datagram before the switch table: the capture numbers its first
switch datagram seq 2, which is only possible if the end-system datagram and
this one precede it.

The 72 records are a broadcast from port 34 to all 32 fibre ports (VL 4419), a
pair per fibre port (4420-4483), and both copper ports wired to port 34 in both
directions (4484-4490). VL 4488 is the one status replies arrive on.
`tests/test_profiles.c` checks the copy stays faithful.

## The VL table is sparse, and not sorted

The captured configuration for round 1 settles this. It is two datagrams, seq 2
and seq 3, carrying 122 records: the 120 fibre VLs and the two health-monitor
taps, in link order with the taps last.

```
VL 1024  ENABLE  04 00 06 02 00 40 95 ee 00 00 00 01 00 00   port  0 -> 16
...
VL 2083  ENABLE  08 23 06 02 00 40 95 ee 00 15 00 00 00 20   port 21 ->  5
VL  100  ENABLE  00 64 06 02 00 40 95 ee 02 0f 00 00 00 00   port 15 -> 33
VL  101  ENABLE  00 65 06 02 00 40 95 ee 02 1f 00 00 00 00   port 31 -> 33
```

VL 100 and 101 come *after* VL 2083, so the ids are not in order — which means
the device reads the id out of each record rather than indexing its table by
position. A contiguous table with the unused ids disabled, which an earlier
reading of the reference blob suggested, is not needed and is not what the
hardware is given.

Two other things the capture settles:

* every record carries flag nibble `0x9`, the health-monitor taps included. The
  `0xD` in the reference blob belongs to VL 4488 specifically, not to taps.
* the closing datagram goes `0x72` → `0x74` → `0x71`. There is no `0x73` port
  table.

`tests/test_config1.c` rebuilds both datagrams and compares them with
`tests/fixtures/config1_switch.bin` byte for byte. Record layout, record order,
flag nibble, block chain, markers, the 104-record split and the sequence
numbering all have to be right at once for it to pass.

## The end-system block, and where the DTN's own health monitor goes

The configuration is three datagrams: the end-system blocks at seq 0, the switch
table at seq 1 and seq 2. RemoteConfigSender sends a fourth, block `0x46`, whose
body enumerates VL 4420-4487 — VLs this table does not contain, so it is left
out.

The end-system block `0x10` differs from RemoteConfigSender's in three bytes:

| data byte | RemoteConfigSender | ours |
|---|---|---|
| 0-1 | `00 01` | `00 01` |
| **2-3** | **`11 88`** (4488) | **`00 26`** (38) |
| 4-5 | `05 ee` (Lmax 1518) | `05 ee` |
| 6-7 | `04 09` | `04 09` |
| **8** | **`c7`** | **`f7`** |
| 9 | `00` | `00` |

Bytes 2-3 are the VL the DTN puts its own health monitor and its `0x52` replies
on. The main ATE software has RemoteConfigSender's value compiled in —
`HEALTH_MONITOR_RESPONSE_VL_IDX`, and the receive filter at
`dpdk/src/HealthMonitor/HealthMonitor.c:660` drops every frame whose destination
MAC does not end `11 88`. Our end-system block says `00 26`, so the device
answers on VL 38 instead. Our receiver does not filter by VL, so it sees either
and reports the id it saw.

`c7` → `f7` sets bits 4 and 5. Two samples are not enough to say what they mean,
so the block is sent verbatim. Nothing in it is recomputed from the VL table.

Neither `0x10` nor `0x17` contains a port number or a destination mask, so a VL
named in the end-system block still needs a switch record to leave the box. The
capture has no such record, which is why the table is 123 rather than 122:

```
VL   38  00 26 06 02 00 40 d5 ee 02 22 00 00 00 00   port 34 -> port 33
```

Flag nibble `0xD` is what the reference gives this VL specifically; every other
record, the fibre-side taps included, uses `0x9`.

`--keep-management` appends VL 4419-4490 verbatim on top, the whole management
path the reference configuration builds. It belongs with RemoteConfigSender's
end-system block, which answers on VL 4488, so it is off by default.

## Copper is sockets, not DPDK — the same as the main software

The two copper end-system ports are `AF_PACKET` raw sockets here. That is not a
simplification of what the rig does; it is what the main ATE software does too.

In `dpdk/`, ports 12-15 are the copper ones and they never reach DPDK. Their PCI
addresses (`01:00.0` through `01:00.3`) are recorded in `Config.h` as
documentation only — the ports stay kernel-owned and
`dpdk/src/RawSocketPort.c` drives them with `socket(AF_PACKET, SOCK_RAW, ...)`.
The health monitor does the same in `dpdk/src/HealthMonitor/HealthMonitor.c:589`,
on `eno12409`. DPDK owns the fibre ports and nothing else.

Where the main software goes further is throughput. To push 960 Mbps out of the
1G copper port it adds `PACKET_TX_RING`/`PACKET_RX_RING` with `TPACKET_V2`,
`mmap`s the rings, sets `PACKET_QDISC_BYPASS`, and spreads receive over several
sockets with `PACKET_FANOUT`. We have no reason to: the whole of a run is four
configuration frames and a health-monitor stream that arrives in six-packet
cycles. Plain `sendto`/`recvfrom` on a bound `AF_PACKET` socket, with
`PACKET_MR_PROMISC` so nothing is filtered out, covers it. If a later test ever
needs line rate on copper, the ring setup in `RawSocketPort.c` is the pattern to
copy — it is the same socket, configured harder.

## The DTN's own health monitor

The device broadcasts a six-packet cycle on its own once 28 V is applied, and
`DTN_HEALTH_MONITOR_VL` routes it to the 100M copper port. These are the same
packets the main ATE software reads; `HealthDecode.c` parses them the same way
`dpdk/src/HealthMonitor/HealthMonitor.c` does, against the offsets in
`dpdk/include/HealthTypes.h`.

```
1187 + 1083            assistant FPGA, ports 0-15
1187 + 1083 + 438      manager FPGA,   ports 16-34
  94                   MCU
```

A 1187-byte packet is a 111-byte device header plus 8 port blocks of 129 bytes;
the other FPGA packets are a 7-byte mini header plus port blocks. Each block
names its own port, so blocks are placed by what they say rather than by which
packet they arrived in — a mini-header packet needs no memory of the one before
it. Shape is decided by size, and every packet carries a byte or two past its
last block, so the sizes are matched with an allowance rather than exactly.

**The device header** answers whether the configuration took. `config_id` moves
when the device accepts one. The three `eth_wrong_*` counters are its account of
frames it threw away, one per field of the payload header (`26 00` LRU, `57`
operation, `10` block address) — all decided before any block data is read, so a
frame counted there was rejected on its header alone, and a configuration the
device dislikes for what is *inside* a block leaves them untouched. The header
also carries both core versions, the FIFO sizes, its time of day, and the FPGA's
supply voltage and temperature. Those last two are bit-packed rather than plain
numbers — millivolts in bits 3-14 with a tenth in bits 0-2, and Kelvin in bits
4-14 with a fraction whose divisor depends on whether it reaches 10.

**The port blocks** give rx/tx and every drop reason the switch distinguishes:
`undef-VL` for a VL id the table does not define, `wrong-src` for one that
arrived on a port that is not its source, `under-Lmin`/`over-Lmax` for a frame
outside the VL's length window, plus CRC, alignment, policy drops and the four
queue overflows. The table prints one line per port and names only the counters
that are not zero, so a clean port is one line and a port in trouble says what
kind.

**The MCU packet** is a different shape: no port blocks, but the 28 V primary
and secondary status, PBIT and CBIT, seven supply rails with their currents, the
board and FO transceiver temperatures and both PHY temperatures. On a vibration
rig that is the part most likely to move, so it is worth as much as the switch
counters.

The live table is split the way the routing is — the round's links, the taps,
the copper pair and the management port — because a counter only means something
next to what the port is supposed to be carrying. `--all-ports` adds everything
the round does not use. The end-of-run log records all 35 regardless, along with
every field decoded.

## The VMC test

The VMC sends its health monitor unasked, so the test configures nothing: it
opens both interfaces, sorts what arrives, and keeps a dashboard up until
Ctrl+C. One interface per side — the first carries FLCS, the second VS.
The reports are the ones `dpdk_vmc` reads; `VmcMessages.h` is that project's
`vmc_message_types.h` copied verbatim, and `VmcHealth.c` sorts and byte-swaps
them the same way `dpdk_vmc/src/health_monitor/health_monitor.c` does.

Seven reports arrive from each of the VMC's two sides, FLCS and VS:

| report | how it is told apart |
|---|---|
| CPU usage | its own VL, `Pcs_profile_stats` |
| PBIT | its own VL, guarded by a message id because other traffic shares it |
| CBIT board monitor | one VL, message id |
| CBIT board flags | the same VL, message id |
| CBIT DTN end system | the same VL, message id |
| CBIT DTN switch | the same VL, message id |
| PHY port counters | its own VL, `REPORT_MSG` — no header at all |

The PHY counter report is the odd one: unlike every other report it carries no
`vmp_cmsw_header_t`, so the payload is the struct and nothing else and the VL id
is the whole of what identifies it. It is also the one report `dpdk_vmc` has no
printer for — it is newer than that code — so that printer is ours, laid out
like the ones beside it and marked `(ATE)` so nobody looks for it over there. It
prints the four counters for each of the six ports, then the totals, because six
rows of near-identical numbers hide the one port that has stopped.

**The dashboard is `dpdk_vmc`'s, not ours.** `VmcPrint.c` is lines 924-2126 of
`dpdk_vmc/src/health_monitor/health_monitor.c` copied verbatim - every printer,
the bitfield tables they walk, and the temperature check the dashboard runs
after each report - and `vmc_health_render` is `hm_print_dashboard` with its
slot access swapped for ours: same banner, same order, same closing `[HM]`
diagnostic line. A report shown here and the same report shown by `dpdk_vmc`
are the same text, so the two can be compared line for line.

Three deliberate departures, none of which change a printed character: the
three `hm_check_*_temps` functions lose their `static` because the dashboard
lives in another file here; `hm_temperature_failed()` is added because the
dashboard cannot read that flag directly; and the temperature limits move to
`VmcPrint.h`, which is where the original keeps them. Anything this application
wants to say about a run is printed after the dashboard, never inside it.

Everything on the wire is big-endian and every struct is packed, so each report
is copied in whole and then swapped field by field. `VmcMessages.h` ends in
static assertions holding the sizes the comments claim — a compiler that lays
one out differently fails the build rather than decoding quiet nonsense.

A DTN report that is all zeros is skipped rather than stored: the VMC sends
those before the DTN has answered it, and overwriting a good report with one
would lose what the run is there to see.

**The side comes from the interface, not the VL id.** That is how the rig is
wired and it is the thing known for certain, while the ids are `dpdk_vmc`'s and
unconfirmed here. A report whose VL id names the other side is still filed under
its cable, and the disagreement is counted and shown — a swapped pair of cables
looks exactly like that and nothing else does.

Both links are polled in turn rather than in order, so two links carrying
traffic at the same rate are read evenly instead of the first starving the
second. Per-link frame and report counts appear under the dashboard, because one
cable going quiet is the thing a two-link rig fails at.

**Everything that could change with the rig is in `AppConfig.c`** — both
interface names with the side each carries, all eight VL ids and all five
message ids, in one struct. The ids are `dpdk_vmc`'s and have not been confirmed
against this rig, which is why they are a table rather than constants spread
through the decoder. Moving to a different environment is one edit there and
nothing else.

## Not yet pinned down

* `BAG`, `PRIORITY` and `FEEDBACKVL` share the `0x0602` word and the flag nibble.
  Every reference record uses `BAG=1MS PRIORITY=LOW FEEDBACKVL=FALSE`, so their
  encodings cannot be derived from it. The XML reader raises rather than guessing
  when a profile deviates.
* Bytes 2-3 of the end-system block `0x10` are `0x1188`. That is VL 4488, the id
  `HEALTH_MONITOR_RESPONSE_VL_IDX` names — and the reference table happens to
  hold 4488 records, so the field reads equally well as a record count. It is
  sent verbatim rather than recomputed: writing the record count there is a good
  candidate for why a smaller table silences the device.
* Whether the VMC's VL ids on this rig are the ones `dpdk_vmc` uses. They are in
  `AppConfig.c` so that finding out costs one edit.
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
