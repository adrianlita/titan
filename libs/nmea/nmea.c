#include "nmea.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#ifndef M_PI
#define M_PI    3.14159265359
#endif

TITAN_DEBUG_FILE_MARK;

#define PARSE_STATE_WAIT        0
#define PARSE_STATE_START       1
#define PARSE_STATE_CRC1        2
#define PARSE_STATE_CRC0        3

static float nmea_internal_coord_to_float(char *coord) {
    float ret = 0;
    uint8_t j = 0;
    while(coord[j + 2] != '.') {
        ret *= 10;
        ret += (coord[j] - '0');
        j++;
    }

    ret += (atof(coord + j) / 60.0);
    return ret;
}

static nmea_talker_t nmea_internal_id_to_talker(uint16_t id) {
    if(id <= 32) {
        return NMEA_TALKER_GPS;
    }
    else if((id >= 65) && (id <= 96)) {
        return NMEA_TALKER_GLONASS;
    }
    else if((id >= 193) && (id <= 200)) {
        return NMEA_TALKER_QZSS;
    }
    else if((id >= 201) && (id <= 235)) {
        return NMEA_TALKER_BEIDOU;
    }
    else if((id >= 301) && (id <= 336)) {
        return NMEA_TALKER_GALILEO;
    }
    else if((id >= 401) && (id <= 437)) {
        return NMEA_TALKER_BEIDOU;
    }
    else {
        return NMEA_TALKER_OTHER;
    }
}

static nmea_fix_quality_t nmea_internal_faa_mode(char c) {
    switch(c) {
        case 'A':
            return NMEA_FIX_QUALITY_AUTONOMOUS;
            break;

        case 'D':
            return NMEA_FIX_QUALITY_DIFF;
            break;

        case 'E':
            return NMEA_FIX_QUALITY_DEAD_RECK;
            break;

        case 'F':
            return NMEA_FIX_QUALITY_FLOAT_RTK;
            break;

        case 'M':
            return NMEA_FIX_QUALITY_MANUAL;
            break;

        case 'P':
            return NMEA_FIX_QUALITY_PPS;
            break;

        case 'R':
            return NMEA_FIX_QUALITY_RTK;
            break;

        case 'S':
            return NMEA_FIX_QUALITY_SIMULATION;
            break;

        case 'N':
        default:
            return NMEA_FIX_QUALITY_NONE;
            break;
    }
}

static void nmea_internal_process(nmea_device_t *dev);
static void nmea_internal_parse_rmc(nmea_device_t *dev);
static void nmea_internal_parse_gga(nmea_device_t *dev);
static void nmea_internal_parse_gsa(nmea_device_t *dev);
static void nmea_internal_parse_gsv(nmea_device_t *dev, nmea_talker_t talker);
static void nmea_internal_parse_gll(nmea_device_t *dev);

void nmea_init(nmea_device_t *dev, nmea_sentence_t final_sentence) {
    assert(dev);

    dev->final_sentence = final_sentence;
    dev->buffer_length = 0;
    dev->parse_code = NMEA_PARSE_CODE_OK;
    dev->parse_state = PARSE_STATE_WAIT;

    dev->navigation.valid = 0;

    dev->time.hour = -1;
    dev->time.min = 0;
    dev->time.sec = 0;

    dev->time.day = 0;
    dev->time.month = 0;
    dev->time.year = 0;

    dev->satellites_added = 0;
    dev->satellites_present = 0;
    for(uint32_t i = 0; i < NMEA_MAX_SATELLITES_TRACKED; i++) {
        dev->satellites[i].id = 0;
        dev->satellites[i].active = 0;
        dev->satellites[i].type = NMEA_TALKER_OTHER;
    }

    dev->init = 1;
}

