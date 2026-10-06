# Plan: three small patches for mainstream

Fork: `sekz/ODR-PadEnc` (origin). Mainstream: `Opendigitalradio/ODR-PadEnc` (upstream).

## Rules
- No "Generated with Claude Code", no Co-Authored-By, no session lines in commits or PR bodies.
- Patches are based on `upstream/next`, use autotools and C++11, and add no new dependencies.
- No PR is created until the user decides.
- Fork-only work (Thai rendering, StreamDAB API, smart DLS, Docker tooling, enhanced MOT) stays in the fork's `master`.

## Status (2026-10-04)
- [x] `master` = upstream v3.1.0 merged (`bba0c3d`), pushed to origin.
- [x] `next` = `upstream/next`, pushed to origin.
- [x] Value assessment done: the eight fork feature groups are not suitable for mainstream as-is (not wired into the autotools build, stubs, or application-level scope). See "Assessment".
- [x] Thai charset claims checked (see "Thai findings").
- [x] Patches below implemented, built and tested (see Results). Nothing submitted upstream.
- [x] Open items worked through on 2026-10-05, see "Open items: resolution". Two more patches came out of it.

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

Exhaustive search of ETSI EN 300 401 V2.1.1 (2017-01), full PDF downloaded from the user's Google Drive (124 pages, all page headers present; text extracted with pdftotext). The first Drive text read was incomplete (page markers stopped at 78), so it was redone on the real PDF. Zero hits for: text control, contextual, base direction, bidi, bi-direction, right-to-left, RTL, Thai, Arabic, Hebrew, glyph, ligature, reordering, complex text, diacritic, presentation form, UTF-16. "combin" only matches unrelated uses (FEC, interleaving, SId combinations). Hits that exist: FIG type 2 encoding flag (UTF-8 or UCS-2, clause 5.2.2.3.2), dynamic label clause 7.4.5.2, and 65 "Rfa" mentions. I read clauses 5.2.2 and 7.4.5.2 in full and checked every hit of the terms above; I did not read the other clauses line by line:
- Layout of the 16-bit dynamic label prefix in V2.1.1: b15 Toggle, b14 First, b13 Last, b12 C flag, b11-b8 Length, b7-b4 Field 2 (Charset when First=1; Rfa + 3-bit SegNum otherwise), b3-b0 Rfa. `dls.cpp` matches this exactly.
- Clause 7.4.5.2: in the first dynamic label segment, "Field 2" is the 4-bit Charset; the following 4-bit field is "Rfa ... shall be set to zero until they are defined". There is **no text control field** in this edition (the words text control, combining, contextual, bidi, base direction do not appear anywhere in the document).
- FIG type 2 (clause 5.2.2.3) in this edition only chooses between UTF-8 and UCS-2, with no flags.
- So upstream's `dls.cpp:330` (low nibble written as 0) is correct for V2.1.1. The text control field, the combining flag required by the Thai profile, and the UTF-16 naming of charset `0110` all come from a later EN 300 401 edition, which TS 101 756 V2.5.1 refers to.
- Bit positions and transport are now confirmed from a primary source, see "Text control field: confirmed definition" below.

## Text control field: confirmed definition (ETSI TS 103 176 V2.4.1 (2020-08), clause 8.3; PDF fetched from the user's Google Drive, 123 pages, extracted with pdftotext)
Source and date:
- Defined in TS 103 176 (Rules of implementation, service information features), not in EN 300 401 V2.1.1. Clause 8.3.3 notes: "The next revision to ETSI EN 300 401 [1] will include these changes." So the field was introduced through TS 103 176 ahead of an EN revision.
- TS 103 176 history: V1.1.1 Aug 2012, V1.1.2 Jul 2013, V1.2.1 May 2016, V2.1.1 Aug 2017, V2.2.1 Oct 2018, V2.3.1 Nov 2019, V2.4.1 Aug 2020. The V2.4.1 PDF does not say which version added clause 8.3.
- Dated by comparing documents from the user's Drive (2026-10-05): **TS 101 756 V2.2.1 (2017-08)** has no text control field, no regional profiles, no Annex E, no Thai profile and no UTF-16 charset name (it still says UCS-2). **TS 101 756 V2.4.1 (2020-08)** has all of them (text control 9 hits, regional profile 24, Thai 13, UTF-16 8). So in TS 101 756 it first appeared in V2.3.1 (Nov 2019) or V2.4.1 (Aug 2020). V2.3.1 is not on the Drive, so the exact one is unknown. TS 103 176 versions before V2.4.1 are not on the Drive; a search listing shows V2.2.1 (Oct 2018) with clause 8.5 (RTL), unverified, so TS 103 176 may carry it earlier. EN 300 401 V2.1.1 (Jan 2017) does not have it.

