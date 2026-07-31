/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/os.h>
#include <bx/semaphore.h>
#include <bx/string.h>
#include <bx/timer.h>

TEST_CASE("getProcessMemoryUsed", "[os]")
{
	if (BX_ENABLED(BX_PLATFORM_EMSCRIPTEN) )
	{
		SKIP("Not supported by wasm.");
	}

	REQUIRE(0 != bx::getProcessMemoryUsed() );
}

TEST_CASE("dlSearchPath-add-remove", "[os]")
{
	const bx::FilePath tmp(bx::Dir::Temp);

	// Removing something that was never added must fail.
	REQUIRE(!bx::dlSearchPathRemove(tmp) );

	REQUIRE(bx::dlSearchPathAdd(tmp) );

	// Duplicates are rejected.
	REQUIRE(!bx::dlSearchPathAdd(tmp) );

	REQUIRE(bx::dlSearchPathRemove(tmp) );
	REQUIRE(!bx::dlSearchPathRemove(tmp) );
}

TEST_CASE("dlSearchPath-relative-is-resolved", "[os]")
{
	// Relative paths are resolved against the current working directory when they are
	// added, so adding "." and removing the absolute CWD must refer to the same entry.
	const bx::FilePath cwd(bx::Dir::Current);

	REQUIRE(bx::dlSearchPathAdd(bx::FilePath(".") ) );
	REQUIRE(!bx::dlSearchPathAdd(cwd) );
	REQUIRE(bx::dlSearchPathRemove(cwd) );
	REQUIRE(!bx::dlSearchPathRemove(bx::FilePath(".") ) );
}

TEST_CASE("dlSearchPath-capacity", "[os]")
{
	const bx::FilePath tmp(bx::Dir::Temp);

	char name[64];
	int32_t num = 0;

	for (; num < 64; ++num)
	{
		bx::FilePath filePath(tmp);
		bx::snprintf(name, sizeof(name), "bx_dl_search_path_%d", num);
		filePath.join(name);

		if (!bx::dlSearchPathAdd(filePath) )
		{
			break;
		}
	}

	// Search path is bounded, but it has to hold at least a few entries.
	REQUIRE(1 < num);
	REQUIRE(64 > num);

	for (int32_t ii = 0; ii < num; ++ii)
	{
		bx::FilePath filePath(tmp);
		bx::snprintf(name, sizeof(name), "bx_dl_search_path_%d", ii);
		filePath.join(name);

		REQUIRE(bx::dlSearchPathRemove(filePath) );
	}
}

TEST_CASE("dlopen-missing", "[os]")
{
	REQUIRE(NULL == bx::dlopen(bx::FilePath("bx-no-such-library." BX_DL_EXT) ) );

	// Registered search paths must not change the outcome for a library that doesn't exist.
	const bx::FilePath tmp(bx::Dir::Temp);
	REQUIRE(bx::dlSearchPathAdd(tmp) );
	REQUIRE(NULL == bx::dlopen(bx::FilePath("bx-no-such-library." BX_DL_EXT) ) );
	REQUIRE(bx::dlSearchPathRemove(tmp) );
}

TEST_CASE("dlopen-system-library", "[os]")
{
	if (!BX_ENABLED(BX_PLATFORM_WINDOWS) )
	{
		SKIP("Test is Windows specific.");
	}

	// LOAD_LIBRARY_SEARCH_DEFAULT_DIRS still includes System32, so this has to resolve.
	void* handle = bx::dlopen(bx::FilePath("kernel32.dll") );
	REQUIRE(NULL != handle);
	REQUIRE(NULL != bx::dlsym(handle, "GetProcAddress") );

	bx::dlclose(handle);
}

#if BX_CONFIG_SUPPORTS_THREADING

TEST_CASE("semaphore_timeout", "[os]")
{
	bx::Semaphore sem;

	int64_t start     = bx::getHPCounter();
	bool    ok        = sem.wait(900);
	int64_t elapsed   = bx::getHPCounter() - start;
	int64_t frequency = bx::getHPFrequency();
	double  ms        = double(elapsed) / double(frequency) * 1000;
	bx::printf("%f\n", ms);
	REQUIRE(!ok);
}

