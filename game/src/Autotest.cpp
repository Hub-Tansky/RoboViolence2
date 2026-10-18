#include "Autotest.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace roboviolence2
{
	bool Autotest::parse(const std::string & script)
	{
		std::istringstream in(script);
		std::string line;
		for (int number = 1; std::getline(in, line); ++number)
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			size_t start = line.find_first_not_of(" \t");
			if (start == std::string::npos || line[start] == '#') continue;
			line = line.substr(start);

			size_t space = line.find(' ');
			std::string word = line.substr(0, space);
			std::string arg = space == std::string::npos ? "" : line.substr(space + 1);
			Step step{number, Command, line, line, 0};
			if (word == "wait")
			{
				char * end = nullptr;
				step.kind = Wait;
				step.seconds = std::strtof(arg.c_str(), &end);
				if (arg.empty() || *end != '\0' || step.seconds < 0)
				{
					m_error = "line " + std::to_string(number) + ": wait needs seconds";
					return false;
				}
			}
			else if (word == "expect-console" || word == "expect-scene")
			{
				step.kind = word == "expect-console" ? ExpectConsole : ExpectScene;
				step.arg = arg;
				if (arg.empty())
				{
					m_error = "line " + std::to_string(number) + ": " + word + " needs an argument";
					return false;
				}
			}
			m_steps.push_back(step);
		}
		return true;
	}

	bool Autotest::load(const std::string & path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			m_error = "can't read " + path;
			return false;
		}
		std::stringstream text;
		text << file.rdbuf();
		return parse(text.str());
	}

	void Autotest::consoleLine(const std::string & line)
	{
		m_lines.push_back(line);
	}

	Autotest::Result Autotest::update(float delay, const std::string & scene, const std::function<void(const std::string &)> & run)
	{
		if (m_result != Running) return m_result;
		m_elapsed += delay;

		while (m_next < m_steps.size())
		{
			const Step & step = m_steps[m_next];
			bool done = true;
			switch (step.kind)
			{
			case Command:
				run(step.arg);
				break;
			case Wait:
				done = m_waited >= step.seconds;
				m_waited = done ? 0 : m_waited + delay;
				break;
			case ExpectScene:
				done = scene == step.arg;
				break;
			case ExpectConsole:
				done = false;
				for (size_t i = m_searchFrom; i < m_lines.size() && !done; ++i)
				{
					if (m_lines[i].find(step.arg) != std::string::npos)
					{
						m_searchFrom = i + 1;
						done = true;
					}
				}
				break;
			}
			if (!done)
			{
				if (m_elapsed >= timeout) m_result = step.kind == Wait ? Timeout : Fail;
				return m_result;
			}
			++m_next;
		}
		m_result = Pass;
		return m_result;
	}

	std::string Autotest::report() const
	{
		if (!m_error.empty()) return "autotest: FAIL " + m_error;
		if (m_result == Pass) return "autotest: PASS";
		if (m_next == m_steps.size()) return "autotest: FAIL the client quit before the script ran";
		const Step & step = m_steps[m_next];
		std::string where = " line " + std::to_string(step.line) + ": " + step.text;
		if (m_result == Running) return "autotest: FAIL the client quit at" + where;
		return std::string("autotest: ") + (m_result == Fail ? "FAIL" : "TIMEOUT") + where;
	}
}
