/* Loads the built bundle like a host and checks what the core test cannot:
 * the wrapper around mono_fm.h. usage: clap_test "build/Mono FM+.clap"
 *  - the bundle offers the three machines, each with its knobs and the AMP page
 *  - at 44.1 kHz each plug-in's output is the core's, exactly, for notes
 *    taken on the 48-frame control grid
 *  - FM+ STAT as a plain carrier sounds a note at the core's frequency (A4
 *    0.8 cents under 440 Hz, as the firmware tunes it) at 44.1 and 48 kHz,
 *    and TUNE at 127 raises it by 197 cents
 *  - releasing the last note fades out; releasing the newest of two held
 *    notes returns to the other
 *  - a note's onset comes no earlier than the reported latency, and at most
 *    one control frame and one block after it, at 22.05 to 96 kHz
 *  - state round trip, rejected bad states, parameter text */
#include <clap/clap.h>
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../source/mono_fm.h"

#define CHECK(c, ...) do { if (!(c)) { printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); puts(""); exit(1); } } while (0)
#define N (192 * 512)   /* whole process calls */
#define NP 14
#define MAXEV 16

typedef union { clap_event_header_t h; clap_event_param_value_t v; clap_event_note_t n; clap_event_midi_t m; } Event;
typedef struct { clap_input_events_t api; Event v[NP + MAXEV]; uint32_t n; } Events;
static uint32_t ev_size(const clap_input_events_t *e) { return ((const Events *)e->ctx)->n; }
static const clap_event_header_t *ev_get(const clap_input_events_t *e, uint32_t i) { return &((const Events *)e->ctx)->v[i].h; }
static const void *host_ext(const clap_host_t *h, const char *id) { (void)h; (void)id; return NULL; }
static void host_cb(const clap_host_t *h) { (void)h; }
static const clap_host_t host = {CLAP_VERSION_INIT, NULL, "test", "", "", "1", host_ext, host_cb, host_cb, host_cb};
static const clap_plugin_factory_t *factory;
static const char *const ids[3] = {"works.reconstruction.monofmstat", "works.reconstruction.monofmpar", "works.reconstruction.monofmdyn"};

typedef struct { uint8_t data[256]; uint64_t size, pos; } Buffer;
static int64_t buf_write(const clap_ostream_t *s, const void *p, uint64_t n) { Buffer *b = s->ctx; if (b->size + n > sizeof b->data) return -1; memcpy(b->data + b->size, p, n); b->size += n; return (int64_t)n; }
static int64_t buf_read(const clap_istream_t *s, void *p, uint64_t n) { Buffer *b = s->ctx; if (n > b->size - b->pos) n = b->size - b->pos; memcpy(p, b->data + b->pos, n); b->pos += n; return (int64_t)n; }

/* A plug-in with these parameter values (NAN: the default), activated at rate. */
static const clap_plugin_t *make(int m, const double v[NP], double rate) {
   const clap_plugin_t *p = factory->create_plugin(factory, &host, ids[m]);
   CHECK(p && p->init(p), "create");
   const clap_plugin_params_t *params = p->get_extension(p, CLAP_EXT_PARAMS);
   Events ev = {{NULL, ev_size, ev_get}, {{{0}}}, 0};
   for (uint32_t i = 0; i < NP; ++i)
      if (!isnan(v[i]))
         ev.v[ev.n++].v = (clap_event_param_value_t){{sizeof ev.v[0].v, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0}, i, NULL, -1, -1, -1, -1, v[i]};
   ev.api.ctx = &ev;
   params->flush(p, &ev.api, NULL);
   CHECK(p->activate(p, rate, 1, 512) && p->start_processing(p), "activate");
   return p;
}
/* Notes: frame, on (1) or off (0), key; sorted by frame. */
typedef struct { int at, on, key; } Note;
static void run(const clap_plugin_t *p, const Note *notes, int count, double out[2][N]) {
   Events ev = {{NULL, ev_size, ev_get}, {{{0}}}, 0};
   ev.api.ctx = &ev;
   int next = 0;
   for (uint32_t at = 0; at < N; at += 512) {
      ev.n = 0;
      for (; next < count && notes[next].at < (int)at + 512; ++next)
         ev.v[ev.n++].n = (clap_event_note_t){{sizeof ev.v[0].n, (uint32_t)(notes[next].at - (int)at), CLAP_CORE_EVENT_SPACE_ID,
            (uint16_t)(notes[next].on ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF), 0}, -1, 0, 0, (int16_t)notes[next].key, 1.0};
      double *o[2] = {out[0] + at, out[1] + at};
      clap_audio_buffer_t ao = {NULL, o, 2, 0, 0};
      clap_process_t pr = {0};
      pr.frames_count = 512; pr.audio_outputs = &ao; pr.audio_outputs_count = 1; pr.in_events = &ev.api;
      CHECK(p->process(p, &pr) == CLAP_PROCESS_CONTINUE, "process");
   }
}
static uint32_t latency_of(const clap_plugin_t *p) { return ((const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY))->get(p); }
static void done(const clap_plugin_t *p) { p->stop_processing(p); p->deactivate(p); p->destroy(p); }
/* The frequency of the left output over [from, to), from its upward crossings of zero. */
static double frequency(double out[2][N], int from, int to, double rate) {
   int first = -1, last = -1, count = 0;
   for (int n = from + 1; n < to; ++n)
      if (out[0][n - 1] < 0 && out[0][n] >= 0) { if (first < 0) first = n; last = n; ++count; }
   return count > 1 ? (count - 1) * rate / (last - first) : 0;
}
/* The carrier's frequency for a settled note at TUNE 64: its phase step at 88.2 kHz, 8192 x 2^24 a cycle. */
static double note_hz(int note) {
   MfPitch g;
   mf_pitch_reset(&g, (note << 11) / 12);
   return (double)mf_sat48(mf_dmul(0x0be37c, mf_freq(mf_pitch(&g, (note << 11) / 12, 64 << 16)))) / 137438953472.0 * 88200;
}
static double peak(double out[2][N], int from, int to) {
   double x = 0;
   for (int n = from; n < to; ++n) x = fmax(x, fabs(out[0][n]));
   return x;
}

