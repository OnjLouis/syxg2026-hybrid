# S-YXG2026 Hybrid

See [`README.html`](README.html) for the accessible user guide, runtime layout,
current validation, and known limitation.

S-YXG2026 Hybrid is a source-only 32-bit VST2 preservation project. It combines:

- sixteen isolated S-YXG2006LE instances as newer per-part dry PCM sources;
- S-YXG50 as both the fallback full-XG voice source and common Yamaha DSP;
- recovered S-YXG100 VL/PVL and SG engines from the existing Hybrid project.

The repository does not contain or distribute Yamaha binaries, tables, songs,
presets, or firmware.

## Architecture

`XglEngine` loads S-YXG2006LE once and creates sixteen independent VST
instances. Each original MIDI channel is remapped to channel zero of its own
instance. This makes per-part audio available before mixing without modifying
the closed Yamaha binary.

The wrapper parses `sxgbnw6l.tbl` to determine whether the selected bank and
program exist in S-YXG2006LE. Defined note-ons are rendered by that part's
2006LE instance. Undefined slots remain on S-YXG50. Releases reach both PCM
engines where necessary so a bank transition cannot strand a held note.

CC91, CC93, and CC94 are retained as per-part send levels but forced to zero in
the private S-YXG2006LE instances. The dry result is copied into eight buses:
dry L/R, reverb L/R, chorus L/R, and variation L/R. Those buses are injected at
the established S-YXG50 effects bridge, alongside VL/PVL and SG buses. Thus the
newer PCM source uses S-YXG50 reverb, chorus, and variation rather than the
XG Lite effects.

System variation uses each part's CC94 send. For insertion variation, the
wrapper safely observes the XG connection and part-assignment parameters rather
than forwarding arbitrary SysEx into S-YXG2006LE. The assigned part is removed
from the dry/reverb/chorus input buses and sent in full to the variation bus,
matching the measured routing of S-YXG50. The variation processor then applies
its own wet/dry balance and downstream reverb/chorus sends. S-YXG50 insertion
effects receive audio before part volume, expression, and pan. While a 2006LE
part owns that slot, its isolated source is therefore rendered at full volume,
full expression, and centre pan; S-YXG50 receives the real controller values
and applies them after the effect.

SG ownership is resolved before PCM note generation. A channel-zero-only replay
of the retained `SG_yuki` research trace is sample-identical between the
accepted S-YXG100 Hybrid and this wrapper. This guards against accidental SG and
2006LE doubling.

## Known limitation

The isolated S-YXG2006LE VST faults when its arbitrary SysEx entry path is used.
The wrapper therefore mirrors short MIDI messages but does not forward SysEx to
the 2006LE instances. S-YXG50, VL/PVL, and SG keep their existing SysEx paths.
Common bank/program/controllers work, but unusual part parameters supplied only
through XG SysEx may need explicit translation in a later revision. XG system
and insertion variation routing are explicitly translated and supported.

## Runtime layout

```text
syxg2026-hybrid.dll       built by this project
syxg2026-vl-worker.exe    built by this project
syxg2026-sg-worker.exe    built by this project
syxg50-engine.bin         user-supplied compatible S-YXG50 VST
syxg2006le-engine.bin     user-supplied original S-YXG2006LE VST
sxgbnw6l.tbl              user-supplied S-YXG2006LE bank table
sxgdat6l.tbl              user-supplied S-YXG2006LE waveform/data table
Sxgpvknl.vxd              user-supplied original PVL runtime
sxgsgknl.vxd              user-supplied original SG runtime
```

The Yamaha files must remain outside source control and public packages.

## Build

Build outside the source directory with a 32-bit Windows compiler:

```text
cmake -S . -B <build-directory> -G Ninja \
  -DCMAKE_CXX_COMPILER=<i686-clang++>
cmake --build <build-directory>
ctest --test-dir <build-directory> --output-on-failure
```

The VST product is `S-YXG2026 Hybrid`, its unique ID is `S2HY`, and the output
file is `syxg2026-hybrid.dll`.

## Diagnostics

- `SYXG2026_DISABLE_XG_EFFECTS=1` disables the S-YXG50 effects bridge for dry
  routing comparisons.
- `SYXG2026_NATIVE_GAIN` overrides the calibrated VL/PVL/SG gain.
- `SYXG2026_HYBRID_LOG` enables bounded wrapper diagnostics.
- `SYXG2026_VL_*` variables retain the inherited low-level VL research
  controls under a project-specific prefix.

`HybridHostProbe` accepts a private `PVTE 1` event trace. Yamaha-derived traces
are research inputs and do not belong in the repository.
