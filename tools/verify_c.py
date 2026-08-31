"""Compile and execute host-only C examples with an explicitly supplied Zig.

Usage: python tools/verify_c.py --zig /path/to/zig
No download or system installation is performed by this script.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / 'video-driver-study/examples'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--zig', required=True)
    args = parser.parse_args()
    zig = str(Path(args.zig).resolve())
    output = ROOT / 'tmp/c-verification'
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env['ZIG_GLOBAL_CACHE_DIR'] = str(ROOT / 'tmp/zig-cache')
    env['ZIG_LOCAL_CACHE_DIR'] = str(ROOT / 'tmp/zig-local')
    version = subprocess.check_output([zig, 'version'], text=True, env=env).strip()
    # Zig's optimized mode may define NDEBUG: keep assert-based tests enabled.
    flags = ['-std=c17', '-Wall', '-Wextra', '-Werror', '-Wconversion', '-pedantic', '-UNDEBUG']
    cases = [
        ('memory_io_debug', ['memory_io/mem_io.c', 'memory_io/raw_uart.c',
                              'memory_io/test_memory_io.c'], '-O0', 'memory_io tests passed'),
        ('memory_io_optimized', ['memory_io/mem_io.c', 'memory_io/raw_uart.c',
                                  'memory_io/test_memory_io.c'], '-O2', 'memory_io tests passed'),
        ('measurement_decode', ['measurement_decode.c'], '-O2', 'Representative decoder checks passed.'),
        ('variadic_demo', ['variadic_demo.c'], '-O2', '10\n20\n30\n99'),
    ]
    results = []
    for name, sources, optimization, expected in cases:
        executable = output / (name + ('.exe' if os.name == 'nt' else ''))
        command = [zig, 'cc', *flags, optimization,
                   *[str(EXAMPLES / src) for src in sources], '-o', str(executable)]
        built = subprocess.run(command, capture_output=True, text=True, env=env, timeout=180)
        if built.returncode:
            raise RuntimeError(f'{name} compile failed:\n{built.stdout}\n{built.stderr}')
        ran = subprocess.run([str(executable)], capture_output=True, text=True, timeout=20)
        if ran.returncode or expected not in ran.stdout:
            raise RuntimeError(f'{name} execution failed:\n{ran.stdout}\n{ran.stderr}')
        results.append({'name': name, 'flags': flags + [optimization],
                        'compile_exit': built.returncode, 'run_exit': ran.returncode,
                        'stdout': ran.stdout.strip(),
                        'source_sha256': {src: hashlib.sha256((EXAMPLES/src).read_bytes()).hexdigest()
                                          for src in sources}})
        print(name + ': PASS', flush=True)
    report = {'compiler': 'Zig cc ' + version, 'host': 'Windows x86_64' if os.name == 'nt' else os.name,
              'hardware_tested': False, 'date': '2026-09-01', 'results': results,
              'not_tested': ['real memory-command backend', 'real chip map', 'board bring-up',
                             'STM32 HAL project', 'all standalone cheatsheet fragments', 'C++ examples']}
    (EXAMPLES/'memory_io/verification.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8', newline='\n')


if __name__ == '__main__':
    main()
