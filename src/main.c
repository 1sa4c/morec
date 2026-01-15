#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wav.h"
#include "morse.h"

#define APP_SAMP_RATE 44100
#define APP_BIT_DEPTH 16
#define APP_FREQ 600
#define APP_WPM 20
#define APP_AUDIO_FORMAT 1
#define APP_VOLUME 0.5         // 0.0 to 1.0

char* get_input_text(int argc, char *argv[]){
    char *input_text = NULL;
    size_t len = 0;

    if (argc >= 2){
        input_text = strdup(argv[1]);  // malloc + strcpy
        if (!input_text) {
            return NULL;
        }
        len = strlen(input_text);
    } else {
        size_t buffer_size = 1024;
        input_text = malloc(buffer_size);
        
        if (!input_text) {
            return NULL;
        }
        
        int c;

        while ((c = fgetc(stdin)) != EOF) {
            input_text[len++] = (char)c;
            if (len == buffer_size) {
                buffer_size *= 2;
                char *new_input_text = realloc(input_text, buffer_size);
                if (!new_input_text) {
                    free(input_text);
                    return NULL;
                }
                input_text = new_input_text;
            }
        }
        input_text[len] = '\0'; // Null terminator
    }

    return input_text;
}

int main(int argc, char *argv[]){

    char *text = get_input_text(argc, argv);
    if (!text) {
        fprintf(stderr, "[!] Empty input or memory allocation error.\n");
        return 1;
    }

    MorseCtx *ctx = morse_init(APP_WPM, APP_FREQ, APP_SAMP_RATE, APP_BIT_DEPTH, APP_VOLUME);
    if (!ctx) {
        fprintf(stderr, "[!] Failed to initialize Morse object.\n");
        free(text);
        return 1;
    }

    uint32_t data_size = morse_calculate_size(ctx, text);
    wav_write_header(stdout, data_size, APP_AUDIO_FORMAT, 1, APP_SAMP_RATE, APP_BIT_DEPTH);

    morse_generate_audio(ctx, text, stdout);

    morse_free(ctx);
    free(text);
    fflush(stdout);
    
    return 0;
}
