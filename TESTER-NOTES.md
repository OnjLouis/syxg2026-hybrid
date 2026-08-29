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

The current probes cover effects-bus injection, present and missing 2006LE
slots, VL coexistence, sample-identical SG ownership, two simultaneous plugin
instances, and 44.1/48 kHz rendering. They do not prove every XG SysEx-defined
part parameter.
