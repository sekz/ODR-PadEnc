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
