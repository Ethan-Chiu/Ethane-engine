#pragma once

// Minimal unit-test harness: TEST_CASE registers a test, CHECK records a failure and continues,
// REQUIRE records a failure and ends the test. Run Ethane-Tests [substring] to run matching tests only.

#include <string>
#include <vector>

namespace EthTest {

	struct Case
	{
		const char* Name;
		void (*Fn)();
		const char* File;
		int Line;
	};

	std::vector<Case>& Cases();

	struct Registrar
	{
		Registrar(const char* name, void (*fn)(), const char* file, int line) { Cases().push_back({ name, fn, file, line }); }
	};

	struct RequireFailed {};

	void Fail(const char* expression, const char* file, int line);

}

#define ETH_TEST_CONCAT_(a, b) a##b
#define ETH_TEST_CONCAT(a, b) ETH_TEST_CONCAT_(a, b)

#define ETH_TEST_CASE_(fn, name)                                                         \
	static void fn();                                                                     \
	static EthTest::Registrar ETH_TEST_CONCAT(fn, _registrar)(name, fn, __FILE__, __LINE__); \
	static void fn()

#define TEST_CASE(name) ETH_TEST_CASE_(ETH_TEST_CONCAT(EthTestCase_, __LINE__), name)

#define CHECK(...)                                                \
	do {                                                          \
		if (!(__VA_ARGS__))                                       \
			EthTest::Fail(#__VA_ARGS__, __FILE__, __LINE__);      \
	} while (0)

#define REQUIRE(...)                                              \
	do {                                                          \
		if (!(__VA_ARGS__)) {                                     \
			EthTest::Fail(#__VA_ARGS__, __FILE__, __LINE__);      \
			throw EthTest::RequireFailed{};                       \
		}                                                         \
	} while (0)
