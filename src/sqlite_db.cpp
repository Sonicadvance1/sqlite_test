#include "sqlite_db.h"

#include <chrono>
#include <cstdint>
#include <format>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace Sqlite {
	void HandleSQLError(int Result) {
		if (Result == SQLITE_OK) return;

		auto ErrStr = std::format("Got sqlite error: {} -> {}\n", Result, sqlite3_errstr(Result));
		write(STDERR_FILENO, ErrStr.c_str(), ErrStr.size());
		__builtin_trap();
	}

	sqlite3_stmt *CompileStatement(sqlite3 *db, std::string_view Statement, bool LongLived) {
		int Res{};
		Res = sqlite3_complete(Statement.data());
		HandleSQLError(Res);

		sqlite3_stmt *stmt;
		uint32_t Flags{};
		if (LongLived) {
			Flags |= SQLITE_PREPARE_PERSISTENT;
		}

		Res = sqlite3_prepare_v3(db,
			Statement.data(),
			Statement.size(), ///< Max length
			Flags, ///< flags
			&stmt,
			nullptr);

		HandleSQLError(Res);
		return stmt;
	}

	void Init() {
		sqlite3_initialize();
	}

	void Shutdown() {
		sqlite3_shutdown();
	}


	void CreateCodeDB(sqlite3 *db) {
		// Basic code db for testing.
		// Layout:
		// - uint64_t:    RIP
		// - binary blob: Code
		const auto TableCreate = R"(
		CREATE TABLE IF NOT EXISTS code(
		rip INT8 PRIMARY KEY,
		code BINARY
		)
		)";
		int Res{};
		auto stmt = CompileStatement(db, TableCreate, false);

		while ((Res = sqlite3_step(stmt)) != SQLITE_DONE) {
			HandleSQLError(Res);
		}
	}

	Sqlite::DB Open() {
		fprintf(stderr, "sqlite version: %s\n", sqlite3_libversion());
		DB db{};

		int Res = sqlite3_open_v2(
			"/tmp/db_test.sqlite",
			&db.db,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
			SQLITE_OPEN_FULLMUTEX |
			//SQLITE_OPEN_MEMORY |
			0,
			nullptr // VFS module to use.
		);
		HandleSQLError(Res);

		// VFS-modules available on linux:
		// - unix-dotfile
		// - unix-excl
		// - unix-none
		// - unix-namedsem (VXWorks only)
		// More documentation: https://www.sqlite.org/vfs.html

		CreateCodeDB(db.db);

		char *Msg;
		Res = sqlite3_exec(db.db, "PRAGMA synchronous = OFF", nullptr, nullptr, &Msg);
		HandleSQLError(Res);
		Res = sqlite3_exec(db.db, "PRAGMA journal_mode = WAL", nullptr, nullptr, &Msg);
		HandleSQLError(Res);

		const auto InsertCode = R"(
		INSERT INTO code(rip, code) VALUES(?1, ?2)
		)";
		db.InsertCode = CompileStatement(db.db, InsertCode, true);
		return db;
	}

	void Close(DB *db) {
		sqlite3_close_v2(db->db);
	}

	Timing InsertCode(DB &db, uint64_t RIP, Code const &Code) {
		int Res{};

		Timing Time{};
		const auto Reset_Now = std::chrono::high_resolution_clock::now();
		Res = sqlite3_reset(db.InsertCode);
		HandleSQLError(Res);
		const auto Reset_End = std::chrono::high_resolution_clock::now();

		const auto Bind_Now = std::chrono::high_resolution_clock::now();
		Res = sqlite3_bind_int64(db.InsertCode, 1, RIP);
		HandleSQLError(Res);
		if (0) {
			Res = sqlite3_bind_zeroblob64(db.InsertCode, 2, Code.Size);
			HandleSQLError(Res);
		}
		else {
			Res = sqlite3_bind_blob64(db.InsertCode, 2, Code.Data, Code.Size, SQLITE_STATIC);
			HandleSQLError(Res);
		}
		const auto Bind_End = std::chrono::high_resolution_clock::now();

		const auto Step_Now = std::chrono::high_resolution_clock::now();
		while ((Res = sqlite3_step(db.InsertCode)) != SQLITE_DONE) {
			HandleSQLError(Res);
		}
		const auto Step_End = std::chrono::high_resolution_clock::now();

		Time.Reset = std::chrono::duration_cast<std::chrono::nanoseconds>(Reset_End - Reset_Now).count();
		Time.Bind = std::chrono::duration_cast<std::chrono::nanoseconds>(Bind_End - Bind_Now).count();
		Time.Step = std::chrono::duration_cast<std::chrono::nanoseconds>(Step_End - Step_Now).count();

		return Time;
	}

}
