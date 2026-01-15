#ifndef MORSE_H
#define MORSE_H

#include <stdint.h>
#include <stdio.h>

typedef struct MorseCtx MorseCtx;

// Constructor
MorseCtx* morse_init(int wpm, int frequency, int sample_rate, int bit_depth, float volume);

// Destructor
void morse_free(MorseCtx *ctx);

// Methods
uint32_t morse_calculate_size(MorseCtx *ctx, const char *text);
void morse_generate_audio(MorseCtx *ctx, const char *text, FILE *output);

#endif