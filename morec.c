#include <stdio.h>
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

int get_char_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return c - '0' + 26;
    return -1;
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
    double max_amplitude = VOLUME * (((1 << (BIT_DEPTH - 1)) - 1));

    for (int i = 0; i < num_samples; i++) {
        current_phase += phase_increment;

        if (current_phase >= 2.0 * M_PI) {
            current_phase -= 2.0 * M_PI;
        }

        sample_val = (int16_t)(sin(current_phase) * VOLUME * max_amplitude);

        fwrite(&sample_val, sizeof(int16_t), 1, fp);
    }
}

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


int main(int argc, char *argv[]){

    if (argc < 2){
        printf("[!] Usage: %s text", argv[0]);
        return 1;
    }
    
    char *input_text = argv[1];
    
    double dot_duration = 1.2 / WPM;
    int samples_per_dot = (int)(dot_duration * SAMP_RATE);
    int bytes_per_dot = (int)(samples_per_dot * (BIT_DEPTH / 8));

    FILE *fp = fopen("output.wav", "wb");
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
    header.data_bytes = 0;
    header.wav_size= 0;

    
    fwrite(&header, sizeof(header), 1, fp);
    printf("[*] Dummy header written.\n");

    for (int i = 0; input_text[i] != '\0'; i++) {
        char c = input_text[i];

        if (c == ' ') {
            generate_silence(fp, 4 * samples_per_dot);
            continue;
        }
        
        int idx = get_char_index(c);
        if (idx == -1) continue;

        const char *code = morse_table[idx];
        
        for (int j = 0; code[j] != '\0'; j++) {
            if (code[j] == '.'){
                generate_tone(fp, samples_per_dot);
            }
            else if (code[j] == '-'){
                generate_tone(fp, 3 * samples_per_dot);
            }

            if (code[j+1] != '\0'){
                generate_silence(fp, samples_per_dot);
            }
        }
        
        generate_silence(fp, 3 * samples_per_dot);
    }
    
    long total_data_bytes = ftell(fp) - 44;
    
    header.data_bytes = (uint32_t) total_data_bytes;
    header.wav_size = header.data_bytes + 36;  // total size - 8 initial bytes
    
    fseek(fp, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, fp);
    printf("[*] Real header written.\n");

    fclose(fp);
    printf("[*] WAV file successfully created.\n");

    return 0;
}
