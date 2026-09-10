# S-YXG2026 Hybrid

## Package 0.1.1

Enables signed updates for existing installations and adds the updater scripts
and version marker to the complete download. This packaging release leaves the
synth engine binaries and audio behaviour unchanged.

See [`README.html`](README.html) for the accessible user guide, runtime layout,
current validation, and known limitation.

S-YXG2026 Hybrid is a source-only 32-bit VST2 preservation project. It combines:

- sixteen isolated S-YXG2006LE instances as newer per-part dry PCM sources;
- S-YXG50 as both the fallback full-XG voice source and common Yamaha DSP;
- recovered S-YXG100 VL/PVL and SG engines from the existing Hybrid project.

The repository does not contain or distribute Yamaha binaries, tables, songs,
presets, or firmware.

## Preservation milestone

This project is believed to be the first publicly documented modern Windows
VST2 integration of S-YXG2006LE PCM voices, S-YXG50 full-XG voices and DSP,
and the Japanese S-YXG100 VL/PVL and SG engines in one real-time instrument.
That description is deliberately qualified: earlier private or unpublished
work may exist.

## Architecture

`XglEngine` loads S-YXG2006LE once and creates sixteen independent VST
instances. Each original MIDI channel is remapped to channel zero of its own
instance. This makes per-part audio available before mixing without modifying
the closed Yamaha binary.

The wrapper parses `sxgbnw6l.tbl` to determine whether the selected bank and
program exist in S-YXG2006LE. Defined note-ons are rendered by that part's
2006LE instance. Undefined slots remain on S-YXG50. Releases reach both PCM
engines where necessary so a bank transition cannot strand a held note.
Channel 10 starts in rhythm mode after reset. In XG mode, a later melodic bank
MSB releases that implicit default so channel 10 can carry pitched voices, and
bank 127 restores rhythm operation. GM1 and GS bank selections preserve their
channel-10 drum default and use S-YXG2006LE bank `120/0` when the selected kit
exists; missing Roland kits remain on S-YXG50. Roland Drum Map 1 and Drum Map 2
assignments are tracked separately, and kit changes are synchronized between
parts sharing a map. GM2 uses bank 120 for rhythm and bank 121 for melodic
selections. Explicit Yamaha part-mode SysEx can switch any part between melodic
and rhythm operation and remains authoritative over later bank changes. XG
rhythm parts use the complete S-YXG2006LE drum bank `127/0` internally.

In GS mode, documented Roland insertion effects with direct named XG
counterparts are translated onto the shared Yamaha variation bus. Per-part
Roland EFX switches become variation sends, allowing the shared effect to reach
multiple parts. Recognized messages are consumed after translation so
S-YXG50's partial GS handling cannot replace the selected effect. GTR Multi 3
translates its distortion or overdrive selection, drive, level, and nearest
supported EQ controls; its internal wah, chorus, and delay sections are not
translated. Unrecognized compound effects remain available to S-YXG50 and are
not replaced with an unrelated approximation.

GM1 System On, GM2 System On, GS Reset, and XG System On all clear retained
routing and controller state. Because S-YXG2006LE cannot safely receive those
SysEx messages and does not fully implement CC121, the wrapper explicitly
restores its supported channel controllers. This includes neutral values for
the GM2/XG sound controls CC71 through CC78. Reset recognition does not turn
the hybrid into a complete Roland GS implementation; the original S-YXG50
engine still determines how the forwarded mode message and sound map behave.

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
through XG SysEx may need explicit translation in a later revision. XG
rhythm-part mode, system variation, and insertion variation routing are
explicitly translated and supported.

The insertion correction is not drum-specific. It applies whenever a part
owned by S-YXG2006LE is assigned to insertion variation, including bass,
guitar, melodic, and drum parts. System variation and parts falling back to
S-YXG50 retain their established routing.

## Runtime layout

The packaged tester archive groups these files in its `VST` folder. Keep them
beside one another in that one runtime directory:

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
syxg2026-hybrid.version.json  updater product/version marker
```

The Yamaha files must remain outside source control and public packages.

## Updater

`Update Yamaha Hybrids.cmd` launches the shared PowerShell updater. The pair
may be placed in this product folder or in a common parent containing both
hybrids. It searches at most two folder levels, detects existing installations
by their unique wrapper DLLs, and never installs a missing synth.

Stable GitHub release metadata is signed with a product-specific RSA key. The
manifest restricts replacement to an allowlist, identifies the exact product,
and supplies SHA-256 hashes for the externally hosted runtime ZIP and every
managed file. The updater stages and verifies the complete package before
changing the live folder. It displays release notes and requires confirmation
unless explicitly invoked for unattended installation.

Before replacement, managed files are stored in one rollback ZIP beneath
`%LOCALAPPDATA%\Onj Research\Yamaha Hybrid Updater\syxg2026-hybrid\backups`.
Only the newest rollback and two bounded logs are retained. Rollback DLLs are
never left loose in a VST search path, unrelated files are preserved, and a
partial replacement is restored automatically. Audio hosts must be closed so
they do not lock the runtime files.

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
