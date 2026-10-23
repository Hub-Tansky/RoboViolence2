# 0034: Player IPs stay on the server, personal data on disk expires, and a privacy policy says so

- Status: Accepted
- Date: 2026-10-23
- Affects: `game/src/ServerRecv.cpp`, `game/src/Retention.*`, `game/src/Server.cpp`, `game/src/SceneNet.cpp`, `game/src/AdminApi.cpp`, `website/privacy.html`, `packaging/windows/roboviolence2.iss`, `packaging/make_deb.py`; owner request in the project thread "Player data the game collects" (2026-10-23)

## Context
- The official servers run in the EU, so GDPR applies: player IP addresses, nicknames and chat are personal data. SignPath Foundation's terms ask that software sending user data to systems the user didn't name (the built-in master, ADR 0028) describes it in a privacy policy, shows it during installation and offers a way to turn it off.
- The server copied every player's IP into the player info and player list packets it sends to all clients, and the client's `playerlist` printed them: any player could read everyone's IP.
- Through the master's `CACHE_LIST_REMOTE` relay (`cachelistremote`), the admin of any public server, which anyone can host, can read the nicks and IPs of the last 50 players of any other public server.
- The server's `banlist` (48-byte name and IP records) and `admin-audit.log` grew forever and were copied into the nightly backups.

## Decision
- The server blanks `playerIP` in `NET_CLSV_SVCL_PLAYER_INFO` and `NET_SVCL_PLAYER_ENUM_STATE`. The field stays in the wire structs (no protocol change); old clients see an empty IP. The client's `playerlist` shows names only; the server console's still shows IPs.
- `CACHE_LIST_REMOTE` stays (owner decision, 2026-10-23: keep it for shared bans and disclose it); the policy names as recipients of a server's last 50 players' nicks and IPs anyone who runs a server or connects to the master (the master checks neither who asks nor that they are a game server).
- Bans move to `banlist.tsv` in the pref dir, one line per ban: `since<TAB>ip<TAB>name` (Unix time), owned by `retention::BanList`, which saves every change at once (temporary file renamed over the old one). On start the server reads the old binary `banlist` once when there is no `banlist.tsv`, dates its entries today, writes `banlist.tsv` and deletes `banlist`; a `banlist` left beside an existing `banlist.tsv` is deleted unread. A ban older than `sv_banDays` (default 365, owner-approved; 0 never) no longer matches; it is dropped from the file at the next start. Matching never changes the list, so `banlist` then `unban <index>` stays correct.
- `admin-audit.log` loses lines older than `retention::AUDIT_DAYS` (90): `AdminApi::update` prunes it hourly; writer and pruner share `retention::utcStamp`.
- `website/privacy.html` states what is processed, why, where and for how long; every page's footer and the download page's SignPath note link it. The contact address is assembled by `site.js` from `data-user`/`data-domain`, so the HTML source never holds it.
- The Windows installer shows a "Privacy policy" link on every page and has an unchecked "Privacy" task that writes `config/local/no-master.cfg` (`cl_masterServers "none"`). The `.deb` description links the policy.

## Consequences
- The policy must change with the code: anything new that stores or sends player data (for example `BV2_WITH_HTTP`, ADR 0002, or `sv_report`) needs a policy update first.
- Retention periods in the policy (365 days bans, 90 days admin log, Docker log rotation in `packaging/docker/compose.example.yml`, backup keep 14 daily + 8 weekly) mirror the code and the prod host setup.
- A ban of 0.0.1 counts its year from the first start of this version.
- Not done: the macOS `.dmg` shows only the app and Applications (ADR 0030), so it has no policy link (owner: stays as is).
- Rejected: stopping `CACHE_LIST_REMOTE` (the owner keeps shared bans from remote lists); hashing IPs in the ban list (bans must match the real address, and the hash of an IPv4 address is trivially reversible).
