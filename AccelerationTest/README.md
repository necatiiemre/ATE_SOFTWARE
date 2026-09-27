# AccelerationTest

Acceleration test rig for the ATE units. The operator picks a unit and the
application runs that unit's acceleration test. All three are written: DTN, VMC
and CMC.

Each unit keeps its own folder — its wire formats, its decoders, its test, its
tests and its fixtures — so reading one means reading one directory. `common/`
holds what they are all built from and knows about none of them; `app/` is the
menu and the registry that says which units exist.

```
app/main.c                 unit menu and command-line switches
app/UnitManager.c          the unit registry: which units the program has

common/include/Unit.h      what a unit's test looks like from the outside
common/src/AppConfig.c     everything that changes with the rig, in one place
common/src/RawSocket.c     AF_PACKET access to one link
common/src/Heartbeat.c     a monotonic clock, and whether the unit is still talking
common/src/SafeShutdown.c  releases sockets on Ctrl-C or any error path
common/src/Log.c           the run log: a transcript of the terminal
common/src/Prompt.c        terminal input

dtn/src/DtnTest.c          the DTN acceleration test
dtn/include/DtnConfig.h    wire format: VL records, config blocks, frame assembly
dtn/src/DtnConfig.c        the encoder
dtn/src/VlProfile.c        the three configuration rounds
dtn/src/DtnHealthFrame.c   recognising the DTN's health-monitor stream
dtn/src/HealthDecode.c     taking those packets apart
dtn/src/VlWatch.c          what arrived on copper, by VL
dtn/tests/                 four test programs
dtn/fixtures/              the reference frames and the config1 capture
dtn/tools/                 analysis-side helpers (see below)
dtn/profiles/              the three rounds as JSON, for those helpers

vmc/src/VmcTest.c          the VMC acceleration test
vmc/include/VmcMessages.h  wire format, verbatim from dpdk_vmc
vmc/src/VmcHealth.c        sorting and byte-swapping the reports
vmc/src/VmcPrint.c         dpdk_vmc's printers, verbatim
vmc/src/VmcPbitRequest.c   the one thing this application transmits
vmc/tests/                 three test programs

cmc/src/CmcTest.c          the CMC acceleration test
cmc/include/CmcPacket.h    wire format: the data-plane frame and PRBS-31
cmc/src/CmcDataPlane.c     the twin senders, the receivers and the loss accounting
cmc/src/CmcVerify.c        what the CMC does to a payload, checked in reverse
cmc/src/CmcStats.c         the two loss tables
cmc/src/CmcPmm.c           the PMM lines: listen only
cmc/src/MmmsHandler.c      the Ctrl+C log handover, copied from dpdk_cmc
cmc/src/health_monitor/    the DSM health monitor, copied from dpdk_cmc whole
cmc/include/health_monitor/ its nine headers, likewise
cmc/tests/                 four test programs
```

`make` builds the application; `make test` builds and runs every unit's tests
from that unit's folder, so the fixtures they read are beside them.

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

The configuration link is the 100M one when its cable is in, because that is the
proven management path, and the 1G one when it is not. It is chosen rather than
fixed for a reason worth knowing: a configuration sent down an unplugged cable is
sent nowhere, and nothing afterwards says so. A raw socket takes the frames
happily, the DTN carries on with whatever table it already had - health monitor
and all - and the run looks alive while every frame it generates is dropped by the
device as an undefined VL. That reads exactly like a *wrong* VL table, which is a
rig session spent looking in the wrong place. So the links are reported before the
run starts:

```
  copper links:
    eno12399   DTN port 32  1G    connected
    eno12409   DTN port 33  100M  no carrier
    -> configuring over eno12399 instead of eno12409, which is not connected
```

and if the device does start dropping our traffic as an undefined VL, the display
says that in one line instead of leaving it to be worked out from the tables.

## The live table

Until the health-monitor payloads are decoded, the run answers a simpler and
more useful question: did the configuration take? Every VL the profile routes to
copper is seeded into a table at zero packets, so one that never arrives shows up
as a row rather than as an absence:

```
unit ALIVE   expected VLs seen 61/123   power interruptions 1

  link       DTN  VL-ID        packets      bytes   last   sizes           status
  ---------- ---  -----------  --------  ---------  -----   --------------  ------
  eno12399    32  100               284     322340   0.0s   1187,1083       ok
  eno12409    33  101                 0          0      -   -               MISSING
  eno12399    32  38                200      60000   0.0s   300             ok
  eno12399    32  4024-4083       12000   18108000   0.0s   1509            ok
  eno12409    33  6024-6083           0          0      -   -               MISSING
  eno12399    32  9999                5        320   0.0s   64              extra
```

