## Repo snapshot

- Language: C++ (Windows-focused)
- Purpose: small modding tools / runtime patches for a Windows game binary
- Key files: `dvarpatches.cpp` (process-memory patcher), `watermark.cpp` (CLI tool to append/read watermark), `main.cpp` (contains `Logger` utility used by tools)

## High-level architecture / why

- This repo contains small, self-contained components rather than a large multi-module system.
- `dvarpatches.cpp` is written to run at module initialization and performs a deterministic memory scan to overwrite the literal "PhotonChat" in process memory (uses `VirtualQuery`, `VirtualProtect`). Treat this as a low-level runtime patcher that must remain minimal and deterministic.
- `watermark.cpp` is a command-line utility that appends a watermark to an executable file. It uses a small on-disk format: <watermark bytes> + uint32_t(len) + 8-byte magic `"CRLYWMK1"` (see `watermark.cpp`).

## Build / dev workflow

- Primary build task is provided: use the workspace VS Code task labelled `build` which runs `msbuild` with `/t:build` and `/property:GenerateFullPaths=true` (see Tasks in workspace). From PowerShell you can run the same msbuild invocation if needed.
- Platform: Windows. Code uses Windows API (`VirtualQuery`, `VirtualProtect`, `GetSystemInfo`, console mode tweaks) — compile and test on Windows with the Windows SDK.

## Project-specific patterns & conventions

- Static init side-effects: `dvarpatches.cpp` registers a global `DvarPatcher` instance whose constructor runs at module load. Avoid heavy allocations, I/O, or non-deterministic behavior in static constructors.
- Minimal, deterministic runtime changes: memory scanning is intentionally simple (byte-scan) and must not change control flow or rely on heuristics that could differ across builds.
- Logging: a lightweight singleton `Logger` is defined in `main.cpp` (Level enum, init(filePath, colors)). Prefer using `Logger::instance().info(...)` for simple instrumentation; if adding file logging, use `init` with a path.

## Integration points & examples to reference

- Overwriting in-memory literals: `dvarpatches.cpp` — direct byte comparison with `memcmp`, `VirtualProtect` to make pages writable, then `memset` to zero out the literal. If changing this behavior, mirror the existing protection/save/restore pattern.
- Watermark format and detection: `watermark.cpp` — detection logic reads the last 8 bytes for the magic `CRLYWMK1`, then reads the preceding uint32 length, then reads the watermark string at that offset. Keep this format to remain compatible with existing watermarking/unmarking logic.

## Safety, testing and debugging notes

- Because code touches process memory and files, test in a contained environment and on copies of target binaries. `watermark.cpp` appends data to files; use copies when experimenting.
- For `dvarpatches.cpp`, test as a module load in a controlled test process. Static-init code runs before main — to debug, attach a debugger early or add minimal logging to verify behavior.

## What to avoid / gotchas

- Do not add slow or non-deterministic routines inside global constructors.
- Avoid changing the watermark file layout (order and sizes) — other tools expect the exact layout.

## Quick checklist for contributors

1. Read `dvarpatches.cpp` before editing runtime patch logic. Preserve the VirtualProtect pattern.
2. When adding logging, use `Logger::instance()` and avoid heavy I/O during static initialization.
3. Test `watermark.cpp` on a copy of a target EXE; verify detection and appending logic by running the binary and checking the last bytes for `CRLYWMK1`.

---
If any section is unclear or you'd like more detail (examples of common edits, suggested tests, or a short dev runbook), tell me which area to expand and I'll iterate.
