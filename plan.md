# Plan: three small patches for mainstream

Fork: `sekz/ODR-PadEnc` (origin). Mainstream: `Opendigitalradio/ODR-PadEnc` (upstream).

## Rules
- No "Generated with Claude Code", no Co-Authored-By, no session lines in commits or PR bodies.
- Patches are based on `upstream/next`, use autotools and C++11, and add no new dependencies.
- No PR is created until the user decides.
- Fork-only work (Thai rendering, StreamDAB API, smart DLS, Docker tooling, enhanced MOT) stays in the fork's `master`.

## Status (2026-10-04)
- [x] `master` = upstream v3.1.0 merged (`0d376e6`), pushed to origin.
- [x] `next` = `upstream/next`, pushed to origin.
- [x] Value assessment done: the eight fork feature groups are not suitable for mainstream as-is (not wired into the autotools build, stubs, or application-level scope). See "Assessment".
- [x] Thai charset claims checked (see "Thai findings").
- [ ] Patches below.

## Assessment of the eight original groups
- Core code differs from `upstream/master` by one `.gitignore` line: no core fixes exist.
- `thai_rendering`, `security_utils`, `enhanced_mot`, `smart_dls`, `api_interface`, `content_manager` are not included from `odr-padenc.cpp` or `Makefile.am`; only the fork's CMake build (C++17, OpenSSL, WebP, HEIF) compiles them.
- `api_interface` and `content_manager` are stubs.
- Verdict: keep all of them in the fork; extract only the ideas below as small patches.

## Thai findings (ETSI TS 101 756)
- The ETSI site is blocked from the build container, so the PDF could not be read directly. Findings come from upstream's own charset enum and help text (`pad_common.h`, `odr-padenc.cpp`), which follow the registry, and from running the code.
- Fork claim "Thai character set identifier 0x0E" is not supported: upstream's registered IDs are 0, 1, 2, 3, 6 and 15. No Thai-specific table is used. The fork code invents ID 0x0E. Do not send that upstream.
- Thai is valid on air only through ISO/IEC 10646 (UCS-2 BE, ID 6, or UTF-8, ID 15). Upstream already supports both via `-c 6`/`-c 15 -C`.
- Real defect found: with default options (`-c 15`, no `-C`), upstream converts to Complete EBU Latin and silently replaces every non-Latin character with a space (`charset.cpp:104`). Test: `สวัสดี DAB` becomes 7 spaces plus `DAB`, with no warning.
- The fork's Buddhist calendar, holiday and cultural-content features are not encoder functions and are not proposed.
- To confirm before the PR: read TS 101 756 v2.5.1 charset table once the PDF is reachable (https://www.etsi.org/deliver/etsi_TS/101700_101799/101756/02.05.01_60/ts_101756v020501p.pdf).

## The three patches
Each branch is cut from `next` (patch 3 from patch 2).

### 1. `next-input-validation`
- [ ] `-o` socket path: error out if the path does not fit in `sun_path` instead of silently truncating (the truncated path is then unlinked and bound).
- [ ] Numeric options (`-c`, `-s`, `-m`, `-l`, `-L`, `-x`): reject non-numeric or out-of-range values instead of `atoi`; `-c` must be 0..15.
- [ ] Build with autotools, no new warnings; manual checks of the failure cases.

### 2. `next-charset-warning`
- [ ] Warn on stderr when a DLS line contains characters that cannot be represented in Complete EBU Latin (count and first offender), and point to `-c 15 -C` / `-c 6`.
- [ ] README note: non-Latin scripts such as Thai need ISO/IEC 10646 (`-c 15 -C` or UCS-2).
- [ ] No change to the encoded output.

### 3. `next-tests`
- [ ] Minimal `make check` using plain asserts, no new dependencies: charset conversion (Latin, accents, unrepresentable character), CRC.
- [ ] Based on patch 2 so the warning behavior is tested.

## Per-patch procedure
- [ ] Cut the branch, implement, `./bootstrap && ./configure && make`, run the checks, commit with a plain message.
- [ ] Record the result below.

## Results
| Branch | Build | Checks | Notes |
|--------|-------|--------|-------|
| next-input-validation | | | |
| next-charset-warning | | | |
| next-tests | | | |

## Decision
- [ ] The user reviews and chooses which patches to submit to `Opendigitalradio/ODR-PadEnc` `next`.