void nmea_parse(nmea_device_t *dev, char *buffer, uint32_t length) {
    assert(dev);
    assert(buffer);
    assert(length);

    uint8_t checksum_calc = 0;
    uint8_t checksum_read = 0;

    dev->parse_code = NMEA_PARSE_CODE_INCOMPLETE;

    for(uint32_t i = 0; i < length; i++) {
        switch(dev->parse_state) {
            case PARSE_STATE_WAIT:
                if((buffer[i] == '$') || (buffer[i] == '!')) {
                    dev->buffer[0] = '$';
                    dev->buffer_length = 1;
                    dev->parse_state = PARSE_STATE_START;
                }
                break;

            case PARSE_STATE_START:
                dev->buffer[dev->buffer_length++] = buffer[i];
                if(buffer[i] == '*') {
                    dev->parse_state = PARSE_STATE_CRC1;
                }
                else if(dev->buffer_length > 80) {
                    dev->parse_state = PARSE_STATE_WAIT;    //error from NMEA specification
                }
                break;

            case PARSE_STATE_CRC1:
                dev->buffer[dev->buffer_length++] = buffer[i];
				dev->parse_state = PARSE_STATE_CRC0;
                break;

            case PARSE_STATE_CRC0:
                dev->buffer[dev->buffer_length++] = buffer[i];
                
                checksum_calc = dev->buffer[1];
                for(uint8_t i = 2; i < dev->buffer_length - 3; i++) {
                    checksum_calc ^= dev->buffer[i];
                }

                if(dev->buffer[dev->buffer_length - 2] <= '9') {
                    checksum_read = (dev->buffer[dev->buffer_length - 2] - '0') * 16;
                }
                else if(dev->buffer[dev->buffer_length - 2] <= 'Z') {
                    checksum_read = (dev->buffer[dev->buffer_length - 2] - 'A' + 10) * 16;
                }
                else {
                    checksum_read = (dev->buffer[dev->buffer_length - 2] - 'z' + 10) * 16;
                }

                if(dev->buffer[dev->buffer_length - 1] <= '9') {
                    checksum_read += (dev->buffer[dev->buffer_length - 1] - '0');
                }
                else if(dev->buffer[dev->buffer_length - 1] <= 'Z') {
                    checksum_read += (dev->buffer[dev->buffer_length - 1] - 'A' + 10);
                }
                else {
                    checksum_read += (dev->buffer[dev->buffer_length - 1] - 'z' + 10);
                }

                if(checksum_read == checksum_calc) {
                    dev->parse_code = NMEA_PARSE_CODE_OK;
                    dev->buffer_length -= 2;
                    dev->buffer[dev->buffer_length - 1] = ',';  //for very convenient processing
                    nmea_internal_process(dev);
                }
                else {
                    dev->parse_code = NMEA_PARSE_CODE_CRC_ERROR;
                }

                dev->parse_state = PARSE_STATE_WAIT;
                break;

            default:
                assert(0);
                break;
        }
    }
}

float knots_to_kmph(float knots) {
    return knots*1.852;
}

float real_distance_meters(float lat1, float lon1, float lat2, float lon2) {
    const float earth_radius = 6371000;

    float lat_r = (lat1 - lat2) * M_PI / 180.0;
    float lon_r = (lon1 - lon2) * M_PI / 180.0;

    lat1 = (lat1) * M_PI / 180.0;
    lat2 = (lat2) * M_PI / 180.0;

    float a = sin(lat_r / 2) * sin(lat_r / 2) + sin(lon_r / 2) * sin(lon_r / 2) * cos(lat1) * cos(lat2); 
    float c = 2 * atan2(sqrt(a), sqrt(1 - a)); 
    return earth_radius * c;
}

