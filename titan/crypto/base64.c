#include "base64.h"
#include <assert.h>

TITAN_DEBUG_FILE_MARK;

static const uint8_t b64_table[256] = {
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

uint32_t base64_decode_get_len(const char *bufcoded) {
    int32_t nbytesdecoded;
    const uint8_t *bufin;
    int32_t nprbytes;

    if(!bufcoded) {
        return 0;
    }

    bufin = (const uint8_t *) bufcoded;
    while (b64_table[*(bufin++)] <= 63);
    nprbytes = (bufin - (const uint8_t *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;
    return nbytesdecoded + 1;
}

uint32_t base64_decode(char *bufplain, const char *bufcoded) {
    int32_t nbytesdecoded;
    const uint8_t *bufin;
    uint8_t *bufout;
    int32_t nprbytes;

    assert(bufcoded);
    assert(bufplain);

    bufin = (const uint8_t *) bufcoded;
    while (b64_table[*(bufin++)] <= 63);
    nprbytes = (bufin - (const uint8_t *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (uint8_t *) bufplain;
    bufin = (const uint8_t *) bufcoded;

    while (nprbytes > 4) {
        *(bufout++) = (uint8_t)(b64_table[*bufin] << 2 | b64_table[bufin[1]] >> 4);
        *(bufout++) = (uint8_t)(b64_table[bufin[1]] << 4 | b64_table[bufin[2]] >> 2);
        *(bufout++) = (uint8_t)(b64_table[bufin[2]] << 6 | b64_table[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if (nprbytes > 1) {
        *(bufout++) = (uint8_t)(b64_table[*bufin] << 2 | b64_table[bufin[1]] >> 4);
    }
    if (nprbytes > 2) {
        *(bufout++) = (uint8_t)(b64_table[bufin[1]] << 4 | b64_table[bufin[2]] >> 2);
    }
    if (nprbytes > 3) {
        *(bufout++) = (uint8_t)(b64_table[bufin[2]] << 6 | b64_table[bufin[3]]);
    }

    *(bufout++) = '\0';
    nbytesdecoded -= (4 - nprbytes) & 3;

    return nbytesdecoded;
}

static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint32_t base64_encode_get_len(uint32_t plain_buf_len) {
    return ((plain_buf_len + 2) / 3 * 4) + 1;
}

uint32_t base64_encode(char *encoded, const char *bufplain, int32_t buflen) {
  int32_t i;
  char *p;

  assert(encoded);
  assert(bufplain);
  assert(buflen);

  p = encoded;
  for (i = 0; i < buflen - 2; i += 3) {
    *p++ = base64[(bufplain[i] >> 2) & 0x3F];
    *p++ = base64[((bufplain[i] & 0x3) << 4) | ((int32_t)(bufplain[i + 1] & 0xF0) >> 4)];
    *p++ = base64[((bufplain[i + 1] & 0xF) << 2) | ((int32_t)(bufplain[i + 2] & 0xC0) >> 6)];
    *p++ = base64[bufplain[i + 2] & 0x3F];
  }
  if (i < buflen) {
    *p++ = base64[(bufplain[i] >> 2) & 0x3F];
    if (i == (buflen - 1)) {
        *p++ = base64[((bufplain[i] & 0x3) << 4)];
        *p++ = '=';
    }
    else {
        *p++ = base64[((bufplain[i] & 0x3) << 4) | ((int32_t)(bufplain[i + 1] & 0xF0) >> 4)];
        *p++ = base64[((bufplain[i + 1] & 0xF) << 2)];
    }
    *p++ = '=';
  }

  *p++ = '\0';

  return p - encoded;
}
