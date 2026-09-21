import hashlib
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("reassemble", Path(__file__).with_name("reassemble-reports.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def part(sequence, offset, data):
    return ({"sequence": sequence, "source": "session.log", "source_offset": offset,
             "source_bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}, data, b"version=test\n")


class ReassembleTests(unittest.TestCase):
    def fixture(self):
        return [part(0, 0, b"start\n"), part(1, 6, b"end\n"),
                ({"sequence": 2, "final": True, "sha256": hashlib.sha256(b"").hexdigest(),
                  "sources": [{"source": "session.log", "initial_offset": 0, "offset": 10, "end": 10}]}, b"", b"version=test\n")]

    def test_complete_and_identical_retry(self):
        parts = self.fixture()
        files, summary, _, _ = module.reconstruct(parts + [parts[0]])
        self.assertTrue(summary["complete"])
        self.assertEqual(files["session.log"], b"start\nend\n")

    def test_missing_middle_is_not_complete(self):
        parts = self.fixture()
        self.assertFalse(module.reconstruct([parts[0], parts[2]])[1]["complete"])

    def test_missing_final_is_not_complete(self):
        self.assertFalse(module.reconstruct(self.fixture()[:2])[1]["complete"])

    def test_gap_is_not_complete(self):
        parts = self.fixture()
        parts[1][0]["source_offset"] = 8
        self.assertFalse(module.reconstruct(parts)[1]["complete"])

    def test_path_traversal_is_rejected(self):
        parts = self.fixture()
        parts[0][0]["source"] = "../secret"
        with self.assertRaises(ValueError):
            module.reconstruct(parts)


if __name__ == "__main__":
    unittest.main()
