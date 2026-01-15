#include "wav.h"
#include <string.h>

struct wav_header {
    // RIFF chunk
    char riff_header[4];
    uint32_t wav_size;
    char wave_header[4];
    
    // FMT chunk
    char fmt_header[4];
    uint32_t fmt_chunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t samp_rate;
    uint32_t byte_rate;
    uint16_t samp_alignment;
    uint16_t bit_depth;
    
    // data chunk
    char data_header[4];
    uint32_t data_bytes;
    
} __attribute__((packed));

void wav_write_header(FILE *output, uint32_t data_size, int audio_format, int num_channels, int sample_rate, int bit_depth){
    struct wav_header header;

    int byte_align = num_channels * (bit_depth / 8);
    int byte_rate = sample_rate * byte_align;

    memcpy(header.riff_header, "RIFF", 4);
    memcpy(header.wave_header, "WAVE", 4);
    memcpy(header.fmt_header, "fmt ", 4);
    memcpy(header.data_header, "data", 4);
    
    header.fmt_chunk_size = 16;
    header.audio_format = audio_format;
    header.num_channels = num_channels;
    header.samp_rate = (uint32_t)sample_rate;
    header.bit_depth = (uint16_t)bit_depth;
    header.samp_alignment = (uint16_t)byte_align;
    header.byte_rate = byte_rate;
    header.data_bytes = data_size;
    header.wav_size= data_size + 36;

    
    fwrite(&header, sizeof(header), 1, output);
}