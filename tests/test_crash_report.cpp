// ROBO-186 (ADR 0048): a crashed process leaves crash-*.txt in the dir given to crashreport::install.
// Runs itself with "crash <dir>" as the child that crashes, then reads what the child left in <dir>.
#include "CrashReport.h"

#include <cstdio>
#ifdef _WIN32
#	include <windows.h>
#endif
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static int failures = 0;

static void check(bool ok, const char* what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		++failures;
	}
}

int main(int argc, char* argv[])
{
	if (argc == 3 && std::string(argv[1]) == "crash")
	{
#ifdef _WIN32
		SetErrorMode(SEM_NOGPFAULTERRORBOX); // no Windows error dialog to block CI after the report
#endif
		crashreport::install(argv[2], "9.9.9-test");
		crashreport::forceCrash();
		return 0; // not reached
	}
	if (argc != 2)
	{
		std::printf("usage: test_crash_report <scratch dir>\n");
		return 2;
	}

	fs::path dir = argv[1];
	fs::remove_all(dir);
	fs::create_directories(dir);

	std::string command = "\"" + std::string(argv[0]) + "\" crash \"" + dir.string() + "\"";
#ifdef _WIN32
	command = "\"" + command + "\""; // cmd.exe strips the outer quotes
#endif
	int status = std::system(command.c_str());
	check(status != 0, "the child crashed");

	std::string report;
	int reports = 0;
	for (const fs::directory_entry& entry : fs::directory_iterator(dir))
	{
		std::string name = entry.path().filename().string();
		if (name.rfind("crash-", 0) == 0 && entry.path().extension() == ".txt")
		{
			++reports;
			std::ifstream in(entry.path());
			std::stringstream text;
			text << in.rdbuf();
			report = text.str();
		}
	}
	check(reports == 1, "one crash-*.txt in the dir");
	check(report.find("Robo Violence 2 9.9.9-test crash report") == 0, "report starts with the version line");
#ifdef _WIN32
	check(report.find("exception 0xc0000005 at test_crash_report.exe+0x") != std::string::npos, "report names the exception and where");
	check(report.find("test_crash_report.exe+0x") != std::string::npos, "report has a stack frame");
#else
	check(report.find("signal SIGSEGV") != std::string::npos, "report names the signal");
	check(report.find("test_crash_report") != std::string::npos, "report has a stack frame");
#endif
	if (failures)
		std::printf("report was:\n%s\n", report.c_str());
	else
		std::printf("ok: crash report\n");
	return failures ? 1 : 0;
}
