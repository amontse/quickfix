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
#include <fstream>

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
			return new SQLiteStore(m_database);
		}
	}

	void SQLiteStoreFactory::destroy(MessageStore* pStore)
	{
		delete pStore;
	}

	MessageStore* SQLiteStoreFactory::create(const SessionID& s, const Dictionary&)
	{
		std::string database = DEFAULT_DATABASE;
		
		try { database = settings.getString(MYSQL_STORE_DATABASE); }
		catch (ConfigError&) {}

		return new SQLiteStore(database);
	}
} // FIX

#endif // HAVE_SQLITE3
