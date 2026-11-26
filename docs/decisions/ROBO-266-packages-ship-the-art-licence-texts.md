# ROBO-266: Packages ship the art licence texts and a NOTICE; the art stays downloaded

- Status: Accepted
- Date: 2026-11-26
- Affects: ROBO-266, `NOTICE.txt`, `packaging/legal/`, `packaging/make-package.py` (`stage_licences`, `installer_text`), `packaging/make_deb.py` (`COPYRIGHT`), `README.md`; follows [ADR ROBO-260](ROBO-260-art-pack-downloads-beside-the-gpl-game.md)

## Context
- ADR ROBO-260 left to the packaging PR how packages carry the licence files. The art pack is downloaded on first start, never packaged, and its archive already holds `LICENSE-ART.txt`, `NOTICE.txt` and `TRADEMARKS.md`.
- A player who installs a package still meets the art (it downloads with no question), so the package should say what that art is and under which licence, before the download.
- Owner, 2026-11-26: licence files in every package, with the owner's legal name in them; no Options switch for the download.

## Decision
- The repo holds `NOTICE.txt` (the program is GPL; the downloaded art pack is a separate proprietary work by Piotr Pełzowski; contact) and verbatim copies of rv2-art's `LICENSE-ART.txt` and `TRADEMARKS.md` in `packaging/legal/`.
- Every package carries `LICENSE.txt`, `NOTICE.txt`, `licenses/LICENSE-ART.txt` and `licenses/TRADEMARKS.md` (the macOS app in `Contents/Resources`, the `.deb` in `/usr/share/doc/roboviolence2/`); `make-package.py` fails a package missing one.
- The Windows installer's info page shows NOTICE.txt, then the GPL, then the art licence, each paragraph on one line. Shown, not agreed to: the GPL needs no acceptance, and the art licence's grants need none either.
- The `.deb` `copyright` file lists `GPL-3+` for its files and a standalone `LicenseRef-RV2-Art` paragraph for the downloaded pack.

## Consequences
- When rv2-art changes `LICENSE-ART.txt` or `TRADEMARKS.md`, the art drop PR copies it into `packaging/legal/`; `installer_licence` compares them when `RV2_ART_DIR` points at an unpacked pack (CI doesn't fetch the art, so the check is manual).
- Rejected: shipping the art pack inside packages (ADR ROBO-260: the public packages build from the GPL repo alone); a click-through acceptance of the art licence (adds a step for no right the player needs).
