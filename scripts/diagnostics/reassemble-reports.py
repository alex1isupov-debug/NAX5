#!/usr/bin/env python3
"""Reassemble schema-2 client report ZIPs without extracting untrusted paths.

Usage: python reassemble-reports.py downloaded-user-reports --out new-output-dir
Exit 2 means an incomplete report. No network or database writes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import uuid
import zipfile


def load_part(path):
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        if "MANIFEST.json" not in names:
            return None  # legacy report, not a part of this protocol
        if len(names) != 3 or set(names) != {"BUILD-INFO", "MANIFEST.json", "log-part.txt"}:
            raise ValueError(f"Unexpected members: {path.name}")
        if sum(i.file_size for i in archive.infolist()) > 1536 * 1024:
            raise ValueError(f"Oversized expanded part: {path.name}")
        manifest = json.loads(archive.read("MANIFEST.json"))
        if manifest.get("schema") != 2:
            raise ValueError("Unsupported diagnostic schema")
        uuid.UUID(manifest["report_id"])
        data = archive.read("log-part.txt")
        if hashlib.sha256(data).hexdigest() != manifest["sha256"]:
            raise ValueError(f"Checksum mismatch: {path.name}")
        return manifest, data, archive.read("BUILD-INFO")


def reconstruct(parts):
    by_sequence = {}
    issues = []
    for part in parts:
        manifest, data, _ = part
        sequence = manifest["sequence"]
        if not isinstance(sequence, int) or sequence < 0:
            raise ValueError("Invalid sequence")
        if sequence in by_sequence:
            old = by_sequence[sequence]
            # A lost HTTP acknowledgement may cause an identical retry.
            if old[0]["sha256"] != manifest["sha256"] or old[0].get("source_offset") != manifest.get("source_offset"):
                issues.append(f"conflicting duplicate sequence {sequence}")
        else:
            by_sequence[sequence] = part
    finals = [p for p in by_sequence.values() if p[0].get("final")]
    if len(finals) != 1:
        issues.append("missing or multiple completion manifests")
    final = max(finals, key=lambda p: p[0]["sequence"]) if finals else None
    last_sequence = final[0]["sequence"] if final else max(by_sequence, default=-1)
    if sorted(by_sequence) != list(range(last_sequence + 1)):
        issues.append("missing or out-of-range sequences")
    source_parts = {}
    for manifest, data, _ in by_sequence.values():
        if manifest.get("final"):
            continue
        source = manifest["source"]
        if not source or Path(source).name != source or "\\" in source or ":" in source:
            raise ValueError("Unsafe source name")
        source_parts.setdefault(source, []).append((manifest["source_offset"], manifest["source_bytes"], data))
    expected = {s["source"]: s for s in final[0]["sources"]} if final else {}
    restored = {}
    for name in sorted(set(source_parts) | set(expected)):
        chunks = sorted(source_parts.get(name, []))
        meta = expected.get(name, {})
        if meta.get("missing") or meta.get("truncated"):
            issues.append(f"source missing or truncated: {name}")
        cursor = meta.get("initial_offset", 0)
        output = bytearray()
        for offset, size, data in chunks:
            if offset != cursor:
                issues.append(f"gap or overlap: {name} at {cursor}, got {offset}")
            cursor = offset + size
            output.extend(data)
        if meta and (cursor != meta["end"] or cursor != meta["offset"]):
            issues.append(f"incomplete source: {name}")
        restored[name] = bytes(output)
    samples = []
    for data in restored.values():
        for line in data.decode("utf-8", errors="replace").splitlines():
            if "[nax5.metrics]" not in line:
                continue
            try:
                sample = json.loads(line.split("[nax5.metrics]", 1)[1].strip())
                if sample.get("event") == "stream_sample":
                    samples.append(sample)
            except (ValueError, TypeError):
                issues.append("malformed stream metric")
    samples.sort(key=lambda s: s.get("elapsed_ms", 0))
    summary = {"complete": not issues, "issues": issues, "part_count": len(by_sequence),
               "sample_count": len(samples),
               "interpretation": "Rolling loss samples are not whole-session packet loss; client logs cannot locate a physical network fault."}
    if samples:
        summary["max_rolling_loss_fraction"] = max(s.get("packet_loss_rolling_fraction", 0) for s in samples)
        summary["max_sample_gap_ms"] = max(s.get("sample_gap_ms", 0) for s in samples)
        summary["max_pending_frame_age_ms"] = max(s.get("pending_frame_age_ms", 0) for s in samples)
    build_info = final[2] if final else (parts[-1][2] if parts else b"")
    return restored, summary, samples, build_info


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    groups = {}
    for path in args.input.rglob("*.zip"):
        part = load_part(path)
        if part:
            groups.setdefault(part[0]["report_id"], []).append(part)
    if not groups:
        parser.error("No schema-2 parts found")
    args.out.mkdir(parents=True, exist_ok=False)
    complete = True
    for report_id, parts in groups.items():
        files, summary, samples, build_info = reconstruct(parts)
        target = args.out / str(uuid.UUID(report_id))
        target.mkdir()
        logs = target / "logs"
        logs.mkdir()
        for name, data in files.items():
            (logs / name).write_bytes(data)
        (target / "BUILD-INFO.txt").write_bytes(build_info)
        (target / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
        (target / "metrics.jsonl").write_text("".join(json.dumps(s) + "\n" for s in samples), encoding="utf-8")
        print(report_id, "COMPLETE" if summary["complete"] else "INCOMPLETE", "parts=", len(parts))
        complete &= summary["complete"]
    return 0 if complete else 2


if __name__ == "__main__":
    raise SystemExit(main())
