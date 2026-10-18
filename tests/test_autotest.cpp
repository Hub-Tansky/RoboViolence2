// Phase T3: the autotest script runner (game/src/Autotest.cpp) without a window: parsing, waits, expectations, exit codes.
#include "Autotest.h"

#include <cstdio>
#include <string>
#include <vector>

using roboviolence2::Autotest;

// The client global the header declares; the runner doesn't use it
Autotest * autotest = nullptr;

static const float step = 1.0f / 30; // the game's fixed step

// Runs `script` against a fake client whose scene is `scene` and whose console echoes each command
static Autotest::Result run(const std::string & script, const std::string & scene, float timeout, std::vector<std::string> * commands, std::string * report)
{
	Autotest test;
	test.timeout = timeout;
	if (!test.parse(script))
	{
		*report = test.report();
		return Autotest::Fail;
	}
	test.consoleLine("Console created");
	Autotest::Result result = Autotest::Running;
	for (int frame = 0; frame < 30 * 600 && result == Autotest::Running; ++frame)
	{
		result = test.update(step, scene, [&](const std::string & command)
		{
			commands->push_back(command);
			test.consoleLine("> " + command);
		});
	}
	*report = test.report();
	return result;
}

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };
	std::vector<std::string> commands;
	std::string report;

	check(run("# comment\n\n  expect-scene menu\r\nexpect-console Console created\nhost\nwait 0.5\nexpect-console > host\n", "menu", 60, &commands, &report) == Autotest::Pass,
		"comments, CRLF, scene, console, command, wait: pass");
	check(report == "autotest: PASS", "pass report");
	check(commands.size() == 1 && commands[0] == "host", "the command ran once");

	commands.clear();
	check(run("a\nexpect-console > a\nexpect-console > a\n", "menu", 1, &commands, &report) == Autotest::Fail, "a console line matches one expect only");
	check(report == "autotest: FAIL line 3: expect-console > a", "fail report names the line");

	check(run("expect-scene game\n", "menu", 2, &commands, &report) == Autotest::Fail, "unmet scene fails at the timeout");
	check(report == "autotest: FAIL line 1: expect-scene game", "scene fail report");

	check(run("wait 5\n", "menu", 2, &commands, &report) == Autotest::Timeout, "a wait past the timeout is a timeout");
	check(report == "autotest: TIMEOUT line 1: wait 5", "timeout report");

	check(run("wait soon\n", "menu", 2, &commands, &report) == Autotest::Fail && report == "autotest: FAIL line 1: wait needs seconds", "bad wait rejected");
	check(run("expect-scene\n", "menu", 2, &commands, &report) == Autotest::Fail && report == "autotest: FAIL line 1: expect-scene needs an argument", "empty expect rejected");

	Autotest missing;
	check(!missing.load("no-such-autotest-script.txt") && missing.report() == "autotest: FAIL can't read no-such-autotest-script.txt", "missing script rejected");

	if (failures == 0) std::printf("autotest runner: all checks passed\n");
	return failures == 0 ? 0 : 1;
}
