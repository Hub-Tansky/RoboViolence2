"""packaging/make-package.py reflow_licence: the Windows installer's licence text keeps every word of LICENSE.txt,
one line per paragraph except centred headings (ctest installer_licence)."""
import importlib.util
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / "packaging"))  # make-package.py imports make_deb from its folder
spec = importlib.util.spec_from_file_location("make_package", root / "packaging" / "make-package.py")
make_package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(make_package)

licence = (root / "LICENSE.txt").read_text(encoding="utf-8")
reflowed = make_package.reflow_licence(licence)
failures = []
if reflowed.split() != licence.split():
    failures.append("the reflowed text lost, added or reordered words")
if licence.count("\n\n") != reflowed.count("\n\n"):
    failures.append("paragraph breaks changed")
unjoined = [p[:40] for p in reflowed.split("\n\n") if not p.startswith(" " * 10) and "\n" in p]
if unjoined:
    failures.append(f"paragraphs not on one line: {unjoined[:3]}")
for failure in failures:
    print("FAIL:", failure)
sys.exit(1 if failures else 0)
