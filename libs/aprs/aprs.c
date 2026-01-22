#include "aprs.h"
#include <string.h>
#include <assert.h>

TITAN_DEBUG_FILE_MARK;

#define APRS_APRS1200_AFSK_SINE_TABLE_SIZE    1024

#define APRS_APRS1200_MARK_ADVANCE            (APRS_APRS1200_AFSK_SINE_TABLE_SIZE*1200/APRS_APRS1200_AUDIO_SAMPLE_FREQUENCY)
#define APRS_APRS1200_SPACE_ADVANCE           (APRS_APRS1200_AFSK_SINE_TABLE_SIZE*2200/APRS_APRS1200_AUDIO_SAMPLE_FREQUENCY)

#define APRS_APRS1200_MARK_ADVANCE_ERROR      (float)(APRS_APRS1200_AFSK_SINE_TABLE_SIZE*1200.0/APRS_APRS1200_AUDIO_SAMPLE_FREQUENCY - APRS_APRS1200_MARK_ADVANCE)
#define APRS_APRS1200_SPACE_ADVANCE_ERROR     (float)(APRS_APRS1200_AFSK_SINE_TABLE_SIZE*2200.0/APRS_APRS1200_AUDIO_SAMPLE_FREQUENCY - APRS_APRS1200_SPACE_ADVANCE)

