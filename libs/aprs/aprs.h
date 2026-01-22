#pragma once

#include <stdint.h>

/*
    customizable vars via titan_config.h

    #define APRS_AX25_HEAD_0FLAG_OCTETS           0
    #define APRS_AX25_HEAD_OCTETS                 4
    #define APRS_AX25_TAIL_OCTETS                 3
*/

#ifndef APRS_AX25_HEAD_0FLAG_OCTETS
#define APRS_AX25_HEAD_0FLAG_OCTETS           0
#endif

#ifndef APRS_AX25_HEAD_OCTETS
#define APRS_AX25_HEAD_OCTETS                 4
#endif

#ifndef APRS_AX25_TAIL_OCTETS
#define APRS_AX25_TAIL_OCTETS                 3
#endif

#define APRS_AX25_FRAME_SIZE_BYTES            (7 + 7 + 1 + 1 + 256 + 2)
#define APRS_AX25_FRAME_MAX_LENGTH_UNSTUFFED  (APRS_AX25_HEAD_0FLAG_OCTETS + APRS_AX25_HEAD_OCTETS + APRS_AX25_FRAME_SIZE_BYTES + APRS_AX25_TAIL_OCTETS)
#define APRS_AX25_FRAME_MAX_LENGTH            (APRS_AX25_FRAME_MAX_LENGTH_UNSTUFFED + (APRS_AX25_FRAME_MAX_LENGTH_UNSTUFFED / 6))

#define APRS_APRS1200_AUDIO_SAMPLE_FREQUENCY  48000
#define APRS_APRS1200_SAMPLES_PER_BIT         (APRS_APRS1200_AUDIO_SAMPLE_FREQUENCY/1200)
#define APRS_APRS1200_AUDIO_MAX_LENGTH        (APRS_APRS1200_SAMPLES_PER_BIT*8*APRS_AX25_FRAME_MAX_LENGTH + 1)
#define APRS_APRS9600_AUDIO_MAX_LENGTH        (5*8*APRS_AX25_FRAME_MAX_LENGTH + 1)

typedef struct __aprs {
    uint8_t source[7];
    uint8_t destination[7];
    uint8_t *information;
    uint8_t information_length;

    uint8_t destination_set;    //flag to check if dst is set
    uint8_t source_set;         //flag to check if src is set
    uint8_t information_set;    //flag to check if info is set

    uint8_t layer1_frame[APRS_AX25_FRAME_MAX_LENGTH_UNSTUFFED];
    uint16_t layer1_frame_length;

    uint8_t *nrzi_frame;
    uint16_t nrzi_frame_length;

    //step-enablers
    uint8_t init;
    uint16_t s;
    uint16_t i;
    uint8_t j;
    uint8_t k;
    uint8_t l;
    uint8_t current_byte;
    uint8_t current_bit;
    uint8_t mark_final;
    uint8_t filter_index;
    float track_error;
} aprs_t;

void aprs_init(aprs_t *aprs, uint8_t *aprs_nrzi_frame);

void aprs_set_destination(aprs_t *aprs, const char* destination, const uint8_t ssid);
void aprs_set_source(aprs_t *aprs, const char* source, const uint8_t ssid);
void aprs_set_information(aprs_t *aprs, const uint8_t* information, const uint8_t information_length);

void aprs_build_ax25_frame(aprs_t *aprs, const uint8_t scramble);   //build the ax25 frame, and choose wheter to use scramble or not (0/1)

//functions to build audio aprs, with existing memory for audio buffer to be stored in. (fastest, but uses lots of memory)
void aprs_build_audio1200(aprs_t *aprs, uint8_t *aprs_audio, uint32_t *aprs_audio_length);
void aprs_build_audio9600(aprs_t *aprs, const uint8_t filter, uint8_t *aprs_audio, uint32_t *aprs_audio_length);

