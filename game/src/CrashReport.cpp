#include "CrashReport.h"

#include "platform.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>

#ifdef BV2_PLATFORM_WINDOWS
#	include <windows.h>
#else
#	include <csignal>
#	include <execinfo.h>
#	include <fcntl.h>
#	include <unistd.h>
#endif

namespace
{
	// Filled by install(): the handler only copies them out, it doesn't allocate
	char g_path[1024];
	char g_header[256];
	const int MAX_FRAMES = 64;

#ifdef BV2_PLATFORM_WINDOWS
	LPTOP_LEVEL_EXCEPTION_FILTER g_previous = NULL; // e.g. the MSVC ASan runtime's

	LONG passOn(EXCEPTION_POINTERS* info)
	{
		return g_previous ? g_previous(info) : EXCEPTION_CONTINUE_SEARCH;
	}

	void writeText(HANDLE file, const char* text)
	{
		DWORD written;
		WriteFile(file, text, (DWORD)strlen(text), &written, NULL);
	}

	// "module.exe+0x1234": the matching build's .pdb resolves the offset
	void writeFrame(HANDLE file, const char* prefix, void* address)
	{
		HMODULE module = NULL;
		char name[MAX_PATH] = "?";
		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)address, &module))
			GetModuleFileNameA(module, name, MAX_PATH);
		const char* base = strrchr(name, '\\');
		char line[MAX_PATH + 64];
		snprintf(line, sizeof(line), "%s%s+0x%llx\n", prefix, base ? base + 1 : name, (unsigned long long)((char*)address - (char*)module));
		writeText(file, line);
	}

	LONG WINAPI onException(EXCEPTION_POINTERS* info)
	{
		HANDLE file = CreateFileA(g_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
			return passOn(info);
		char line[64];
		writeText(file, g_header);
		snprintf(line, sizeof(line), "exception 0x%08lx at ", info->ExceptionRecord->ExceptionCode);
		writeFrame(file, line, info->ExceptionRecord->ExceptionAddress);
		void* frames[MAX_FRAMES];
		USHORT count = CaptureStackBackTrace(0, MAX_FRAMES, frames, NULL);
		for (USHORT i = 0; i < count; ++i)
			writeFrame(file, "", frames[i]);
		CloseHandle(file);
		return passOn(info); // the previous filter or Windows error reporting runs as before
	}
#else
	const int SIGNALS[] = {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS};
	struct sigaction g_previous[sizeof(SIGNALS) / sizeof(SIGNALS[0])];

	const char* signalName(int sig)
	{
		switch (sig)
		{
			case SIGSEGV:
				return "signal SIGSEGV\n";
			case SIGABRT:
				return "signal SIGABRT\n";
			case SIGFPE:
				return "signal SIGFPE\n";
			case SIGILL:
				return "signal SIGILL\n";
			case SIGBUS:
				return "signal SIGBUS\n";
			default:
				return "signal ?\n";
		}
	}

	// Only async-signal-safe calls, plus backtrace (loaded by install, so it doesn't allocate here)
	void onSignal(int sig, siginfo_t* info, void*)
	{
		int fd = open(g_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd >= 0)
		{
			const char* name = signalName(sig);
			(void)!write(fd, g_header, strlen(g_header));
			(void)!write(fd, name, strlen(name));
			void* frames[MAX_FRAMES];
			backtrace_symbols_fd(frames, backtrace(frames, MAX_FRAMES), fd);
			close(fd);
		}
		// Hand the signal to the handler that was there before (the default action, or ASan's report), so the process
		// dies as without the crash report. A fault (si_code > 0) happens again when this handler returns, with its own
		// siginfo; a sent signal (raise, abort, kill) is sent again and runs once this handler returns.
		for (size_t i = 0; i < sizeof(SIGNALS) / sizeof(SIGNALS[0]); ++i)
			if (SIGNALS[i] == sig)
				sigaction(sig, &g_previous[i], nullptr);
		if (info->si_code <= 0)
			raise(sig);
	}
#endif
}

namespace crashreport
{
	std::string install(const std::string& dir, const char* version)
	{
		char stamp[32];
		std::time_t now = std::time(nullptr);
		std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&now));
		std::string path = (std::filesystem::path(dir) / ("crash-" + std::string(stamp) + ".txt")).string();
		snprintf(g_path, sizeof(g_path), "%s", path.c_str());
		snprintf(g_header, sizeof(g_header), "Robo Violence 2 %s crash report, started %s\n", version, stamp);
#ifdef BV2_PLATFORM_WINDOWS
		g_previous = SetUnhandledExceptionFilter(onException);
#else
		void* frames[1];
		backtrace(frames, 1);
		struct sigaction action = {};
		action.sa_sigaction = onSignal;
		action.sa_flags = SA_SIGINFO | SA_ONSTACK; // SA_ONSTACK: a stack overflow runs on ASan's alternate stack, if any
		sigemptyset(&action.sa_mask);
		for (size_t i = 0; i < sizeof(SIGNALS) / sizeof(SIGNALS[0]); ++i)
			sigaction(SIGNALS[i], &action, &g_previous[i]);
#endif
		return g_path;
	}

	void forceCrash()
	{
#ifdef BV2_PLATFORM_WINDOWS
		RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
#else
		raise(SIGSEGV);
#endif
	}
}
