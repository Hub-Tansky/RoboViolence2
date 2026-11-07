// A client crash leaves a text report in the pref dir for the player to attach to a bug report (ROBO-186, ADR 0048).
#pragma once
#include <string>

namespace crashreport
{
	// Installs the crash handler. On SIGSEGV, SIGABRT, SIGFPE, SIGILL or SIGBUS (an unhandled exception on Windows) it
	// writes <dir>/crash-<start time>.txt with the version, the signal or exception code and a stack trace, then the
	// process dies as before. Returns the report's path.
	std::string install(const std::string& dir, const char* version);

	// Crashes on purpose (an access violation), for the Debug build's `crash` console command and tests.
	void forceCrash();
}