static void nmea_internal_process(nmea_device_t *dev) {
    char *buffer = dev->buffer;
    nmea_talker_t talker;
    nmea_sentence_t sentence;

    if((dev->satellites_present != 0) && (dev->satellites_added == 0)) {
        dev->satellites_present = 0;
        for(uint32_t i = 0; i < NMEA_MAX_SATELLITES_TRACKED; i++) {
            dev->satellites[i].id = 0;
            dev->satellites[i].active = 0;
            dev->satellites[i].type = NMEA_TALKER_OTHER;
        }
    }

    switch(buffer[1]) {
        case 'G':
            switch(buffer[2]) {
                case 'A':
                    talker = NMEA_TALKER_GALILEO;
                    break;

                case 'B':
                    talker = NMEA_TALKER_BEIDOU;
                    break;

                case 'L':
                    talker = NMEA_TALKER_GLONASS;
                    break;

                case 'N':
                    talker = NMEA_TALKER_MULTIPLE;
                    break;

                case 'P':
                    talker = NMEA_TALKER_GPS;
                    break;

                default:
                    talker = NMEA_TALKER_OTHER;
                    break;
            }
            break;

        case 'B':
            if(buffer[2] == 'D') {
                talker = NMEA_TALKER_BEIDOU;
            }
            else {
                talker = NMEA_TALKER_OTHER;
            }
            break;

        case 'Q':
            if(buffer[2] == 'Z') {
                talker = NMEA_TALKER_QZSS;
            }
            else {
                talker = NMEA_TALKER_OTHER;
            }
            break;
    }

    switch(buffer[3]) {
        case 'R':
            if((buffer[4] == 'M') && (buffer[5] == 'C')) {
                sentence = NMEA_SENTENCE_RMC;
            }
            else {
                sentence = NMEA_SENTENCE_OTHER;
            }
            break;

        case 'G':
            switch(buffer[4]) {
                case 'G':
                    if(buffer[5] == 'A') {
                        sentence = NMEA_SENTENCE_GGA;
                    }
                    else {
                        sentence = NMEA_SENTENCE_OTHER;
                    }
                    break;

                case 'S':
                    if(buffer[5] == 'A') {
                        sentence = NMEA_SENTENCE_GSA;
                    }
                    else if(buffer[5] == 'V') {
                        sentence = NMEA_SENTENCE_GSV;
                    }
                    else {
                        sentence = NMEA_SENTENCE_OTHER;
                    }
                    break;

                case 'L':
                    if(buffer[5] == 'L') {
                        sentence = NMEA_SENTENCE_GLL;
                    }
                    else {
                        sentence = NMEA_SENTENCE_OTHER;
                    }
                    break;

                default:
                    sentence = NMEA_SENTENCE_OTHER;
                    break;
            }
            break;

        default:
            sentence = NMEA_SENTENCE_OTHER;
            break;
    }

    switch(sentence) {
        case NMEA_SENTENCE_RMC:
            nmea_internal_parse_rmc(dev);
            break;

        case NMEA_SENTENCE_GGA:
            nmea_internal_parse_gga(dev);
            break;

        case NMEA_SENTENCE_GSA:
            nmea_internal_parse_gsa(dev);
            break;

        case NMEA_SENTENCE_GSV:
            nmea_internal_parse_gsv(dev, talker);
            break;

        case NMEA_SENTENCE_GLL:
            nmea_internal_parse_gll(dev);
            break;
    }

    if(sentence == dev->final_sentence) {
        //checkAL aici putem baga callback sau ceva
        dev->parse_code = NMEA_PARSE_CODE_FINISHED;
        dev->satellites_present = dev->satellites_added;
        dev->satellites_added = 0;
    }
}

static void nmea_internal_parse_rmc(nmea_device_t *dev) {
    char *buffer = dev->buffer;
    uint8_t length = dev->buffer_length;
    uint8_t param = 0;

    char s[15];
    uint8_t si = 0;

    uint8_t i = 7;
    while(i < length) {
        if(buffer[i] == ',') {
            switch(param) {
                case 0: //hhmmss.ss
                    if((si == 6) || (si == 9)) {
                        dev->time.hour = ((s[0] - '0') * 10) + (s[1] - '0');
                        dev->time.min = ((s[2] - '0') * 10) + (s[3] - '0');
                        dev->time.sec = ((s[4] - '0') * 10) + (s[5] - '0');
                    }
                    else {
                        dev->time.hour = -1;
                        dev->time.min = 0;
                        dev->time.sec = 0;
                    }
                    break;

                case 1: //A / V
                    dev->navigation.valid = 0;
                    if(si && (s[0] == 'A')) {
                        dev->navigation.valid = 1;
                    }
                    break;

                case 2: //latitude
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.latitude = nmea_internal_coord_to_float(s);
                    }
                    else {
                        dev->navigation.latitude = 0.0;
                    }
                    break;

                case 3: //latitude direction
                    if(si & s[0] == 'S') {
                        dev->navigation.latitude = -dev->navigation.latitude;
                    }
                    break;

                case 4: //longitude
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.longitude = nmea_internal_coord_to_float(s);
                    }
                    else {
                        dev->navigation.longitude = 0.0;
                    }
                    break;

                case 5: //longitude direction
                    if(si & s[0] == 'W') {
                        dev->navigation.longitude = -dev->navigation.longitude;
                    }
                    break;

                case 6: //speed in knots
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.speed = atof(s);
                    }
                    else {
                        dev->navigation.speed = -1.0;
                    }
                    break;

                case 8: //ddmmyy
                    if(si == 6) {
                        dev->time.day = ((s[0] - '0') * 10) + (s[1] - '0');
                        dev->time.month = ((s[2] - '0') * 10) + (s[3] - '0');
                        dev->time.year = ((s[4] - '0') * 10) + (s[5] - '0') + 2000;
                    }
                    else {
                        dev->time.day = 0;
                        dev->time.month = 0;
                        dev->time.year = 0;
                    }
                    break;
            }
            param++;
            si = 0;
        }
        else {
            s[si++] = buffer[i];
        }
        i++;
    }
}