static const uint8_t aprs_full_sine[APRS_APRS1200_AFSK_SINE_TABLE_SIZE] = {
    127, 128, 129, 129, 130, 131, 132, 132, 133, 134, 135, 136, 136, 137, 138, 139, 139, 140, 141,
    142, 143, 143, 144, 145, 146, 146, 147, 148, 149, 150, 150, 151, 152, 153, 153, 154, 155, 156,
    156, 157, 158, 159, 159, 160, 161, 162, 163, 163, 164, 165, 166, 166, 167, 168, 168, 169, 170,
    171, 171, 172, 173, 174, 174, 175, 176, 177, 177, 178, 179, 179, 180, 181, 182, 182, 183, 184,
    184, 185, 186, 186, 187, 188, 188, 189, 190, 191, 191, 192, 193, 193, 194, 195, 195, 196, 197,
    197, 198, 198, 199, 200, 200, 201, 202, 202, 203, 204, 204, 205, 205, 206, 207, 207, 208, 208,
    209, 210, 210, 211, 211, 212, 213, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 219,
    220, 220, 221, 221, 222, 223, 223, 224, 224, 225, 225, 226, 226, 227, 227, 228, 228, 228, 229,
    229, 230, 230, 231, 231, 232, 232, 233, 233, 233, 234, 234, 235, 235, 236, 236, 236, 237, 237,
    238, 238, 238, 239, 239, 239, 240, 240, 241, 241, 241, 242, 242, 242, 243, 243, 243, 244, 244,
    244, 244, 245, 245, 245, 246, 246, 246, 247, 247, 247, 247, 248, 248, 248, 248, 249, 249, 249,
    249, 249, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 252,
    253, 253, 253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 254, 254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 253, 253, 253, 253, 253, 253, 253, 253,
    252, 252, 252, 252, 252, 252, 252, 251, 251, 251, 251, 251, 250, 250, 250, 250, 250, 249, 249,
    249, 249, 249, 248, 248, 248, 248, 247, 247, 247, 247, 246, 246, 246, 245, 245, 245, 244, 244,
    244, 244, 243, 243, 243, 242, 242, 242, 241, 241, 241, 240, 240, 239, 239, 239, 238, 238, 238,
    237, 237, 236, 236, 236, 235, 235, 234, 234, 233, 233, 233, 232, 232, 231, 231, 230, 230, 229,
    229, 228, 228, 228, 227, 227, 226, 226, 225, 225, 224, 224, 223, 223, 222, 221, 221, 220, 220,
    219, 219, 218, 218, 217, 217, 216, 215, 215, 214, 214, 213, 213, 212, 211, 211, 210, 210, 209,
    208, 208, 207, 207, 206, 205, 205, 204, 204, 203, 202, 202, 201, 200, 200, 199, 198, 198, 197,
    197, 196, 195, 195, 194, 193, 193, 192, 191, 191, 190, 189, 188, 188, 187, 186, 186, 185, 184,
    184, 183, 182, 182, 181, 180, 179, 179, 178, 177, 177, 176, 175, 174, 174, 173, 172, 171, 171,
    170, 169, 168, 168, 167, 166, 166, 165, 164, 163, 163, 162, 161, 160, 159, 159, 158, 157, 156,
    156, 155, 154, 153, 153, 152, 151, 150, 150, 149, 148, 147, 146, 146, 145, 144, 143, 143, 142,
    141, 140, 139, 139, 138, 137, 136, 136, 135, 134, 133, 132, 132, 131, 130, 129, 129, 128, 127,
    126, 125, 125, 124, 123, 122, 122, 121, 120, 119, 118, 118, 117, 116, 115, 115, 114, 113, 112,
    111, 111, 110, 109, 108, 108, 107, 106, 105, 104, 104, 103, 102, 101, 101, 100,  99,  98,  98,
    97,  96,  95,  95,  94,  93,  92,  91,  91,  90,  89,  88,  88,  87,  86,  86,  85,  84,  83,
    83,  82,  81,  80,  80,  79,  78,  77,  77,  76,  75,  75,  74,  73,  72,  72,  71,  70,  70,
    69,  68,  68,  67,  66,  66,  65,  64,  63,  63,  62,  61,  61,  60,  59,  59,  58,  57,  57,
    56,  56,  55,  54,  54,  53,  52,  52,  51,  50,  50,  49,  49,  48,  47,  47,  46,  46,  45,
    44,  44,  43,  43,  42,  41,  41,  40,  40,  39,  39,  38,  37,  37,  36,  36,  35,  35,  34,
    34,  33,  33,  32,  31,  31,  30,  30,  29,  29,  28,  28,  27,  27,  26,  26,  26,  25,  25,
    24,  24,  23,  23,  22,  22,  21,  21,  21,  20,  20,  19,  19,  18,  18,  18,  17,  17,  16,
    16,  16,  15,  15,  15,  14,  14,  13,  13,  13,  12,  12,  12,  11,  11,  11,  10,  10,  10,
    10,   9,   9,   9,   8,   8,   8,   7,   7,   7,   7,   6,   6,   6,   6,   5,   5,   5,   5,
    5,   4,   4,   4,   4,   4,   3,   3,   3,   3,   3,   2,   2,   2,   2,   2,   2,   2,   1,
    1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   2,
    2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,
    5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  10,
    10,  11,  11,  11,  12,  12,  12,  13,  13,  13,  14,  14,  15,  15,  15,  16,  16,  16,  17,
    17,  18,  18,  18,  19,  19,  20,  20,  21,  21,  21,  22,  22,  23,  23,  24,  24,  25,  25,
    26,  26,  26,  27,  27,  28,  28,  29,  29,  30,  30,  31,  31,  32,  33,  33,  34,  34,  35,
    35,  36,  36,  37,  37,  38,  39,  39,  40,  40,  41,  41,  42,  43,  43,  44,  44,  45,  46,
    46,  47,  47,  48,  49,  49,  50,  50,  51,  52,  52,  53,  54,  54,  55,  56,  56,  57,  57,
    58,  59,  59,  60,  61,  61,  62,  63,  63,  64,  65,  66,  66,  67,  68,  68,  69,  70,  70,
    71,  72,  72,  73,  74,  75,  75,  76,  77,  77,  78,  79,  80,  80,  81,  82,  83,  83,  84,
    85,  86,  86,  87,  88,  88,  89,  90,  91,  91,  92,  93,  94,  95,  95,  96,  97,  98,  98,
    99, 100, 101, 101, 102, 103, 104, 104, 105, 106, 107, 108, 108, 109, 110, 111, 111, 112, 113,
    114, 115, 115, 116, 117, 118, 118, 119, 120, 121, 122, 122, 123, 124, 125, 125, 126
};

