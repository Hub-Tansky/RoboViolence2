/*
	Copyright 2012 bitHeads inc.

	This file is part of the BaboViolent 2 source code.

	The BaboViolent 2 source code is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by the 
	Free Software Foundation, either version 3 of the License, or (at your option) 
	any later version.

	The BaboViolent 2 source code is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with the 
	BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

// A prepared SQLite statement with bound parameters (Phase B step 6): data is bound, never formatted into SQL.
// The game (game/src) and the master server (masterserver/src) each have a copy.

#ifndef SQLSTATEMENT_H
#define SQLSTATEMENT_H

#include <sqlite3.h>

#include <cstdio>
#include <cstring>
#include <string>

class SqlStatement
{
public:
	SqlStatement(sqlite3 * db, const char * sql)
	{
		if (db && sqlite3_prepare_v2(db, sql, -1, &m_stmt, 0) != SQLITE_OK)
			fprintf(stderr, "SQL: %s (%s)\n", sqlite3_errmsg(db), sql); // schema drift: say so, don't fail silently
	}
	~SqlStatement() { sqlite3_finalize(m_stmt); }
	SqlStatement(const SqlStatement &) = delete;
	SqlStatement & operator=(const SqlStatement &) = delete;

	// Parameters count from 1. Text stops at its '\0' or after max bytes (a fixed-size field from the wire).
	SqlStatement & bindText(int i, const char * text, size_t max = (size_t)-1)
	{
		sqlite3_bind_text(m_stmt, i, text, (int)strnlen(text, max), SQLITE_TRANSIENT);
		return *this;
	}
	// A fixed-size char array (a wire field): bounded by its size, so a missing '\0' can't overread
	template <size_t N> SqlStatement & bindText(int i, const char (&text)[N]) { return bindText(i, text, N); }
	SqlStatement & bindInt(int i, long long value)
	{
		sqlite3_bind_int64(m_stmt, i, value);
		return *this;
	}

	// Steps once: true while a result row is available
	bool row() { return m_stmt && sqlite3_step(m_stmt) == SQLITE_ROW; }
	// Runs a statement without results: true on success
	bool run() { return m_stmt && sqlite3_step(m_stmt) == SQLITE_DONE; }

	// Columns of the current row count from 0
	std::string text(int col) const
	{
		const unsigned char * t = sqlite3_column_text(m_stmt, col);
		return t ? std::string((const char *)t) : std::string();
	}
	long long integer(int col) const { return sqlite3_column_int64(m_stmt, col); }

	// The first column of the first row of a constant query, "" when there is none
	static std::string value(sqlite3 * db, const char * sql)
	{
		SqlStatement s(db, sql);
		return s.row() ? s.text(0) : std::string();
	}

private:
	sqlite3_stmt * m_stmt = 0;
};

#endif
