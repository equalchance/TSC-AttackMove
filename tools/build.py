"""Build the pinned x86 Vinifera port with Visual Studio 2022 and CMake."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def environment():
    env = {k.upper(): v for k, v in os.environ.items()}
    vswhere = Path(env.get('PROGRAMFILES(X86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    install = Path(subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip())
    setup = install / 'VC/Auxiliary/Build/vcvarsall.bat'
    result = subprocess.run(f'"{setup}" x86 >nul && set', shell=True, capture_output=True, text=True, check=True)
    for line in result.stdout.splitlines():
        if '=' in line and not line.startswith('='):
            key, value = line.split('=', 1)
            env[key.upper()] = value
    cmake_root = install / 'Common7/IDE/CommonExtensions/Microsoft/CMake'
    compiler = Path(env['VCTOOLSINSTALLDIR']) / 'bin/Hostx64/x86'
    sdk = Path(env['WINDOWSSDKDIR']) / 'bin' / env['WINDOWSSDKVERSION'].rstrip('\\/') / 'x86'
    env['PATH'] = ';'.join(map(str, [compiler, sdk, cmake_root / 'CMake/bin', cmake_root / 'Ninja'])) + ';' + env.get('PATH', '')
    return env


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'build')
    parser.add_argument('--jobs', type=int, default=6)
    parser.add_argument('--configure-only', action='store_true')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    env = environment()
    cmake = shutil.which('cmake', path=env['PATH'])
    ninja = shutil.which('ninja', path=env['PATH'])
    if not cmake or not ninja:
        raise SystemExit('Install the Visual Studio CMake tools component.')
    # A failed first compiler probe can cache empty configuration flags. Refresh
    # that cache so a nominal release cannot silently become an unoptimized dev DLL.
    cache = out / 'CMakeCache.txt'
    refresh = cache.exists() and 'CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=\n' in cache.read_text()
    commands = [[cmake, *(['--fresh'] if refresh else []), '-S', str(ROOT / 'engine'), '-B', str(out), '-G', 'Ninja',
                 '-DCMAKE_BUILD_TYPE=RelWithDebInfo', '-DOPTION_TS_CLIENT_BUILD=ON', '-DOPTION_RELEASE_BUILD=ON',
                 '-DCMAKE_MAKE_PROGRAM=' + ninja]]
    if not args.configure_only:
        commands.append([cmake, '--build', str(out), '--target', 'Vinifera', '-j', str(args.jobs)])
    for index, command in enumerate(commands):
        log = out / ('configure.log' if index == 0 else 'build.log')
        with log.open('w', encoding='utf-8') as stream:
            result = subprocess.run(command, env=env, stdout=stream, stderr=subprocess.STDOUT)
        print(f'{log.name}: exit {result.returncode}', flush=True)
        if result.returncode:
            print(log.read_text(errors='replace')[-12000:])
            raise SystemExit(result.returncode)
    if not args.configure_only:
        dll = out / 'Vinifera.dll'
        database = json.loads((out / 'compile_commands.json').read_text())
        command = next(row['command'] for row in database if row['file'].endswith('attackmove.cpp'))
        assert 'NDEBUG' in command and '/O1' in command and any(flag in command.split() for flag in ('/MD', '-MD')), 'Release flags missing from engine compilation.'
        receipt = {'status': 'COMPILE_LINK_PASS_RUNTIME_PENDING', 'dll_sha256': sha(dll),
                   'sources': {p.relative_to(ROOT / 'engine').as_posix(): sha(p) for p in sorted((ROOT / 'engine/src').rglob('*')) if p.is_file()},
                   'multiplayer_verified': False}
        (out / 'build-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
        print('Built ' + str(dll), flush=True)


if __name__ == '__main__':
    main()
