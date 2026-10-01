"""Compile and run the immutable shop-progression companion-frame contract."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

char_inter = (ROOT / 'src/char/inter.cpp').read_text()
map_intif = (ROOT / 'src/map/intif.cpp').read_text()
map_chrif = (ROOT / 'src/map/chrif.cpp').read_text()
for label, text in (('map-to-char', char_inter), ('char-to-map inter', map_intif),
                    ('char-to-map core', map_chrif)):
    if 'packet length' not in text or 'set_eof(fd);' not in text or '< 4' not in text:
        raise AssertionError(f'{label} dynamic framing must fail closed on lengths below its header')
char_mapif = (ROOT / 'src/char/char_mapif.cpp').read_text()
shop_sql = (ROOT / 'src/custom/shop_sql.inc').read_text()
storage = (ROOT / 'src/char/int_storage.cpp').read_text()
if 'pn_shop_progression_disconnect_impl(int32 fd)' not in shop_sql or \
        'void pn_shop_progression_disconnect(int32 fd)' not in storage:
    raise AssertionError('staged progression frames need an explicit disconnect purge')
if char_mapif.count('pn_shop_progression_disconnect(') < 2 or '#include "int_storage.hpp"' not in char_mapif:
    raise AssertionError('every map-server close path must declare and invoke staged-frame purge')

with tempfile.TemporaryDirectory(prefix='pn-shop-progression-wire-') as directory:
    binary = Path(directory) / 'wire-test'
    subprocess.run([
        'g++', '-std=c++17', '-O1', '-fsanitize=address,undefined',
        '-fno-sanitize-recover=all', '-I' + str(ROOT / 'src'),
        str(ROOT / 'tools/ci/shop_progression_wire_test.cpp'), '-o', str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