static void nmea_internal_parse_gga(nmea_device_t *dev) {
    char *buffer = dev->buffer;
    uint8_t length = dev->buffer_length;
    uint8_t param = 0;

    char s[15];
    uint8_t si = 0;

    uint8_t i = 7;
    while(i < length) {
        if(buffer[i] == ',') {
            switch(param) {
                case 0: //hhmmss.ss
                    if((si == 6) || (si == 9)) {
                        dev->time.hour = ((s[0] - '0') * 10) + (s[1] - '0');
                        dev->time.min = ((s[2] - '0') * 10) + (s[3] - '0');
                        dev->time.sec = ((s[4] - '0') * 10) + (s[5] - '0');
                    }
                    else {
                        dev->time.hour = -1;
                        dev->time.min = 0;
                        dev->time.sec = 0;
                    }
                    break;

                case 1: //latitude
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.latitude = nmea_internal_coord_to_float(s);
                    }
                    else {
                        dev->navigation.latitude = 0.0;
                    }
                    break;

                case 2: //latitude direction
                    if(si & s[0] == 'S') {
                        dev->navigation.latitude = -dev->navigation.latitude;
                    }
                    break;

                case 3: //longitude
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.longitude = nmea_internal_coord_to_float(s);
                    }
                    else {
                        dev->navigation.longitude = 0.0;
                    }
                    break;

                case 4: //longitude direction
                    if(si & s[0] == 'W') {
                        dev->navigation.longitude = -dev->navigation.longitude;
                    }
                    break;

                case 5: //fix quality
                    if(si) {
                        dev->fix_info.quality = (nmea_fix_quality_t)(s[0] - '0');
                    } else {
                        dev->fix_info.quality = NMEA_FIX_QUALITY_NONE;
                    }
                    break;

                case 6: //sats in use
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->fix_info.satellites_in_use = atoi(s);
                    }
                    else {
                        dev->fix_info.satellites_in_use = 0;
                    }
                    break;

                case 7: //hdop
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->fix_info.hdop = atof(s);
                    }
                    else {
                        dev->fix_info.hdop = 99.99;
                    }
                    break;

                case 8: //altitude
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.altitude = atof(s);
                    }
                    else {
                        dev->navigation.altitude = 0.0;
                    }
                    break;

                case 10: //geoidal separation
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.altitude_geoidal_sepparation = atof(s);
                    }
                    else {
                        dev->navigation.altitude_geoidal_sepparation = 0.0;
                    }
                    break;

                case 12: //dgps age
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->fix_info.dgps_age = atoi(s);
                    }
                    else {
                        dev->fix_info.dgps_age = -1;
                    }
                    break;

                case 13: //dgps station id
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->fix_info.dgps_station_id = atoi(s);
                    }
                    else {
                        dev->fix_info.dgps_station_id = -1;
                    }
                    break;
            }
            param++;
            si = 0;
        }
        else {
            s[si++] = buffer[i];
        }
        i++;
    }
}

static void nmea_internal_parse_gsa(nmea_device_t *dev) {
    char *buffer = dev->buffer;
    uint8_t length = dev->buffer_length;
    uint8_t param = 0;

    char s[15];
    uint8_t si = 0;

    uint8_t i = 7;
    while(i < length) {
        if(buffer[i] == ',') {
            switch(param) {
                //don't actually care for selection mode

                case 1: //fix mode
                    if(si != 0) {
                        dev->navigation.fix_mode = (nmea_fix_mode_t)(s[0] - '0');
                    }
                    else {
                        dev->navigation.fix_mode = NMEA_FIX_MODE_NONE;
                    }
                    break;

                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        uint16_t sat_no = atoi(s);
                        if(dev->satellites_added < NMEA_MAX_SATELLITES_TRACKED) {
                            dev->satellites[dev->satellites_added].id = sat_no;
                            dev->satellites[dev->satellites_added].active = 1;
                            dev->satellites[dev->satellites_added].type = nmea_internal_id_to_talker(sat_no);
                            dev->satellites_added++;
                        }
                    }
                    break;

                case 14: //pdop
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->fix_info.pdop = atof(s);
                    }
                    else {
                        dev->fix_info.pdop = 99.99;
                    }
                    break;

                case 15: //hdop
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->fix_info.hdop = atof(s);
                    }
                    else {
                        dev->fix_info.hdop = 99.99;
                    }
                    break;

                case 16: //vdop
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->fix_info.vdop = atof(s);
                    }
                    else {
                        dev->fix_info.vdop = 99.99;
                    }
                    break;

            }
            param++;
            si = 0;
        }
        else {
            s[si++] = buffer[i];
        }
        i++;
    }
}