`MISSING` means the profile routes that VL to that copper port but nothing has
arrived — either it is not being generated or it is not being routed. `extra` is
a VL nobody asked for. Packet contents are ignored.

config1 routes 123 VLs to copper, and 123 rows would push the health tables off
the screen, so neighbouring VLs that agree - same link, same verdict, same frame
sizes - print as one row over their range with the packets added up. A VL that
disagrees with its neighbours breaks the run and gets its own row, which is the
one worth looking at.

Power is operated separately; the test only observes a unit that is already
live. What it does watch for is power *dropping* mid-run - on a vibration rig
that is a likely fault and probably the most valuable thing a run can catch.
When the health monitor goes quiet and comes back, the DTN has rebooted and lost
its VL table, so the test re-sends the configuration and records both the loss
and the recovery with timestamps.

## The log is the terminal

`LOGS/<unit>/<profile>_<timestamp>.log` holds **what was on the screen**, in the
order it appeared, and nothing else. Once the log is open, stdout is a tee: the
banners, the live tables, the health-monitor dashboards and the end-of-run
summaries all reach the file as well as the screen, and nothing is written to
one that is not written to the other. Reading the log afterwards is reading the
run.

That is done by replacing `stdout` with a stream that writes twice, rather than
by asking every caller to. It catches output this project did not write — the
VMC dashboard is `dpdk_vmc`'s printers verbatim and needed no change to be
recorded.

The one thing the file does not get is the escape sequence that redraws the
screen in place. It is an instruction to a terminal rather than something anyone
printed, and in a file it turns every redraw into a jumble.

Each write is flushed, so a run that ends abruptly — the rig cutting power to
the workstation is the expected way for one to end — still leaves everything it
saw.

The log opens before the run's plan is printed, so which round was chosen and
what routing was written are the first things in it.

**A live dashboard redrawn once a second is a lot of text.** The VMC's is about
120 KB per redraw, which is around 400 MB an hour. `display_interval_ms` in
`common/src/AppConfig.c` is how often it is redrawn; raising it divides the log
by the same amount.

Raw sockets need root or `CAP_NET_RAW`.

## Adding a unit's test

Add a `<unit>/` folder with `src/<Unit>Test.c` exposing a
`unit_result_t <unit>_test_run(void)`, list it in `UNITS` in the Makefile, and
flip its `implemented` flag in the table at the top of `app/UnitManager.c`.
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

All three rounds have been re-cut for the new rig. Each is written out on its own
rather than generated from a shared macro: they no longer have a shape in common,
and one that did would hide which round changed.

| round | fibre ports | pairs | the VMC's two |
|---|---|---|---|
| config1 | 0-9 | 0↔4, 1↔5, 2↔6, 3↔7 | 8 and 9 |
| config2 | 10-19 | 10↔14, 11↔15, 12↔16, 13↔17 | 18 and 19 |
| config3 | 20-31 | 20↔25, 21↔26, 22↔27, 23↔28, 24↔29 | 30 and 31 |

config3 has two ports more than the others, and they go into the fibre pairs:
five pairs of five rather than four of four. The VL ids keep counting in tens
from 1024 either way, so its forward group runs one link further - to 1073 where
the others stop at 1063. Everything else about the three is the same.

### The shape of a round

Writing `B` for the first fibre port of the round, `N` for how many pairs it has
and `V` for the first of the two the VMC gets:

```
fibre         B <-> B+N,  B+1 <-> B+N+1,  ...   VL 1024.. / 2024.., ten a link
VMC's HM      port V   -> copper 32   VL 100
              port V+1 -> copper 33   VL 101
DTN's own HM  port 34  -> copper 32   VL 38
copper legs   copper 32 -> port V     VL 3024-3083   workstation -> VMC
              port V   -> copper 32   VL 4024-4083   VMC -> workstation
              copper 33 -> port V+1   VL 5024-5083   workstation -> VMC
              port V+1 -> copper 33   VL 6024-6083   VMC -> workstation
```

323 records for config1 and config2, 343 for config3, which is four switch
datagrams where the capture needed two, and six frames in all. The VL ids are the
same in every round: they never run at once, and keeping them identical means one
less thing that differs between a round that works and a round that does not.

