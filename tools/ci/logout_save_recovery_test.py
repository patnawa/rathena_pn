"""Run the production logout/reconnect path with a lossy transport and torn-down session."""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'

def run(work):
    work.mkdir(parents=True, exist_ok=True)
    text = (ROOT / 'src/map/chrif.cpp').read_text()
    signatures = [
        'bool chrif_save_available(', 'bool chrif_save_packet(',
        'static bool chrif_sd_to_auth(', 'static bool chrif_send_retained_final(',
        'static bool chrif_send_retained_logout(', 'static void chrif_save_barrier_ack(',
        'static void chrif_save_dependencies(',
        'bool chrif_auth_achievement_saved(', 'static bool chrif_auth_logout(',
        'int32 chrif_save(', 'static int32 chrif_reconnect(',
    ]
    (work / 'logout-production.inc').write_text('\n'.join(function(text, s) for s in signatures) +
        function((ROOT / 'src/char/char_mapif.cpp').read_text(), 'static int32 chmapif_parse_logout_barrier('))
    subprocess.run(['g++', '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize=alignment', '-fno-sanitize-recover=all',
                    '-I' + str(ROOT / 'src'), '-I' + str(work),
                    str(Path(__file__).with_suffix('.cpp')), '-o', str(work / 'logout-test')], check=True)
    subprocess.run([str(work / 'logout-test')], check=True)
    serializers = (ROOT / 'src/map/intif.cpp').read_text()
    (work / 'logout-serializers.inc').write_text('\n'.join(function(serializers, signature) for signature in (
        'bool intif_storage_save(', 'int32 intif_quest_save(', 'int32 intif_save_petdata(',
        'int32 intif_homunculus_requestsave(', 'int32 intif_mercenary_save(', 'int32 intif_elemental_save(',
    )))
    subprocess.run(['g++', '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize=alignment', '-fno-sanitize-recover=all',
                    '-I' + str(ROOT / 'src'), '-I' + str(work),
                    str(ROOT / 'tools/ci/logout_save_serializers_test.cpp'),
                    '-o', str(work / 'logout-serializers')], check=True)
    subprocess.run([str(work / 'logout-serializers')], check=True)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir:
        run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='logout-save-') as directory:
            run(Path(directory))