Function (8.3.1, 8.4): an indication of label complexity so a receiver can tell whether it has the rendering capability to present the label. A receiver reassembles the label, analyses the flags, and if it lacks a required capability the label cannot be presented and an alternative label strategy applies. Receivers for regions needing more than EBU Latin shall decode the field in FIG type 2 labels and in dynamic labels. Not carried for FIG type 1 (EBU Latin only).

Encoding (8.3.2, figure 5), 4 bits:
- b3 Bidi flag: 1 = label contains bidirectional text (numerals excluded).
- b2 Base direction: 0 = LTR, 1 = RTL (always set to the desired direction).
- b1 Contextual flag: 1 = contextual characters present (glyph depends on position or surrounding characters, e.g. Arabic forms).
- b0 Combining flag: 1 = combining characters present (receiver must compose glyphs from parts; non-spacing or spacing marks).

Transport in dynamic labels (8.3.3.2, figure 7):
- The text control field replaces the 4-bit Rfa (Field 3, b3-b0 of the second prefix byte) only when C flag = 0 **and** First flag = 1, i.e. in the first segment of a message. Other segments keep Rfa = 0 (b7 Rfa, b6-b4 SegNum, b3-b0 Rfa).
- FIG type 2 uses the Rfu bit to switch the layout; not relevant to a PAD encoder.
- This matches the derivation from TS 101 756 Annex E (Arab States FIG 2 = `0100b`, Thai `00xxb`).

Other points in clause 8 relevant to odr-padenc:
- 8.3.1: use UTF-8 or UTF-16 (fewest bytes) for non-Latin scripts; charset `0110` is UTF-16 (BMP only). No more 8-bit character sets are envisioned.
- 8.1: the formatting characters 0x0A, 0x0B and 0x1F formerly provided for the dynamic label "shall not be used" (reserved in Complete EBU Latin). `dls.cpp` joins multiple DLS lines with `\n` (0x0A). Separate compliance question, not part of any patch yet.

Decision (2026-10-05): the user chose **B1**, an explicit `--text-control=N` option. Implemented as patch 4, `next-dls-text-control` (`7fe5b19`), see below. With the field at 0 the output is identical to today and to V2.1.1 (bits stay Rfa = 0).

Consequences for the patches:
- `next-charset-warning` stays valid (the silent replacement is a defect). Its text and README only say UTF-8 / `--charset=15 --raw-dls`, which matches Table 1.
- Patch 4, `next-dls-text-control` (done, option B1): the user sets the text control flags with `--text-control=N` for raw DLS, so Thai DLS can carry the combining flag required by Annex E.4.2.2. Automatic detection (B2) was not chosen.
- Optional candidate 5: choose Charset automatically per label (E.2).

