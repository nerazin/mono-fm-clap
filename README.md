# Mono FM+

The FM+ machines of the Elektron Monomachine (FM+ STAT, FM+ PAR and FM+
DYN) as three CLAP instruments for Bitwig (macOS, universal binary), each
with the track's AMP envelope.

The DSP is a fixed-point C port of these machines and of the track kernel's
pitch and AMP sections in Monomachine OS 1.32B, written from its DSP56300
code and checked sample for sample against that code running in an
emulator. No firmware code is in this repository, and all its tables are
generated from formulas at start-up.

The bundle `Mono FM+.clap` holds three plug-ins: **Mono FM+ STAT**, **Mono
FM+ PAR** and **Mono FM+ DYN**. Each is a monophonic synth, as a
Monomachine track is.

## Controls

Every plug-in has the machine's eight knobs (page "Machine"), seven of the
AMP page's (page "Amp") and three pitch parameters. The knobs are 0–127
like the unit's knobs, can be automated and modulated in Bitwig, and
default to the unit's values.

The operators in all three machines are sines whose frequency is a ratio to
the carrier's (the note's).

**Mono FM+ STAT** (two operators, one with feedback):

| Parameter   | On the unit | What it does |
|-------------|-------------|--------------|
| 1 Frequency | 1FRQ | Operator 1's ratio, from a list of 24: 1/64, 1/32, 1/16, 3/32, 1/8, 5/32, 3/16, 1/4, 5/16, 3/8, 7/16, 1/2, 5/8, 3/4, 7/8, 1, 1¼, 1½, 1¾, 2, 2½, 3, 3½, 4. The display shows the ratio. |
| 1 Fine      | 1FIN | Operator 1's ratio × 0.75 to × 1.25 (64: × 1). |
| 1 Envelope  | 1ENV | Operator 1's depth, 4 × (1ENV/128)², full at 64. Above 64 the depth also falls after each note, faster towards 127: by 1 − (1ENV/128 − ½)⁴ every 16 samples. |
| 1 Feedback  | 1FB  | Operator 1 modulates its own phase, by up to half a cycle at 127. |
| 2 Frequency | 2FRQ | Operator 2's ratio, from the same list. |
| 2 Volume    | 2VOL | Operator 2's depth, 4 × (2VOL/128)², full at 64. Above 64 each of its samples also gets the previous one added, up to × 8 (a lift of its top end). |
| Tone        | TONE | The corner of the low-passes after each operator and on their sum. |
| Tune        | TUNE | The pitch, ±2 semitones (−200 to +197 cents). |

**Mono FM+ PAR** (three operators in parallel): **1/2/3 Frequency** (1FRQ,
2FRQ, 3FRQ) are ratios from the list above; **1/2/3 Envelope** (1ENV, 2ENV,
3ENV) are each operator's depth as 1ENV in STAT, and above 64 also the lift
of 2VOL; **Tone** and **Tune** as in STAT.

**Mono FM+ DYN** (two operators with their own envelopes):

| Parameter       | On the unit | What it does |
|-----------------|-------------|--------------|
| 1 Frequency     | 1FRQ | Operator 1's ratio, 2 × 1FRQ/128 (1 at 64), continuous. |
| 1 Freq Envelope | 1FEN | A pitch envelope on operator 1: each note moves its frequency by an amount that grows with the square of the distance from 64 (upwards above 64; downwards and a quarter as large below), then returns. The further from 64, the larger and quicker; at 64, none. |
| 1 Volume        | 1VOL | Operator 1's depth, 4 × ((1VOL/128)² + E), with E from 1VEN; where 1VOL/128 + E is above ½ also the lift of STAT's 2VOL. |
| 1 Vol Envelope  | 1VEN | A level envelope on operator 1: each note sets E to 4 × (1VEN/128 − ½) (× 1VOL/128 below 64, and at most 1), which then returns to 0 like 1FEN's. |
| 2 Frequency     | 2FRQ | Operator 2's ratio, 4 × (2FRQ/128)² (1 at 64). |
| 2 Envelope      | 2ENV | Operator 2's depth, as STAT's 1ENV. |
| 2 Feedback      | 2FB  | Operator 2 modulates its own phase, by up to half a cycle at 127. |
| Tune            | TUNE | The pitch, ±2 semitones. |

The Amp page, in all three:

