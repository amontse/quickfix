#ifndef HAVE_SQLITE3
#error SQLiteStore.h included, but HAVE_SQLITE3 not defined
#endif

#ifdef HAVE_SQLITE3
#ifndef FIX_SQLITESTORE_H
#define FIX_SQLITESTORE_H

#include "MessageStore.h"
#include "SessionSettings.h"
#include "sqlite3.h"
#include <fstream>
#include <string>

namespace FIX
{
	static const std::string DEFAULT_DATABASE;

	/// Creates a MySQL based implementation of MessageStore.
	class SQLiteStoreFactory : public MessageStoreFactory
	{
	public:
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
		SQLiteStore(const std::string& database);
		~SQLiteStore();

		bool set(int, const std::string&) EXCEPT(IOException);
		void get(int, int, std::vector < std::string >&) const EXCEPT(IOException);

		int getNextSenderMsgSeqNum() const EXCEPT(IOException);
		int getNextTargetMsgSeqNum() const EXCEPT(IOException);
		void setNextSenderMsgSeqNum(int value) EXCEPT(IOException);
		void setNextTargetMsgSeqNum(int value) EXCEPT(IOException);
		void incrNextSenderMsgSeqNum() EXCEPT(IOException);
		void incrNextTargetMsgSeqNum() EXCEPT(IOException);

		UtcTimeStamp getCreationTime() const EXCEPT(IOException);

		void reset() EXCEPT(IOException);
		void refresh() EXCEPT(IOException);

	private:
		void populateCache();

		MemoryStore m_cache;
	};
} // namespace FIX

#endif // FIX_SQLITESTORE_H
#endif // HAVE_SQLITE3