Ports V and V+1 are the VMC's. Three things happen on them and they are easy to
confuse: the VMC's own health monitor comes *out* of them on VL 100 and 101, the
workstation's traffic goes *in* through them on the outbound legs, and the VMC's
answer comes back *in* at the same port and out of the same copper link. The
opposite directions are why a port can be a tap source and a leg destination at
once without the validator objecting.

The copper legs are the point of the round: one flow proves the DTN forwards
copper to fibre and back *and* that the VMC at the far end is alive, rather than
two separate tests that each prove half of it.

The workstation generates the traffic and the VMC answers it, so the frame and
the transform are dpdk_vmc's, field for field. The workstation sends

```
[seq 8][PRBS 1459]
```

and gets back

```
[seq 8][SplitMix XOR 64][CRC32C 4][PRBS ...][last byte]
```

- the sequence is a raw host-order 64-bit word, which is what
  `*seq_ptr = sequence_number` in the reference writes and what its receiver
  reads straight back out. Not big-endian: reading it the other way round changes
  which PRBS offset is regenerated, so this is the one field where a guess would
  make every frame read as bad
- the SplitMix zone is the original PRBS bytes XOR'd with `splitmix64` fed from
  8 × the big-endian form of the sequence plus the block index, written
  big-endian, eight bytes at a time
- the CRC covers the sequence and that zone, is big-endian on the wire, and uses
  the unit's own table rather than standard CRC-32C
- the last payload byte is the DTN's: it writes its own sequence there as the
  frame passes through, which is why both reference receivers leave it out of
  their PRBS comparison and why this one does too

dpdk_cmc applies the same transform plus one XOR'd byte; dpdk_vmc has no such
byte, so the overhead is 68 where the CMC's is 69. That difference is the whole
of what `splitmix_layout_t` in `common/SplitmixVerify.h` expresses, and both
units go through the same verifier.

Frames are 1509 bytes with nothing after the payload - a data-plane frame is
exactly its IP total_length long. That is what dpdk_vmc's frames are once the
switch has stripped their 802.1Q tag, which is to say it is what the VMC already
answers. The management path is different and keeps its trailing AFDX byte: the
reference configuration carries one and so does the device's own health monitor.

The PRBS stream is generated once at the start of a run (`common/Prbs31.c`,
shared with the CMC test) and read at an offset the sequence decides - never per
frame. So the receiver regenerates what it should have got from the sequence
alone and remembers nothing between frames: a frame that arrives late still
verifies on its own, and loss is the gap between the sequence that arrived and
the one expected, per VL, counted once at the frame that reveals it.

Each VL keeps its own sequence, because each is its own AFDX stream with its own
BAG, and because the DTN may reorder between VLs but not within one. The source
IP names the copper port the frame came from.

BAG is 1 ms on every VL record - one frame per VL per millisecond - and 100
Mbit/s of 1509-byte frames over sixty VLs works out at about 138 a second each,
comfortably inside it. `dtn_legs_rate_plan` says so before the run rather than
leaving a policed rate to look like loss during it.

Both legs are held to 100 Mbit/s (`app_config_dtn_leg_mbps`). DTN port 33 is the
100M link, so that is its ceiling, and matching the 1G leg to it is what keeps the
two comparable: a difference between them is then the unit's rather than the
cable's.

### The capture is no longer any round

The captured configuration - six fibre pairs 0-5 to 16-21, taps from ports 15 and
31, the DTN's own health monitor out of port 33 - is the only evidence there is
that the encoder emits frames real hardware accepts. Now that every round has
moved, that capture lives on as `vl_profile_reference()`: a profile that is not
in the menu, does not change when the rounds do, and is what `test_rounds` checks
byte for byte.

The rounds themselves get a structural check - one function serves all three,
taking where the fibre pairs start, how many there are, and the VMC's two ports.
Every record has to be the VL id and port pair its profile declares, the routing
has to validate, and it still has to fit in frames.

Without that split, re-cutting a round would have quietly thrown away the only
hardware-validated proof in the repository.

### The rounds as they stand

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
`dtn/tests/test_profiles.c` checks the copy stays faithful.

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

`dtn/tests/test_rounds.c` rebuilds both datagrams and compares them with
`dtn/fixtures/config1_switch.bin` byte for byte. Record layout, record order,
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

Eight reports arrive from each of the VMC's two sides, FLCS and VS:

