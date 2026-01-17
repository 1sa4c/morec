#include "morse.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct MorseCtx {
    int wpm;
    int frequency;
    int sampling_rate;
    int bit_depth;
    float volume;
    int audio_format;

    double current_phase;
    int32_t max_amplitude;
    int samples_per_dot;
};

static const char *morse_table[36] = {
    ".-",       // A
    "-...",     // B
    "-.-.",     // C
    "-..",      // D
    ".",        // E
    ".-.",      // F
    "--.",      // G
    "....",     // H
    "..",       // I
    "-.-.",     // J
    "-.-",      // K
    ".-..",     // L
    "--",       // M
    "-.",       // N
    "---",      // O
    ".--.",     // P
    "--.-",     // Q
    ".-.",      // R
    "...",      // S
    "-",        // T
    "..-",      // U
    "...-",     // V
    ".--",      // W
    "-..-",     // X
    "-.--",     // Y
    "--..",     // Z
    "-----",    // 0
    ".----",    // 1
    "..---",    // 2
    "...--",    // 3
    "....-",    // 4
    ".....",    // 5
    "-....",    // 6
    "--...",    // 7
    "---..",    // 8
    "----.",    // 9
};

static int get_char_index(char c);
static int generate_tone(MorseCtx *ctx, FILE *fp, int num_samples);
static int generate_silence(FILE *fp, int num_samples);

MorseCtx* morse_init(int wpm, int frequency, int sampling_rate, int bit_depth, float volume){
    MorseCtx *ctx = malloc(sizeof(MorseCtx));
    if (!ctx) return NULL;

    ctx->wpm = wpm;
    ctx->frequency = frequency;
    ctx->sampling_rate = sampling_rate;
    ctx->bit_depth = bit_depth;
    ctx->volume = volume;
    
    ctx->current_phase = 0.0;
    ctx->max_amplitude = (int32_t)(volume * (((1 << (bit_depth - 1)) - 1)));
    
    double dot_duration = 1.2 / wpm;
    ctx->samples_per_dot = (int)(dot_duration * sampling_rate);

    return ctx;
}

void morse_free(MorseCtx *ctx){
    free(ctx);
}

uint32_t morse_calculate_size(MorseCtx *ctx, const char *text) {
    long total_dots = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c == ' ' || c == '\n') {
            total_dots += 4; 
            continue;
        }
        int idx = get_char_index(c);
        if (idx == -1) continue;

        const char *code = morse_table[idx];
        for (int j = 0; code[j] != '\0'; j++) {
            total_dots += (code[j] == '.') ? 1 : 3;
            if (code[j+1] != '\0') total_dots += 1;
        }
        total_dots += 3;
    }
    return total_dots * ctx->samples_per_dot * (ctx->bit_depth / 8);
}

void morse_generate_audio(MorseCtx *ctx, const char *text, FILE *output){
    for (int i = 0; text[i] != '\0'; i++) {
        char ch = text[i];
        
        if (ch == ' ' || ch == '\n') {
            generate_silence(output, 4 * ctx->samples_per_dot);
            continue;
        }

        int idx = get_char_index(ch);
        if (idx == -1) continue;

        const char *code = morse_table[idx];
        
        for (int j = 0; code[j] != '\0'; j++) {
            int duration = (code[j] == '.') ? 1 : 3;
            generate_tone(ctx, output, duration * ctx->samples_per_dot);
            
            if (code[j+1] != '\0') {
                generate_silence(output, ctx->samples_per_dot);
            }
        }
        generate_silence(stdout, 3 * ctx->samples_per_dot);
    }
}

static int get_char_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return c - '0' + 26;
    return -1;
}

static int generate_tone(MorseCtx *ctx, FILE *fp, int num_samples){
    int16_t sample_val;
    double phase_increment = 2.0 * M_PI * ctx->frequency / ctx->sampling_rate;


    for (int i = 0; i < num_samples; i++) {
        ctx->current_phase += phase_increment;

        if (ctx->current_phase >= 2.0 * M_PI) {
            ctx->current_phase -= 2.0 * M_PI;
        }

        sample_val = (int16_t)(sin(ctx->current_phase) * ctx->max_amplitude);

        fwrite(&sample_val, sizeof(int16_t), 1, fp);
    }

    return 0;
}

static int generate_silence(FILE *fp, int num_samples){
    int16_t silence_sample = 0;
    
    for(int i = 0; i < num_samples; i++) {
        fwrite(&silence_sample, sizeof(int16_t), 1, fp);
    }
    return 0;
}