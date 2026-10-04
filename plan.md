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
- [x] Patches below implemented, built and tested (see Results). Nothing submitted upstream.

## Assessment of the eight original groups
- Core code differs from `upstream/master` by one `.gitignore` line: no core fixes exist.
- `thai_rendering`, `security_utils`, `enhanced_mot`, `smart_dls`, `api_interface`, `content_manager` are not included from `odr-padenc.cpp` or `Makefile.am`; only the fork's CMake build (C++17, OpenSSL, WebP, HEIF) compiles them.
- `api_interface` and `content_manager` are stubs.
- Verdict: keep all of them in the fork; extract only the ideas below as small patches.

## Thai findings (ETSI TS 101 756 V2.5.1, 2025-06; PDF supplied by the user, checked 2026-10-04)
Correction: the first version of this section was written without access to the standard and was partly wrong.

Confirmed in the standard:
- Table 1 (clause 5.2): only three Charset values are registered: `0000` Complete EBU Latin, `0110` ISO/IEC 10646 using **UTF-16** big endian (BMP only, "an extension of UCS-2"), `1111` UTF-8. All other values are reserved.
- The fork's invented Thai charset ID `0x0E` is not registered (reserved). Not for upstream.
- Wrong earlier: upstream's enum also lists IDs 1, 2, 3 (`EBU_LATIN_CY_GR`, `EBU_LATIN_AR_HE_CY_GR`, `ISO_LATIN_ALPHABET_2`). They are not registered in V2.5.1. ID 6 is now named UTF-16 BE (upstream still calls it UCS-2 BE; same for BMP characters).
- Wrong earlier: "no Thai-specific table". Annex E.4 defines a **Thai regional profile**: glyph set is Complete EBU Latin plus U+0E00 to U+0E7F, sent as UTF-8 or UTF-16 dynamic labels (FIG type 2 / dynamic label charset). Thai text is therefore valid DLS; the fork's own UTF-8-to-"DAB Thai" mapping is not needed or defined by the standard.
- E.4.2.2: Thai labels "shall have the combining flag set to 1" (except the rare labels needing no glyph combination), the base direction and bidi flags are 0, and the contextual flag is 1 only for labels using contextual characters. UTF-16 is recommended for predominantly Thai text.
- E.2: the transmission system should pick the most efficient Charset per dynamic label (EBU Latin when every code point is in the repertoire, otherwise UTF-8/UTF-16).

Verified in code:
- `dls.cpp:330` writes the low nibble of the second DLS prefix byte as 0, so the text control field (where the combining flag lives) is always 0. Thai labels sent by odr-padenc therefore do not meet the Thai profile.
- Default options convert to EBU Latin and silently replace every non-Latin character with a space (`charset.cpp:104`): `สวัสดี DAB` becomes spaces plus `DAB`.

Checked in ETSI EN 300 401 V2.1.1 (2017-01) (Google Drive copy supplied by the user; read clause 5.2.2 labels and clause 7.4.5.2 dynamic label, found by searching the full text; I did not read the rest of the 228k-character text):
- Clause 7.4.5.2: in the first dynamic label segment, "Field 2" is the 4-bit Charset; the following 4-bit field is "Rfa ... shall be set to zero until they are defined". There is **no text control field** in this edition (the words text control, combining, contextual, bidi, base direction do not appear anywhere in the document).
- FIG type 2 (clause 5.2.2.3) in this edition only chooses between UTF-8 and UCS-2, with no flags.
- So upstream's `dls.cpp:330` (low nibble written as 0) is correct for V2.1.1. The text control field, the combining flag required by the Thai profile, and the UTF-16 naming of charset `0110` all come from a later EN 300 401 edition, which TS 101 756 V2.5.1 refers to.
- The bit positions are therefore still not confirmed from a primary source. A web summary lists Bidi, Base direction, Contextual, Combining (most to least significant), consistent with the TS 101 756 profile table, but is not authoritative.

Decision: candidate 4 (`next-dls-text-control`) is on hold. It needs the later EN 300 401 edition (the one that defines the text control field) before any code is written; implementing the flags from V2.1.1 would be wrong (the bits are Rfa there) and from the summary would be a guess.

Consequences for the patches:
- `next-charset-warning` stays valid (the silent replacement is a defect). Its text and README only say UTF-8 / `--charset=15 --raw-dls`, which matches Table 1.
- Candidate 4, `next-dls-text-control` (ON HOLD, needs a later EN 300 401 edition, see above): let the user set the text control flags (combining/contextual) for raw DLS, or set the combining flag automatically for Thai code points, so raw UTF-8/UTF-16 Thai DLS meets Annex E.4.2.2.
- Optional candidate 5: choose Charset automatically per label (E.2).

## The three patches
Each branch is cut from `next` (patch 3 from patch 2).

### 1. `next-input-validation`
- [x] `-o` socket path: error out if the path does not fit in `sun_path` instead of silently truncating (the truncated path is then unlinked and bound).
- [x] Numeric options (`-c`, `-s`, `-m`, `-l`, `-L`, `-x`): reject non-numeric or out-of-range values instead of `atoi`; `-c` must be 0..15.
- [x] Build with autotools, no new warnings; manual checks of the failure cases.

### 2. `next-charset-warning`
- [x] Warn on stderr when a DLS line contains characters that cannot be represented in Complete EBU Latin (count and first offender), and point to `-c 15 -C` / `-c 6`.
- [x] README note: non-Latin scripts such as Thai need ISO/IEC 10646 (`-c 15 -C` or UCS-2).
- [x] No change to the encoded output.

### 3. `next-tests`
- [x] Minimal `make check` using plain asserts, no new dependencies: charset conversion (Latin, accents, unrepresentable character), CRC.
- [x] Based on patch 2 so the warning behavior is tested.

## Per-patch procedure
- [x] Cut the branch, implement, `./bootstrap && ./configure && make`, run the checks, commit with a plain message.
- [x] Record the result below.

## Results
| Branch | Build | Checks | Notes |
|--------|-------|--------|-------|
| next-input-validation (`17d2313`) | OK, no new warnings | Manual: `-c 99`, `-c abc`, `-c 3x`, `-s -1`, `-m 0`, `-X 0`, `-l ''` all exit 2 with a clear message; a 127-character `-o` path is refused instead of truncated; a normal `-o /tmp/ptest` still binds `/tmp/ptest.padenc` | Warnings in `dls.h` (uninitialised `content_type`/`start_marker`) already exist upstream |
| next-charset-warning (`855f3df`) | OK, no new warnings | With a fake audio encoder: Thai file prints exactly 1 warning (first U+0E2A), Latin file 0, `-c 15 -C` 0. First version repeated the warning every ~1.2 s; fixed to once per distinct line | Raw UTF-8 path (`--charset=15 --raw-dls`) not verified on air; based on code reading of `dls.cpp`. README wording says so only as "not all receivers can display" |
| next-tests (`a67f6f8`) | OK | `make check`: 2/2 pass (`test_charset`, `test_crc`); `make dist` includes `tests/` | Branch is stacked on next-charset-warning |

## Decision
- [ ] The user reviews and chooses which patches to submit to `Opendigitalradio/ODR-PadEnc` `next`.
