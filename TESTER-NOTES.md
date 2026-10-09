# S-YXG2026 Hybrid tester notes

## Current package: 0.1.11

This update preserves native voice edits when a VL/PVL worker starts later
in a song. Compare Timeless's closing lead: CC11 should retain Volume
Expression Mode without the previous timbre/level jumps. Intentional later
program changes still reload presets normally. Full-volume gain, SG, 2006LE,
effects, mapping state and master-volume handling are unchanged. The separate
Falcosoft-on-Wine fade report is not claimed fixed.

The preceding release restores converted MU-exclusive sub-banks (including Anathema),
corrects SFX Techno Kit selection, and translates independent MU distortion
and overdrive for external VL/PVL and a single routed SG part. The private
processor honours Dry/Wet and feeds the song's system sends once.
Please compare FatPizz's wind, kit, distorted lead and delay, then test your
own midstream effect assignments at your normal sample rate and block size.
Levels differ from Mu2026; use host headroom rather than expecting identical
hardware DSP or loudness. Unsupported algorithms, chained assignments and
multi-part SG insertion remain bypassed. The Wine editor issue remains open.

The fallback AWM engine now embeds an MU1000-derived conversion. Defined
2006LE voices keep their precedence. No separate MU ROMs or conversion tools
are needed. QWS and REAPER definitions use the Native map, with 2006LE overrides
and VL/PVL/SG entries. Check voice selection, drum kits, sustained loops,
VL/SG balance, and effect tails. Some multi-element voices and effects are
approximated. The reported Wine bridged-host GUI issue remains unresolved.
Read the HTML manual for attribution and the conversion licence notice.

This is a working preservation build. Keep the existing S-YXG100 Hybrid installed
and select `S-YXG2026 Hybrid` explicitly in the host while evaluating it.

## Focus areas

The 0.1.8-rc.1 editor test targets issue 3: disappearing controls under
Wine 11.19 Staging with VSTHost 1.58 x64 and Bridge 1.13 on X11/XFCE.
Follow EDITOR-TEST.html for installation, movement/playback tests and rollback.
The 22 Windows tests pass, including focused invalidation, selection/focus,
accessibility and editor resource checks. This is not confirmation that the
reported Wine failure is fixed; direct 32-bit VSTHost remains the reporter's
confirmed workaround. Audio/MIDI processing is unchanged.

Package 0.1.7 corrects an SG startup memory error exposed by Wine. Protected-
memory tests reproduce the original crash at the reporters' instruction address
and confirm that the corrected worker starts, renders, changes sample rate, and
shuts down without the invalid access. Falcosoft 6.6 renders `SG_yuki.MID`
through the corrected worker under Wine, and all retained SG demo excerpts keep
their exact Windows and Wine sample hashes.

Package 0.1.6 translates checksum-valid Roland GS Part Pitch Key Shift messages
into Channel Coarse Tuning for the affected isolated S-YXG2006LE source part.
The retained `suplex.mid` regression requires Part 2 to sound one octave below
the unshifted source. The translation must not alter other parts, corrupt the
MIDI file's active RPN or NRPN selection, or react to malformed GS SysEx.

1. Test ordinary GM and XG files at 44.1 and 48 kHz.
2. Compare voices against both S-YXG2006LE and S-YXG50 where possible.
3. Test bank changes, repeated notes, sustain, pitch bend, expression, pan, and
   rapid controller movement.
4. Check that reverb, chorus, and variation sound like S-YXG50 rather than the
   lighter 2006LE effects.
5. Re-test known VL/PVL and SG songs for regressions.
6. Report any wrong fallback voice, doubled note, stuck release, missing first
   note, controller stepping, effect loss, crash, or prolonged buffering.
7. Exercise insertion variation on S-YXG2006LE-owned melodic, bass, guitar,
   and drum parts. The source must enter S-YXG50 DSP before part volume,
   expression, and pan; Be-Bop is the known distortion reference.
8. Exercise channel-10 percussion, melodic XG channel-10 files such as
   `Neptuns_sphere`, GS files such as `DEMO0002.MID`, and files that use Yamaha
   part-mode SysEx to create additional rhythm parts. XG bank `127/0` must play
   drums, GM1 and GS bank-zero setup must preserve percussion, GM2 must observe
   banks 120 and 121, and an explicit XG melodic bank must play a pitched voice.
9. Alter CC71 through CC78, then send GM1 System On, GM2 System On, GS Reset,
   or XG System On. A subsequent note must use the neutral controller state;
   CC71 through CC78 return to 64.
10. With no MU Voice Map Select message, confirm the normal automatic hybrid
    selection remains unchanged. Then send `F0 43 10 49 00 00 12 00 F7` and
    confirm basic bank `0/0` uses S-YXG50. Send the same message with value
    `01` and confirm automatic hybrid selection returns. The chosen map must
    survive GM, GM2, GS, and XG resets without changing variation banks, drum
    banks, VL/PVL, or SG.
11. Play a GS file that sets Part Pitch Key Shift, such as `suplex.mid`, and
    compare it with S-YXG2006LE or S-YXG100 Hybrid. The affected part must use
    the documented semitone offset while all unrelated parts remain unchanged.

The current probes cover effects-bus injection, present and missing 2006LE
slots, VL coexistence, sample-identical SG ownership, two simultaneous plugin
instances, 44.1/48 kHz rendering, defeasible channel-10 rhythm defaults, and
translated XG part-mode changes. They do not prove every XG SysEx-defined part
parameter.
Reset-message recognition and the explicit private-engine controller defaults
are covered by focused regression tests. MU voice-map parsing, default hybrid
selection, unsupported values, bank scope, and reset persistence are also
covered by focused regression tests. GS Part Pitch Key Shift parsing covers
the retained Suplex message, multi-byte part data, checksum rejection,
documented value bounds, unrelated parameters, and live child-engine output.

Yamaha model `0x64` Plug-in Voice bulk transactions are assembled and applied
at their transaction footer. The retained regression song changes through all
26 embedded voices during one playback pass. VSTHost also confirms that the
accessible status page follows the selected VL bank and program. Foobar2000
MIDI Player can render ahead and open the editor against a separate idle
instance, so its status page is not a reliable live-playback probe.