## Open items: resolution (2026-10-05)
1. **Raw UTF-8 / UTF-16 DLS not verified.** Done as far as software allows. With the DEBUG build every emitted segment was decoded: CRC residue 0x1D0F on each segment, length field equals payload, charset nibble `f` (UTF-8) and `6` (UTF-16), text control only in the first segment, reassembled bytes identical to the input file (106 bytes in 7 segments for UTF-8, 72 bytes in 5 for UTF-16, both decode back to the Thai text). Characters may be split across segments, which EN 300 401 V2.1.1 7.4.5.2 permits. **Still open:** display on real receivers cannot be tested here.
2. **Version that introduced the text control field.** Narrowed, see the dating paragraph above: TS 101 756 V2.3.1 (Nov 2019) or V2.4.1 (Aug 2020); TS 103 176 possibly earlier. **Still open:** which one exactly (needs TS 101 756 V2.3.1 and TS 103 176 V2.2.1/V2.3.1).
3. **0x0A between DLS lines.** EN 300 401 V2.1.1 (the reference chosen by the user) says code 0x0A "may be inserted to indicate a preferred line break", so the current behaviour conforms to it. TS 103 176 V2.4.1 clause 8.1 says the formatting codes 0x0A, 0x0B and 0x1F "shall not be used", and TS 101 756 V2.5.1 Annex C lists them as "reserved for future definition and are not displayed". On such receivers lines run together. Resolved with the opt-in `--line-break=space` (patch 6); the default is unchanged. `charset.cpp` still passes two typed characters through as control codes: U+000B (vertical tab) becomes 0x0B (end of headline) and U+0082 becomes 0x1F (preferred word break). Tested 2026-10-05. A typed newline cannot reach the converter because the file is read line by line, so the only 0x0A on air is the separator between lines (now controlled by `--line-break`); the earlier wording that U+000A passes through was wrong. A CR from a Windows CRLF file becomes a trailing space. Not changed: both codes are valid in EN 300 401 V2.1.1, they are only produced when someone types those unusual characters, and a receiver that treats them as reserved simply shows no mark.
4. **Only relevant clauses read line by line.** More read: TS 103 176 clause 8.1, 8.2 in full, 8.3, 8.4, 8.5.1 and the start of 8.5.2, and every other "dynamic label" mention; TS 101 756 Annex C notes and Annex D (multiplexer FIG 0/7 transition, not for a PAD encoder). Result: for an encoder, texts must be in Unicode logical order, which odr-padenc already does by passing text through. **Still unread:** the end of 8.5.2 and 8.5.3 (receiver algorithms, no encoder requirement), and everything else in the three documents, which was searched by term only.
5. **New finding:** the 128-byte cut could split a UTF-8 character (patch 5).
6. **Noted, not patched:** odr-padenc calls charset ID 6 "UCS-2 BE" while the registry now names it UTF-16 BE (same for 2-byte characters); the enum also lists IDs 1, 2 and 3, which are no longer registered.

## The patches
Patches 1, 2, 5 and 6 are cut from `next`, patch 3 from patch 2, patch 4 from patch 1.

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

### 4. `next-dls-text-control` (option B1, chosen by the user)
- [x] `--text-control=N` (0 to 15), default 0, written into the 4 low bits of the second prefix byte of the **first** dynamic label segment only (TS 103 176 8.3.3.2). Other segments keep Rfa = 0.
- [x] Requires `--raw-dls`; without it the program exits with an error, because converted texts use Complete EBU Latin.
- [x] Strict parsing through the same helper as patch 1, so this branch is stacked on `next-input-validation`.
- [x] Usage text and README updated.
- [ ] Not done: automatic detection of the flags from the text (option B2), no unit test (the segment code is private; checked end to end instead).

### 5. `next-dls-truncation` (found while resolving the open items)
- [x] Texts over 128 bytes were cut at exactly 128 bytes. With raw UTF-8 this can leave half a character at the end. Reproduced: 50 x U+0E01 (150 bytes) sent as 128 bytes ending `e0 b8`, invalid UTF-8.
- [x] Now the whole character that would be cut is dropped (126 bytes, valid). UTF-16 texts are cut to an even length. Converted EBU Latin text is cut as before.
- [x] The warning says bytes instead of characters and gives the shortened length. Based on `next`.

### 6. `next-dls-line-break` (from the 0x0A open item)
- [x] `--line-break=preferred|space`. Default `preferred` keeps the current output (0x0A between lines). `space` joins lines with a space (`00 20` in UTF-16 texts). Length is unchanged, so DL Plus positions are not affected.
- [x] README now documents how multi-line DLS files are joined, which it did not before. Based on `next`.

## Per-patch procedure
- [x] Cut the branch, implement, `./bootstrap && ./configure && make`, run the checks, commit with a plain message.
- [x] Record the result below.

