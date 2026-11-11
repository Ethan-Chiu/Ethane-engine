#include "Test.h"

#include "Ethane/Core/Log.h"

#include <cstdio>
#include <exception>
#include <string_view>

namespace EthTest {

	std::vector<Case>& Cases()
	{
		static std::vector<Case> cases;
		return cases;
	}

	static int s_Failures = 0;

	void Fail(const char* expression, const char* file, int line)
	{
		s_Failures++;
		std::printf("    FAILED %s:%d: %s\n", file, line, expression);
	}

}

int main(int argc, char** argv)
{
	Ethane::Log::Init();

	const std::string_view filter = argc > 1 ? argv[1] : "";
	int run = 0, failedCases = 0;
	for (const EthTest::Case& testCase : EthTest::Cases())
	{
		if (!filter.empty() && std::string_view(testCase.Name).find(filter) == std::string_view::npos)
			continue;

		run++;
		const int failuresBefore = EthTest::s_Failures;
		std::printf("[ RUN ] %s\n", testCase.Name);
		try
		{
			testCase.Fn();
		}
		catch (const EthTest::RequireFailed&)
		{
		}
		catch (const std::exception& e)
		{
			EthTest::Fail(e.what(), testCase.File, testCase.Line);
		}
		if (EthTest::s_Failures != failuresBefore)
			failedCases++;
	}

	std::printf("\n%d test(s), %d failed, %d failed check(s)\n", run, failedCases, EthTest::s_Failures);
	return failedCases == 0 ? 0 : 1;
}