| Parameter | On the unit | What it does |
|-----------|-------------|--------------|
| Attack    | ATK  | From where the level is up to full: 0.7 ms at 0, 152 ms at 64, 30 s at 127. |
| Hold      | HOLD | How long the level stays full, in beats of the host's tempo: Hold/32 beats (127 is about a bar). |
| Decay     | DEC  | The fall after the hold: −60 dB in 23 ms at 0, 1.7 s at 64, 112 s at 126; 127 holds. |
| Release   | REL  | The fall after the note ends, with the same times. |
| Volume    | VOL  | The level, (VOL/128)². |
| Pan       | PAN  | Equal-power panning (−64 to +63 on the display). In the centre the left side is 0.11 dB lower than the right, as on the unit. |
| Portamento | PORT | The glide to a new note, in beats of the host's tempo, whatever the interval: 0 is instant, 32 about 1/30 beat, 64 a third of a beat, 96 three beats, 127 about 27 beats (2^(13 Portamento/128)/276 beats). |

The pitch parameters (module "Pitch"), in all three:

| Parameter   | On the unit | What it does |
|-------------|-------------|--------------|
| Pitch Mod   | where the LFOs aimed at PTCH land (block word 30) | 64 is no change; each step is 2.06 semitones (352/2048 octave), and fractions of a step are kept. Modulate it in Bitwig as the unit's LFOs modulate pitch. 0 and 127 are −132 and +130 semitones; the pitch itself stops at the kernel's 11-octave range. |
| Bend Range  | KIT > ASSIGN, JOY R/L, PAGE PTCH, DEST | How far MIDI pitch bend goes at its ends: 1/12, 2/12 (the default, ±2.01 semitones), 7/12, 1OCT, 2OCT, 4OCT, 8OCT or 16OC. |
| Master Tune | GLOBAL > AUDIO > MASTER TUNE | 400.0 to 499.9 Hz in steps of 0.1 Hz, default 440.0. |

MIDI pitch bend works as the joystick's left and right do on the unit with
KIT > ASSIGN set to MIRR on, PAGE PTCH, DEST the Bend Range and ADD +63.
Only the bend's top 8 bits count, as on the unit: 128 steps each way.

The Monomachine's envelope has no sustain: the level falls with Decay even
while the note is held (use Decay 127 to hold it), and a note's end starts
the Release from wherever the level is.

## How it behaves

This is what the firmware does; the plug-in keeps all of it.

- **The machines.** They run at twice the rate, 88.2 kHz. Each operator is a
  sine read from an 8192-point table: interpolated, its frequency ramped
  across each 16-sample block; or, with feedback, not interpolated. Each
  operator's output is differentiated and low-passed (by Tone in STAT and
  PAR, fixed in DYN), and the operators are summed with their depths into a
  modulator, low-passed once more. The modulator is added to the carrier's
  phase increment (FM). The carrier is then halved in rate by two stages of
  allpass pairs. The output is mono.
- **Notes.** Each note restarts the machine's envelopes (1ENV, 2ENV, 3ENV,
  1FEN, 1VEN) and the AMP envelope's attack, from where its level is. With
  Portamento at 0 the pitch reaches a new note in two 16-sample blocks.
  Above 0 it glides in a straight line (in pitch), covering the interval
  from the glide's start in the Portamento time. A note that comes mid-glide
  in the same direction keeps that start; one in the other direction turns
  from where the pitch is, at the speed for the interval from the note it
  was heading to.
  At Master Tune 440.0, A4 is 439.95 Hz, 0.19 cents under 440 Hz, as the
  firmware tunes it.
- **Knobs.** Parameter changes reach the machine the way the Monomachine's
  CPU sends a knob: every 384 samples the value moves a quarter of the way to
  where you set it. Modulation is added on top, unsmoothed, every 48
  samples, as the unit's LFOs are. Notes and the tempo are taken every 48
  samples too.
