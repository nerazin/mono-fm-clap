/* Regression test for mono_fm.h that needs no firmware.
 *
 * The expected hashes come from Monomachine OS 1.32B itself: the tables were
 * read from the emulated DSP's memory, and the same stimulus was fed into the
 * firmware's FM+ STAT, FM+ PAR and FM+ DYN machines on a synth track running
 * in an emulator (see the README), the AMP section starting idle and the
 * track filter's output replaced by the machine's output x 0.4455, as the
 * plug-in has it. The stimulus plays and releases notes over the whole range,
 * moves every control, including to the ends of its range and the
 * portamento, and changes the tempo. Each block hashes the machine's output and the AMP section's left
 * and right output. */
#include <stdio.h>
#include "../source/mono_fm.h"

static uint32_t seed = 1;
static uint32_t rnd(uint32_t n) { seed = seed * 1664525u + 1013904223u; return (seed >> 8) % n; }

/* The block words of one block: the SYN and AMP pages, the trig word (40),
 * the note word (41) and the tick (42, 24 x BPM; word 35 is 0x800000 / tick).
 * Notes and the tempo change only every third block, where the Monomachine's
 * CPU sends them. */
typedef struct { int32_t syn[8], amp[8], trig, note, tick; } Words;
static void stimulus(int block, Words *w) {
   static const int amp_words[7] = {MF_ATK, MF_HOLD, MF_DEC, MF_REL, MF_VOL, MF_PAN, MF_PORT};
   if (block == 0) {
      memset(w, 0, sizeof *w);
      for (int k = 0; k < 8; ++k) w->syn[k] = 64 << 16;
      w->amp[MF_DEC] = w->amp[MF_REL] = w->amp[MF_VOL] = w->amp[MF_PAN] = 64 << 16;
      w->note = (60 << 11) / 12;
      w->tick = 2880;
   }
   w->trig = block == 0 ? 0x80 : 0;   /* the machine's assignment */
   if (block % 23 == 0 && block) {                  /* knob-like moves, also between steps and to the ends */
      const int k = (int)rnd(15);
      const int32_t v = rnd(4) == 0 ? (rnd(2) ? 0x7fffff : 0) : (int32_t)rnd(0x7f0000);
      if (k < 8) w->syn[k] = v; else w->amp[amp_words[k - 8]] = k == 10 && v > 0x700000 ? 0x700000 : v;
   }
   if (block % 3 == 0 && block) {
      const uint32_t r = rnd(100);
      if (r < 6) { w->trig = 0x81; w->note = ((rnd(5) ? 24 + (int)rnd(85) : (int)rnd(128)) << 11) / 12; }
      else if (r < 10) w->trig = 0x02;
      else if (r < 11) w->tick = 720 + (int)rnd(6481);
   }
}

static uint64_t hash = 1469598103934665603ULL;
static void mix(const int32_t *v, int n) {
   for (int i = 0; i < n; ++i)
      for (int b = 0; b < 4; ++b) hash = (hash ^ (((uint32_t)v[i] >> (8 * b)) & 0xff)) * 1099511628211ULL;
}

#define TEST_BLOCKS 12000
#define FILTER_LEVEL 0x3905a5

#ifndef TEST_CORE_NO_MAIN
int main(void) {
   static const uint64_t want[MF_MACHINES] = {0x8a967fe6b76387caULL, 0xbd1e3bbe13d89b71ULL, 0xd0549a4c257d94a8ULL};
   const uint64_t want_tables = 0x1a66828f2f85b180ULL;
   mf_tables();
   mix(mf_sine, 0x2000); mix(mf_exp, 0x800); mix(mf_tone, 128); mix(mf_ratio, 24); mix(mf_attack, 128); mix(mf_decay, 129); mix(mf_port, 128);
   int fail = hash != want_tables;
   printf("tables %016llx\n", (unsigned long long)hash);
   for (int m = 0; m < MF_MACHINES; ++m) {
      static MonoFm f;
      MfAmp env = {3, 0, 0, {0, 0}};
      MfPitch glide;
      Words w;
      seed = 1 + (uint32_t)m;
      hash = 1469598103934665603ULL;
      mf_init(&f, m);
      for (int block = 0; block < TEST_BLOCKS; ++block) {
         stimulus(block, &w);
         if (block == 0) mf_pitch_reset(&glide, w.note);
         int32_t out[MF_FRAMES], g[2][MF_FRAMES], lr[2][MF_FRAMES];
         mf_block(&f, w.syn, mf_pitch(&glide, w.note, w.syn[7], w.amp[MF_PORT], w.tick), w.trig & 0x80, out);
         mf_amp(&env, w.trig & 0x7f, w.amp, 0x800000 / w.tick, g);
         for (int c = 0; c < 2; ++c)
            for (int i = 0; i < MF_FRAMES; ++i) lr[c][i] = mf_apply(g[c][i], mf_mpy(out[i], FILTER_LEVEL));
         mix(out, MF_FRAMES); mix(lr[0], MF_FRAMES); mix(lr[1], MF_FRAMES);
      }
      printf("machine %d audio %016llx\n", m, (unsigned long long)hash);
      fail |= hash != want[m];
   }
   if (fail) { puts("FAIL: output differs from the firmware"); return 1; }
   puts("PASS core tables and audio match the firmware");
   return 0;
}
#endif
