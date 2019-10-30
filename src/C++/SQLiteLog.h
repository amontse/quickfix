#ifndef HAVE_SQLITE3
#error SQLiteLog.h included, but HAVE_SQLITE3 not defined
#endif

#ifdef HAVE_SQLITE3
#ifndef FIX_SQLITELOG_H
#define FIX_SQLITELOG_H

#include "Log.h"
#include "SessionSettings.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "SQLiteUtils.h"
#include <limits>
#include <memory>
#include <string>

namespace FIX
{
	class stmt_insert_log
	{
	public:
		stmt_insert_log(SQLite::Database& db, const std::string& table_name)
			: m_stmt(db, std::string("insert into ") + table_name +
				std::string("(time, time_milliseconds, beginstring, sendercompid, targetcompid, session_qualifier, text) values (?,?,?,?,?,?,?)"))
		{}

		int exec(const std::string & time, int subsecond
			, const std::string & beginstring, const std::string & sendercompid
			, const std::string & targetcompid, const std::string & session_qualifier
			, const std::string & value)
		{
			SQLiteStatementReset stmt_reset(m_stmt);

			m_stmt.bind(1, time);
			m_stmt.bind(2, subsecond);
			m_stmt.bind(3, beginstring);
			m_stmt.bind(4, sendercompid);
			m_stmt.bind(5, targetcompid);
			m_stmt.bind(6, session_qualifier);
			m_stmt.bind(7, value.c_str()
				, (value.size() > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(value.size())));

			return m_stmt.exec();
		}

		//all four key values are null
		int exec(const std::string & time, int subsecond, const std::string & value)
		{
			SQLiteStatementReset stmt_reset(m_stmt);

			m_stmt.bind(1, time);
			m_stmt.bind(2, subsecond);
			m_stmt.bind(3);
			m_stmt.bind(4);
			m_stmt.bind(5);
			m_stmt.bind(6);
			m_stmt.bind(7, value.c_str()
				, (value.size() > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(value.size())));

			return m_stmt.exec();
		}
		
	private:
		SQLite::Statement m_stmt;
	};

	class stmt_delete_log
	{
	public:
		stmt_delete_log(SQLite::Database& db, const std::string& table_name)
			: m_stmt(db, std::string("delete from ") + table_name + std::string(" where beginstring=? and " \
				"sendercompid=? and targetcompid=? and session_qualifier=?"))
		{}

		int exec(const std::string& beginstring, const std::string& sendercompid
			, const std::string& targetcompid, const std::string& session_qualifier)
		{
			SQLiteStatementReset stmt_reset(m_stmt);

			m_stmt.bind(1, beginstring);
			m_stmt.bind(2, sendercompid);
			m_stmt.bind(3, targetcompid);
			m_stmt.bind(4, session_qualifier);

			return m_stmt.exec();
		}

		//all four key values are null
		int exec()
		{
			m_stmt.bind(1);
			m_stmt.bind(2);
			m_stmt.bind(3);
			m_stmt.bind(4);
			SQLiteStatementReset stmt_reset(m_stmt);
			return m_stmt.exec();
		}

	private:
		SQLite::Statement m_stmt;
	};

	/// SQLite based implementation of Log.
	class SQLiteLog : public Log
	{
	public:
		SQLiteLog(const SessionID& s, const std::string& database);
		SQLiteLog(const std::string& database);

		~SQLiteLog();

		void clear();
		void backup();
		void setIncomingTable(const std::string& incomingTable)
		{
			m_incomingTable = incomingTable;
		}
		void setOutgoingTable(const std::string& outgoingTable)
		{
			m_outgoingTable = outgoingTable;
		}
		void setEventTable(const std::string& eventTable)
		{
			m_eventTable = eventTable;
		}

		void onIncoming(const std::string& value)
		{
			insert(m_incomingTable, value);
		}
		void onOutgoing(const std::string& value)
		{
			insert(m_outgoingTable, value);
		}
		void onEvent(const std::string& value)
		{
			insert(m_eventTable, value);
		}

		void init();

	private:
		
		void insert(const std::string& table, const std::string& value);

		std::string m_incomingTable;
		std::string m_outgoingTable;
		std::string m_eventTable;
		SQLite::Database m_db;
		std::unique_ptr<SessionID> m_pSessionID;

		std::unique_ptr<stmt_insert_log> m_stmt_insert_incoming_table;
		std::unique_ptr<stmt_insert_log> m_stmt_insert_outgoing_table;
		std::unique_ptr<stmt_insert_log> m_stmt_insert_event_table;

		std::unique_ptr<stmt_delete_log> m_stmt_delete_incoming_table;
		std::unique_ptr<stmt_delete_log> m_stmt_delete_outgoing_table;
		std::unique_ptr<stmt_delete_log> m_stmt_delete_event_table;
	};

	/// Creates a SQLite based implementation of Log.
	class SQLiteLogFactory : public LogFactory
	{
	public:
		static const std::string DEFAULT_DATABASE;

		SQLiteLogFactory(const SessionSettings& settings)
			: m_settings(settings), m_useSettings(true)
		{
		}

		SQLiteLogFactory(const std::string& database)
			: m_database(database), m_useSettings(false)
		{
		}

		SQLiteLogFactory()
			: m_database(DEFAULT_DATABASE), m_useSettings(false)
		{
		}

		Log* create();
		Log* create(const SessionID&);
		void destroy(Log*);

	private:
		void init(const Dictionary& settings, std::string& database);

		void initLog(const Dictionary& settings, SQLiteLog& log);

		SessionSettings m_settings;
		std::string m_database;
		bool m_useSettings;
	};
} //FIX

#endif // FIX_SQLITELOG_H
#endif // HAVE_SQLITE3
