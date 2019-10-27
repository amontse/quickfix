#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#ifdef HAVE_SQLITE3

#include "SQLiteLog.h"
#include "SessionID.h"
#include "SessionSettings.h"
#include "Utility.h"
#include "strptime.h"
#include "SQLiteUtils.h"

namespace FIX
{

	const std::string SQLiteLogFactory::DEFAULT_DATABASE = "quickfix.db";

	SQLiteLog::SQLiteLog(const SessionID& s, const std::string& database)
		: m_incomingTable("messages_log"), m_outgoingTable("messages_log"), m_eventTable("event_log")
		, m_db(database, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
		, m_pSessionID(std::move(std::make_unique<SessionID>(s)))
	{
	}

	SQLiteLog::SQLiteLog(const std::string& database)
		: m_incomingTable("messages_log"), m_outgoingTable("messages_log"), m_eventTable("event_log")
		, m_db(database, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
	{
	}

	void SQLiteLog::init()
	{
		m_db.exec("create table if not exists " + m_incomingTable + " ("
			" id INTEGER PRIMARY KEY, "
			" time TEXT NOT NULL, "
			" time_milliseconds INTEGER NOT NULL, "
			" beginstring TEXT, "
			" sendercompid TEXT, "
			" targetcompid TEXT, "
			" session_qualifier TEXT, "
			" text BLOB NOT NULL)");

		m_db.exec("create table if not exists " + m_outgoingTable + " ("
			" id INTEGER PRIMARY KEY, "
			" time TEXT NOT NULL, "
			" time_milliseconds INTEGER NOT NULL, "
			" beginstring TEXT, "
			" sendercompid TEXT, "
			" targetcompid TEXT, "
			" session_qualifier TEXT, "
			" text BLOB NOT NULL)");

		m_db.exec("create table if not exists " + m_eventTable + " ("
			" id INTEGER PRIMARY KEY, "
			" time TEXT NOT NULL, "
			" time_milliseconds INTEGER NOT NULL, "
			" beginstring TEXT, "
			" sendercompid TEXT, "
			" targetcompid TEXT, "
			" session_qualifier TEXT, "
			" text BLOB NOT NULL)");

		m_stmt_insert_incoming_table = std::make_unique<stmt_insert_log>(m_db, m_incomingTable);
		m_stmt_insert_outgoing_table = std::make_unique<stmt_insert_log>(m_db, m_outgoingTable);
		m_stmt_insert_event_table = std::make_unique<stmt_insert_log>(m_db, m_eventTable);

		m_stmt_delete_incoming_table = std::make_unique<stmt_delete_log>(m_db, m_incomingTable);
		m_stmt_delete_outgoing_table = std::make_unique<stmt_delete_log>(m_db, m_outgoingTable);
		m_stmt_delete_event_table = std::make_unique<stmt_delete_log>(m_db, m_eventTable);
	}

	SQLiteLog::~SQLiteLog()
	{
	}

	void SQLiteLog::clear()
	{
		SQLite::Transaction transaction(m_db);

		try
		{
			if (m_pSessionID)
			{
				if (m_stmt_delete_incoming_table)
				{
					m_stmt_delete_incoming_table->exec(
						m_pSessionID->getBeginString().getValue()
						, m_pSessionID->getSenderCompID().getValue()
						, m_pSessionID->getTargetCompID().getValue()
						, m_pSessionID->getSessionQualifier());
				}

				if (m_stmt_delete_outgoing_table)
				{
					m_stmt_delete_outgoing_table->exec(
						m_pSessionID->getBeginString().getValue()
						, m_pSessionID->getSenderCompID().getValue()
						, m_pSessionID->getTargetCompID().getValue()
						, m_pSessionID->getSessionQualifier());
				}

				if (m_stmt_delete_event_table)
				{
					m_stmt_delete_event_table->exec(
						m_pSessionID->getBeginString().getValue()
						, m_pSessionID->getSenderCompID().getValue()
						, m_pSessionID->getTargetCompID().getValue()
						, m_pSessionID->getSessionQualifier());
				}
			}
			else
			{
				if (m_stmt_delete_incoming_table)
				{
					m_stmt_delete_incoming_table->exec();
				}

				if (m_stmt_delete_outgoing_table)
				{
					m_stmt_delete_outgoing_table->exec();
				}

				if (m_stmt_delete_event_table)
				{
					m_stmt_delete_event_table->exec();
				}
			}

			transaction.commit();
		}
		catch (std::exception & e)
		{

		}
	}

	void SQLiteLog::backup()
	{
	}

	void SQLiteLog::insert(const std::string& table, const std::string& value)
	{
		UtcTimeStamp time;
		int year, month, day, hour, minute, second, millis;
		time.getYMD(year, month, day);
		time.getHMS(hour, minute, second, millis);

		char sqlTime[20];
		STRING_SPRINTF(sqlTime, "%d-%02d-%02d %02d:%02d:%02d",
			year, month, day, hour, minute, second);


		if (table == m_incomingTable)
		{
			if (m_pSessionID)
			{
				if (m_stmt_insert_incoming_table)
				{
					m_stmt_insert_incoming_table->exec(sqlTime, millis
						, m_pSessionID->getBeginString().getValue()
						, m_pSessionID->getSenderCompID().getValue()
						, m_pSessionID->getTargetCompID().getValue()
						, m_pSessionID->getSessionQualifier(), value);
				}
			}
			else
			{
				if (m_stmt_insert_incoming_table)
				{
					m_stmt_insert_incoming_table->exec(sqlTime, millis, value);
				}
			}
		}
		else if (table == m_outgoingTable)
		{
			if (m_pSessionID)
			{
				if (m_stmt_insert_outgoing_table)
				{
					m_stmt_insert_outgoing_table->exec(sqlTime, millis
						, m_pSessionID->getBeginString().getValue()
						, m_pSessionID->getSenderCompID().getValue()
						, m_pSessionID->getTargetCompID().getValue()
						, m_pSessionID->getSessionQualifier(), value);
				}
			}
			else
			{
				if (m_stmt_insert_outgoing_table)
				{
					m_stmt_insert_outgoing_table->exec(sqlTime, millis, value);
				}
			}
		}
		else if (table == m_eventTable)
		{
			if (m_pSessionID)
			{
				if (m_stmt_insert_event_table)
				{
					m_stmt_insert_event_table->exec(sqlTime, millis
						, m_pSessionID->getBeginString().getValue()
						, m_pSessionID->getSenderCompID().getValue()
						, m_pSessionID->getTargetCompID().getValue()
						, m_pSessionID->getSessionQualifier(), value);
				}
			}
			else
			{
				if (m_stmt_insert_event_table)
				{
					m_stmt_insert_event_table->exec(sqlTime, millis, value);
				}
			}
		}
	}


	Log* SQLiteLogFactory::create()
	{
		std::string database;

		init(m_settings.get(), database);
		SQLiteLog* result = new SQLiteLog(database);
		initLog(m_settings.get(), *result);
		return result;
	}

	Log* SQLiteLogFactory::create(const SessionID& s)
	{
		std::string database;

		Dictionary settings;
		if (m_settings.has(s))
			settings = m_settings.get(s);

		init(settings, database);
		SQLiteLog* result = new SQLiteLog(s, database);
		initLog(settings, *result);
		return result;
	}

	void SQLiteLogFactory::init(const Dictionary& settings, std::string& database)
	{
		database = DEFAULT_DATABASE;

		if (m_useSettings)
		{
			try { database = settings.getString(SQLITE_LOG_DATABASE); }
			catch (ConfigError&) {}
		}
		else
		{
			database = m_database;
		}
	}

	void SQLiteLogFactory::initLog(const Dictionary& settings, SQLiteLog& log)
	{
		try { log.setIncomingTable(settings.getString(SQLITE_LOG_INCOMING_TABLE)); }
		catch (ConfigError&) {}

		try { log.setOutgoingTable(settings.getString(SQLITE_LOG_OUTGOING_TABLE)); }
		catch (ConfigError&) {}

		try { log.setEventTable(settings.getString(SQLITE_LOG_EVENT_TABLE)); }
		catch (ConfigError&) {}

		log.init();
	}

	void SQLiteLogFactory::destroy(Log* pLog)
	{
		delete pLog;
	}

} // namespace FIX

#endif // HAVE_SQLITE3