static const uint8_t aprs9600_output_filter[32][5] = {
    { 25,  25,  25,  25,  25},
    { 32,  36,  33,  25,  13},
    {  0,   3,  29,  78, 140},
    {  7,  13,  37,  77, 128},
    {196, 230, 230, 196, 140},
    {203, 241, 238, 196, 128},
    {171, 208, 234, 249, 254},
    {178, 218, 242, 248, 242},
    { 78,  29,   3,   0,  13},
    { 84,  39,  11,   0,   1},
    { 53,   6,   6,  53, 128},
    { 59,  17,  14,  52, 115},
    {249, 234, 208, 171, 128},
    {255, 244, 216, 171, 115},
    {224, 211, 211, 224, 242},
    {230, 222, 219, 223, 230},
    { 25,  33,  36,  32,  25},
    { 31,  44,  44,  31,  13},
    {  0,  11,  39,  84, 140},
    {  6,  21,  47,  84, 128},
    {196, 238, 241, 203, 140},
    {202, 249, 249, 202, 128},
    {171, 216, 244, 255, 254},
    {177, 226, 252, 255, 242},
    { 77,  37,  13,   7,  13},
    { 84,  47,  21,   6,   1},
    { 52,  14,  17,  59, 128},
    { 59,  25,  25,  59, 115},
    {248, 242, 218, 178, 128},
    {255, 252, 226, 177, 115},
    {223, 219, 222, 230, 242},
    {230, 230, 230, 230, 230}
};

static uint16_t aprs_crc_ccitt(const uint8_t *frame, const uint16_t frame_len) {
    assert(frame);

    uint16_t i, j;
    uint16_t crc = 0xffff;

    for (i = 0; i < frame_len; i++) {
        for (j = 0; j < 8; j++) {
            uint8_t bit = (frame[i] >> j) & 0x01;
            if ((crc & 0x0001) != bit) {
                crc = (crc >> 1) ^ 0x8408;
            } else {
                crc = crc >> 1;
            }
        }
    }

    crc ^= 0xffff;
    return crc;
}

static uint8_t aprs_five_ones(const uint8_t test) {
    if ((test & 0x1F) == 0x1F) {
        return 1;
    } else {
        return 0;
    }
}

static uint8_t aprs_rev_byte(uint8_t cbyte) {
    cbyte = (cbyte & 0xF0) >> 4 | (cbyte & 0x0F) << 4;
    cbyte = (cbyte & 0xCC) >> 2 | (cbyte & 0x33) << 2;
    cbyte = (cbyte & 0xAA) >> 1 | (cbyte & 0x55) << 1;
    return cbyte;
}

void aprs_init(aprs_t *aprs, uint8_t *aprs_nrzi_frame) {
    assert(aprs);
    assert(aprs_nrzi_frame);

    aprs->source_set = 0;
    aprs->destination_set = 0;
    aprs->information_set = 0;
    aprs->nrzi_frame = aprs_nrzi_frame;

    aprs->init = 1;
    aprs->s = 0;
    aprs->i = 0;
    aprs->j = 0;
    aprs->k = 0;
    aprs->l = 0;
    aprs->current_byte = 0;
    aprs->current_bit = 0;
    aprs->mark_final = 0;
    aprs->filter_index = 0;
    aprs->track_error = 0.0;
}

void aprs_set_destination(aprs_t *aprs, const char *destination, const uint8_t ssid) {
    assert(aprs);
    assert(destination);
    assert(strlen(destination) <= 6);
    assert(ssid <= 0x0F);

    uint8_t i = 0;
    uint8_t j = 0;
    while (destination[i] != 0) {
        aprs->destination[j] = destination[i] << 1;
        i++;
        j++;
    }

    while (j < 6) {
        aprs->destination[j] = (' ' << 1);
        j++;
    }

    aprs->destination[6] = 0x60 | ((ssid & 0x0F) << 1);
    aprs->destination_set = 1;
}

void aprs_set_source(aprs_t *aprs, const char *source, const uint8_t ssid) {
    assert(aprs);
    assert(source);
    assert(strlen(source) <= 6);
    assert(ssid <= 0x0F);

    uint8_t i = 0;
    uint8_t j = 0;
    while (source[i] != 0) {
        aprs->source[j] = source[i] << 1;
        i++;
        j++;
    }

    while (j < 6) {
        aprs->source[j] = (' ' << 1);
        j++;
    }

    aprs->source[6] = 0x61 | ((ssid & 0x0F) << 1);
    aprs->source_set = 1;
}

