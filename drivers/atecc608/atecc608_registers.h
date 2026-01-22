#pragma once

#define ATECC608_WA_RESET                   0x00
#define ATECC608_WA_SLEEP                   0x01
#define ATECC608_WA_IDLE                    0x02
#define ATECC608_WA_COMMAND                 0x03

#define ATECC608_REG_CHECKMAC               0x28         // CheckMac command op-code
#define ATECC608_REG_DERIVE_KEY             0x1C         // DeriveKey command op-code
#define ATECC608_REG_INFO                   0x30         // Info command op-code
#define ATECC608_REG_GENDIG                 0x15         // GenDig command op-code
#define ATECC608_REG_GENKEY                 0x40         // GenKey command op-code
#define ATECC608_REG_HMAC                   0x11         // HMAC command op-code
#define ATECC608_REG_LOCK                   0x17         // Lock command op-code
#define ATECC608_REG_MAC                    0x08         // MAC command op-code
#define ATECC608_REG_NONCE                  0x16         // Nonce command op-code
#define ATECC608_REG_PAUSE                  0x01         // Pause command op-code
#define ATECC608_REG_PRIVWRITE              0x46         // PrivWrite command op-code
#define ATECC608_REG_RANDOM                 0x1B         // Random command op-code
#define ATECC608_REG_READ                   0x02         // Read command op-code
#define ATECC608_REG_SIGN                   0x41         // Sign command op-code
#define ATECC608_REG_UPDATE_EXTRA           0x20         // UpdateExtra command op-code
#define ATECC608_REG_VERIFY                 0x45         // GenKey command op-code
#define ATECC608_REG_WRITE                  0x12         // Write command op-code
#define ATECC608_REG_ECDH                   0x43         // ECDH command op-code
#define ATECC608_REG_COUNTER                0x24         // Counter command op-code
#define ATECC608_REG_SHA                    0x47         // SHA command op-code
#define ATECC608_REG_AES                    0x51         // AES command op-code
#define ATECC608_REG_KDF                    0x56         // KDF command op-code
#define ATECC608_REG_SECUREBOOT             0x80         // Secure Boot command op-code
#define ATECC608_REG_SELFTEST               0x77         // Self test command op-code

#define ATECC608A_INFO_REVISION_ID          0x60         // This should always be replied by ATECC608

#define ATECC608_INFO_REVISION              0x00
#define ATECC608_INFO_KEYVALID              0x01
#define ATECC608_INFO_STATE                 0x02
#define ATECC608_INFO_GPIO                  0x03

#define ATECC608_ZONE_CONFIG                0x00
#define ATECC608_ZONE_DATA                  0x01
#define ATECC608_ZONE_SLOT                  0x02

#define ATECC608_CONFIG_BLOCK0              0x00
#define ATECC608_CONFIG_BLOCK1              0x08
#define ATECC608_CONFIG_BLOCK2              0x10
#define ATECC608_CONFIG_BLOCK3              0x18

#define ATECC608_RESPONSE_SUCCESS           0x00
#define ATECC608_RESPONSE_MISCOMPARE        0x01
#define ATECC608_RESPONSE_PARSE_ERROR       0x03
#define ATECC608_RESPONSE_ECC_FAULT         0x05
#define ATECC608_RESPONSE_EXECUTION_ERROR   0x0F
#define ATECC608_RESPONSE_WOKEUP            0x11
#define ATECC608_RESPONSE_WATCHDOG          0xEE
#define ATECC608_RESPONSE_CRC               0xFF
