# Plan: sync with mainstream and submit feature PRs

Fork: `sekz/ODR-PadEnc` (origin). Mainstream: `Opendigitalradio/ODR-PadEnc` (upstream).

## Rules
- No "Generated with Claude Code", no Co-Authored-By, no session lines in commits or PR bodies.
- Do not push to any branch other than the ones named here without asking.
- Do not create PRs until the user has chosen which feature groups to submit.
- Do not commit local artifacts: `build_validation.log`, `test-minimal.cpp`, `validate_implementation.sh`.

## Status (2026-10-04)
- [x] Step 1: merge `upstream/master` (v3.1.0) into local `master`. Merge commit `0d376e6`.
  - Only conflict was `README.md`: kept the fork README and appended upstream usage notes.
  - Autotools build of the merged tree passes.
- [x] Step 2: local `next` equals `upstream/next` (`0de84b9`).
- [ ] Push `master` and `next` to origin. **Blocked: 403, Claude GitHub App has no access to `sekz/ODR-PadEnc`.**
  - Fix: reconnect GitHub at https://claude.ai/connect-github, or install the app on the repo, then start a new session if needed.
- [ ] Step 3: feature branches (below).

## Step 3: feature groups
Each `next-<feature>` branch is cut from `next` and stacked in this order, because later groups depend on earlier ones.

| # | Branch | Contents |
|---|--------|----------|
| 1 | `next-test-infra` | root and `tests/` CMakeLists, `test_main`, `test_performance`, `mock-odr-padenc.sh` |
| 2 | `next-security` | `security_utils.*`, `test_security` |
| 3 | `next-thai` | `thai_rendering.*`, `test_thai_rendering`, `test_charset_edgecases` |
| 4 | `next-enhanced-mot` | `enhanced_mot.*`, `test_mot_slideshow` |
| 5 | `next-smart-dls` | `smart_dls.*`, `test_dls_processing` |
| 6 | `next-api-content` | `api_interface.*`, `content_manager.*`, `test_api_interface` |
| 7 | `next-docker-runtime` | `entrypoint*.sh`, `health_server.py`, `.dockerignore`, dynamic UID/GID |
| 8 | `next-docs` | `README.md`, `TODO.md` |
| 0 | `next-core-fixes` | changes in `src/odr-padenc.*`, `pad_*`, `sls.*` (not yet inspected; best upstream candidates if independent) |

## Per-branch procedure
- [ ] Create the branch from `next` (or the previous branch in the stack).
- [ ] Apply only that group's files from `master`.
- [ ] Build: autotools (`./bootstrap && ./configure && make`) and CMake where relevant.
- [ ] Run the group's tests.
- [ ] Record the result (pass/fail, warnings) in the table below.
- [ ] Commit with a plain message and no attribution lines.

## Results
| Branch | Build | Tests | Notes |
|--------|-------|-------|-------|
| next-test-infra | | | |
| next-security | | | |
| next-thai | | | |
| next-enhanced-mot | | | |
| next-smart-dls | | | |
| next-api-content | | | |
| next-docker-runtime | | | |
| next-docs | | | |
| next-core-fixes | | | |

## Decision
- [ ] The user reviews the results and chooses which groups go to mainstream.
- [ ] Create one PR per chosen group against `Opendigitalradio/ODR-PadEnc` `next`.

## Value assessment for mainstream (evidence from the code)

Findings:
- Mainstream builds with autotools and C++11. `Makefile.am` lists only the original 8 modules.
- None of the new modules (`thai_rendering`, `security_utils`, `enhanced_mot`, `smart_dls`, `api_interface`, `content_manager`) is included from `odr-padenc.cpp` or built by autotools. Only the fork's CMake build compiles them, and it needs C++17, OpenSSL, WebP and HEIF.
- `api_interface.cpp` and `content_manager.cpp` are stubs ("Implementation stub", "Placeholder for ETSI compliance validation"). The HTTP server is only described in comments.
- Core code differs from `upstream/master` only by one `.gitignore` line, so there are no core bug fixes to submit.
- Mainstream already ships MOT slideshow and DLS (README states DL Plus support).

| Group | Verdict | Reason |
|-------|---------|--------|
| `next-core-fixes` | Drop | Nothing to submit; only `.gitignore` differs. |
| `next-test-infra` | Hold | Mainstream has no tests. A small gtest suite for the existing `dls`, `charset` and `crc` code could be useful, but the current suite targets the fork's new modules. Rework before proposing. |
| `next-thai` | Maybe, needs proof | Useful only if it changes the real `charset`/`dls` path. Verify the Thai handling against ETSI TS 101 756 before claiming compliance. Cultural features (Buddhist calendar, holidays) do not belong in an encoder. Split out any pure charset fix. |
| `next-security` | Maybe, small part | Path-traversal checks for the slide folder and file inputs could be worth a small patch on the existing code. The large standalone library is not wired in and is too big to review. |
| `next-enhanced-mot` | Maybe, small part | Idea of WebP/HEIF input is plausible, but mainstream already reads any ImageMagick format and re-encodes to JPEG/PNG. Not wired in. Only the duplicate-detection or quality ideas could be reworked as small patches on `sls.cpp`. |
| `next-smart-dls` | Drop for mainstream | Priority queues, social media and RSS integration are application logic, not encoder logic. Not wired in. Keep in the fork. |
| `next-api-content` | Drop for mainstream | StreamDAB-specific, stubs only, adds HTTP/WebSocket server scope. Keep in the fork. |
| `next-docker-runtime` | Drop for mainstream | Fork deployment (health server, multi-instance entrypoints). Keep in the fork. |
| `next-docs` | Partial | Mainstream README already updated upstream. Offer only generic fixes, if any. Fork-specific README and `TODO.md` stay in the fork. |
| `DL Plus (TODO.md)` | Check first | Mainstream already advertises DL Plus. Verify what is missing before planning work. |

Recommendation: submit nothing as-is. Candidate upstream patches, each small and on the autotools/C++11 build: (1) path/input validation hardening, (2) any real Thai charset fix proven against TS 101 756, (3) a minimal test harness for existing modules. Everything else stays in the fork.