| report | how it is told apart |
|---|---|
| CPU usage | its own VL, `Pcs_profile_stats` |
| PBIT | its own VL, guarded by a message id because other traffic shares it |
| CBIT board monitor | one VL, message id |
| CBIT board flags | the same VL, message id |
| CBIT DTN end system | the same VL, message id, **network type 0** |
| CBIT DTN switch end system | the same VL, the same message id, **network type 1** |
| CBIT DTN switch | the same VL, message id |
| PHY port counters | its own VL, `REPORT_MSG` — no header at all |

The end-system CBIT report arrives **twice** — once for the end system itself
and once for the switch's embedded end system — on the same VL, with the same
message id, and only the network type byte tells them apart. One slot meant
whichever arrived last overwrote the other and half the report was never seen,
so they are kept and printed as two: `[VS ES]` and `[VS SW-ES]`, with the
`Network Type` line inside each confirming which. A third value is neither, and
is counted and named rather than filed as one of them.

The DTN switch CBIT report arrives twice as well, but for a different reason:
each side sends two of them at once and one is for a link that is carrying
nothing. Same VL, same message id, same length — `comm_status` is the only thing
that separates them, so whichever landed last won, and half the time that was
the empty one. Now only the report whose `comm_status` matches
`sw_comm_status_live` is kept; the other is counted and left out rather than
printed over the one that had something in it.

That value is a property of the rig, so it is one line in
`common/src/AppConfig.c`:

```c
    .sw_filter_by_comm_status = true,
    .sw_comm_status_live      = 1,
```

It is not guessed either. Every `comm_status` that actually arrives is counted
per side, together with how many of them carried data, and the dashboard prints
the census next to the value in use:

```
[ATE] FLCS DTN SW comm_status: 0 (18, 0 with data), 1 (18, 18 with data) - keeping 1, left out 18
```

So the right value is read off a run. If the filter ever keeps nothing at all —
a rig that numbers the two the other way round — it says so instead of leaving
an empty panel and no reason for it, and names the file to change. Setting
`sw_filter_by_comm_status` to `false` goes back to keeping whichever arrived
last.

The PHY counter report is the odd one twice over. Unlike every other report it
carries no `vmp_cmsw_header_t`, so the payload is the struct and nothing else
and the VL id is the whole of what identifies it — and it came with no byte
order either. Every other VMC report is big-endian and says so; this one is a
bare packed C struct, which is what a sender that copies its own memory onto the
wire produces, and that is host order, whichever the VMC's is.

So it is not guessed. Both readings are taken and the plausible one kept: a
packet counter needs a century at line rate to reach 2⁴⁸, and the same bytes
cannot be small both ways round, so of the two readings at most one is a count
and that is the one the device meant. The table says which way it was read, and
so does the log. `counters_order` in `common/src/AppConfig.c` forces one
(`VMC_COUNTERS_BIG` or `VMC_COUNTERS_LITTLE`) if a rig ever needs it;
`VMC_COUNTERS_AUTO` is the default.

Two more things guard it, both because a good report was seen being replaced by
nonsense. Its length must be the struct's, give or take the AFDX sequence byte:
without a message id the length is the whole of what says a frame on this VL is
a counter report, and anything longer is a different message that used to get
read as one — those are counted and named on the dashboard instead. And if
neither reading is plausible the table says so in a line under the totals,
rather than letting the numbers stand. The first report from each side also goes
into the log whole — the payload length, all 192 bytes, and the largest counter
each way round — so a byte order or a moved field can be settled from the run's
own transcript rather than a screenshot. It is also the one report `dpdk_vmc` has no
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

**PBIT is the one thing that has to be asked for.** It is a power-on result the
VMC holds until requested, so without a request the two PBIT slots stay empty
for the whole run. The request is
`Test_Starters/vmc/src/main.c`'s `send_pbit_request`: an 11-byte cmsw header
with message identifier 50, a length of 11, a zero timestamp and the sequence
byte last — 0 once, then 1..255 cycling, never 0 again — wrapped in Ethernet,
an optional 802.1Q tag, IPv4 from 10.0.0.0 to 224.224.`<VL>` and UDP 100→100,
padded to the 64-byte Ethernet minimum. It goes out on the link that carries
that side and repeats every two seconds until that side answers on its response
VL, where the decoder picks it up like any other report. Sides that have already
answered are left alone, because PBIT does not change while the VMC is up.

That request is the only thing this test transmits.