#endif // BX_CONFIG_SUPPORTS_THREADING

TEST_CASE("memoryPageSize", "[os]")
{
	const size_t pageSize = bx::memoryPageSize();
	REQUIRE(0 != pageSize);
	// Page sizes are always powers of two on supported platforms.
	REQUIRE(0 == (pageSize & (pageSize - 1) ) );

	// Large/huge page queries must not return a value smaller than the base page size.
	const size_t large = bx::memoryPageSize(bx::Memory::PageLarge);
	const size_t huge  = bx::memoryPageSize(bx::Memory::PageHuge);
	REQUIRE( (0 == large || large >= pageSize) );
	REQUIRE( (0 == huge  || huge  >= pageSize) );
}

TEST_CASE("memoryMap-readWrite-roundtrip", "[os]")
{
	const size_t size = 4 * bx::memoryPageSize();

	bx::Error err;
	void* addr = bx::memoryMap(NULL, size, 0, bx::Memory::ReadWrite, &err);
	REQUIRE(err.isOk() );
	REQUIRE(nullptr != addr);

	uint8_t* bytes = (uint8_t*)addr;
	for (size_t ii = 0; ii < size; ++ii)
	{
		bytes[ii] = uint8_t(ii & 0xff);
	}
	for (size_t ii = 0; ii < size; ++ii)
	{
		REQUIRE(uint8_t(ii & 0xff) == bytes[ii]);
	}

	bx::memoryUnmap(addr, size, &err);
	REQUIRE(err.isOk() );
}

TEST_CASE("memoryMap-reserve-then-commit", "[os]")
{
	const size_t pageSize = bx::memoryPageSize();
	const size_t size     = 8 * pageSize;

	bx::Error err;
	void* addr = bx::memoryMap(NULL, size, 0, bx::Memory::Reserve | bx::Memory::ProtectNone, &err);
	REQUIRE(err.isOk() );
	REQUIRE(nullptr != addr);

	// Commit one page inside the reservation and write to it.
	uint8_t* bytes = (uint8_t*)addr;
	bx::memoryMap(bytes + pageSize, pageSize, 0, bx::Memory::Commit | bx::Memory::ProtectReadWrite, &err);
	REQUIRE(err.isOk() );

	for (size_t ii = 0; ii < pageSize; ++ii)
	{
		bytes[pageSize + ii] = uint8_t( (ii * 7) & 0xff);
	}
	for (size_t ii = 0; ii < pageSize; ++ii)
	{
		REQUIRE(uint8_t( (ii * 7) & 0xff) == bytes[pageSize + ii]);
	}

	// Decommit the page; reservation stays, but physical backing is dropped.
	bx::memoryMap(bytes + pageSize, pageSize, 0, bx::Memory::Decommit | bx::Memory::ProtectNone, &err);
	REQUIRE(err.isOk() );

	// Release the whole reservation.
	bx::memoryUnmap(addr, size, &err);
	REQUIRE(err.isOk() );
}

TEST_CASE("memoryMap-protection-change", "[os]")
{
	const size_t size = bx::memoryPageSize();

	bx::Error err;
	void* addr = bx::memoryMap(NULL, size, 0, bx::Memory::ReadWrite, &err);
	REQUIRE(err.isOk() );
	REQUIRE(nullptr != addr);

	uint8_t* bytes = (uint8_t*)addr;
	bytes[0] = 0xab;
	REQUIRE(0xab == bytes[0]);

	// Flip to read-only, then back to read/write. Neither call should fail.
	bx::memoryMap(addr, size, 0, bx::Memory::ProtectRead, &err);
	REQUIRE(err.isOk() );
	REQUIRE(0xab == bytes[0]);

	bx::memoryMap(addr, size, 0, bx::Memory::ProtectReadWrite, &err);
	REQUIRE(err.isOk() );
	bytes[0] = 0xcd;
	REQUIRE(0xcd == bytes[0]);

	bx::memoryUnmap(addr, size, &err);
	REQUIRE(err.isOk() );
}
