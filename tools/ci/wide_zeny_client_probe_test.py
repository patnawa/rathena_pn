"""PE parser boundaries and refusal to infer economic support from metadata."""
import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("zeny_probe", ROOT / "client-patch/wide_zeny/probe.py")
PROBE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PROBE)


def fixture():
    data = bytearray(1024)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 60, 128)
    data[128:132] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", data, 132, 332, 1, 0, 0, 0, 224, 0)
    struct.pack_into("<H", data, 152, 0x10B)
    struct.pack_into("<I", data, 168, 0x1000)
    struct.pack_into("<I", data, 244, 16)
    struct.pack_into("<8sIIIIIIHHI", data, 376, b".text", 512, 0x1000, 512, 512, 0, 0, 0, 0, 0x60000020)
    return data


class ProbeTests(unittest.TestCase):
    def test_normal_pe_does_not_claim_support(self):
        report = PROBE.inspect_pe(fixture())
        self.assertEqual(report["entry_section"], ".text")
        self.assertFalse(report["wide_zeny"]["enabled"])
        self.assertEqual(report["wide_zeny"]["verified_capabilities"], [])

    def test_each_truncated_structure_is_rejected(self):
        for length in (0, 2, 63, 131, 151, 200, 375, 415, 1000):
            with self.subTest(length=length), self.assertRaises(ValueError):
                PROBE.inspect_pe(fixture()[:length])

    def test_optional_header_cannot_overlap_sections(self):
        data = fixture()
        struct.pack_into("<H", data, 148, 96)
        with self.assertRaises(ValueError):
            PROBE.inspect_pe(data)

    def test_protection_marker_is_reported_without_claiming_impossibility(self):
        data = fixture()
        data[376:384] = b".themida"
        report = PROBE.inspect_pe(data)
        self.assertEqual(report["protection_section_names"], [".themida"])
        self.assertFalse(report["wide_zeny"]["enabled"])


if __name__ == "__main__":
    unittest.main()