void aprs_set_information(aprs_t *aprs, const uint8_t *information, const uint8_t information_length) {
    assert(aprs);
    assert(information);

    aprs->information = (uint8_t*)information;

    aprs->information_length = information_length;
    aprs->information_set = 1;
}

void aprs_build_ax25_frame(aprs_t *aprs, const uint8_t scramble) {
    assert(aprs);
    assert(aprs->source_set);
    assert(aprs->destination_set);
    assert(aprs->information_set);

    uint16_t k = 0;

//add 0x00 pre-start (optional, N1 can be 0)
#if (APRS_AX25_HEAD_0FLAG_OCTETS > 0)
    for (uint8_t i = 0; i < APRS_AX25_HEAD_0FLAG_OCTETS; i++) {
        aprs->layer1_frame[k] = 0;
        k++;
    }
#endif

    //add 0x7E sync start flags
    for (uint8_t i = 0; i < APRS_AX25_HEAD_OCTETS; i++) {
        aprs->layer1_frame[k] = 0x7E;
        k++;
    }

    /*
    now, dive into frame payload
  */

    //first add destination
    for (uint8_t i = 0; i < 7; i++) {
        aprs->layer1_frame[k] = aprs->destination[i];
        k++;
    }

    //add source
    for (uint8_t i = 0; i < 7; i++) {
        aprs->layer1_frame[k] = aprs->source[i];
        k++;
    }

    //NO ROUTING PATH (because is optional and not supported here)

    //add control and PID
    aprs->layer1_frame[k] = 0x03;
    k++;
    aprs->layer1_frame[k] = 0xF0;
    k++;

    //add information
    for (uint8_t i = 0; i < aprs->information_length; i++) {
        aprs->layer1_frame[k] = aprs->information[i];
        k++;
    }

    /*
    now, come back to layer 1
  */

    //add CRC
    uint16_t crc = aprs_crc_ccitt(aprs->layer1_frame + APRS_AX25_HEAD_0FLAG_OCTETS + APRS_AX25_HEAD_OCTETS, k - APRS_AX25_HEAD_0FLAG_OCTETS - APRS_AX25_HEAD_OCTETS);
    aprs->layer1_frame[k] = (crc & 0xFF);
    k++;
    aprs->layer1_frame[k] = ((crc >> 8) & 0xFF);
    k++;

    //add 0x7E sync end flags
    for (uint8_t i = 0; i < APRS_AX25_TAIL_OCTETS; i++) {
        aprs->layer1_frame[k] = 0x7E;
        k++;
    }

    aprs->layer1_frame_length = k;

    /*
    ----AX.25 frame finished-----
  */

    /*
    bit stuffing
  */

    k = 0;           //iterator for aprs->layer1_frame
    uint16_t l = 0;  //iterator for aprs->layer1_frame_stuffed

    uint8_t bbyte = 0;  //buffer byte
    uint8_t bbyte_len = 0;
    uint16_t aprs_layer1_frame_stuffed_length;

    //but not for N1 and N2 buffers and not for tail (checksum and N3)
    while (k < APRS_AX25_HEAD_0FLAG_OCTETS + APRS_AX25_HEAD_OCTETS) {
        uint8_t cbyte = aprs->layer1_frame[k];
        for (uint8_t j = 0; j < 8; j++) {
            bbyte <<= 1;
            bbyte |= (cbyte & 0x01);
            cbyte >>= 1;
            bbyte_len++;

            if (bbyte_len == 8) {
                aprs->nrzi_frame[l] = bbyte;
                l++;
                bbyte_len = 0;
            }
        }

        k++;
    }

    //this is the part where bit stuffing happens
    while (k < aprs->layer1_frame_length - APRS_AX25_TAIL_OCTETS) {
        uint8_t cbyte = aprs->layer1_frame[k];
        for (uint8_t j = 0; j < 8; j++) {
            bbyte <<= 1;
            bbyte |= (cbyte & 0x01);
            cbyte >>= 1;
            bbyte_len++;

            if (bbyte_len == 8) {
                aprs->nrzi_frame[l] = bbyte;
                l++;
                bbyte_len = 0;
            }

            if (aprs_five_ones(bbyte)) {
                bbyte <<= 1;
                bbyte_len++;
                if (bbyte_len == 8) {
                    aprs->nrzi_frame[l] = bbyte;
                    l++;
                    bbyte_len = 0;
                }
            }
        }

        k++;
    }

    while (k < aprs->layer1_frame_length) {
        uint8_t cbyte = aprs->layer1_frame[k];
        for (uint8_t j = 0; j < 8; j++) {
            bbyte <<= 1;
            bbyte |= (cbyte & 0x01);
            cbyte >>= 1;
            bbyte_len++;

            if (bbyte_len == 8) {
                aprs->nrzi_frame[l] = bbyte;
                l++;
                bbyte_len = 0;
            }
        }

        k++;
    }

    if (bbyte_len != 0) {
        while (bbyte_len != 8) {
            bbyte <<= 1;
            bbyte_len++;
        }
        aprs->nrzi_frame[l] = bbyte;
        l++;
    }

    aprs_layer1_frame_stuffed_length = l;

    /*
    scrambling and NRZI
  */
    uint32_t scramble_byte = 0;
    uint8_t out_bit = 0;
    for (uint16_t i = 0; i < aprs_layer1_frame_stuffed_length; i++) {
        uint8_t cbyte = aprs_rev_byte(aprs->nrzi_frame[i]);
        uint8_t dbyte = 0;
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t cbit = 0;
            if ((cbyte & 0x01) == 0) {
                out_bit = !out_bit;
            }
            cbyte >>= 1;
            l--;

            cbit = (out_bit & 0x01);
            if (scramble) {
                cbit = cbit ^ ((scramble_byte >> 11) & 0x01) ^ ((scramble_byte >> 16) & 0x01);
            }
            dbyte <<= 1;
            dbyte |= (cbit & 0x01);

            scramble_byte <<= 1;
            scramble_byte |= (cbit & 0x01);
        }

        aprs->nrzi_frame[i] = aprs_rev_byte(dbyte);
    }

    aprs->nrzi_frame_length = aprs_layer1_frame_stuffed_length;
}

