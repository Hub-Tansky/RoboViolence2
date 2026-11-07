#include "CrashReport.h"

#include "platform.h"
#include <cstdio>
#include <cstring>
#include <ctime>

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
	void writeText(HANDLE file, const char* text)
	{
		DWORD written;
		WriteFile(file, text, (DWORD)strlen(text), &written, NULL);
	}

	LONG WINAPI onException(EXCEPTION_POINTERS* info)
	{
		HANDLE file = CreateFileA(g_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
			return EXCEPTION_EXECUTE_HANDLER;
		char line[1200];
		writeText(file, g_header);
		snprintf(line, sizeof(line), "exception 0x%08lx at %p\n", info->ExceptionRecord->ExceptionCode, info->ExceptionRecord->ExceptionAddress);
		writeText(file, line);
		// One frame per line: module file name + offset, which the matching build's .pdb resolves
		void* frames[MAX_FRAMES];
		USHORT count = CaptureStackBackTrace(0, MAX_FRAMES, frames, NULL);
		for (USHORT i = 0; i < count; ++i)
		{
			HMODULE module = NULL;
			char name[MAX_PATH] = "?";
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)frames[i], &module))
				GetModuleFileNameA(module, name, MAX_PATH);
			const char* base = strrchr(name, '\\');
			snprintf(line, sizeof(line), "%s+0x%llx\n", base ? base + 1 : name, (unsigned long long)((char*)frames[i] - (char*)module));
			writeText(file, line);
		}
		CloseHandle(file);
		return EXCEPTION_EXECUTE_HANDLER; // ends the process without the Windows error dialog
	}
#else
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
	void onSignal(int sig)
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
		raise(sig); // SA_RESETHAND restored the default action: the process dies as without the handler
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
		snprintf(g_path, sizeof(g_path), "%s/crash-%s.txt", dir.c_str(), stamp);
		snprintf(g_header, sizeof(g_header), "Robo Violence 2 %s crash report, started %s\n", version, stamp);
#ifdef BV2_PLATFORM_WINDOWS
		SetUnhandledExceptionFilter(onException);
#else
		void* frames[1];
		backtrace(frames, 1);
		struct sigaction action = {};
		action.sa_handler = onSignal;
		action.sa_flags = SA_RESETHAND;
		sigemptyset(&action.sa_mask);
		for (int sig : {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS})
			sigaction(sig, &action, nullptr);
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