The request goes out **untagged** — there is no VLAN anywhere in it. The starter
tags its requests because it reaches the VMC through the Mellanox switch and the
tag is what steers them there; nothing steers a direct cable.

Only the first answer from each side is kept, as the starter keeps it: PBIT is
the power-on result, so a later copy can only be the same thing, and the first
is the one that answered the request that was sent.

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

## The CMC test

Four Ethernet links where `dpdk_cmc` has one fibre port:

| interface | module | what it carries |
|---|---|---|
| `ens6f0` | DSM-A | network A of the data plane, and its health monitor |
| `ens6f1` | DSM-B | network B of the data plane, and its health monitor |
| `ens6f2` | PMM1 | listen only: the SMMM's stream to the first PMM |
| `ens6f3` | PMM2 | listen only: the SMMM's stream to the second PMM |

The reference multiplexes all four flows onto one fibre port and tells them
apart by the 802.1Q tag the Cumulus switch adds and strips. Here each flow has
its own interface, so the interface is what tells them apart — which is also why
the third PMM is absent rather than present and always empty: the rig has two.
`cmc_config_t` in `AppConfig.h` holds the whole map, and re-cabling is one edit.

### The frames are untagged, and that changes nothing the unit sees

The reference sends tagged, the switch strips the tag before the CMC gets the
frame, the CMC answers untagged, and the switch tags it again on the way back.
So the frame the unit actually handles is the untagged one, and that is the frame
this puts on a direct cable — 1509 bytes, with the payload kept at the 1467 bytes
the reference builds for a tagged frame. Keeping the payload length fixed is the
whole point: it makes the direct-cable frame byte for byte the one the CMC
already answers. `vlan_tagged` puts the tag back if a switch is ever put in
between.

### Both networks get the same frame

One sender walks the 104 VL ids in turn and, for each, puts the *same* frame on
both links: same VL id, same sequence, same payload, same trailing `DTN_SEQ`
byte, differing only in the last byte of the source MAC — `0x20` for network A,
`0x40` for B — which is what names the network. That is the test. Two networks
given identical traffic, so a difference in what comes back is a difference in
the unit rather than in what it was given.

The sequence belongs to the VL id and is shared by the twins, and it advances
only once network A's frame is actually on the wire. A refused frame retries the
same sequence rather than skipping one; the one case where the pair comes apart —
A sent, B not — is left to show up as loss on B, which is what it is.

### What the CMC returns, and how it is checked

The frame that comes back is not the frame that went out. The CMC rewrites the
front of the payload and returns it on a VL id 520 higher:

```
[seq 8][SplitMix XOR 64][CRC32C 4][XOR byte 1][PRBS …][DTN_SEQ 1]
 0..7   8..71            72..75    76          77..     last
```

- the SplitMix zone is the original PRBS bytes XOR'd with `splitmix64` fed from
  the big-endian sequence, eight bytes at a time
- the CRC covers bytes 0..71, big-endian on the wire, and uses the unit's own
  CRC table, which is **not** standard CRC-32C — see `CmcPayloadVerify.h`
- the XOR byte is the original PRBS byte at that offset run through the chain
  `{6,7,8,13,15}`, which folds to a single XOR with `0x0B`
- the rest is the PRBS stream untouched, short of the last byte, which carries
  `DTN_SEQ` and is skipped

Nothing is remembered between packets. The sequence in the payload says which
PRBS offset to regenerate, so every check is against something derived, and a
packet that arrives out of order still verifies on its own. Loss is the gap
between the sequence that arrived and the one expected, per VL and per network,
counted once at the packet that reveals it; a late packet adds nothing and does
not move the expectation backwards, which would make the next packet look like a
fresh gap.

### The health monitors, and the copies

`cmc/src/health_monitor/` and `cmc/include/health_monitor/` are eleven files
copied from `dpdk_cmc` byte for byte. None of them touches DPDK — the health
monitor rides on the data-plane links and its decoder only ever sees a UDP
payload — so there was nothing to rewrite, and every printer is the reference's,
field for field. `diff` against the reference is the test that matters, and the
commit that changes that is the commit to argue with.

Two things were needed to make eleven unmodified files build here.
`health_monitor_cmc.c` includes `"Config.h"` for `DEBUG_MODE`, and the
reference's `Config.h` is a DPDK application's configuration — rate limits, queue
counts, VLAN templates — so a `Config.h` holding that one symbol sits beside the
copies. And one symbol collides: `dpdk_vmc` and `dpdk_cmc` each have a
`print_pcs_profile_stats` for their own unit's CPU-usage report, and both copies
are in this binary, so the CMC's is renamed on the compiler command line rather
than in the file. `make symbols` compares every global the copies define against
every other global in the binary, so the next collision arrives as a sentence
saying what to do.

