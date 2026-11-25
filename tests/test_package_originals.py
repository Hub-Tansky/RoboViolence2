"""packaging/make-package.py refuses a stage that holds an original BV2 file (ADR ROBO-261): original_files finds a
file by its SHA-256 in the hash list, whatever its name, and passes a clean stage.
Usage: test_package_originals.py <scratch dir>
"""
import hashlib
import importlib.util
import pathlib
import shutil
import sys

root = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / "packaging"))  # make-package imports make_deb
spec = importlib.util.spec_from_file_location("make_package", root / "packaging" / "make-package.py")
make_package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(make_package)

scratch = pathlib.Path(sys.argv[1]) / "package-originals"
shutil.rmtree(scratch, ignore_errors=True)
stage = scratch / "stage"
(stage / "main" / "sounds").mkdir(parents=True)
(stage / "main" / "clean.txt").write_text("clean\n")
failures = []
if make_package.original_files(stage):
    failures.append("a clean stage is refused")
renamed = stage / "main" / "sounds" / "renamed.wav"
renamed.write_bytes(b"pretend original\n")
sha_list = scratch / "hashes.sha256"
sha_list.write_text(hashlib.sha256(renamed.read_bytes()).hexdigest() + "  BaboViolent2/Content/main/sounds/x.wav\n")
if make_package.original_files(stage, sha_list) != ["main/sounds/renamed.wav"]:
    failures.append("an original file under another name is not found")
if len(make_package.original_files(stage)) != 0:
    failures.append("the repo's hash list matches a file it should not")

for f in failures:
    print("FAIL", f)
if not failures:
    print("ok: package originals")
sys.exit(1 if failures else 0)
