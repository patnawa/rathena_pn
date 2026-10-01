"""Compile the actual storage codec with sanitizers and exercise adversarial inputs."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='receipt-codec-') as directory:
    binary=Path(directory)/'test'
    subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                    '-I'+str(ROOT/'src'),str(Path(__file__).with_suffix('.cpp')),'-lz','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
