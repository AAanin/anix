import argparse
import json
import os
import shlex
import shutil
import struct
import subprocess
import sys
import zlib
from pathlib import Path
root = Path(__file__).resolve().parents[1]
os.chdir(root)
os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(root / 'build/zig-global')
os.environ['ZIG_LOCAL_CACHE_DIR'] = str(root / 'build/zig-local')
sys.path.insert(0, str(root))
sys.path.insert(0, str(root / '.tools'))
os.environ['PYTHONPATH'] = str(root / '.tools') + os.pathsep + str(root)
parser = argparse.ArgumentParser()
parser.add_argument('mode', choices=['build', 'test', 'clean'])
parser.add_argument('--cc')
args = parser.parse_args()
if args.mode == 'clean':
    shutil.rmtree(root / 'build', ignore_errors=True); sys.exit(0)
from compiler.builder import build
Path('build').mkdir(exist_ok=True)
graph = json.loads(Path('examples/policy.json').read_text())
b = build(graph)
Path('build/policy.bin').write_bytes(b)
graph['nodes'][1]['action'] = 20
alt = build(graph)
cycle = bytearray(b);struct.pack_into('<I', cycle, 68+8+20, 8)
struct.pack_into('<I', cycle, 20, zlib.crc32(cycle[68:]))
Path('build/fixture.h').write_text('\n'.join('static const uint8_t '+name+'[] = {'+','.join(str(v) for v in blob)+'};' for name, blob in [('fixture',b),('alternate',alt),('cycle_fixture',cycle)]))
zig = root / '.tools/ziglang/zig.exe'
cc = shlex.split(args.cc) if args.cc else ([str(zig), 'cc'] if zig.exists() else ['cc'])
flags = ['-std=c99','-ffreestanding','-Wall','-Wextra','-Werror','-pedantic','-O2','-Iinclude']
def run(command):
    print('+', ' '.join(command), flush=True)
    subprocess.run(command, check=True)
run(cc + flags + ['-c','src/anix.c','-o','build/anix.o'])
if args.mode == 'test':
    run(cc + flags + ['-Ibuild','src/anix.c','tests/test_runtime.c','-o','build/test_runtime.exe'])
    run([str(root / 'build/test_runtime.exe')])
    run([sys.executable,'-m','unittest','discover','-s','tests','-p','test_*.py','-v'])
    run(cc + flags + ['-Ibuild','src/anix.c','tests/benchmark.c','-o','build/benchmark.exe'])
    run([str(root / 'build/benchmark.exe')])
    print('All checks passed; C compiler emitted zero warnings.', flush=True)