//functions to build audio aprs, without the need for audio sample storage (data is built step by step). (slower, but requires no memory for audio)
//returns 1 when the sample is the final one
uint8_t aprs_build_audio1200_step(aprs_t *aprs, uint8_t *current_sample);
uint8_t aprs_build_audio9600_step(aprs_t *aprs, const uint8_t filter, uint8_t *current_sample);


/*
    usage:

    aprs_t aprs;
    uint8_t aprs_nrzi_frame[APRS_AX25_FRAME_MAX_LENGTH];    //buffer for storing nrzi frame

    aprs_init(&aprs, aprs_nrzi_frame);
    aprs_set_destination(&aprs, "EARTH", 0);
    aprs_set_source(&aprs, "CUBSAT", 0);
    aprs_set_information(&aprs, "I wanna send this really well!", 30);

    above part is common on any mode

    now, if full buffers are used:



    uint8_t aprs_audio[APRS_APRS1200_AUDIO_MAX_LENGTH];
    uint32_t aprs_audio_len;

    //example APRS1200 (aprs.fi) - scrambling off
    aprs_build_ax25_frame(&aprs, 0);
    aprs_build_audio1200(&aprs, aprs_audio, &aprs_audio_len);
    now_output_to_dac(aprs_audio, aprs_audio_len, sampling_freq=48000);

    //example APRS9600 - scrambling on, and filter is RECOMMENDED
    aprs_build_ax25_frame(&aprs, 1);
    aprs_build_audio9600(&aprs, 1, aprs_audio, &aprs_audio_len);
    now_output_to_dac(aprs_audio, aprs_audio_len, sampling_freq=48000);

    //example APRS19200 - scrambling on, and filter is RECOMMENDED --- same as APRS9600, but with twice the output sampling rate
    aprs_build_ax25_frame(&aprs, 1);
    aprs_build_audio9600(&aprs, 1, aprs_audio, &aprs_audio_len);
    now_output_to_dac(aprs_audio, aprs_audio_len, sampling_freq=96000);


    OR for step-enabled, no buffers functions are used:

    aprs_build_ax25_frame(&aprs, 1);
    uint8_t sample;
    while(aprs_build_audio9600_step(&aprs, 1, &sample) == 0) {
        output_to_dac(sample)
    }

    OR for step-enabled, with interrupt (async) from peripheral (DAC):
    aprs_build_ax25_frame(&aprs, 1);
    dac_start(required_sampling_rate);

    void on_dac() {
        uint8_t sample;
        if(aprs_build_audio9600_step(&aprs, 1, &sample)) {
            dac_stop();
        }
        dac_set_value(sample);
    }



    ----------- full examples ---------------

    #include <periph/gpio.h>
    #include <periph/timer.h>
    #include <periph/cpu.h>
    #include <aprs/aprs.h>

    static aprs_t aprs;
    static uint8_t aprs_nrzi_frame[APRS_AX25_FRAME_MAX_LENGTH];    //buffer for storing nrzi frame

    void on_dac() {
        uint8_t sample;
        if(aprs_build_audio1200_step(&aprs, &sample)) {     //or, for 9600 and 19200: if(aprs_build_audio9600_step(&aprs, 1, &sample)) {
            timer_stop((timer_t)TIM6);
            timer_clear((timer_t)TIM6);
        }

        gpio_analog_write(DAC_OUT, sample);
    }


    //init section
    gpio_init_analog(DAC_OUT, GPIO_MODE_ANALOG_OUT, 8);
    timer_init((timer_t)TIM6, cpu_clock_speed() / 10000000, on_dac);


    //sending a message section
    aprs_init(&aprs, aprs_nrzi_frame);
    aprs_set_destination(&aprs, "EARTH", 0);
    aprs_set_source(&aprs, "CUBSAT", 0);
    
    aprs_set_information(&aprs, "I wanna send this really well!", 30);
    aprs_build_ax25_frame(&aprs, 0);    //1 if audio9600 is used
    timer_start((timer_t)TIM6, 208);  //equivalent to 48KHz  // or 96KHz for APRS19200
*/
