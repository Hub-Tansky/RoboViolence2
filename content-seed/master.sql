-- Master server database seed: sqlite3 master.db < content-seed/master.sql
-- Schema recovered from the SQL in MasterServer/Source/src/cMasterServer.cpp (no original dump exists).
-- Column order of BanList follows BV2_BAN_* in cMasterServer.h; no rows are shipped.
CREATE TABLE Settings (Name TEXT PRIMARY KEY, Value TEXT);
CREATE TABLE BanList (ID INTEGER PRIMARY KEY, Date INTEGER, Duration INTEGER, IP TEXT, MAC TEXT, Nick TEXT);
INSERT INTO Settings (Name, Value) VALUES ('DBVersion', '8');
INSERT INTO Settings (Name, Value) VALUES ('AccountURL', '');
-- The web.db game list has its own seed: content-seed/web.sql
