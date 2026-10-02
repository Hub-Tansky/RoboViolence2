// Creates a SQLite database from a SQL file: seed_db <out.db> <in.sql> [<in.sql>...]
// Used by the build to generate bv2.db and master.db from content-seed/*.sql.
#include <sqlite3.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		std::fprintf(stderr, "usage: seed_db <out.db> <in.sql>...\n");
		return 2;
	}
	std::remove(argv[1]);
	sqlite3* db = nullptr;
	if (sqlite3_open(argv[1], &db) != SQLITE_OK)
	{
		std::fprintf(stderr, "seed_db: cannot open %s\n", argv[1]);
		return 1;
	}
	for (int i = 2; i < argc; ++i)
	{
		std::ifstream in(argv[i], std::ios::binary);
		if (!in)
		{
			std::fprintf(stderr, "seed_db: cannot read %s\n", argv[i]);
			sqlite3_close(db);
			return 1;
		}
		std::stringstream sql;
		sql << in.rdbuf();
		char* err = nullptr;
		if (sqlite3_exec(db, sql.str().c_str(), nullptr, nullptr, &err) != SQLITE_OK)
		{
			std::fprintf(stderr, "seed_db: %s: %s\n", argv[i], err ? err : "error");
			sqlite3_free(err);
			sqlite3_close(db);
			return 1;
		}
	}
	sqlite3_close(db);
	return 0;
}
