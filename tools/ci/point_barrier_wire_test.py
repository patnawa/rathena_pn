"""Exact login barrier FIFO handler with explicit SQL and ownership doubles."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'src/login/loginchrif.cpp').read_text()
# Guard the production ordering assumed by the registry ingress recovery tests.
map_source=(ROOT/'src/map/chrif.cpp').read_text()
save=map_source[map_source.index('int32 chrif_save('):map_source.index('int32 chrif_connect(')]
assert save.index('chrif_save_dependencies(sd, flag)') < save.index('WFIFOW(char_fd,0) = 0x2b01')
dependencies=map_source[map_source.index('static void chrif_save_dependencies('):map_source.index('int32 chrif_save(')]
assert 'intif_saveregistry(sd)' in dependencies
ready=map_source[map_source.index('void chrif_on_ready('):]
assert ready.index('send_users_tochar()') < ready.index('auth_db->foreach(auth_db,chrif_reconnect)')
start=source.index('static int32 logchrif_point_barrier(')
end=source.index('\nint32 logchrif_parse(',start)
with tempfile.TemporaryDirectory(prefix='pn-point-wire-') as directory:
    path=Path(directory)
    (path/'point_barrier_body.inc').write_text(source[start:end])
    binary=path/'point-wire'
    subprocess.run(['g++','-std=c++17','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'src'),'-I'+str(path),str(ROOT/'tools/ci/point_barrier_wire_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
