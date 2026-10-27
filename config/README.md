# Config policy

- **Tracked:** `config/*.example.cfg`, defaults and empty placeholders only.
- **Local, untracked:** `config/local/*.cfg`, the user pref path, or environment variables `BV2_MASTER_SERVERS`, `BV2_ACCOUNT_URL`, `BV2_SV_PASSWORD`, `BV2_ADMIN_PASS` (15 characters or more for in-game admin login), `BV2_ADMIN_API_PORT`, `BV2_ADMIN_API_BIND`, `BV2_ADMIN_API_PROXY`, `BV2_PUBLIC_ADDRESS` (server), `BV2_MASTER_BAN_PASS` (master, 15 characters or more).
- **Production** server config and credentials live in a deployment secret store, never in the repo.
- Loader order ([ADR 0007](../docs/decisions/0007-data-root-pref-dir-config-layers.md)): `main/bv2.example.cfg`, pref-dir `bv2.cfg`, `config/local/*.cfg` (beside the package's `main/`, [ADR 0016](../docs/decisions/0016-player-data-lookup-and-package-content.md)) and `--config <file>`, environment (the variables above). The last two layers are never saved. `cl_masterServers` (built in: `master.roboviolence.com:10207`) is never saved and `bv2.cfg` cannot set it; `none` turns it off ([ADR 0028](../docs/decisions/0028-built-in-public-master.md)).
- Never commit a non-empty `sv_password`, `zsv_adminPass`, `cl_password`, `cl_accountPassword`, `cl_accountUsername` or `cl_lastUsedIP`; no public IPs or hosts. `.gitleaks.toml` and the pre-commit hook reject them.
- The build generates the master's `master.db` and `web.db` from `content-seed/*.sql`. The client creates its `bv2.db` in the pref dir on first start (`game/src/Paths.cpp` `openClientDb`); `MasterServers` and `AccountURL` start empty.