static void nmea_internal_parse_gsv(nmea_device_t *dev, nmea_talker_t talker) {
    char *buffer = dev->buffer;
    uint8_t length = dev->buffer_length;
    uint8_t param = 0;

    char s[15];
    uint8_t si = 0;

    int16_t sat_list_no = -1;

    uint8_t i = 7;
    while(i < length) {
        if(buffer[i] == ',') {
            switch(param) {
                //don't actually care for total number of sentences [0]
                //don't care also for sentence number [1]
                //don't care for sats in view [2]
                
                case 3: //sat id
                case 7:
                case 11:
                case 15:
                    sat_list_no = -1;
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        uint16_t sat_no = atoi(s);

                        uint32_t i = 0;
                        while(i < dev->satellites_added) {
                            if(dev->satellites[i].id == sat_no) {
                                sat_list_no = i;
                                break;
                            }
                            i++;
                        }

                        //not found, check if we can add it to the list
                        if(i == dev->satellites_added) {
                            if(dev->satellites_added < NMEA_MAX_SATELLITES_TRACKED) {
                                dev->satellites[dev->satellites_added].id = sat_no;
                                dev->satellites[dev->satellites_added].type = nmea_internal_id_to_talker(sat_no);
                                sat_list_no = dev->satellites_added;
                                dev->satellites_added++;
                            }
                        }
                    }
                    break;

                case 4: //elevation
                case 8:
                case 12:
                case 16:
                    if(sat_list_no != -1) {
                        if(si != 0) {
                            s[si] = 0;  //null-terminate string
                            dev->satellites[sat_list_no].elevation = atoi(s);
                        }
                        else {
                            dev->satellites[sat_list_no].elevation = 0;
                        }
                    }
                    break;

                case 5: //azimuth
                case 9:
                case 13:
                case 17:
                    if(sat_list_no != -1) {
                        if(si != 0) {
                            s[si] = 0;  //null-terminate string
                            dev->satellites[sat_list_no].azimuth = atoi(s);
                        }
                        else {
                            dev->satellites[sat_list_no].azimuth = 0;
                        }
                    }
                    break;

                case 6: //snr
                case 10:
                case 14:
                case 18:
                    if(sat_list_no != -1) {
                        if(si != 0) {
                            s[si] = 0;  //null-terminate string
                            dev->satellites[sat_list_no].snr = atoi(s);
                        }
                        else {
                            dev->satellites[sat_list_no].snr = 0;
                        }
                    }
                    break;
            }
            param++;
            si = 0;
        }
        else {
            s[si++] = buffer[i];
        }
        i++;
    }
}

static void nmea_internal_parse_gll(nmea_device_t *dev) {
    char *buffer = dev->buffer;
    uint8_t length = dev->buffer_length;
    uint8_t param = 0;

    char s[15];
    uint8_t si = 0;

    uint8_t i = 7;
    while(i < length) {
        if(buffer[i] == ',') {
            switch(param) {
                case 0: //latitude
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.latitude = nmea_internal_coord_to_float(s);
                    }
                    else {
                        dev->navigation.latitude = 0.0;
                    }
                    break;

                case 1: //latitude direction
                    if(si & s[0] == 'S') {
                        dev->navigation.latitude = -dev->navigation.latitude;
                    }
                    break;

                case 2: //longitude
                    if(si != 0) {
                        s[si] = 0;  //null-terminate string
                        dev->navigation.longitude = nmea_internal_coord_to_float(s);
                    }
                    else {
                        dev->navigation.longitude = 0.0;
                    }
                    break;

                case 3: //longitude direction
                    if(si & s[0] == 'W') {
                        dev->navigation.longitude = -dev->navigation.longitude;
                    }
                    break;

                case 4: //hhmmss.ss
                    if((si == 6) || (si == 9)) {
                        dev->time.hour = ((s[0] - '0') * 10) + (s[1] - '0');
                        dev->time.min = ((s[2] - '0') * 10) + (s[3] - '0');
                        dev->time.sec = ((s[4] - '0') * 10) + (s[5] - '0');
                    }
                    else {
                        dev->time.hour = -1;
                        dev->time.min = 0;
                        dev->time.sec = 0;
                    }
                    break;

                case 5: //status
                    dev->navigation.valid = 0;
                    if(si) {
                        if(s[0] == 'A')
                        dev->navigation.valid = 1;
                    }
                    break;

                case 6: //FAA mode
                    if(si != 0) {
                        dev->fix_info.quality = nmea_internal_faa_mode(s[0]);
                    }
                    else {
                        dev->fix_info.quality = NMEA_FIX_QUALITY_NONE;
                    }
                    break;
            }
            param++;
            si = 0;
        }
        else {
            s[si++] = buffer[i];
        }
        i++;
    }
}



