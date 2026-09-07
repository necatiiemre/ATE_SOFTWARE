# DTN configuration test — runbook

Two programs, one server, one DTN. The acceleration test drives the DTN over
copper and configures it; the fibre emulator stands in for the unit that will
eventually sit on the fibre ports and proves the configuration took.

```
                    ┌──────────────── server ────────────────┐
                    │                                        │
   copper 01:00.x   │  AccelerationTest    FibreEmulator     │  fibre 21/41/64/81:00.x
   (kernel,         │  configures the DTN  injects probes    │  (DPDK)
    AF_PACKET)      │  watches the VLs     counts returns    │
                    └────────────────────────────────────────┘
                              │                    │
                              │                    ▼
                              │            Mellanox switch (VLAN steering)
                              │                    │
                              ▼                    ▼
                       DTN ports 32,33      DTN ports 0-31
                       (end system)         (fibre)
```

---

## 0. Prerequisites, once per machine

* DPDK installed and `pkg-config --exists libdpdk` succeeds
* Hugepages configured (the rig's existing DPDK app already needs this)
* `sshpass` installed — the emulator uses it to configure the Mellanox switch
* The fibre NICs bound to a DPDK driver, the copper NICs **left on the kernel**
* Root, for raw sockets and DPDK

Check the split before anything else:

```bash
dpdk-devbind.py --status | head -40
```

`21:00.x`, `41:00.x`, `64:00.x`, `81:00.x` should be under a DPDK driver.
`01:00.0` and `01:00.1` (`eno12399`, `eno12409`) must still be kernel network
devices — the acceleration test reaches them through AF_PACKET.

**Stop the rig's original `dpdk_app` if it is running.** It claims the same fibre
ports and the same copper raw sockets, and nothing here will work alongside it.

```bash
sudo pkill dpdk_app
```

---

## 1. Build, once

```bash
cd AccelerationTest
make && make test

cd ../FibreEmulator
make test          # logic only, no DPDK needed
make               # needs DPDK
```

Both test runs should end in `PASS`. `AccelerationTest` runs three: it rebuilds
the 47 frames of a configuration the hardware is known to accept, reproduces all
three rounds against the captured config1 byte for byte, and checks the profiles
are self-consistent. A pass means the encoder is still emitting valid frames.

---

## 2. Before each run

```bash
ip link show eno12399
ip link show eno12409
```

Both `UP` with a carrier. If not, `sudo ip link set eno12399 up`.

Power the DTN. Both programs must be run from their own directory — they read
`cumulus/interfaces` and write `LOGS/` relative to it.

---

## 3. Run

### Terminal 1 — acceleration test

```bash
cd AccelerationTest
sudo ./build/acceleration_test
```

1. `3` for DTN
2. pick the round (`1`, `2` or `3`)
3. read the routing it is about to write, then `y`

It waits for the DTN to start talking, sends the configuration — 4 frames,
about 16 ms — then draws the live table and keeps it up until Ctrl+C.

The configuration is three datagrams plus a `0x52` status query:

| seq | blocks | what |
|---|---|---|
| 0 | `0x10` `0x17` | end system |
| 1 | `0x70` `0x72` | switch table, 104 records |
| 2 | `0x72` `0x74` `0x71` | switch table, 19 records |
| 3 | — | status query |

The 122 fibre records are byte-identical to the capture, in its order, which
`make test` checks. The 123rd is VL 38 from the management port out to the 100M
copper port — the DTN's own health monitor, which the capture leaves without a
way off the box.

`--keep-management` appends VL 4419-4490 on top (195 records), the whole
management path the reference configuration builds. The fibre part of the table
is unchanged either way.

All three rounds are built the same way: only the six port pairs and, in round
3, the two tap ports differ. The VL ids, the flag nibbles, the framing and the
VL 38 record are identical across them.

**Leave this running.** The DTN needs its VL table before anything the emulator
sends can be forwarded.

### Terminal 2 — fibre emulator

Once the acceleration test says `configuration sent`:

```bash
cd FibreEmulator
sudo ./build/fibre_emulator -l 0-3 -n 4 \
     -a 0000:21:00.0 -a 0000:21:00.1 -a 0000:41:00.0 -a 0000:41:00.1 \
     -a 0000:64:00.0 -a 0000:64:00.1 -a 0000:81:00.0 -a 0000:81:00.1 \
     --
```

Everything before `--` is EAL's; everything after is ours. Pick **the same
round**. It configures the Mellanox switch, then sends continuously — one packet
per VL per cycle, on all 122 VLs — and redraws its table once a second until
Ctrl+C.

The `-a` list is not optional: it keeps EAL off the copper NICs. `-l 0-3` is
plenty and leaves the other program a core to poll on.

Our options, all after the `--`:

| option | default | what |
|---|---|---|
| `--skip-cumulus` | off | leave the Mellanox switch alone; use it once it is already set up |
| `--cycle-ms N` | 4 | ms between cycles; anything below the 1 ms BAG is refused |
| `--duration N` | 0 | stop after N seconds instead of waiting for Ctrl+C |

---

## 4. Reading the two tables

The emulator owns the fibre links:

```
  DTN link      VLAN in/out    sent   returned   loss   last
  port  0 -> 16    97 / 241    3000       3000     0%   0.0s
  port  3 -> 19   100 / 244    3000          0   100%      -   NOTHING BACK
```

The acceleration test owns everything that reaches copper:

```
  link       DTN  VL-ID   packets      bytes   last   sizes           status
  eno12409    33    100       284     322340   0.0s   1187,1083       ok
  eno12409    33    101         0          0      -   -               MISSING
  eno12409    33     38        30      35610   0.0s   1187            ok
```

VL 100 and 101 are the fibre-side unit's health monitor, routed fibre → copper
by the taps. VL 38 is the DTN's own — the record we add on top of the capture.

Under it, the numbers decoded out of that health-monitor stream:

```
  assistant config 0x0007  beat  42  ports 35  mode 1   rx 1002233 tx 981234   wrong dev/op/type 0/0/0
  manager   config 0x0007  beat  43  ports 35  mode 1   rx 998110 tx 977402    wrong dev/op/type 0/0/0

  port  speed         rx         tx  undef-VL  wrong-src  Lmin  Lmax   CRC  drop
  ----  -----  ---------  ---------  --------  ---------  ----  ----  ----  ----
     6  1G        120222      98006         0          0     0     0     0     0
    22  1G        119006      97506      4471          0     0     0     0     0
```

Only the ports the round touches are listed. This is where a configuration that
did not take shows itself:

| symptom | what it means |
|---|---|
| `config` the same as before the run | the device did not take the configuration |
| `wrong dev/op/type` climbing | it saw the frames and rejected them — wrong LRU id, wrong operation, unknown address |
| `wrong dev/op/type` all zero and `config` unchanged | the frames never reached it |
| `rx` climbing, `undef-VL` climbing with it | the port receives, but the VL is not in the table |
| `wrong-src` climbing | the VL is in the table under a different source port |
| `Lmin`/`Lmax` climbing | frames are outside the VL's length window |
| `rx` flat at 0 | nothing is arriving on that port at all — cabling or the far side |
| `no data` | the port was in no health packet; the FPGA that owns it is not reporting |

Neither can confirm the whole path alone:

| what it proves | where to look |
|---|---|
| DTN forwards fibre to fibre | emulator, `ok` on a link row |
| DTN forwards fibre to copper | acceleration test, VL 100/101 not `MISSING` |
| DTN's own health monitor is alive | acceleration test, VL 38 `ok` |
| DTN accepted the configuration | both — a table of `ok` rows is the answer |

---

## 5. When it does not work

**Every emulator row says NOTHING BACK.** The DTN has no usable VL table, or
frames never reach it. In order: was the acceleration test's `configuration sent`
line printed before you started the emulator? Did the switch setup succeed? Did
the acceleration test say `no status reply` — the configuration may have been
rejected.

**Some rows work, some do not.** The configuration went in but a specific link is
down. Check the fibre cable for that DTN port and the switch's breakout port —
DTN port N is `swp(25 + N/4)s(N%4)`.

**The acceleration test never says `unit is up`.** Nothing is arriving on either
copper link within 90 seconds. Either the DTN is not powered, the copper cables
are on the wrong ports, or the NICs got bound to DPDK after all.

**`server port N was not found`.** EAL did not get that PCI address in its `-a`
list, or the NIC is not bound to a DPDK driver.

**Everything on copper is MISSING but the fibre links are fine.** The DTN is
forwarding fibre to fibre but not to copper. That points at the copper cabling,
not at the fibre configuration.

**VL 38 never appears.** The DTN's own health monitor is not coming out. Either
the end-system block's VL field is not what we think it is, or the VL 38 record
is not enough on its own — try `--keep-management`, which sends the whole
management path the reference configuration builds instead.

---

## 6. All three rounds

Each round covers 12 of the DTN's 32 fibre ports; together they cover all of
them. Rounds 2 and 3 overlap on ports 10-11 and 26-27, which is intended.

| round | fibre ports | health-monitor taps |
|---|---|---|
| config1 | 0-5 ↔ 16-21 | ports 15, 31 |
| config2 | 6-11 ↔ 22-27 | ports 15, 31 |
| config3 | 10-15 ↔ 26-31 | ports 0, 16 |

To move to the next round: Ctrl+C the emulator, Ctrl+C the acceleration test,
start the acceleration test again with the next round, then the emulator with the
same one. The DTN is reconfigured from scratch each time.

Round 3 moves the taps because ports 15 and 31 carry fibre traffic in that round.

---

## 7. What a run leaves behind

`AccelerationTest/LOGS/DTN/<round>_<timestamp>.log` — every event with a
timestamp, flushed line by line, including which VLs were seen and which never
arrived. The emulator prints its table and exits 0 only when every fibre link
returned traffic, so it can be scripted.
