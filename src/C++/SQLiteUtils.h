#ifndef HAVE_SQLITE3
#error SQLiteUtils.h included, but HAVE_SQLITE3 not defined
#endif

#ifdef HAVE_SQLITE3
#ifndef FIX_SQLITEUTILS_H
#define FIX_SQLITEUTILS_H

#include "SQLiteCpp/SQLiteCpp.h"

namespace FIX
{
	struct SQLiteStatementReset
	{
		SQLiteStatementReset(SQLite::Statement& stmt) : m_raii(&stmt) {}

		struct statement_reset
		{
			void operator()(SQLite::Statement* stmt)
			{
				if (stmt != 0)
				{
					stmt->tryReset();
					stmt->clearBindings();
					//std::cout << "Statement " << stmt->getQuery() << " is reset." << std::endl;
				}
			}
		};

	private:
		std::unique_ptr<SQLite::Statement, statement_reset> m_raii;
	};

} //FIX

#endif // FIX_SQLITELOG_H
#endif // HAVE_SQLITE3
