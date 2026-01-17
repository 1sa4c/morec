#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "wav.h"
#include "morse.h"

#define DEFAULT_SAMP_RATE 44100
#define DEFAULT_BIT_DEPTH 16
#define DEFAULT_FREQ 600
#define DEFAULT_WPM 20
#define DEFAULT_ENCODING 1
#define DEFAULT_VOLUME 0.5         // 0.0 to 1.0

typedef struct {
    int wpm;
    int freq;
    int sampling_rate;
    int bit_depth;
    float volume;
    int encoding;
    char *input_text;
    int show_help;
} AppConfig;

static void print_help(const char *prog_name);
static AppConfig parse_args(int argc, char *argv[]);
static char* read_stdin();

int main(int argc, char *argv[]){

    AppConfig config = parse_args(argc, argv);

    if (config.show_help) {
        print_help(argv[0]);
        return 0;
    }

    if (config.input_text == NULL) {
        if (isatty(STDIN_FILENO)) {
            fprintf(stderr, "[!] Error: No input text.\n");
            print_help(argv[0]);
            return 1;
        }

        config.input_text = read_stdin();
    }

    if (!config.input_text || strlen(config.input_text) == 0) {
        fprintf(stderr, "[!] Error: No input text\n");
        print_help(argv[0]);
        return 1;
    }

    MorseCtx *ctx = morse_init(config.wpm, config.freq, config.sampling_rate, config.bit_depth, config.volume);
    if (!ctx) {
        fprintf(stderr, "[!] Failed to initialize Morse object.\n");
        free(config.input_text);
        return 1;
    }

    uint32_t data_size = morse_calculate_size(ctx, config.input_text);
    wav_write_header(stdout, data_size, config.encoding, 1, config.sampling_rate, config.bit_depth);

    morse_generate_audio(ctx, config.input_text, stdout);

    morse_free(ctx);
    free(config.input_text);
    fflush(stdout);
    
    return 0;
}

static AppConfig parse_args(int argc, char *argv[]){
    AppConfig config = {
        .wpm = DEFAULT_WPM,
        .freq = DEFAULT_FREQ,
        .sampling_rate = DEFAULT_SAMP_RATE,
        .bit_depth = DEFAULT_BIT_DEPTH,
        .volume = DEFAULT_VOLUME,
        .encoding = DEFAULT_ENCODING,
        .input_text = NULL,
        .show_help = 0
    };

    static struct option long_options[] = {
        {"wpm",         required_argument, 0, 'w'},
        {"freq",        required_argument, 0, 'f'},
        {"sample-rate", required_argument, 0, 'r'},
        {"bit-depth",   required_argument, 0, 'b'},
        {"volume",      required_argument, 0, 'v'},
        {"encoding",    required_argument, 0, 'e'},
        {"help",        no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "w:f:r:b:v:e:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'w':
                config.wpm = atoi(optarg);
                if (config.wpm <= 0){
                    fprintf(stderr, "[*] Warning: Speed: %d WPM not supported. Using default (%d WPM).\n", config.wpm, DEFAULT_WPM);
                    config.wpm = DEFAULT_WPM;
                }
                break;
            case 'f':
                config.freq = atoi(optarg);
                if (config.freq <= 0){
                    fprintf(stderr, "[*] Warning: Frequency: %dHz not supported. Using default (%dHz).\n", config.freq, DEFAULT_FREQ);
                    config.freq = DEFAULT_FREQ;
                }
                break;
            case 'r':
                config.sampling_rate = atoi(optarg);
                if (config.sampling_rate <= 0){
                    fprintf(stderr, "[*] Warning: Sampling rate: %dHz not supported. Using default (%dHz).\n", config.sampling_rate, DEFAULT_SAMP_RATE);
                    config.sampling_rate = DEFAULT_SAMP_RATE;
                }
                break;
            case 'b':
                config.bit_depth = atoi(optarg);
                if (config.bit_depth != 8 && config.bit_depth != 16) {
                    fprintf(stderr, "[*] Warning: Bit depth: %db not supported. Using default (%db).\n", config.bit_depth, DEFAULT_BIT_DEPTH);
                    config.bit_depth = DEFAULT_BIT_DEPTH;
                }
                break;
            case 'v':
                config.volume = atof(optarg);
                if (config.volume < 0.0 || config.volume > 1.0){
                    fprintf(stderr, "[*] Warning: Volume %.2f not supported. Using default (%.2f).\n", config.volume, DEFAULT_VOLUME);
                    config.volume = DEFAULT_VOLUME;
                }
                break;
            case 'e':
                config.encoding = atoi(optarg);
                if (config.encoding != 1){
                    fprintf(stderr, "[*] Warning: Audio format %d not supported. Using default (%d - PCM).\n", config.encoding, DEFAULT_ENCODING);
                    config.encoding = DEFAULT_ENCODING;
                }
                break;
            case 'h':
                config.show_help = 1;
                break;
            case '?':
                break;
        }
    }

    if (optind < argc) {
        config.input_text = strdup(argv[optind]);
    }

    return config;
}

static char* read_stdin() {
    size_t buffer_size = 1024;
    size_t len = 0;
    char *input = malloc(buffer_size);
    if (!input) return NULL;

    int c;
    while ((c = fgetc(stdin)) != EOF) {
        input[len++] = (char)c;
        if (len == buffer_size) {
            buffer_size *= 2;
            char *new_ptr = realloc(input, buffer_size);
            if (!new_ptr) {
                free(input);
                return NULL;
            }
            input = new_ptr;
        }
    }
    input[len] = '\0';
    return input;
}

static void print_help(const char *prog_name) {
    fprintf(stderr, "Use: %s [OPTIONS] [TEXT]\n", prog_name);
    fprintf(stderr, "Converts text to Morse code in WAV format (stdout).\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -w, --wpm <num>          Speed in Words per Minute (Default: %d)\n", DEFAULT_WPM);
    fprintf(stderr, "  -f, --freq <Hz>          Tone frequency (Default: %dHz)\n", DEFAULT_FREQ);
    fprintf(stderr, "  -r, --sampling-rate <Hz> Sampling rate (Default: %d)\n", DEFAULT_SAMP_RATE);
    fprintf(stderr, "  -b, --bit-depth <num>    Bit depth (Default: %d)\n", DEFAULT_BIT_DEPTH);
    fprintf(stderr, "  -v, --volume <num>       Volume from 0 to 1 (Default: %.2f)\n", DEFAULT_VOLUME);
    fprintf(stderr, "  -e, --encoding <num>     Audio encoding format (Default: %d (PCM))\n", DEFAULT_ENCODING);
    fprintf(stderr, "  -h, --help               Shows this message\n\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s -w 15 \"SOS\"\n", prog_name);
    fprintf(stderr, "  echo \"Vasco da Gama\" | %s --freq 800 > output.wav\n", prog_name);
}