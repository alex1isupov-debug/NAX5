"""Exercise the real Django multipart parser/storage with C++ generated ZIPs.

Database association is mocked: this is a wire/storage contract test, NOT a
PostgreSQL integration test. It never connects to a database or a remote API.
"""
import argparse
from pathlib import Path
import os
import sys
import tempfile
from types import SimpleNamespace
from unittest.mock import patch

parser = argparse.ArgumentParser()
parser.add_argument("backend", type=Path)
parser.add_argument("fixtures", type=Path)
args = parser.parse_args()
sys.path.insert(0, str(args.backend.resolve()))
os.environ.setdefault("DJANGO_SETTINGS_MODULE", "config.settings")
import django
django.setup()
from django.core.files.uploadedfile import SimpleUploadedFile
from django.test import RequestFactory, override_settings
from apps.client_reports import views

factory = RequestFactory()
user = SimpleNamespace(pk=123)
session = SimpleNamespace(public_id="session-A")
archives = sorted(args.fixtures.glob("*.zip"))
assert archives, "No C++ archive fixtures"
with tempfile.TemporaryDirectory() as root:
    with override_settings(CLIENT_REPORTS_DIR=root, CLIENT_REPORT_MAX_ARCHIVE_BYTES=2 * 1024 * 1024):
        for path in archives:
            data = path.read_bytes()
            request = factory.post("/api/v1/client-reports/", {
                "kind": "quit", "client_version": "alpha-0.5-build-7-rc1", "client_sha": "contract-test",
                "session_id": "session-A", "archive": SimpleUploadedFile("report.zip", data, "application/zip")})
            with patch.object(views, "_resolve_session", return_value=session) as resolve:
                with patch.object(views, "_store_report", return_value=SimpleNamespace(public_id="test")) as store:
                    response = views._handle_archive_report(request, user)
                    assert response.status_code == 201, (path.name, response.status_code)
                    resolve.assert_called_once_with(user, "session-A")
                    saved = store.call_args.kwargs
                    assert saved["session"] is session
                    assert (Path(root) / saved["storage_path"]).read_bytes() == data
                    assert saved["stored_size"] == len(data)
        request = factory.post("/api/v1/client-reports/", {"kind": "quit", "archive": SimpleUploadedFile("big.zip", b"PK\x03\x04" + b"x" * (2 * 1024 * 1024))})
        assert views._handle_archive_report(request, user).status_code == 413
print(f"PASS: {len(archives)} C++ parts accepted and stored byte-for-byte; oversized ZIP rejected")
print("NOT TESTED: PostgreSQL transaction, native authentication, public edge, VPS disk durability")
