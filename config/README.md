# Config policy

- **Tracked:** `config/*.example.cfg`, defaults and empty placeholders only.
- **Local, untracked:** `config/local/*.cfg`, the user pref path, or environment variables `BV2_MASTER_SERVERS`, `BV2_ACCOUNT_URL`, `BV2_ADMIN_PASS`.
- **Production** server config and credentials live in a deployment secret store, never in the repo.
- Loader order (step 4 §4.5): example defaults, pref-path `bv2.cfg`, `*.local.cfg`, environment.
- Never commit a non-empty `sv_password`, `zsv_adminPass`, `cl_password`, `cl_accountPassword`, `cl_accountUsername` or `cl_lastUsedIP`; no public IPs or hosts. `.gitleaks.toml` and the pre-commit hook reject them.
- Databases are generated from `content-seed/*.sql` (`sqlite3 bv2.db < content-seed/bv2.sql`); `MasterServers` and `AccountURL` start empty.
