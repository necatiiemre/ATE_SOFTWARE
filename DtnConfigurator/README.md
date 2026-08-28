# DtnConfigurator

Builds DTN (LRU `0x2600`) configuration frames from a VL profile and sends them
over a plain Ethernet raw socket. No DPDK, no fibre, no VLAN — the workstation
only needs the two copper links to the DTN's end-system ports.

```
tools/dtn_config.py       wire format: VL records, config blocks, frame assembly
tools/vl_xml.py           vendor <VL .../> XML  ->  VL records
tools/build_config.py     profile -> the frames to send
tools/verify_reference.py regression test against the known-good configuration
profiles/config1.json     round 1 of 3
```

## Verifying the encoder

`verify_reference.py` re-encodes the 47-frame configuration that
`RemoteConfigSender/main.cpp` ships as hard-coded hex and compares it byte for
byte. That configuration is known to be accepted by the hardware, so a passing
run is the strongest correctness evidence available without the DUT:

```
$ python3 tools/verify_reference.py
...
ALL 47 REFERENCE FRAMES REPRODUCED BYTE-FOR-BYTE
```

## Building a profile

```
$ python3 tools/build_config.py profiles/config1.json --list
$ python3 tools/build_config.py round1.xml --out /tmp/frames
```

## Wire format

See the module docstring in `tools/dtn_config.py`. In short: every frame is
`ETH + IPv4 + UDP(100->100) + AFDX payload + 1 trailing sequence byte`, where the
payload is `LRU_ID(2) OpType(1)` followed by one or more
`Addr(1) Len(2) Data(Len)` blocks separated by a one-byte marker. `OpType` is
`0x57` to write configuration and `0x52` to read device/port status.

The VL table lives at address `0x72`, 14 bytes per VL, and maps one-to-one onto
the vendor XML attributes — `ID`, `SRCPORT`, `DESTPORT` (35-bit mask, port 34
leftmost), `LMIN`, `LMAX`, `JITTER`.

## Not yet pinned down

* `BAG`, `PRIORITY` and `FEEDBACKVL` share the `0x0602` word and the flag nibble
  of the `LMAX` field. Every reference record uses `BAG=1MS PRIORITY=LOW
  FEEDBACKVL=FALSE`, so their encodings cannot be derived from it. `vl_xml.py`
  raises rather than guessing when a profile deviates.
* The end-system VL-count field is written as the number of records. In the
  reference configuration the VL IDs are contiguous, so "count" and "span" are
  indistinguishable there.
* Whether the device accepts `0x57` writes on a copper end-system port. Reads
  are proven (the health monitor polls over `eno12409`); writes have only ever
  been sent over the tagged fibre path. The device's `eth_wrong_op_cnt` /
  `eth_wrong_type_cnt` / `config_id` fields answer this in one round trip.
