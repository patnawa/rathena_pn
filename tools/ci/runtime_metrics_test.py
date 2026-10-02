"""Bounded histogram, pending lifecycle and hot-path timing smoke test."""
from pathlib import Path
from datetime import datetime, timezone
import importlib.util
import json
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='pn-metrics-') as directory:
    binary=Path(directory)/'metrics'
    subprocess.run(['g++','-std=c++17','-O2','-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all','-I'+str(ROOT/'src'),
                    str(ROOT/'tools/ci/runtime_metrics_test.cpp'),'-o',str(binary)],check=True)
    result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
    print(result.stdout, end='')
    spec = importlib.util.spec_from_file_location('health', ROOT/'tools/admin/health_check.py')
    health = importlib.util.module_from_spec(spec); spec.loader.exec_module(health)
    # Feed the actual producer output to the deployed consumer, rather than a
    # hand-maintained JSON fixture that can drift from Runtime.report().
    reports = [line for line in result.stdout.splitlines() if line.startswith('{')]
    assert len(reports) == 2, result.stdout
    for report in reports:
        data = json.loads(report)
        now = datetime.fromtimestamp(data['utc'], timezone.utc)
        status = health.metrics_status('[Info]: PN_METRICS '+report, now,
                                       300, max_pending_seconds=60)
        assert status['passed'], status
        strict = health.metrics_status('[Info]: PN_METRICS '+report, now, 300)
        assert strict['passed'] == (data['shop_pending'] == 0), strict
    print('METRICS_HEALTH_INTEGRATION_PASS')
