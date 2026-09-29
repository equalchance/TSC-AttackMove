"""Export and verify the focused patch against the pinned upstream commit."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = '71a692a5f4a7d61cd1654f2544ee4d17fe473d70'
DEPENDENCIES = {'TSpp': '266750b28539ff039eb9838320bff9da8bc4ee2f',
                'SDL': '63901401cab9051012d85c4bf245bea23687dce0',
                'imgui': 'b61e56346a92cfcaf1f43a545ca37b0b32239654',
                'miniaudio': '7dface14c22c20f704974bfb0c4a5a18ac14f1d8'}


def git(*args, cwd=None):
    return subprocess.check_output(['git', '-C', str(cwd or ROOT / 'engine'), *args])


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--export', action='store_true')
    args = parser.parse_args()
    patches = ROOT / 'patches'
    patches.mkdir(exist_ok=True)
    engine = ROOT / 'engine'
    assert git('rev-parse', 'HEAD').decode().strip() == BASE
    for name, pin in DEPENDENCIES.items():
        assert git('rev-parse', 'HEAD', cwd=engine / name).decode().strip() == pin, name
        assert not git('status', '--porcelain', cwd=engine / name).strip(), name
    # Use an isolated index so exporting newly added source never stages the working tree.
    import os
    with tempfile.TemporaryDirectory(prefix='tscam-index-') as temporary:
        env = os.environ.copy()
        env['GIT_INDEX_FILE'] = str(Path(temporary) / 'index')
        subprocess.run(['git', '-C', str(engine), 'read-tree', BASE], env=env, check=True)
        subprocess.run(['git', '-C', str(engine), 'add', '--', 'src', 'CMakeLists.txt'], env=env, check=True)
        patch = subprocess.check_output(['git', '-C', str(engine), 'diff', '--cached', '--binary', BASE, '--', 'src', 'CMakeLists.txt'], env=env)
    patch_path = patches / 'attack-move.patch'
    if args.export:
        patch_path.write_bytes(patch)
    assert patch_path.read_bytes() == patch, 'Exported patch is stale; rerun with --export.'
    with tempfile.TemporaryDirectory(prefix='tscam-verify-') as temporary:
        dest = Path(temporary)
        baseline = git('archive', BASE)
        with tarfile.open(fileobj=io.BytesIO(baseline)) as archive:
            archive.extractall(dest, filter='data')
        subprocess.run(['git', 'apply', '--check', str(patch_path)], cwd=dest, check=True)
        subprocess.run(['git', 'apply', str(patch_path)], cwd=dest, check=True)
        expected = {p.relative_to(engine / 'src').as_posix(): digest(p.read_bytes().replace(b'\r\n', b'\n')) for p in (engine / 'src').rglob('*') if p.is_file()}
        actual = {p.relative_to(dest / 'src').as_posix(): digest(p.read_bytes().replace(b'\r\n', b'\n')) for p in (dest / 'src').rglob('*') if p.is_file()}
        assert expected == actual, 'Patched source differs from build input.'
        assert (dest / 'CMakeLists.txt').read_bytes().replace(b'\r\n', b'\n') == (engine / 'CMakeLists.txt').read_bytes().replace(b'\r\n', b'\n')
    receipt = {'status': 'PASS', 'baseline_commit': BASE, 'dependencies': DEPENDENCIES,
               'patch_sha256': digest(patch), 'source_file_count': len(expected), 'comparison': 'all source bytes after CRLF to LF normalization'}
    (ROOT / 'validation').mkdir(exist_ok=True)
    (ROOT / 'validation/source.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
