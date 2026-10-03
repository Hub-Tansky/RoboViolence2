-- Client database seed: sqlite3 bv2.db < content-seed/bv2.sql
-- No master servers or account endpoint are shipped. Add them locally (see config/README.md).
CREATE TABLE LauncherSettings (Name TEXT, Value TEXT);
CREATE TABLE MasterServers (Score NUMERIC, ID INTEGER PRIMARY KEY, IP TEXT, Location TEXT, Port NUMERIC);
INSERT INTO LauncherSettings (Name, Value) VALUES ('Version', '2.11');
INSERT INTO LauncherSettings (Name, Value) VALUES ('DBVersion', '8');
INSERT INTO LauncherSettings (Name, Value) VALUES ('AccountURL', '');
INSERT INTO LauncherSettings (Name, Value) VALUES ('DidSurvey', '1');