void aprs_build_audio1200(aprs_t *aprs, uint8_t *aprs_audio, uint32_t *aprs_audio_length) {
    assert(aprs);
    assert(aprs_audio);
    assert(aprs_audio_length);

    uint8_t current_byte = 0;
    uint32_t l = 0;
    uint16_t s = 0;
    float track_error = 0.0;

    for (uint16_t i = 0; i < aprs->nrzi_frame_length; i++) {
        current_byte = aprs->nrzi_frame[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t current_bit = (current_byte & 0x01);
            current_byte >>= 1;

            for (uint8_t k = 0; k < APRS_APRS1200_SAMPLES_PER_BIT; k++) {
                aprs_audio[l] = aprs_full_sine[s];
                l++;
                if (current_bit == 1) {
                    s += APRS_APRS1200_MARK_ADVANCE;
                    track_error += APRS_APRS1200_MARK_ADVANCE_ERROR;
                } else {
                    s += APRS_APRS1200_SPACE_ADVANCE;
                    track_error += APRS_APRS1200_SPACE_ADVANCE_ERROR;
                }

                if (track_error >= 1.0) {
                    s--;
                    track_error -= 1.0;
                }

                if (s >= APRS_APRS1200_AFSK_SINE_TABLE_SIZE) {
                    s -= APRS_APRS1200_AFSK_SINE_TABLE_SIZE;
                }
            }
        }
    }

    //set idle DAC output to half the signal
    aprs_audio[l] = 128;
    l++;
    *aprs_audio_length = l;
}

