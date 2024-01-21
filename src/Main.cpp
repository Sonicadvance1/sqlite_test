#include "sqlite_db.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <sys/mman.h>

int main(int argc, char **argv) {
	Sqlite::Init();

	auto DB = Sqlite::Open();
	Sqlite::Close(&DB);
	Sqlite::Shutdown();

	uint64_t Rip{};
	auto Data = mmap(nullptr, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	std::srand(std::time(nullptr));

	for (size_t i = 0; i < 10; ++i) {
		Rip = std::rand();

		const auto Now = std::chrono::high_resolution_clock::now();
		auto Timing = Sqlite::InsertCode(DB,
			Rip,
			Sqlite::Code {
			.Data = (uint8_t*)Data,
			.Size = 4096,
		});
		const auto End = std::chrono::high_resolution_clock::now();
		const auto Diff = End - Now;

		fprintf(stderr, "Took %ld ns\n", std::chrono::duration_cast<std::chrono::nanoseconds>(Diff).count());
		fprintf(stderr, "\tReset took %ld ns\n", Timing.Reset);
		fprintf(stderr, "\tBind took  %ld ns\n", Timing.Bind);
		fprintf(stderr, "\tStep took  %ld ns\n", Timing.Step);
	}

	munmap(Data, 4096);

	return 0;
}
