# S-YXG2026 Hybrid

## Package 0.1.5

Prepares the eight VL helpers and the SG helper in a serialized, below-normal-
priority background phase when the host activates the plug-in. This removes
process creation and Yamaha-engine initialization from first-note MIDI
processing. A single active plug-in instance reserves approximately 60-70 MB
for its ready helpers; hosts that prebuffer more than one instance multiply
that figure.

## Package 0.1.4

Adds an accessible native editor and handles complete Yamaha model `0x64`
Plug-in Voice bulk transactions. Songs can now load and change their embedded
VL voices during playback, including the associated bank, program, volume,
mono/poly, pitch-bend range, portamento, reverb, and chorus settings.

## Package 0.1.3

Preserves S-YXG2006LE's native fallback for Yamaha panel banks `0/112` through
`0/127`. This keeps DGX/PSR-era panel-voice MIDI on the newer source engine
when an exact panel slot is absent from its compact table. Undefined ordinary
XG variation slots still fall back to S-YXG50.

## Package 0.1.2

Adds Yamaha MU Voice Map Select support. MU Basic selects S-YXG50 for basic
bank `0/0`, while MU Native restores the normal automatic hybrid voice choice.
MIDI files that send neither message retain the existing automatic routing
without an audible change.

See [`README.html`](README.html) for the accessible user guide, runtime layout,
current validation, and known limitation.

S-YXG2026 Hybrid is a source-only 32-bit VST2 preservation project. It combines:

- sixteen isolated S-YXG2006LE instances as newer per-part dry PCM sources;
- S-YXG50 as both the fallback full-XG voice source and common Yamaha DSP;
- recovered S-YXG100 VL/PVL and SG engines from the existing Hybrid project.

The repository does not contain or distribute Yamaha binaries, tables, songs,
presets, or firmware.

## Accessible editor

The plug-in editor opens a native Windows dashboard with three pages: live
status, under-the-hood routing, and the original Yamaha editor. Status uses
standard named controls and a 16-channel report list that exposes routing as
`2006LE/XG50`, VL/PVL, or SG alongside bank, program, note activity,
controllers, pitch bend, and effect sends. It also reports whether the 2006LE
runtime and Yamaha effects bridge loaded. The small engine activity graphic is
supplemental; the same values are always present in text.

Use `Alt+S`, `Alt+U`, or `Alt+Y` for the three pages, `F5` or `Alt+R` to
refresh, and `Alt+C` to copy a complete text report. Tab and Shift+Tab move
through the controls, and arrow keys switch among the page buttons. Live
updates change rows in place without rebuilding the list or moving focus.

The status page describes the live plug-in instance owned by its host. VSTHost
therefore exposes current channel activity. Foobar2000 MIDI Player may render a
MIDI file ahead of playback and later open the editor on a new, idle instance;
in that host the status page can consequently show defaults while Foobar2000 is
playing its already-rendered audio.

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
2006LE instance. Undefined ordinary XG slots remain on S-YXG50. Defined Yamaha
panel banks `0/112` through `0/127` stay on S-YXG2006LE even when an individual
program slot is absent, preserving the original engine's native basic-voice
fallback for DGX/PSR-era MIDI. Releases reach both PCM engines where necessary
so a bank transition cannot strand a held note.
When a MIDI file sends no MU Voice Map Select message, this automatic hybrid
choice remains unchanged. The standard MU Voice Map Select parameter
`F0 43 1n 49 00 00 12 mm F7` is also observed: value `00` sends basic bank
`0/0` voices to S-YXG50, while value `01` restores the normal hybrid choice.
Variation banks, drum banks, VL/PVL, and SG are unaffected. The selected map
persists across GM, GM2, GS, and XG resets, matching Yamaha's documented MU
behaviour. Unsupported values are ignored.

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

## Wine compatibility

The 32-bit plug-in and its separate native VL/SG workers can run under Wine,
but the workers require Wine's newer WoW64 process arrangement. A user reported
that an incompatible prefix let ordinary XG play while the VL worker faulted
as soon as a VL note sounded, causing the part to fall back to piano. For Wine
11, use a 64-bit prefix and force the newer WoW64 mode when launching the
32-bit VST host, for example:

```sh
WINEPREFIX="$HOME/.wine-syxg" WINEARCH=win64 wineboot
WINEPREFIX="$HOME/.wine-syxg" WINEARCH=wow64 wine /path/to/vst-host.exe
```

`WINEARCH=wow64` requires a prefix that was created as 64-bit, which is Wine's
default. It cannot convert a pure `WINEARCH=win32` prefix; create a separate
64-bit prefix instead. This workaround was reported in
[S-YXG100 Hybrid issue 4](https://github.com/OnjLouis/syxg100-hybrid/issues/4)
and agrees with
[Wine 11's documented new-WoW64 behaviour](https://list.winehq.org/hyperkitty/list/wine-releases%40list.winehq.org/thread/UL6L2GJ55VYUJ5KUMBZ3TZSXRFJ52QG6/).
Wine remains community-tested rather than a primary supported platform.

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
