#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#ifdef HAVE_SQLITE3

#include "SQLiteStore.h"
#include "SessionID.h"
#include "SessionSettings.h"
#include "FieldConvertors.h"
#include "Parser.h"
#include "Utility.h"
#include "strptime.h"
#include "SQLiteUtils.h"
#include <memory>
#include <limits>

namespace FIX
{
	const std::string SQLiteStoreFactory::DEFAULT_DATABASE = "quickfix.db";

	MessageStore* SQLiteStoreFactory::create(const SessionID& s)
	{
		if (m_useSettings)
		{
			return create(s, m_settings.get(s));
		}
		else if (m_useDictionary)
		{
			return create(s, m_dictionary);
		}
		else
		{
			return new SQLiteStore(s, m_database);
		}
	}

	void SQLiteStoreFactory::destroy(MessageStore* pStore)
	{
		delete pStore;
	}

	MessageStore* SQLiteStoreFactory::create(const SessionID& s, const Dictionary& settings)
	{
		std::string database = DEFAULT_DATABASE;
		
		try { database = settings.getString(SQLITE_STORE_DATABASE); }
		catch (ConfigError&) {}

		return new SQLiteStore(s, database);
	}

	SQLiteStore::SQLiteStore(const SessionID& s, const std::string& database)
		: m_sessionID(s)
		, m_db(database, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
	{
		m_db.exec("create table if not exists sessions ("
			" beginstring TEXT NOT NULL, "
			" sendercompid TEXT NOT NULL, "
			" targetcompid TEXT NOT NULL, "
			" session_qualifier TEXT NOT NULL, "
			" creation_time TEXT NOT NULL, "
			" incoming_seqnum INTEGER NOT NULL, "
			" outgoing_seqnum INTEGER NOT NULL, "
			" PRIMARY KEY(beginstring, sendercompid, targetcompid, session_qualifier))");

		m_db.exec("create table if not exists messages ("
			" beginstring TEXT NOT NULL, "
			" sendercompid TEXT NOT NULL, "
			" targetcompid TEXT NOT NULL, "
			" session_qualifier TEXT NOT NULL, "
			" msgseqnum INTEGER NOT NULL, "
			" message BLOB NOT NULL, "
			" PRIMARY KEY(beginstring, sendercompid, targetcompid, session_qualifier, msgseqnum))");

		m_stmt_insert_messages = std::make_unique<SQLite::Statement>(m_db, "insert into messages "
			"(beginstring, sendercompid, targetcompid, session_qualifier, msgseqnum, message) "
			"values (?,?,?,?,?,?)");
		m_stmt_update_messages = std::make_unique<SQLite::Statement>(m_db, "update messages set message=? where "
			"beginstring = ? and sendercompid = ? and targetcompid = ? and session_qualifier = ? ");
		m_stmt_select_messages = std::make_unique<SQLite::Statement>(m_db, "select message from messages where "
			"beginstring=? and sendercompid=? and targetcompid=? and session_qualifier=? and "
			"msgseqnum>=? and msgseqnum<=?");
		m_stmt_update_outgoing_seqnum = std::make_unique<SQLite::Statement>(m_db, "update sessions set outgoing_seqnum=? where "
			"beginstring=? and sendercompid=? and targetcompid=? and session_qualifier=?");
		m_stmt_update_incoming_seqnum = std::make_unique<SQLite::Statement>(m_db, "update sessions set incoming_seqnum=? where "
			"beginstring=? and sendercompid=? and targetcompid=? and session_qualifier=?");
		m_stmt_delete_messages = std::make_unique<SQLite::Statement>(m_db, "delete from messages where "
			"beginstring=? and sendercompid=? and targetcompid=? and session_qualifier=?");
		m_stmt_update_sessions = std::make_unique<SQLite::Statement>(m_db, "update sessions set creation_time=?, "
			"incoming_seqnum=?, outgoing_seqnum=? where "
			"beginstring=? and sendercompid=? and targetcompid=? and session_qualifier=?");
		m_stmt_select_sessions = std::make_unique<SQLite::Statement>(m_db, "select creation_time, incoming_seqnum, outgoing_seqnum FROM sessions where "
			"beginstring=? and sendercompid=? and targetcompid=? and session_qualifier=?");
		m_stmt_insert_sessions = std::make_unique<SQLite::Statement>(m_db, "insert into sessions (beginstring, sendercompid, targetcompid, session_qualifier,"
			"creation_time, incoming_seqnum, outgoing_seqnum) VALUES(?,?,?,?,?,?,?)");
	}

	SQLiteStore::~SQLiteStore()
	{

	}

	bool SQLiteStore::set(int msgSeqNum, const std::string& msg)
	{
		std::string query;

		try
		{
			SQLiteStatementReset stmt_reset(*m_stmt_insert_messages);

			m_stmt_insert_messages->bind(1, m_sessionID.getBeginString().getValue());
			m_stmt_insert_messages->bind(2, m_sessionID.getSenderCompID().getValue());
			m_stmt_insert_messages->bind(3, m_sessionID.getTargetCompID().getValue());
			m_stmt_insert_messages->bind(4, m_sessionID.getSessionQualifier());
			m_stmt_insert_messages->bind(5, msgSeqNum);
			m_stmt_insert_messages->bind(6, msg.c_str()
				, (msg.size() > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(msg.size())));

			query = m_stmt_insert_messages->getExpandedSQL();

			try
			{
				m_stmt_insert_messages->exec();
			}
			catch (std::exception&)
			{
				SQLiteStatementReset stmt_reset(*m_stmt_update_messages);

				m_stmt_update_messages->bind(1, msg.c_str()
					, (msg.size() > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(msg.size())));
				m_stmt_update_messages->bind(2, m_sessionID.getBeginString().getValue());
				m_stmt_update_messages->bind(3, m_sessionID.getSenderCompID().getValue());
				m_stmt_update_messages->bind(4, m_sessionID.getTargetCompID().getValue());
				m_stmt_update_messages->bind(5, m_sessionID.getSessionQualifier());

				query = m_stmt_update_messages->getExpandedSQL();

				m_stmt_update_messages->exec();
			}
		}
		catch (std::exception & e)
		{
			throw IOException(std::string("Query failed [") + query + std::string("] ") + std::string(e.what()));
		}

		return true;
	}

	void SQLiteStore::get(int begin, int end, std::vector<std::string>& result) const
	{
		result.clear();

		std::string query;

		try
		{
			SQLiteStatementReset stmt_reset(*m_stmt_select_messages);

			m_stmt_select_messages->bind(1, m_sessionID.getBeginString().getValue());
			m_stmt_select_messages->bind(2, m_sessionID.getSenderCompID().getValue());
			m_stmt_select_messages->bind(3, m_sessionID.getTargetCompID().getValue());
			m_stmt_select_messages->bind(4, m_sessionID.getSessionQualifier());
			m_stmt_select_messages->bind(5, begin);
			m_stmt_select_messages->bind(6, end);

			query = m_stmt_select_messages->getExpandedSQL();

			while (m_stmt_select_messages->executeStep())
			{
				SQLite::Column col = m_stmt_select_messages->getColumn(0);
				const void* msg_blob = col.getBlob();
				auto msg_blob_size = col.getBytes();
				std::string msg(static_cast<const char*>(msg_blob), static_cast<const char*>(msg_blob) + msg_blob_size);
				result.push_back(msg);
			}
		}
		catch (std::exception & e)
		{
			throw IOException(std::string("Query failed [") + query + std::string("] ") + std::string(e.what()));
		}
	}

	int SQLiteStore::getNextSenderMsgSeqNum() const
	{
		return m_cache.getNextSenderMsgSeqNum();
	}

	int SQLiteStore::getNextTargetMsgSeqNum() const
	{
		return m_cache.getNextTargetMsgSeqNum();
	}

	void SQLiteStore::setNextSenderMsgSeqNum(int value)
	{
		std::string query;

		try
		{
			SQLiteStatementReset stmt_reset(*m_stmt_update_outgoing_seqnum);
			m_stmt_update_outgoing_seqnum->bind(1, value);
			m_stmt_update_outgoing_seqnum->bind(2, m_sessionID.getBeginString().getValue());
			m_stmt_update_outgoing_seqnum->bind(3, m_sessionID.getSenderCompID().getValue());
			m_stmt_update_outgoing_seqnum->bind(4, m_sessionID.getTargetCompID().getValue());
			m_stmt_update_outgoing_seqnum->bind(5, m_sessionID.getSessionQualifier());

			query = m_stmt_update_outgoing_seqnum->getExpandedSQL();

			m_stmt_update_outgoing_seqnum->exec();

			m_cache.setNextSenderMsgSeqNum(value);
		}
		catch (std::exception & e)
		{
			throw IOException(std::string("Query failed [") + query + std::string("] ") + std::string(e.what()));
		}
	}

	void SQLiteStore::setNextTargetMsgSeqNum(int value)
	{
		std::string query;

		try
		{
			SQLiteStatementReset stmt_reset(*m_stmt_update_incoming_seqnum);

			m_stmt_update_incoming_seqnum->bind(1, value);
			m_stmt_update_incoming_seqnum->bind(2, m_sessionID.getBeginString().getValue());
			m_stmt_update_incoming_seqnum->bind(3, m_sessionID.getSenderCompID().getValue());
			m_stmt_update_incoming_seqnum->bind(4, m_sessionID.getTargetCompID().getValue());
			m_stmt_update_incoming_seqnum->bind(5, m_sessionID.getSessionQualifier());

			query = m_stmt_update_incoming_seqnum->getExpandedSQL();

			m_stmt_update_incoming_seqnum->exec();

			m_cache.setNextTargetMsgSeqNum(value);
		}
		catch (std::exception & e)
		{
			throw IOException(std::string("Query failed [") + query + std::string("] ") + std::string(e.what()));
		}
	}

	void SQLiteStore::incrNextSenderMsgSeqNum()
	{
		m_cache.incrNextSenderMsgSeqNum();
		setNextSenderMsgSeqNum(m_cache.getNextSenderMsgSeqNum());
	}

	void SQLiteStore::incrNextTargetMsgSeqNum()
	{
		m_cache.incrNextTargetMsgSeqNum();
		setNextTargetMsgSeqNum(m_cache.getNextTargetMsgSeqNum());
	}

	UtcTimeStamp SQLiteStore::getCreationTime() const
	{
		return m_cache.getCreationTime();
	}

	void SQLiteStore::reset()
	{
		std::string query;

		try
		{
			SQLiteStatementReset stmt_reset(*m_stmt_delete_messages);
			m_stmt_delete_messages->bind(1, m_sessionID.getBeginString().getValue());
			m_stmt_delete_messages->bind(2, m_sessionID.getSenderCompID().getValue());
			m_stmt_delete_messages->bind(3, m_sessionID.getTargetCompID().getValue());
			m_stmt_delete_messages->bind(4, m_sessionID.getSessionQualifier());

			query = m_stmt_delete_messages->getExpandedSQL();

			m_stmt_delete_messages->exec();
		}
		catch (std::exception & e)
		{
			throw IOException(std::string("Query failed [") + query + std::string("] ") + std::string(e.what()));
		}

		m_cache.reset();
		UtcTimeStamp time = m_cache.getCreationTime();

		int year, month, day, hour, minute, second, millis;
		time.getYMD(year, month, day);
		time.getHMS(hour, minute, second, millis);

		char sqlTime[20];
		STRING_SPRINTF(sqlTime, "%d-%02d-%02d %02d:%02d:%02d",
			year, month, day, hour, minute, second);

		try
		{
			SQLiteStatementReset stmt_reset(*m_stmt_update_sessions);
			m_stmt_update_sessions->bind(1, sqlTime);
			m_stmt_update_sessions->bind(2, m_cache.getNextTargetMsgSeqNum());
			m_stmt_update_sessions->bind(3, m_cache.getNextSenderMsgSeqNum());
			m_stmt_update_sessions->bind(4, m_sessionID.getBeginString().getValue());
			m_stmt_update_sessions->bind(5, m_sessionID.getSenderCompID().getValue());
			m_stmt_update_sessions->bind(6, m_sessionID.getTargetCompID().getValue());
			m_stmt_update_sessions->bind(7, m_sessionID.getSessionQualifier());

			query = m_stmt_update_sessions->getExpandedSQL();

			m_stmt_update_sessions->exec();
		}
		catch (std::exception & e)
		{
			throw IOException(std::string("Query failed [") + query + std::string("] ") + std::string(e.what()));
		}
	}

	void SQLiteStore::refresh()
	{
		m_cache.reset();
		populateCache();
	}

	void SQLiteStore::populateCache()
	{
		std::string query;
		size_t rows = 0;
		struct tm time;
		int incoming_seqnum = 0;
		int outgoing_seqnum = 0;

		try
		{
			SQLiteStatementReset stmt_reset(*m_stmt_select_sessions);

			m_stmt_select_sessions->bind(1, m_sessionID.getBeginString().getValue());
			m_stmt_select_sessions->bind(2, m_sessionID.getSenderCompID().getValue());
			m_stmt_select_sessions->bind(3, m_sessionID.getTargetCompID().getValue());
			m_stmt_select_sessions->bind(4, m_sessionID.getSessionQualifier());

			while (m_stmt_select_sessions->executeStep())
			{
				++rows;

				if (rows > 1)
				{
					break;
				}

				std::string sqlTime = m_stmt_select_sessions->getColumn(0).getText();
				incoming_seqnum = m_stmt_select_sessions->getColumn(1).getInt();
				outgoing_seqnum = m_stmt_select_sessions->getColumn(2).getInt();

				::strptime(sqlTime.c_str(), "%Y-%m-%d %H:%M:%S", &time);
			}
		}
		catch (std::exception&)
		{
			throw ConfigError("No entries found for session in database");
		}

		if (rows > 1)
		{
			throw ConfigError("Multiple entries found for session in database");
		}
		else if (rows == 1)
		{
			m_cache.setCreationTime(UtcTimeStamp(&time));
			m_cache.setNextTargetMsgSeqNum(incoming_seqnum);
			m_cache.setNextSenderMsgSeqNum(outgoing_seqnum);
		}
		else //rows = 0
		{
			UtcTimeStamp time = m_cache.getCreationTime();
			char sqlTime[20];
			int year, month, day, hour, minute, second, millis;
			time.getYMD(year, month, day);
			time.getHMS(hour, minute, second, millis);
			STRING_SPRINTF(sqlTime, "%d-%02d-%02d %02d:%02d:%02d",
				year, month, day, hour, minute, second);

			SQLiteStatementReset stmt_reset(*m_stmt_insert_sessions);

			m_stmt_insert_sessions->bind(1, m_sessionID.getBeginString().getValue());
			m_stmt_insert_sessions->bind(2, m_sessionID.getSenderCompID().getValue());
			m_stmt_insert_sessions->bind(3, m_sessionID.getTargetCompID().getValue());
			m_stmt_insert_sessions->bind(4, m_sessionID.getSessionQualifier());
			m_stmt_insert_sessions->bind(5, sqlTime);
			m_stmt_insert_sessions->bind(6, m_cache.getNextTargetMsgSeqNum());
			m_stmt_insert_sessions->bind(7, m_cache.getNextSenderMsgSeqNum());

			try
			{
				m_stmt_insert_sessions->exec();
			}
			catch (std::exception&)
			{
				throw ConfigError("Unable to create session in database");
			}
		}
	}

} // FIX

#endif // HAVE_SQLITE3
