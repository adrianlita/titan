#pragma once

#include <stdint.h>

/*
    customizable vars via titan_config.h

    #define NMEA_MAX_SATELLITES_TRACKED                 48
*/

#ifndef NMEA_MAX_SATELLITES_TRACKED
#define NMEA_MAX_SATELLITES_TRACKED                 48
#endif


#if(NMEA_MAX_SATELLITES_TRACKED < 12)
#error "NMEA_MAX_SATELLITES_TRACKED must not be at least 12"
#endif

//implemented according to https://gpsd.gitlab.io/gpsd/NMEA.html
#define NMEA_PARSE_CODE_OK          0
#define NMEA_PARSE_CODE_FINISHED    1
#define NMEA_PARSE_CODE_INCOMPLETE  2
#define NMEA_PARSE_CODE_CRC_ERROR   3

typedef enum {
    NMEA_TALKER_OTHER = 0,

    NMEA_TALKER_GPS,
    NMEA_TALKER_GALILEO,
    NMEA_TALKER_GLONASS,
    NMEA_TALKER_BEIDOU,
    NMEA_TALKER_MULTIPLE,
    NMEA_TALKER_QZSS,
} nmea_talker_t;

typedef enum {
    NMEA_SENTENCE_RMC       = 0x01,
    NMEA_SENTENCE_GGA       = 0x02,
    NMEA_SENTENCE_GSA       = 0x04,
    NMEA_SENTENCE_GSV       = 0x08,
    NMEA_SENTENCE_GLL       = 0x10,
    
    NMEA_SENTENCE_OTHER     = 0x80,
} nmea_sentence_t;

typedef enum {
    NMEA_FIX_MODE_NONE      = 1,
    NMEA_FIX_MODE_2D        = 2,
    NMEA_FIX_MODE_3D        = 3,
} nmea_fix_mode_t;

typedef enum {
    NMEA_FIX_QUALITY_NONE       = 0,
    NMEA_FIX_QUALITY_AUTONOMOUS = 1,    //aka GPS
    NMEA_FIX_QUALITY_DIFF       = 2,    //aka DGPS
    NMEA_FIX_QUALITY_PPS        = 3,
    NMEA_FIX_QUALITY_RTK        = 4,
    NMEA_FIX_QUALITY_FLOAT_RTK  = 5,
    NMEA_FIX_QUALITY_DEAD_RECK  = 6,    //aka estimated
    NMEA_FIX_QUALITY_MANUAL     = 7,
    NMEA_FIX_QUALITY_SIMULATION = 8,
} nmea_fix_quality_t;

typedef struct __nmea_device {
    uint8_t init;
    nmea_sentence_t final_sentence;

    struct {
        int8_t hour;                        //-1 if data is not valid
        uint8_t min;
        uint8_t sec;

        uint8_t day;                        //0 if data is invalid
        uint8_t month;
        uint16_t year;
    } time;

    struct {
        uint8_t valid;
        nmea_fix_mode_t fix_mode;

        float latitude;
        float longitude;

        float altitude;                     //in meters
        float altitude_geoidal_sepparation; //in meters

        float speed;                        //in knots
    } navigation;

    struct {
        float pdop;
        float hdop;                         //in meters
        float vdop;

        uint8_t satellites_in_use;

        nmea_fix_quality_t quality;
        int32_t dgps_age;                   //in seconds
        int32_t dgps_station_id;        
    } fix_info;

    struct {
        nmea_talker_t type;                 //which constelation
        uint16_t id;
        uint8_t active;                     //whether used in computation or not

        int8_t elevation;                   //in degrees
        uint16_t azimuth;                   //in degrees
        uint8_t snr;                        //in dB
    } satellites[NMEA_MAX_SATELLITES_TRACKED];

    uint32_t satellites_present;

    char buffer[82];
    uint8_t buffer_length;
    uint8_t satellites_added;
    uint8_t parse_state;
    uint32_t parse_code;
} nmea_device_t;


void nmea_init(nmea_device_t *dev, nmea_sentence_t final_sentence);
void nmea_parse(nmea_device_t *dev, char *buffer, uint32_t length);

float knots_to_kmph(float knots);
float real_distance_meters(float lat1, float lon1, float lat2, float lon2);


/*

    //example so far:

    nmea_parse(&nmea, &c, 1);   //add chars to the parser

    if(nmea.parse_code == NMEA_PARSE_CODE_FINISHED) {
        if(nmea.satellites_present) {
            printf("Viewing %d sats | Fix mode: %d | Fix qual: %d ...\n", nmea.satellites_present, nmea.navigation.fix_mode, nmea.fix_info.quality);
            for(uint8_t i = 0; i < nmea.satellites_present; i++) {
                if(nmea.satellites[i].active) {
                    printf("SAT[%d] ID:%d type:%d (%3d, %3d):%3d \n", i, nmea.satellites[i].id, nmea.satellites[i].type, nmea.satellites[i].elevation, nmea.satellites[i].azimuth, nmea.satellites[i].snr);
                }
                else {
                    printf("sat[%d] ID:%d type:%d (%3d, %3d):%3d \n", i, nmea.satellites[i].id, nmea.satellites[i].type, nmea.satellites[i].elevation, nmea.satellites[i].azimuth, nmea.satellites[i].snr);
                }
            }

            if(nmea.time.hour != -1) {
                printf("Got time %2d:%2d:%2d\n", nmea.time.hour, nmea.time.min, nmea.time.sec);
            }

            if(nmea.time.day != 0) {
                printf("Got date %2d:%2d:%4d\n", nmea.time.day, nmea.time.month, nmea.time.year);
            }

            if(nmea.navigation.valid) {
                printf("Fix: %d | Lat: %3.5f | Lon: %3.5f | Alt: %5.2f | Geoid: %5.2f\n", nmea.navigation.fix_mode, nmea.navigation.latitude, nmea.navigation.longitude, nmea.navigation.altitude, nmea.navigation.altitude_geoidal_sepparation);
            }

            printf("\n");
        }
        else {
            printf("No sats in view\n");
        }
    }

*/