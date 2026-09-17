# S-YXG2026 Hybrid tester notes

This is a source release candidate. Keep the existing S-YXG100 Hybrid installed
and select `S-YXG2026 Hybrid` explicitly in the host while evaluating it.

## Focus areas

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

The current probes cover effects-bus injection, present and missing 2006LE
slots, VL coexistence, sample-identical SG ownership, two simultaneous plugin
instances, 44.1/48 kHz rendering, defeasible channel-10 rhythm defaults, and
translated XG part-mode changes. They do not prove every XG SysEx-defined part
parameter.
Reset-message recognition and the explicit private-engine controller defaults
are covered by focused regression tests. MU voice-map parsing, default hybrid
selection, unsupported values, bank scope, and reset persistence are also
covered by focused regression tests.

Yamaha model `0x64` Plug-in Voice bulk transactions are assembled and applied
at their transaction footer. The retained regression song changes through all
26 embedded voices during one playback pass. VSTHost also confirms that the
accessible status page follows the selected VL bank and program. Foobar2000
MIDI Player can render ahead and open the editor against a separate idle
instance, so its status page is not a reliable live-playback probe.
