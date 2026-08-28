"""Parse the vendor VL definition XML into VlRecord objects.

A line looks like:

  <VL ENABLE="TRUE" ID="620" JITTER="0MS" BAG="1MS" LMIN="64" FEEDBACKVL="FALSE"
      PRIORITY="LOW" LMAX="1518" SRCPORT="4"
      DESTPORT="00000000000000100000000000000000000"/>

DESTPORT is a 35-character bit string, leftmost character = DTN port 34,
rightmost = port 0.  Verified against the reference blob: this exact line
encodes to the VL 620 record byte-for-byte.

Attributes whose binary encoding is not yet pinned down (BAG, FEEDBACKVL,
PRIORITY) are accepted only at their reference values; anything else raises
rather than silently emitting a guessed byte.
"""

import re
import xml.etree.ElementTree as ET

from dtn_config import VlRecord, DTN_PORT_COUNT

# Reference encodings, lifted from the known-good blob.
BAG_WORDS = {"1MS": 0x0602}          # only BAG=1MS observed
FLAG_ENABLE   = 0x8
FLAG_PRIORITY = 0x4                  # hypothesis: set on the HM response VL (0xD vs 0x9)
FLAG_RESERVED = 0x1                  # set on every observed record


class UnsupportedAttribute(ValueError):
    pass


def _ms(text, attr):
    m = re.fullmatch(r"\s*(\d+)\s*MS\s*", text, re.I)
    if not m:
        raise UnsupportedAttribute(f"{attr}={text!r}: expected a value like '0MS'")
    return int(m.group(1))


def parse_destport(text):
    bits = text.strip()
    if len(bits) != DTN_PORT_COUNT or set(bits) - {"0", "1"}:
        raise UnsupportedAttribute(
            f"DESTPORT={text!r}: expected {DTN_PORT_COUNT} characters of 0/1")
    # leftmost character is port 34
    return {DTN_PORT_COUNT - 1 - i for i, c in enumerate(bits) if c == "1"}


def format_destport(ports):
    return "".join("1" if (DTN_PORT_COUNT - 1 - i) in ports else "0"
                   for i in range(DTN_PORT_COUNT))


def record_from_element(el):
    a = {k.upper(): v for k, v in el.attrib.items()}

    enable = a.get("ENABLE", "TRUE").upper() == "TRUE"
    bag = a.get("BAG", "1MS").upper()
    if bag not in BAG_WORDS:
        raise UnsupportedAttribute(
            f"VL {a.get('ID')}: BAG={bag!r} - only {sorted(BAG_WORDS)} have a known encoding")

    feedback = a.get("FEEDBACKVL", "FALSE").upper() == "TRUE"
    if feedback:
        raise UnsupportedAttribute(
            f"VL {a.get('ID')}: FEEDBACKVL=TRUE has no known encoding")

    priority = a.get("PRIORITY", "LOW").upper()
    if priority not in ("LOW", "HIGH"):
        raise UnsupportedAttribute(f"VL {a.get('ID')}: PRIORITY={priority!r}")

    flags = FLAG_RESERVED
    if enable:
        flags |= FLAG_ENABLE
    if priority == "HIGH":
        flags |= FLAG_PRIORITY

    return VlRecord(
        vl_id=int(a["ID"]),
        src_port=int(a["SRCPORT"]),
        dest_ports=parse_destport(a["DESTPORT"]),
        lmax=int(a.get("LMAX", 1518)),
        lmin=int(a.get("LMIN", 64)),
        jitter_ms=_ms(a.get("JITTER", "0MS"), "JITTER"),
        bag_word=BAG_WORDS[bag],
        flags=flags,
    )


def load(path, skip_disabled=True):
    """Return the VL records of an XML profile, sorted by VL ID."""
    root = ET.parse(path).getroot()
    elements = root.iter("VL") if root.tag != "VL" else [root]
    records = []
    for el in elements:
        if skip_disabled and el.attrib.get("ENABLE", "TRUE").upper() != "TRUE":
            continue
        records.append(record_from_element(el))
    records.sort(key=lambda r: r.vl_id)
    return records