static double out[2][N];

int main(int argc, char **argv) {
   CHECK(argc == 2, "usage: clap_test bundle.clap");
   char path[1024];
   snprintf(path, sizeof path, "%s/Contents/MacOS/Mono FM+", argv[1]);
   void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
   CHECK(lib, "dlopen %s", path);
   const clap_plugin_entry_t *entry = dlsym(lib, "clap_entry");
   CHECK(entry && entry->init(argv[1]), "entry");
   factory = entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
   CHECK(factory && factory->get_plugin_count(factory) == 3, "three plug-ins");
   for (uint32_t m = 0; m < 3; ++m) CHECK(!strcmp(factory->get_plugin_descriptor(factory, m)->id, ids[m]), "descriptor %u", m);
   mf_tables();
   const double defaults[NP] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};

   /* exact against the core at 44.1 kHz: notes on and off at and between control frames */
   static const Note seq[] = {{100, 1, 48}, {6000, 0, 48}, {9000, 1, 67}, {9047, 1, 30}, {15000, 0, 67}, {20000, 0, 30}, {30001, 1, 90}, {60000, 0, 90}, {70000, 1, 60}};
   const int nseq = (int)(sizeof seq / sizeof seq[0]);
   for (int m = 0; m < 3; ++m) {
      const clap_plugin_t *p = make(m, defaults, 44100);
      CHECK(latency_of(p) == 0, "latency at 44.1 kHz");
      const clap_plugin_params_t *params = p->get_extension(p, CLAP_EXT_PARAMS);
      CHECK(params->count(p) == NP, "parameter count");
      int32_t syn[8], amp[8] = {0}, note = 60, word = (60 << 11) / 12, pending = 0, k = 0, bad = 0;
      for (uint32_t i = 0; i < NP; ++i) {
         clap_param_info_t info;
         CHECK(params->get_info(p, i, &info), "info");
         const int32_t w = (int32_t)info.default_value << 16;
         if (i < 8) syn[i] = w;
         else amp[(const int[]){MF_ATK, MF_HOLD, MF_DEC, MF_REL, MF_VOL, MF_PAN}[i - 8]] = w;
      }
      run(p, seq, nseq, out);
      done(p);
      static MonoFm f;
      MfAmp env = {3, 0, 0, {0, 0}};
      MfPitch glide;
      mf_init(&f, m);
      mf_pitch_reset(&glide, (60 << 11) / 12);
      for (int b = 0; b < N / MF_FRAMES; ++b) {
         for (; k < nseq && seq[k].at <= b * MF_FRAMES; ++k)   /* no release here falls back to a note still held */
            if (seq[k].on) { pending = 1; note = seq[k].key; }
            else if (seq[k].key == note) pending = 2;
         int trig = 0;
         if (b % 3 == 0) { trig = b ? (pending == 1 ? 0x81 : pending == 2 ? 0x02 : 0) : 0x80 | (pending == 1 ? 0x81 : 0); pending = 0; word = (note << 11) / 12; }
         int32_t y[MF_FRAMES], g[2][MF_FRAMES];
         mf_block(&f, syn, mf_pitch(&glide, word, syn[7]), trig & 0x80, y);
         mf_amp(&env, trig & 0x7f, amp, 0x800000 / 2880, g);
         for (int c = 0; c < 2; ++c)
            for (int i = 0; i < MF_FRAMES; ++i) bad += out[c][b * MF_FRAMES + i] != mf_apply(g[c][i], mf_mpy(y[i], 0x3905a5)) / 8388608.0;
      }
      CHECK(bad == 0, "machine %d: %d frames differ from the core", m, bad);
      CHECK(peak(out, 0, N) > 0.05, "machine %d is silent", m);
   }

   /* pitch: FM+ STAT with 2VOL and 1ENV at 0 is the carrier alone */
   const double plain[NP] = {NAN, NAN, 0, NAN, NAN, 0, NAN, NAN, NAN, NAN, 127, NAN, 127, NAN};
   static const Note a4[] = {{0, 1, 69}};
   for (int r = 0; r < 2; ++r) {
      const double rate = r ? 48000 : 44100;
      const clap_plugin_t *p = make(MF_STAT, plain, rate);
      run(p, a4, 1, out);
      done(p);
      const double hz = frequency(out, N / 4, N, rate);
      CHECK(fabs(hz - note_hz(69)) < 0.01 && fabs(1200 * log2(hz / 440)) < 1, "A4 at %.0f Hz is %.3f Hz, not %.3f", rate, hz, note_hz(69));
   }
   double tuned[NP];
   memcpy(tuned, plain, sizeof tuned);
   tuned[7] = 127;
   {
      const clap_plugin_t *p = make(MF_STAT, tuned, 44100);
      run(p, a4, 1, out);
      done(p);
      const double cents = 1200 * log2(frequency(out, N / 4, N, 44100) / 440);
      CHECK(fabs(cents - 197) < 1, "TUNE 127 is %+.1f cents", cents);
   }

   /* the note stack and the release */
   {
      double v[NP];
      memcpy(v, plain, sizeof v);
      v[11] = 0;   /* Release 0 */
      static const Note stack[] = {{0, 1, 57}, {20000, 1, 69}, {40000, 0, 69}, {70000, 0, 57}};
      const clap_plugin_t *p = make(MF_STAT, v, 44100);
      run(p, stack, 4, out);
      done(p);
      CHECK(fabs(frequency(out, 5000, 20000, 44100) - note_hz(57)) < 0.05, "first note");
      CHECK(fabs(frequency(out, 25000, 40000, 44100) - note_hz(69)) < 0.05, "second note");
      CHECK(fabs(frequency(out, 45000, 70000, 44100) - note_hz(57)) < 0.05, "back to the first note");
      CHECK(peak(out, 50000, 70000) > 0.1 && peak(out, 75000, N) < 1e-6, "release to silence (%g)", peak(out, 75000, N));
   }

   /* onset against the reported latency */
   static const double rates[] = {22050, 44100, 48000, 96000};
   for (int r = 0; r < 4; ++r) {
      const double rate = rates[r], ratio = rate / 44100;
      for (int t = 0; t < 4; ++t) {
         const int at = 10000 + 37 * t;
         const Note one[] = {{at, 1, 45}};
         const clap_plugin_t *p = make(MF_STAT, plain, rate);
         const int latency = (int)latency_of(p);
         run(p, one, 1, out);
         done(p);
         const double level = peak(out, at + 4000, at + 8000);
         int onset = 0;
         while (onset < N && fabs(out[0][onset]) < 0.05 * level) ++onset;
         CHECK(onset >= at + latency - 2 && onset <= at + latency + (int)ceil(66 * ratio) + 2,
               "%.0f Hz: note at %d, latency %d, onset %d", rate, at, latency, onset);
      }
   }

   /* state, parameter text */
   for (int m = 0; m < 3; ++m) {
      double v[NP];
      for (int i = 0; i < NP; ++i) v[i] = (i * 37 + m * 11) % 128;
      const clap_plugin_t *p = make(m, v, 44100), *q = make(m, defaults, 44100), *other = make((m + 1) % 3, defaults, 44100);
      const clap_plugin_state_t *st = p->get_extension(p, CLAP_EXT_STATE);
      Buffer b = {{0}, 0, 0};
      clap_ostream_t os = {&b, buf_write};
      CHECK(st->save(p, &os), "save");
      clap_istream_t is = {&b, buf_read};
      CHECK(((const clap_plugin_state_t *)q->get_extension(q, CLAP_EXT_STATE))->load(q, &is), "load");
      const clap_plugin_params_t *pp = q->get_extension(q, CLAP_EXT_PARAMS);
      for (uint32_t i = 0; i < NP; ++i) { double x; CHECK(pp->get_value(q, i, &x) && x == v[i], "restored value %u", i); }
      b.pos = 0;
      CHECK(!((const clap_plugin_state_t *)other->get_extension(other, CLAP_EXT_STATE))->load(other, &is), "another machine's state accepted");
      b.pos = 0; b.data[8] ^= 0xff; b.data[15] = 0x7f;
      CHECK(!st->load(p, &is), "bad value accepted");
      char text[64];
      CHECK(pp->value_to_text(q, 7, 127, text, sizeof text) && strstr(text, "+197 cents"), "TUNE text %s", text);
      done(p); done(q); done(other);
   }
   {
      const clap_plugin_t *p = make(MF_STAT, defaults, 44100);
      const clap_plugin_params_t *pp = p->get_extension(p, CLAP_EXT_PARAMS);
      char text[64];
      CHECK(pp->value_to_text(p, 0, 80, text, sizeof text) && !strcmp(text, "80.0 (x1)"), "ratio text %s", text);
      CHECK(pp->value_to_text(p, 1, 64, text, sizeof text) && !strcmp(text, "64.0 (+0)"), "bipolar text %s", text);
      double x;
      CHECK(pp->text_to_value(p, 3, "100", &x) && x == 100, "text to value");
      done(p);
   }
   entry->deinit();
   puts("PASS");
   return 0;
}
