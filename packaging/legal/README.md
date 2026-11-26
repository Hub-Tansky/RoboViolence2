# Legal texts shipped in every package

`LICENSE-ART.txt` and `TRADEMARKS.md` are verbatim copies of the files at the root of Hub-Tansky/rv2-art (the art pack's licence and the name and logo policy). Packages put them in `licenses/` beside `LICENSE.txt` and `NOTICE.txt` (`packaging/make-package.py`, ADR ROBO-266), since the art pack itself is downloaded, not packaged (ADR ROBO-260). When rv2-art changes either file, copy it here in the same art drop PR; `tests/test_installer_licence.py` checks the copies match the art pack's own when `RV2_ART_DIR` points at an unpacked pack.
