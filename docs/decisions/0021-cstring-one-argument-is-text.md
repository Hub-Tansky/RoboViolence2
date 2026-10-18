# 0021: A one-argument CString is text; formatting needs arguments

- Status: Accepted
- Date: 2026-10-17
- Affects: [Phase C step 5](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-c-installers-and-fixes/step5-client-engine-fixes.md) task 5.1; PNS-34

## Context
- `CString(char* fmt, ...)` was the only way to build a `CString` from text, and it was implicit: a player name, chat line or server `set` command passed as one argument ran as a printf format (R16, R21–R23; a `%n` crashes the reader).
- Making it `explicit` broke ~800 call sites (mostly literals) and still formatted direct calls such as `CString(packet.name)`.

## Decision
- `CString(const char* text)`: one argument, copied as is (null gives ""). Implicit, so literals, arrays and `char*` all convert safely.
- `template <class A, class... R> CString(const char* fmt, A, R...)`: two or more arguments format (`vsnprintf`, at most `MAX_CARAC - 1` characters).

## Consequences
- No call site can run text as a format by passing it alone; the format is always the first of several arguments.
- A one-argument literal containing `%` is no longer formatted (none relied on it; `%%` now stays `%%`).
- Passing a `CString` object as a format argument is a compile error on GCC and Clang (MSVC accepts it and copies the object's bytes); the required Linux and macOS CI builds catch it. Pass `.s`.
- One-argument text is no longer cut at `MAX_CARAC - 1` (511) characters; only formatting is.
- `CString::set(char* fmt, ...)` still formats with one argument; no call site passes wire text to it.
- Rejected: `explicit` plus a verbatim array constructor (direct-init of a non-const array still picked the format constructor).