## Results
| Branch | Build | Checks | Notes |
|--------|-------|--------|-------|
| next-input-validation (`be30c66`) | OK, no new warnings | Manual: `-c 99`, `-c abc`, `-c 3x`, `-s -1`, `-m 0`, `-X 0`, `-l ''` all exit 2 with a clear message; a 127-character `-o` path is refused instead of truncated; a normal `-o /tmp/ptest` still binds `/tmp/ptest.padenc` | Warnings in `dls.h` (uninitialised `content_type`/`start_marker`) already exist upstream |
| next-charset-warning (`9aff52d`) | OK, no new warnings | With a fake audio encoder: Thai file prints exactly 1 warning (first U+0E2A), Latin file 0, `-c 15 -C` 0. First version repeated the warning every ~1.2 s; fixed to once per distinct line | Raw UTF-8 path (`--charset=15 --raw-dls`) not verified on air; based on code reading of `dls.cpp`. README wording says so only as "not all receivers can display" |
| next-tests (`f27a241`) | OK | `make check`: 2/2 pass (`test_charset`, `test_crc`); `make dist` includes `tests/` | Branch is stacked on next-charset-warning |
| next-dls-text-control (`7fe5b19`) | OK, no new warnings (2 existing `dls.h` warnings) | DEBUG build prints each segment. `-c 15 -C`: first segment byte `f0`, later segments `10`, `20`, `30`. With `--text-control=1`, `4`, `15`: first segment `f1`, `f4`, `ff`, later segments unchanged. Rejected with exit 2: `--text-control=1` without `--raw-dls`, `16`, `abc`, `-1` | Stacked on `next-input-validation`. README text sits next to the charset README note, so combining the branches may need a trivial merge. No unit test |
| next-dls-truncation (`34ced3d`) | OK, no new warnings | DEBUG build, segments decoded: 150-byte Thai text now 126 bytes valid UTF-8 (was 128, invalid); 500-byte Thai text 128 valid; raw ASCII 200 bytes stays 128; UTF-16BE 140 bytes becomes 128, valid; converted Latin 200 bytes stays 128; a 106-byte text is untouched and prints no warning | Independent of the other branches |
| next-dls-line-break (`5a1f664`) | OK, no new warnings | DEBUG build: default and `preferred` give `0a` between lines (unchanged); `space` gives `20`, also in raw UTF-8; UTF-16BE gives `00 0a` by default and `00 20` with `space`; `--line-break=tab` exits 2 | Independent. Its README text sits near the other README notes, so combining branches may need a small merge |

## Decision
- [x] 2026-10-05: the user approved submitting in the suggested order: `next-input-validation`, `next-dls-truncation`, `next-charset-warning` with `next-tests`, `next-dls-text-control`, then `next-dls-line-break` last. Stacking: `next-tests` sits on `next-charset-warning` and `next-dls-text-control` on `next-input-validation`, so those pairs go together or in order.
- [ ] Open: where the PRs are opened. This session can only reach `sekz/ODR-PadEnc`, not `Opendigitalradio/ODR-PadEnc`.
- [ ] (superseded) The user reviews and chooses which of the six patches to submit to `Opendigitalradio/ODR-PadEnc` `next`.

## History rewrite (2026-10-06)
- The user asked for all commits authored by `Claude <noreply@anthropic.com>` to be re-authored as `Seksan Poltree <seksan.poltree@gmail.com>`, with no "Generated with Claude Code", Co-Authored-By or session lines in commits or PR bodies.
- Audit before: 18 fork-only commits had the Claude identity as author and committer. No commit message contained attribution lines. 4 older commits (`b78e91f`, `013493a`, `44fed6d`, `1daeff8`) used the placeholder identity `Developer <user@example.com>`; the user asked for those to be changed too, which was done in a second pass the same day.
- Method: only the author and committer lines of the 18 commits were changed, parents re-pointed, and the old SSH signatures (made by the session key, invalid after any change) removed. All other commits are byte-identical, so the 63 upstream commits and `next` kept their hashes and the history still connects to `Opendigitalradio/ODR-PadEnc`. Every branch tree is identical to before. `git filter-branch` was tried first and rejected: it recreated all 63 upstream commits (it drops GitHub's signatures), which would have disconnected the history.
- Result: all 33 fork-only commits are authored and committed by `Seksan Poltree <seksan.poltree@gmail.com>`; 0 commits with a Claude or Anthropic identity or message, and none with the placeholder identity, on any branch. The rewritten commits are unsigned. Commit hashes in this plan were updated; the old ones no longer exist on the fork.
- This repo now commits as the user and no longer signs with the session key (`commit.gpgsign=false`, local config).
- Backup of everything before the rewrite: `pre-rewrite-all.bundle` (all branches and tags, 23.9 MB) in the session scratchpad.
- Second pass (placeholder identity): 27 commits were re-created (the 4 plus their descendants on `master` and `claude/pensive-cerf-1ta08s`). The six patch branches are based on `next`, are not descendants, and kept their hashes. The 63 upstream commits and all branch contents are unchanged. Backup before the second pass: `pre-rewrite2-all.bundle` in the session scratchpad.
