"""Real paid point-shop caller/planner and registry fence boundaries; no SQL here."""
from pathlib import Path
import tempfile
from shop_transaction_native_test import ROOT,run
if __name__=='__main__':
 with tempfile.TemporaryDirectory(prefix='pn-point-native-') as directory:
  run(Path(directory),ROOT/'src/map/npc.cpp',ROOT/'src/map/cashshop.cpp',ROOT/'tools/ci/point_shop_native_test.cpp')
