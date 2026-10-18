// Client autotest mode (`bv2 --autotest <script> [--timeout <s>]`, ADR 0025). Plain C++: tests/test_autotest.cpp drives it without a window.
#ifndef ROBOVIOLENCE2_AUTOTEST_H
#define ROBOVIOLENCE2_AUTOTEST_H

#include <functional>
#include <string>
#include <vector>

namespace roboviolence2
{
	class Autotest
	{
	public:
		enum Result { Running = -1, Pass = 0, Fail = 1, Timeout = 2 }; // values are the process exit codes

		// One step per line: `# comment`, `wait <s>`, `expect-console <text>`, `expect-scene <name>`, or a console command.
		// False when a line is malformed; report() names it.
		bool parse(const std::string & script);
		bool load(const std::string & path);

		// Every console line, in order; expect-console searches the lines after its previous match.
		void consoleLine(const std::string & line);

		// Called once per fixed step. Runs commands through `run` and returns Running until the script ends,
		// an expect is still unmet when the time budget runs out (Fail), or a wait outlasts it (Timeout).
		Result update(float delay, const std::string & scene, const std::function<void(const std::string &)> & run);

		// "autotest: PASS", "autotest: FAIL line N: <step>" or "autotest: TIMEOUT line N: <step>".
		std::string report() const;

		float timeout = 60;

	private:
		enum Kind { Command, Wait, ExpectConsole, ExpectScene };
		struct Step
		{
			int line;
			Kind kind;
			std::string text; // the script line, for reports
			std::string arg;
			float seconds;
		};

		std::vector<Step> m_steps;
		std::vector<std::string> m_lines;
		size_t m_next = 0;
		size_t m_searchFrom = 0;
		float m_elapsed = 0;
		float m_waited = 0;
		Result m_result = Running;
		std::string m_error;
	};
}

// Defined in main.cpp (client): the running autotest, or null
extern roboviolence2::Autotest * autotest;

#endif
