"""Bounded histogram, pending lifecycle and hot-path timing smoke test."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='pn-metrics-') as directory:
    binary=Path(directory)/'metrics'
    subprocess.run(['g++','-std=c++17','-O2','-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all','-I'+str(ROOT/'src'),
                    str(ROOT/'tools/ci/runtime_metrics_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
