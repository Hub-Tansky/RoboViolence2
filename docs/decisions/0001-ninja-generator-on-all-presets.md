# 0001: Use the Ninja generator on all presets, MSVC included

- Status: Accepted
- Date: 2026-10-02
- Affects: [step 2 §2.4](../refactoring/phase-a-modern-portable-build/step2-cmake-build.md#24-presets), [step 5 §5.4](../refactoring/phase-a-modern-portable-build/step5-code-hygiene-and-ci.md)

## Context

- Step 2 originally used Ninja everywhere except `win-x64-msvc`, which used the Visual Studio generator.
- The Visual Studio generator doesn't write `compile_commands.json`. clangd, clang-tidy (step 5) and AI agents need it to understand the code, so Windows would be the one OS without it.
- Ninja ships with Visual Studio 2022+, is one package on macOS/Linux (`ninja`, `ninja-build`), and vcpkg already uses it for its own builds.

## Decision

Every preset uses `"generator": "Ninja"`. `win-x64-msvc` uses Ninja with the MSVC toolchain (`toolset`/`architecture` strategy `external`). An optional `win-x64-vs` preset keeps the Visual Studio generator for people who want a `.sln`.

## Consequences

- `compile_commands.json` exists on all three OSes; one build tool and one set of commands everywhere.
- Ninja + MSVC needs the MSVC environment: a Developer PowerShell on the command line, automatic in Visual Studio "Open Folder" and VS Code CMake Tools, `ilammy/msvc-dev-cmd` in CI.
- `VS_DEBUGGER_WORKING_DIRECTORY` (step 2 §2.6) only applies to `win-x64-vs`. Ninja builds set the working directory in the debugger launch config.
- Rejected: Visual Studio generator only (no `compile_commands.json`); Ninja Multi-Config (debug/release in one tree isn't needed; one build dir per preset is simpler).
