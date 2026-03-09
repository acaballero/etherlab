# Agent notes (trx)

## Build / upload / test
- Primary build system is **PlatformIO** (`platformio.ini`), default env: `genericSTM32F427VGT`.
- Build: `pio run` or `pio run -e genericSTM32F427VGT` (also `genericSTM32F303CC`).
- Clean: `pio run -e genericSTM32F427VGT -t clean`.
- Upload (STLink): `pio run -e genericSTM32F427VGT -t upload`.
- CLion/CMake integration uses: `platformio -c clion run …` (see `CMakeListsUser.txt`).
- Generate compile DB for clangd/tools: `pio run -t compiledb` (writes `compile_commands.json`).
- Tests (PlatformIO Test Runner): `pio test -e genericSTM32F427VGT`.
- Run a single test (or pattern): `pio test -e genericSTM32F427VGT -f "test_*"` (filter by test name/dir).

## Code style / conventions
- C++ is built as **C++17**; keep embedded-friendly code (no heavy allocations in hot paths).
- Formatting: match existing K&R braces, ~4-space indentation; keep diffs minimal and consistent with surrounding files.
- Includes: project headers first (`"module/file.h"`), then C/C++ std headers (`<cstdint>`, `<cstring>`); avoid unnecessary relative `../` includes.
- Naming: types `PascalCase` (e.g., `GPIOPin`), namespaces/functions `snake_case` (e.g., `periodic_task`), macros/feature flags `UPPER_SNAKE`.
- Types: prefer fixed-width ints (`uint32_t`, `int32_t`) and `enum class` for new enums; keep POD structs (often `st_*`) simple.
- Error handling: prefer explicit status returns / `Result<TValue,TError>` over exceptions; always check HAL/driver return codes.
