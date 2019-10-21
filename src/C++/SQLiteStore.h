#ifndef HAVE_SQLITE3
#error SQLiteStore.h included, but HAVE_SQLITE3 not defined
#endif

#ifdef HAVE_SQLITE3
#ifndef FIX_SQLITESTORE_H
#define FIX_SQLITESTORE_H

#include "MessageStore.h"
#include "SessionSettings.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include <string>

namespace FIX
{
	/// Creates a MySQL based implementation of MessageStore.
	class SQLiteStoreFactory : public MessageStoreFactory
	{
	public:

		static const std::string DEFAULT_DATABASE;

		SQLiteStoreFactory(const SessionSettings& settings)
			: m_settings(settings), m_useSettings(true), m_useDictionary(false)
		{
		}

		SQLiteStoreFactory(const Dictionary& dictionary)
			: m_dictionary(dictionary), m_useSettings(false), m_useDictionary(true)
		{
		}

		SQLiteStoreFactory(const std::string& database) : m_database(database)
		{
		}

		SQLiteStoreFactory() : m_database(DEFAULT_DATABASE)
		{
		}

		MessageStore* create(const SessionID&);
		void destroy(MessageStore*);

	private:
		MessageStore* create(const SessionID& s, const Dictionary&);

		SessionSettings m_settings;
		Dictionary m_dictionary;
		std::string m_database;
		bool m_useSettings;
		bool m_useDictionary;
	};

	class SQLiteStore : public MessageStore
	{
	public:
		SQLiteStore(const SessionID& s, const std::string& database);
		~SQLiteStore();

		bool set(int, const std::string&);
		void get(int, int, std::vector <std::string>&) const;

		int getNextSenderMsgSeqNum() const;
		int getNextTargetMsgSeqNum() const;
		void setNextSenderMsgSeqNum(int value);
		void setNextTargetMsgSeqNum(int value);
		void incrNextSenderMsgSeqNum();
		void incrNextTargetMsgSeqNum();

		UtcTimeStamp getCreationTime() const;

		void reset();
		void refresh();

	private:
		void populateCache();

		MemoryStore m_cache;
		SessionID m_sessionID;
		SQLite::Database m_db;
		mutable SQLite::Statement m_stmt_insert_messages;
		mutable SQLite::Statement m_stmt_update_messages;
		mutable SQLite::Statement m_stmt_select_messages;
		mutable SQLite::Statement m_stmt_update_outgoing_seqnum;
		mutable SQLite::Statement m_stmt_update_incoming_seqnum;
		mutable SQLite::Statement m_stmt_delete_messages;
		mutable SQLite::Statement m_stmt_update_sessions;
		mutable SQLite::Statement m_stmt_select_sessions;
		mutable SQLite::Statement m_stmt_insert_sessions;
	};
} // namespace FIX

#endif // FIX_SQLITESTORE_H
#endif // HAVE_SQLITE3