- **The pitch** is the kernel's sum, once per block: the note's glide,
  then Master Tune, Tune, Pitch Mod and the bend, clamped to the kernel's
  11 octaves. The glide does not smooth Pitch Mod or the bend.
  - Master Tune is the main OS's table, trunc(2048 log2(f/440)) in 1/2048
    octaves (0 at 440.0 Hz, −281 at 400.0 Hz).
  - A pitch bend at its end adds 127 × 63 × the range's step to word 30.
    The steps are 4, 8, 28, 48, 96, 192, 384 and 768 (the main OS's table).
    The left side's ADD is −63, as the unit's mirror makes it. The result is
    kept to 0..$7EFFFF, on the side it moves.
- **The AMP gains** move in a straight line across each 16-sample block.

## Differences from the hardware

- **The track filter.** On the unit the machine's output goes through the
  track's filter and distortion before the AMP section. The plug-in leaves
  them out ([Mono Filter Distortion](https://github.com/nerazin/mono-filter-distortion-clap)
  is that filter, as an effect) and stands in for them with the filter's
  plain level at its default settings, 0.4455 (−7.0 dB), without its
  inversion, the level that Mono Filter Distortion treats as unity. Measured
  on an FM+ track at the default FILT page, the filter passes the machine at
  0.42 to 0.45 of its level depending on the note, and shapes it slightly
  (up to 0.8 dB around 800 Hz, with a roll-off at the top).
- **The rest of the track.** EQ, delay, the LFOs and the track's LEVEL are
  not in the plug-in. Bitwig's own modulators can take the LFOs' place:
  modulating Pitch Mod is what an LFO on the PTCH page does. The unit's LFO
  takes a new value every 96 samples and moves a quarter of the way to it
  each time; a Bitwig modulator is read every 48 samples, unsmoothed.
- **Pitch bend** follows one KIT ASSIGN setting (above). On the unit the
  joystick and bend can also go to other parameters with other ADD amounts,
  and velocity and key position can be assigned too. Those are left out.
- **The MULTI TRIG transpose** (SEQ START / SEQ TRNSP) reaches the kernel as
  a second glide (block words 34 and 28). It needs the unit's sequencer, so
  it is left out; with it at rest the kernel adds nothing.
- **Master tune before 0.3.0** was the word Monomodule's CPU model sends,
  −1 at 440 Hz. The main OS's table gives 0, so notes are now 1/2048 octave
  (0.59 cents) higher than in 0.2.0.
- **Notes.** One voice: the newest note held sounds. Releasing it returns to
  the previous note still held, with a new trig; releasing the last ends the
  note. Velocity is ignored. This note handling is the plug-in's; the unit's
  own MIDI handling was not studied.
- **Start.** The plug-in starts as a track does when the machine is assigned
  (the machine's init and trig routines run), with the AMP envelope idle at
  0. On a unit just switched on, the firmware's envelope starts with an
  attack of its own.
- **Other sample rates.** The machines run at 44.1 kHz, the unit's rate. At
  other rates the plug-in resamples its output (windowed sinc), so it is no
  longer bit-exact, but the pitch is the same. The resampler reads ahead, so
  the plug-in reports a latency: 0 samples at 44.1 kHz, 40 at 22.05 kHz, 44
  at 48 kHz and 87 at 96 kHz. Notes land on the unit's 48-sample control
  grid, so their timing has that much spread anyway.
- **Levels.** 0 dBFS in the plug-in is full scale after the AMP section. The
  machines' own output peaks at −18.1 dBFS; after the AMP section a held
  note peaks at about −16.1 dBFS at Volume 64 and −4.2 dBFS at 127.

## Accuracy

The reference is Monomachine OS 1.32B running in the DSP56300 emulator of
[Monomodule](https://github.com/shnolk/monomodule), with the track's controls
and notes built by Monomodule's model of the Monomachine's CPU. It has not
been compared with a physical Monomachine, and the plug-ins have not been
opened in Bitwig as part of this work.

| Check | Result |
|-------|--------|
| Core vs. firmware, for each machine on a synth track: random notes on and off, knob moves through the CPU model (Portamento included), raw control words anywhere from 0 to $7FFFFF (also for the AMP page, the trig word and Pitch Mod's word 30), Master Tune words from the whole table, tempos from 30 to 300 BPM; 400,000 blocks each, 18,000 to 36,000 of them mid-glide per 50,000 | every machine output sample identical; every machine state word identical after every block; the AMP envelope's state, its gain ramps and its products with the firmware's own filter output identical in every block; the glide's state, the pitch index (glide, TUNE, Pitch Mod and Master Tune) and the frequency word identical to the kernel's in every block |
| Built plug-in code vs. firmware, with the firmware's filter output replaced by the machine's output × 0.4455 as in the plug-in: knob moves on both pages (Portamento included) through the CPU model, modulation, overlapping notes, tempo changes, Pitch Mod moves and modulation, MIDI pitch bends, Bend Range and Master Tune changes (240,000 blocks per plug-in: 24,122 moves, 1,831 of them modulation, 3,735 pitch-parameter changes and 1,877 bends; 4,694 notes on and 3,690 off) | every output sample identical |
| The main CPU's pitch words vs. the main OS (the OS image's tables, not the DSP's) | identical: all 1,000 Master Tune words and the 8 bend-range steps |
| Tables vs. firmware memory | identical, every entry the machines and the AMP section read |

The tables: the sine is the boot code's recurrence, reproduced exactly (the
firmware holds it in both X and Y memory; FM+ PAR's carrier reads the Y
copy); the exponential pitch table is 2^(22 + k/2048) with a mask for its 416
entries one LSB lower in the firmware; Tone's one-pole coefficients are the
machines' shared formula for 10 Hz × 2000^(k/128), with three entries
corrected (as in [Mono Reverb](https://github.com/nerazin/mono-reverb-clap));
the 24 ratios follow a rule; the attack steps are 0.5 × e^(−0.08354855518 k),
rounded to six decimals and then to 24 bits; the decay factors are
10^(−3/64 × e^(−0.06730291836 k)), with 127 holding; and the portamento
speeds are 2^30 / (14400 × 2^(13k/128)), with 0 marked negative for the
instant glide.

`tests/test_core.c` pins this without the firmware. Its expected hashes were
produced by feeding the same stimulus into the firmware's machines and AMP
section, which the core matched. Its stimulus also moves Pitch Mod's word
and the Master Tune word. It also hashes the Master Tune words and every
bend value against a hash of the main OS's own tables.

## Build and install

```bash
./build_macos.sh
```

```bash
cp -R "build/Mono FM+.clap" ~/Library/Audio/Plug-Ins/CLAP/
```

The build script needs Xcode's command-line tools.

## Tests

```bash
cc -std=c99 -O2 -Wno-unused-function tests/test_core.c -lm -o tests/test_core && tests/test_core
```

```bash
xcrun clang -std=c11 -O2 -Wno-unused-function -Wno-missing-field-initializers -I vendor/clap/include tests/clap_test.c -o tests/clap_test && tests/clap_test "build/Mono FM+.clap"
```

`test_core` checks the tables, the main CPU's pitch words and 12,000 blocks
of each machine's output and AMP output against the firmware-derived hashes. `clap_test` loads the built
bundle and checks:

- the three plug-ins, each with its eighteen parameters;
- at 44.1 kHz, each plug-in's output equal to the core's, for notes on and
  off at and between control frames, with Portamento at 0 and at 50 (and
  that 50 glides), and Pitch Mod and Master Tune away from their defaults;
- FM+ STAT as a plain carrier at the core's frequency for A4 at 44.1 and
  48 kHz, and 197 cents higher with Tune at 127;
- its pitch at the core's frequency with a full bend up and down
  (±201 cents at 2/12), Pitch Mod 65 (+206 cents) and Master Tune 415.3 Hz
  (−100 cents);
- releasing the newer of two held notes returning to the other, and the
  release ending in silence;
- a note's onset no earlier than the reported latency and at most one
  control frame and one block later, at 22.05, 44.1, 48 and 96 kHz;
- state (also another machine's state rejected, a state saved by 0.1.0,
  from before Portamento, loaded with Portamento at 0, and one saved by
  0.2.0 loaded with the pitch parameters at their defaults) and the
  parameter text.

Both tests also pass with AddressSanitizer and UndefinedBehaviorSanitizer,
and as Intel builds under Rosetta. The bundle passes clap-trap's `validate`
(all three plug-ins, at 44.1, 48 and 96 kHz) and `state --roundtrip`.

## Source

- `source/mono_fm.h`: the three machines, the kernel's pitch and AMP
  section, and the main CPU's master tune and bend words (C99 and libm).
- `source/mono_fm_clap.c`: the three CLAP plug-ins around it.
- `vendor/clap`: CLAP 1.2 headers (MIT).

## License

GPL-3.0-only for this project's code. The DSP is an independent
reimplementation of the Monomachine's FM+ machines and track kernel, derived
from analysis of Elektron's firmware; no firmware code or data is included.
Monomachine and Elektron are trademarks of Elektron Music Machines; this
project is not affiliated with Elektron.