`MmmsHandler.c` is a copy too, with two regions changed and a comment at the top
saying which: the include block, and the body of `mmms_send_trigger`, which the
reference builds into a DPDK mbuf and which here goes into a buffer and out of a
raw socket.

### The PMM lines

Listen only — there is no transmit path at all. `MSG` is 1036 bytes: an 8-byte
sequence, 1024 of data, a 4-byte CRC, with the SMMM's own 2-byte header
sometimes in front and told apart by the UDP length being 1038 instead.

Two things about it are not pinned down by any document, and both are handled
rather than assumed, as the reference handles them. The sequence is big-endian:
read the other way round, a real captured counter came out as 10¹⁶ and the loss
column filled with nonsense. And the CRC's algorithm, coverage and byte order
are found by trying the eight combinations on the first packets and locking onto
the one that matches — the verified one is first in the list, so on a healthy
line the lock happens on the first packet, and the table says which one is in
use.

A sequence jump too large to be loss is a resync rather than a million lost
packets, and an address that does not match the configured one is counted rather
than dropped, so a wrong assumption in `AppConfig.c` shows up in the table
instead of throwing the line's traffic away.

### The output, and the one thing that moved

The tables are the reference's, column for column and box for box. Two things in
them differ, both because there is no DPDK underneath: the "Server Port" column
holds the interface name instead of a DPDK port number, which is 0 in every row
of the reference and is the thing an operator needs here when a row goes quiet;
and the packet and byte counts are the ones this program kept rather than a NIC's
hardware counters.

The order differs in exactly one way: the reference prints the loss tables first
and the health monitor after, and this prints **the health monitor first and the
loss tables last**. The tables are what gets watched, and the bottom of the
screen is where they stay put while the health monitor above them grows and
shrinks.

The warnings block gains one line the reference has no need for — a frame the
kernel would not take for sending. The kernel is in the send path here, so it can
happen, it is this end rather than the unit, and without a line of its own it
would read as loss.

### What is left out, and why

The PSU telemetry table. It exists to show the 1 Hz V/I/W stream MainSoftware
publishes while it drives the supply; nothing drives a supply here — the unit is
switched on by hand — so the table would print empty every second.

### What has actually been run

Everything below `make test` — the frame field by field, the PRBS stream, the
verification path against an independent statement of the CMC's transform, the
loss arithmetic, the PMM channel, the health-monitor decode and every printer.

And `sudo make smoke`, which is the only one that uses a socket: the real data
plane against the loopback interface with a thread standing in for the unit,
reading what the sender emits, applying the transform, moving the VL id into the
return range and sending it back. That covers the sockets, the threads, the
pacing and the whole loop, and it is what found the one thing unit tests could
not: a packet socket opened for every protocol is handed outgoing frames too, so
without `PACKET_IGNORE_OUTGOING` the receiver counted every frame this end sent
as an arrival on an unexpected VL.

What none of it covers is the unit. The stand-in does what we believe the CMC
does, so agreeing with it means the two ends of *this* program agree. Whether the
CMC accepts an untagged 1509-byte frame, returns it on a VL id 520 higher, and
applies that exact transform, only the CMC can say.

### Two departures in the mechanics

Pacing sleeps the bulk of each slot and spins only the last 60 µs, where the
reference busy-waits the whole gap. That is free on a dedicated DPDK lcore and
rude on a shared one; the pacing it produces is the same, one frame per slot with
no catching up, so falling behind still cannot turn into a burst.

And the sockets ask for 16 MB of receive buffer and bypass the qdisc. The kernel
is in the path here and its default buffers are a few hundred kilobytes, so a
scheduling hiccup on a receiver would otherwise look exactly like loss on the
unit's side.

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

## dtn/tools/

Python helpers from the reverse-engineering work, kept until the C application
covers the same ground:

* `build_config.py` — expands a JSON profile into frames; a second implementation
  to diff the C encoder against. That cross-check has already caught a real bug,
  and confirms all three rounds still encode identically after refactoring.
* `vl_xml.py` — vendor `<VL .../>` XML to VL records.
* `dump_reference_fixture.py` — regenerates `dtn/fixtures/reference_frames.bin`.