void aprs_build_audio9600(aprs_t *aprs, const uint8_t filter, uint8_t *aprs_audio, uint32_t *aprs_audio_length) {
    assert(aprs);
    assert(aprs_audio);
    assert(aprs_audio_length);

    uint8_t current_byte = 0;
    uint8_t filter_index = 0;
    uint32_t k = 0;

    for (uint16_t i = 0; i < aprs->nrzi_frame_length; i++) {
        current_byte = aprs->nrzi_frame[i];
        for (uint8_t j = 0; j < 8; j++) {
            filter_index <<= 1;
            filter_index |= (current_byte & 0x01);
            filter_index &= 0x1F;

            for (uint8_t l = 0; l < 5; l++) {
                if (filter) {
                    aprs_audio[k] = aprs9600_output_filter[filter_index][l];
                } else {
                    aprs_audio[k] = (current_byte & 0x01) * 255;
                }

                k++;
            }

            current_byte >>= 1;
        }
    }

    //set idle DAC output to half the signal
    aprs_audio[k] = 128;
    k++;
    *aprs_audio_length = k;
}

uint8_t aprs_build_audio1200_step(aprs_t *aprs, uint8_t *current_sample) {
    assert(aprs);
    assert(current_sample);

    if (aprs->init) {
        aprs->s = 0;
        aprs->track_error = 0;
        aprs->current_byte = aprs->nrzi_frame[aprs->i];
        aprs->current_bit = (aprs->current_byte & 0x01);
        aprs->current_byte >>= 1;
        aprs->init = 0;
    }

    if (aprs->mark_final) {
        aprs->mark_final = 0;
        *current_sample = 128;
        aprs->init = 1;
        return 1;
    }

    *current_sample = aprs_full_sine[aprs->s];
    if (aprs->current_bit == 1) {
        aprs->s += APRS_APRS1200_MARK_ADVANCE;
        aprs->track_error += APRS_APRS1200_MARK_ADVANCE_ERROR;
    } else {
        aprs->s += APRS_APRS1200_SPACE_ADVANCE;
        aprs->track_error += APRS_APRS1200_SPACE_ADVANCE_ERROR;
    }

    if (aprs->track_error >= 1.0) {
        aprs->s--;
        aprs->track_error -= 1.0;
    }

    if (aprs->s >= APRS_APRS1200_AFSK_SINE_TABLE_SIZE) {
        aprs->s -= APRS_APRS1200_AFSK_SINE_TABLE_SIZE;
    }

    aprs->k++;
    if (aprs->k >= APRS_APRS1200_SAMPLES_PER_BIT) {
        aprs->k = 0;
        aprs->j++;
        if (aprs->j >= 8) {
            aprs->j = 0;
            aprs->i++;
            if (aprs->i >= aprs->nrzi_frame_length) {
                aprs->i = 0;
                aprs->mark_final = 1;
            }
            aprs->current_byte = aprs->nrzi_frame[aprs->i];
        }

        aprs->current_bit = (aprs->current_byte & 0x01);
        aprs->current_byte >>= 1;
    }

    return 0;
}

uint8_t aprs_build_audio9600_step(aprs_t *aprs, const uint8_t filter, uint8_t *current_sample) {
    assert(aprs);
    assert(current_sample);

    if (aprs->init) {
        aprs->current_byte = aprs->nrzi_frame[aprs->i];
        aprs->filter_index = 0;
        aprs->filter_index |= (aprs->current_byte & 0x01);
        aprs->filter_index &= 0x1F;
        aprs->current_byte >>= 1;
        aprs->init = 0;
    }

    if (aprs->mark_final) {
        aprs->mark_final = 0;
        *current_sample = 128;
        aprs->init = 1;
        return 1;
    }

    if (filter) {
        *current_sample = aprs9600_output_filter[aprs->filter_index][aprs->l];
    } else {
        *current_sample = (aprs->current_byte & 0x01) * 255;
    }

    aprs->l++;
    if (aprs->l >= 5) {
        aprs->l = 0;

        aprs->j++;
        if (aprs->j >= 8) {
            aprs->j = 0;
            aprs->i++;
            if (aprs->i >= aprs->nrzi_frame_length) {
                aprs->i = 0;
                aprs->mark_final = 1;
            }
            aprs->current_byte = aprs->nrzi_frame[aprs->i];
        }

        aprs->filter_index <<= 1;
        aprs->filter_index |= (aprs->current_byte & 0x01);
        aprs->filter_index &= 0x1F;
        aprs->current_byte >>= 1;
    }

    return 0;
}
