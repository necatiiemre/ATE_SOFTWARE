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
$ sudo ./build/fibre_emulator -l 0-3 -n 4 \
      -a 0000:21:00.0 -a 0000:21:00.1 -a 0000:41:00.0 -a 0000:41:00.1 \
      -a 0000:64:00.0 -a 0000:64:00.1 -a 0000:81:00.0 -a 0000:81:00.1 \
      -- --packets 10
```

Options after `--`: `--packets N` (default 5), `--skip-cumulus` when the switch
is already set up.

## Sharing the server with the acceleration test

Both can run on the same machine, and on this rig they do. Two things keep them
out of each other's way:

* **The NICs are on different buses.** The fibre ports are `21/41/64/81:00.x`
  and the copper ports the acceleration test uses are `01:00.x`. Give EAL an
  explicit `-a` allowlist of the fibre addresses so it cannot claim a copper NIC;
  the copper ones must stay bound to the kernel for AF_PACKET to reach them.
* **Ports are resolved by PCI address, not by DPDK's numbering.** An allowlist
  renumbers whatever it does not name, and sending on the wrong port looks
  exactly like a link that does not forward. The startup banner prints the
  mapping it resolved, worth a glance on the first run.

Give EAL a small core list (`-l 0-3`); it does not need more and the other
program wants a core to poll on. Do not run the rig's original `dpdk_app` at the
same time — it claims the same fibre ports and the same copper raw sockets.

Order matters: the acceleration test has to configure the DTN before anything
sent from here has a VL table to be forwarded by.

```
    DTN link      VLAN in/out   VLs   sent   returned   status
    ------------  -----------  ----  -----  ---------   ------
    port  0 -> 16    97 / 241     10     50         50   ok
    port  3 -> 19   100 / 244     10     50          0   NOTHING BACK
```

Exit status is 0 when every link returned traffic.

## When everything reads 100% loss

The loss column alone cannot tell "the DTN forwards nothing" from "it forwards
and we fail to recognise it", so every run also reports what the receive path
actually saw:

```
  receive path
    server port 2: 1043 frame(s)
    server port 4: 1005 frame(s)
    4096 ours, 0 foreign, 0 unmatched, 0 on an unexpected VLAN
```

* **Nothing at all** — no frame reached any receive port. The DTN is not
  forwarding, or the switch is not configured, or the ports are wrong. Look at
  the acceleration test's log next: did the configuration go in, was there a
  status reply.
* **Frames arrive, all foreign** — something is on the link but it is not ours.
  Our probes are not getting through; the DTN is forwarding something else.
* **Frames arrive, ours, but unmatched** — they came back on VL ids nothing is
  waiting for. The two programs are on different rounds.
* **Ours, matched, on an unexpected VLAN** — the path works and the tagging is
  not what the map predicts. Not counted as loss: a probe that came back proves
  the DTN forwarded it, whatever tag it wears.

A probe is identified by its VL id, never by its VLAN. Some NICs strip the tag
on receive and report it out of band, which is read from the mbuf when it
happens.

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
