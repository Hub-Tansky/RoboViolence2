"""packaging/make-package.py reflow_licence and installer_text: the Windows installer's info page keeps every word of
NOTICE.txt, LICENSE.txt and the art licence, one line per paragraph except centred headings; the legal texts every
package ships exist (ctest installer_licence). With RV2_ART_DIR set to an unpacked art pack, the copies in
packaging/legal/ must match its own."""
import importlib.util
import os
import pathlib
import shutil
import sys
import tempfile

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

# The installer page: NOTICE, then the GPL, then the art licence, every word kept
with tempfile.TemporaryDirectory() as tmp:
    stage = pathlib.Path(tmp)
    (stage / "licenses").mkdir()
    shutil.copy2(root / "LICENSE.txt", stage)
    shutil.copy2(root / "NOTICE.txt", stage)
    for f in make_package.LEGAL:
        shutil.copy2(root / "packaging" / "legal" / f, stage / "licenses")
    page = make_package.installer_text(stage)
notice = (root / "NOTICE.txt").read_text(encoding="utf-8")
art = (root / "packaging" / "legal" / "LICENSE-ART.txt").read_text(encoding="utf-8")
if page.split() != (notice + licence + art).split():
    failures.append("the installer page is not NOTICE.txt, LICENSE.txt and LICENSE-ART.txt in that order")
if not page.startswith("Robo Violence 2\n") or "ROBO VIOLENCE 2 ART LICENCE" not in page:
    failures.append("the installer page lacks the notice or the art licence")
if "Piotr Pełzowski" not in notice or "floss@pelzowski.eu" not in notice:
    failures.append("NOTICE.txt lacks the owner's name or contact")

art_dir = os.environ.get("RV2_ART_DIR")
if art_dir:
    for f in make_package.LEGAL:
        if (pathlib.Path(art_dir) / f).read_bytes() != (root / "packaging" / "legal" / f).read_bytes():
            failures.append(f"packaging/legal/{f} differs from {art_dir}/{f}")

for failure in failures:
    print("FAIL:", failure)
sys.exit(1 if failures else 0)
