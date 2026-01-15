#ifndef WAV_H
#define WAV_H

#include <stdio.h>
#include <stdint.h>

void wav_write_header(FILE *output, uint32_t data_size, int audio_format, int num_channels, int sample_rate, int bit_depth);

#endif