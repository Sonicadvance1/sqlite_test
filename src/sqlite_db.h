#pragma once
#include <sqlite3.h>

#include <chrono>
#include <cstdint>

namespace Sqlite {
	void Init();
	void Shutdown();

	struct DB {
		sqlite3 *db{};

		// Statement handlers
		sqlite3_stmt *InsertCode;
	};

	DB Open();
	void Close(DB *db);

	struct Code {
		uint8_t *Data;
		uint64_t Size;
	};

	struct Timing {
		uint64_t Reset, Bind, Step;
	};

	Timing InsertCode(DB &db, uint64_t RIP, Code const &Code);
}
