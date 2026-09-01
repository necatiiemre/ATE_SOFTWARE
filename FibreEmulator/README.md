# FibreEmulator

Stands in for the unit that will eventually sit on the DTN's fibre ports, long
enough to answer one question: **did the DTN take its configuration?**

It injects a handful of packets on every VL a round defines and reports which
ones came back. Packet contents are ignored — the DTN's VL table matches on the
VL id in the destination MAC and on the VLAN tag, and those are all that matter
here.

## How it fits with the acceleration test

Two independent programs, one rig:

| | runs on | owns |
|---|---|---|
| **AccelerationTest** | workstation | the copper links, and configuring the DTN |
| **FibreEmulator** | server | the fibre links, and the Mellanox switch |

Order of operations: start the acceleration test, let it power up and configure
the DTN, then start this on the server. A VL the DTN routes from fibre out to
copper is injected here and confirmed on the workstation — neither program can
claim that path on its own.

## The fibre path

The server's ports do not touch the DTN directly. A Mellanox switch fans eight
server ports out to the DTN's 32 fibre ports, and the VLAN tag alone decides
where a frame lands:

```
server -> DTN    VLAN = 97 + dtn_port     (the switch strips the tag)
DTN -> server    VLAN = 225 + dtn_port    (the switch adds it)
```

Both rules hold for all 32 breakout ports. Which server port carries which DTN
port is fixed by the cabling and lives in `src/FibreMap.c`.

So for round 1, injecting VL 1024 means: send from server port 2 tagged VLAN 97,
which reaches DTN port 0; the DTN forwards it to port 16; it comes back to
server port 4 tagged VLAN 241. A VL that goes out and never returns is the
finding.

## Running

```
$ make test                       # the logic, no DPDK needed
$ make                            # needs DPDK
$ sudo ./build/fibre_emulator -l 0-15 -n 4 -- --packets 10
```

Options after `--`: `--packets N` (default 5), `--skip-cumulus` when the switch
is already set up.

```
    DTN link      VLAN in/out   VLs   sent   returned   status
    ------------  -----------  ----  -----  ---------   ------
    port  0 -> 16    97 / 241     10     50         50   ok
    port  3 -> 19   100 / 244     10     50          0   NOTHING BACK
```

Exit status is 0 when every link returned traffic.

## Layout

```
src/FibreMap.c      DTN port <-> server port and VLAN
src/Scenario.c      the three rounds, seen from the fibre side
src/VlFrame.c       building and recognising the probe frames
src/Report.c        what was sent and what came back
src/CumulusSetup.c  configuring the Mellanox switch over ssh
src/PortRunner.c    the only file that touches DPDK
cumulus/interfaces  the switch configuration this rig expects
tests/test_logic.c  everything except the DPDK shell
```

`make test` builds and runs the logic on any machine, which keeps the parts that
go quietly wrong — a VLAN off by one, a link mapped to the wrong server port —
out of the lab. `src/PortRunner.c` is the exception: it cannot be built without
DPDK, so it is the one file to watch on the first run.

## Requirements

DPDK on the server (the same one the traffic generator uses) and `sshpass` for
the switch. Raw ports need root.
