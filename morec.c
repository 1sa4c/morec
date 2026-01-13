#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#define SAMP_RATE 44100
#define BIT_DEPTH 16
#define FREQ 600
#define WPM 20
#define VOLUME 0.5         // 0.0 to 1.0

const char *morse_table[36] = {
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

const int32_t MAX_AMPLITUDE = (int32_t)(VOLUME * (((1 << (BIT_DEPTH - 1)) - 1)));

int get_char_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return c - '0' + 26;
    return -1;
}

int calculate_dots_cost(char c) {
    if (c == ' ') return 4;
    
    int idx = get_char_index(c);
    if (idx == -1) return 0;

    const char *code = morse_table[idx];
    int dots = 0;

    for (int j = 0; code[j] != '\0'; j++) {
        if (code[j] == '.') dots += 1;
        else if (code[j] == '-') dots += 3;
        
        if (code[j+1] != '\0') dots += 1;
    }
    
    dots += 3; 
    return dots;
}

int generate_silence(FILE *fp, int num_samples){
    int16_t silence_sample = 0;
    
    for(int i = 0; i < num_samples; i++) {
        fwrite(&silence_sample, sizeof(int16_t), 1, fp);
    }
    return 0;
}

static double current_phase = 0.0;

int generate_tone(FILE *fp, int num_samples){
    int16_t sample_val;
    double phase_increment = 2.0 * M_PI * FREQ / SAMP_RATE;


    for (int i = 0; i < num_samples; i++) {
        current_phase += phase_increment;

        if (current_phase >= 2.0 * M_PI) {
            current_phase -= 2.0 * M_PI;
        }

        sample_val = (int16_t)(sin(current_phase) * MAX_AMPLITUDE);

        fwrite(&sample_val, sizeof(int16_t), 1, fp);
    }
}


int main(int argc, char *argv[]){

    char *input_text = NULL;
    size_t len = 0;


    if (argc >= 2){
        input_text = strdup(argv[1]);  // malloc + strcpy
        if (!input_text) {
            fprintf(stderr, "[!] Error allocating memory.\n");
            return 1;
        }
        len = strlen(input_text);
    } else {
        size_t buffer_size = 1024;
        input_text = malloc(buffer_size);
        
        if (!input_text) {
            fprintf(stderr, "[!] Error allocating memory.\n");
            return 1;
        }
        
        int c;

        while ((c = fgetc(stdin)) != EOF) {
            input_text[len++] = (char)c;
            if (len == buffer_size) {
                buffer_size *= 2;
                char *new_input_text = realloc(input_text, buffer_size);
                if (!new_input_text) {
                    free(input_text);
                    fprintf(stderr, "[!] Memory error (buffer too large).\n");
                    return 1;
                }
                input_text = new_input_text;
            }
        }
        input_text[len] = '\0'; // Null terminator
    }

    double dot_duration = 1.2 / WPM;
    int samples_per_dot = (int)(dot_duration * SAMP_RATE);
    int bytes_per_sample = (int)(BIT_DEPTH / 8);
    
    long total_dots_needed = 0;
    for (size_t i = 0; i < len; i++) {
        total_dots_needed += calculate_dots_cost(input_text[i]);
    }
    
    uint32_t data_size = total_dots_needed * samples_per_dot * bytes_per_sample;
    
    struct wav_header header;
    memcpy(header.riff_header, "RIFF", 4);
    memcpy(header.wave_header, "WAVE", 4);
    memcpy(header.fmt_header, "fmt ", 4);
    memcpy(header.data_header, "data", 4);
    
    header.fmt_chunk_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = 1; // Mono
    header.samp_rate = SAMP_RATE;
    header.bit_depth = BIT_DEPTH;
    header.samp_alignment = header.num_channels * (header.bit_depth / 8);
    header.byte_rate = header.samp_rate * header.samp_alignment;
    header.data_bytes = data_size;
    header.wav_size= data_size + 36;

    
    fwrite(&header, sizeof(header), 1, stdout);
    
    for (size_t i = 0; i < len; i++) {
        char ch = input_text[i];
        
        if (ch == ' ' || ch == '\n') {
            generate_silence(stdout, 4 * samples_per_dot);
            continue;
        }

        int idx = get_char_index(ch);
        if (idx == -1) continue;

        const char *code = morse_table[idx];
        
        for (int j = 0; code[j] != '\0'; j++) {
            if (code[j] == '.') generate_tone(stdout, samples_per_dot);
            else if (code[j] == '-') generate_tone(stdout, 3 * samples_per_dot);

            if (code[j+1] != '\0') generate_silence(stdout, samples_per_dot);
        }
        generate_silence(stdout, 3 * samples_per_dot);
    }

    free(input_text);
    fflush(stdout);
    
    return 0;
}
