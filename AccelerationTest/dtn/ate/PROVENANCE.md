# Where the copied health monitors came from

Both units' health monitors are the ATE software's own code, copied byte for byte
rather than reimplemented:

* the DTN's, in this directory — `HealthMonitor.c`, `HealthMonitor.h`,
  `HealthTypes.h`;
* the CMC's, in `cmc/src/health_monitor/` and `cmc/include/health_monitor/` —
  two source files and nine headers.

`PsuTelemetryReceiver.h` and `ShutdownSnapshot.h` here, and `Config.h` and
`ShutdownSnapshot.h` beside the CMC's, are stand-ins rather than copies — see
their own comments. `make ate-diff` checks everything else and skips those.

| | |
|---|---|
| repository | `necatiiemre/update-deneme` |
| branch | `claude/peaceful-johnson-9xhutk` |
| commit | `8df47cc` — *dpdk_cmc HM: decode the 113-byte CPU usage packet as Pcs_profile_stats* |
| taken from | `dpdk/src/HealthMonitor/…` for the DTN, `dpdk_cmc/{src,include}/health_monitor/` for the CMC |

The files use CRLF line endings there, and the copies keep them. `make ate-diff`
is a byte-exact comparison, so normalising them would break the one check that
says the copy is still a copy:

```
make ate-diff ATE_REF=/path/to/update-deneme/dpdk \
              CMC_REF=/path/to/update-deneme/dpdk_cmc
```

## What this revision changed, and why it matters here

It is newer than the copy that was in `ATE_SOFTWARE/dpdk`, and two of the changes
land directly on this test.

**Port speeds were being misread.** The old table mapped the speed code as
`0=1000M 1=10M 2=100M`. It is actually:

| code | speed |
|---|---|
| 0 | `1-GBPS` |
| 1 | `10-MBPS` |
| 2 | `UNDEFINED` |
| 3 | `100-MBPS` |

So a port with no negotiated speed used to print as `100M` — a link that is not
there, reported as a working 100 Mbit link, on the row an operator would check
first.

**The port counters are absolute.** The device never clears them, so a reading on
its own counts from whenever it was last powered. This revision adds a baseline
and a start/end pair of tables; `AteHealth.c` drives them, marking the baseline on
the first cycle of a run.

**VL 0 is flooded.** `HEALTH_MONITOR_QUERY_VL_IDX 0x0000` is new, with a comment
saying why: the `0x52` query's destination MAC is `03:00:00:00:00:00`, so its VL
id reads back as 0, *and the switch floods those queries to the other ports* —
which is why the ATE software's own PRBS filters had to learn to ignore them. That
is the reason this test does not poll by default; see
`app_config_dtn_health_poll`.

## What it changed on the CMC side

Two reports the unit sends were not being decoded at all. Both fell through to the
unknown-length histogram, which is exactly where a report goes when nobody has
told the program it exists.

**113 bytes — `Pcs_profile_stats`, the CPU and memory profile.** The struct was
already there, but its memory-profile fields were declared `size_t`. The firmware
is built for a 32-bit target, so they are 4 bytes on the wire; on x86_64 `size_t`
made them 8 and the struct 136, so the dispatch was waiting for a 137-byte packet
that never comes. Now `uint32_t`, 112 bytes, and a `_Static_assert` holds it
there. The byte order of those fields changed with it — `be32` where it used to
be `be64`.

**417 bytes — `COUNTERS_DPM_52`, 52 RX/TX counter pairs.** New. Each of DPM-1..5
(VL 2021, 2042, 2063, 2084, 2105) sends one, 52 × {`rx_count`, `tx_count`} as
big-endian `uint32`, 416 bytes plus the sequence trailer.

The second one behaves unlike every other report: **the packet carries a second's
counts, not a total.** So the dashboard adds each packet to a running sum per DPM
(`dpm52_accumulate`, called for every drained packet before de-duplication, so two
packets from one DPM in one tick both land) and prints the cumulative figure.
A printer that showed the packet's own numbers would look perfectly reasonable and
be wrong by however long the run had been going.
