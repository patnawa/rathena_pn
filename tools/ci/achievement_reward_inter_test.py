"""Compile the actual map reward send/ACK handlers with deterministic FIFOs."""
import argparse
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def function(path, signature):
    source = path.read_text()
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


def run(build_dir, cxx):
    build_dir.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'src/map/intif.cpp'
    (build_dir / 'achievement-reward-send.inc').write_text(
        function(source, 'int32 intif_achievement_reward('))
    (build_dir / 'achievement-reward-ack.inc').write_text(
        function(source, 'void intif_parse_achievementreward('))
    (build_dir / 'achievement-reward-check.inc').write_text(
        function(ROOT / 'src/map/achievement.cpp', 'void achievement_check_reward('))
    binary = build_dir / 'achievement-reward-inter'
    subprocess.run([
        cxx, '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
        '-fno-sanitize=alignment', '-fno-sanitize-recover=all',
        '-I' + str(build_dir), str(Path(__file__).with_suffix('.cpp')),
        '-o', str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cxx', default='g++')
    args = parser.parse_args()
    if args.build_dir:
        run(args.build_dir.resolve(), args.cxx)
    else:
        with tempfile.TemporaryDirectory(prefix='achievement-reward-inter-') as directory:
            run(Path(directory), args.cxx)


if __name__ == '__main__':
    main()
