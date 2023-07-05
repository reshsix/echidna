# Echidna
HSM using ATmega328p: Provides AES256 with a device generated key

## Requirements
- A POSIX environment
- AVR GCC, AVR Libc, AVRDUDE
- A board with an ATmega328p

## Installation
```sh
make all
make flash
sudo make install
```

## Workings
The AES256 KEY is (GKEY xor DKEY xor EKEY)
- GKEY (Generator key) is created during compilation,
used to generate random numbers by encrypting PC0/A0 noise in AES-CTR mode
- DKEY (Device key) is created in the first startup
by the RNG and stored on the device EEPROM, it can be zeroized using INT0
(falling edge with pull up)
- EKEY (External key) is provided by the computer through the library

It's recommended to burn the code using ICSP to prevent it from being
read by USB due to a bootloader. Fuses can help on difficulting key extration

The device provides ECB mode for sake of simplicity, it's recommended to
implement safer modes on top of it

The RNG will wait until getting the necessary entropy, which defaults to
50 samples between 0xF and 0xF0 (non-inclusive). It may halt if PC0/A0 is
grounded or floating without a noise antenna, and it's operation is
slow

## Example
```c
#include <stdio.h>
#include <stdlib.h>

#include <echidna/types.h>
#include <echidna/client.h>
#include <echidna/config.h>

static bool
error(const char *str)
{
    fprintf(stderr, "%s\n", str);
    return false;
}

extern int
main(void)
{
    bool ret = true;

    int dev = echidna_open("/dev/ttyUSB0", 0);
    if (dev < 0)
        ret = error("Couldn't open device");

    if (ret)
    {
        u8 ekey[ECHIDNA_LENGTH_KEY] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                       0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                                       0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
                                       0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                       0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
                                       0x1E, 0x1F};
        if (!echidna_keyset(dev, ekey))
            ret = error("Failed to set EKEY");
    }

    if (ret && !echidna_sane(dev))
        ret = error("Device is not working properly");

    u8 block[ECHIDNA_LENGTH_BLOCK] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                      0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD,
                                      0xEE, 0xFF};
    if (ret)
    {
        if (echidna_encrypt(dev, block))
        {
            printf("Encrypted block: ");
            for (int i = 0; i < sizeof(block); i++)
                printf("%02x ", block[i]);
            printf("\n");
        }
        else
            ret = error("Failed to encrypt block");
    }

    if (ret)
    {
        if (echidna_decrypt(dev, block))
        {
            printf("Decrypted block: ");
            for (int i = 0; i < sizeof(block); i++)
                printf("%02x ", block[i]);
            printf("\n");
        }
        else
            ret = error("Failed to decrypt block");
    }

    if (ret)
    {
        if (echidna_random(dev, block))
        {
            printf("Random numbers:  ");
            for (int i = 0; i < sizeof(block); i++)
                printf("%02x ", block[i]);
            printf("\n");
        }
        else
            ret = error("Failed to provide random numbers");
    }

    echidna_close(dev);

    return (ret) ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

```sh
gcc -static test.c -lechidna
```
