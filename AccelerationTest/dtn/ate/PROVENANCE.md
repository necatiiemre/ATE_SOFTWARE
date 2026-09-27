# Where these files came from

`HealthMonitor.c`, `HealthMonitor.h` and `HealthTypes.h` are copies, byte for
byte, of the ATE software's DTN health monitor. `PsuTelemetryReceiver.h` and
`ShutdownSnapshot.h` beside them are stand-ins, not copies — see their own
comments.

| | |
|---|---|
| repository | `necatiiemre/update-deneme` |
| branch | `claude/peaceful-johnson-9xhutk` |
| commit | `8df47cc` — *dpdk_cmc HM: decode the 113-byte CPU usage packet as Pcs_profile_stats* |
| taken from | `dpdk/src/HealthMonitor/HealthMonitor.c`, `dpdk/include/HealthMonitor.h`, `dpdk/include/HealthTypes.h` |

The files use CRLF line endings there, and the copies keep them. `make ate-diff`
is a byte-exact comparison, so normalising them would break the one check that
says the copy is still a copy:

```
make ate-diff ATE_REF=/path/to/update-deneme/dpdk
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
