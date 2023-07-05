/*
This file is part of Echidna.

Echidna is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Echidna is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Echidna. If not, see <https://www.gnu.org/licenses/>.
*/

#include <aes.h>
#include <string.h>

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <echidna/types.h>
#include <echidna/config.h>
#include <echidna/commands.h>

static void
in(u8 *data, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        /* Serial input */
        while (!(UCSR0A & _BV(RXC0)));
        data[i] = UDR0;
    }
}

static void
out(u8 *data, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        /* Serial output */
        while (!(UCSR0A & _BV(UDRE0)));
        UDR0 = data[i];
    }
}

static void
entropy(u8 *data)
{
    #define ENTROPY_GATE 0xF
    #define ENTROPY_STEPS 50

    for (int j = 0; j < ENTROPY_STEPS; j++)
    {
        for (int i = 0; i < ECHIDNA_LENGTH_BLOCK; i++)
        {
            u8 val = 0;
            /* Do it again if byte has low entropy */
            do
            {
                /* Get entropy from analog input */
                ADCSRA = _BV(ADEN) | _BV(ADSC) | 0x7;
                while (ADCSRA & _BV(ADSC));
                val += ADCH;
                ADCSRA = 0;
            }
            while (!(val > ENTROPY_GATE && val < (0xFF - ENTROPY_GATE)));

            /* Apply entropy to data */
            if (data[i])
                data[i] *= val;
            else
                data[i] -= val;
        }
    }
}

static void
random(u8 *data)
{
    entropy(data);

    /* Encrypt entropy data in AES-CTR mode */
    static u64 counter = 0;

    u8 state[ECHIDNA_LENGTH_BLOCK] = {0};
    entropy(state);
    memcpy(data, &counter, sizeof(counter));
    aes_encrypt(state);

    for (int i = 0; i < ECHIDNA_LENGTH_BLOCK; i++)
        data[i] ^= state[i];

    counter++;
}

static char magic[] = "ECHIDNA";
static void
keygen(u8 *gkey, u8 *ekey)
{
    u8 key[ECHIDNA_LENGTH_KEY] = {0};

    /* Check if EEPROM has magic string */
    char mbuf[sizeof(magic)] = {0};
    eeprom_read_block(mbuf, NULL, sizeof(mbuf));
    if (strcmp(magic, mbuf) != 0)
    {
        aes_context(gkey);

        u8 ratio = (ECHIDNA_LENGTH_KEY / ECHIDNA_LENGTH_BLOCK);
        if (ratio == 0 || ECHIDNA_LENGTH_KEY % ECHIDNA_LENGTH_BLOCK != 0)
            ratio++;

        /* Generate dkey using gkey and noise */
        u8 kbuf[ECHIDNA_LENGTH_BLOCK * ratio];
        for (int i = 0; i < ratio; i++)
            random(&(kbuf[ECHIDNA_LENGTH_BLOCK * i]));
        memcpy(key, kbuf, ECHIDNA_LENGTH_KEY);

        /* Save it on the EEPROM */
        eeprom_write_block(magic, NULL, sizeof(magic));
        eeprom_write_block(key, (void *)sizeof(magic),
                           ECHIDNA_LENGTH_KEY);
    }
    else
    {
        /* Load dkey from EEPROM */
        eeprom_read_block(key, (void *)sizeof(magic),
                          ECHIDNA_LENGTH_KEY);
    }

    /* XOR dkey with ekey and gkey to create key */
    for (int i = 0; i < ECHIDNA_LENGTH_KEY; i++)
        key[i] ^= ekey[i] ^ gkey[i];

    /* Set key as current context */
    aes_context(key);
    memset(key, 0, sizeof(key));
}

static u8 buf[ECHIDNA_LENGTH_MAX] = {0};
static u8 gkey[ECHIDNA_LENGTH_KEY] = {GKEY};

ISR(INT0_vect, ISR_BLOCK)
{
    /* Clear EEPROM dkey */
    memset(buf, 0xff, sizeof(buf));
    eeprom_write_block(buf, NULL, sizeof(magic));
    eeprom_write_block(buf, (void *)sizeof(magic),
                       ECHIDNA_LENGTH_KEY);

    /* Clear ctx */
    aes_context(buf);

    /* Fast software reset */
    asm volatile ("jmp 0");
}

extern void __attribute__((noreturn))
main(void)
{
    /* Serial 115200 8n1 */
    const u16 ubrr = (F_CPU / 4 / 115200 - 1) / 2;
    UBRR0H = (u8)(ubrr >> 8);
    UBRR0L = (u8)(ubrr);
    UCSR0A = _BV(U2X0);
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
    UCSR0C = (0x3 << UCSZ00);

    /* PC0 as A0 input (noise antenna) */
    DDRC &= ~_BV(PC0);
    ADMUX = _BV(REFS0) | _BV(REFS1) | _BV(ADLAR);

    /* PD2 as INT0 falling edge with pull-up (jumper removal) */
    DDRD &= ~_BV(PD2);
    PORTD |= _BV(PD2);
    MCUCR &= ~_BV(PUD);
    EICRA = _BV(ISC01);
    EIMSK = _BV(INT0);
    sei();

    /* Generate key using empty ekey */
    memset(buf, 0, sizeof(buf));
    keygen(gkey, buf);

    /* Parse commands */
    u8 command = 0, zero = 0;
    while (true)
    {
        in(&command, sizeof(u8));
        switch (command)
        {
            case ECHIDNA_CMD_NOOP:
                break;
            case ECHIDNA_CMD_PING:
                in(buf, sizeof(u8));
                out(buf, sizeof(u8));
                break;
            case ECHIDNA_CMD_ENCRYPT:
                in(buf, ECHIDNA_LENGTH_BLOCK);
                aes_encrypt(buf);
                out(buf, ECHIDNA_LENGTH_BLOCK);
                break;
            case ECHIDNA_CMD_DECRYPT:
                in(buf, ECHIDNA_LENGTH_BLOCK);
                aes_decrypt(buf);
                out(buf, ECHIDNA_LENGTH_BLOCK);
                break;
            case ECHIDNA_CMD_RANDOM:
                random(buf);
                out(buf, ECHIDNA_LENGTH_BLOCK);
                break;
            case ECHIDNA_CMD_KEYSET:
                in(buf, ECHIDNA_LENGTH_KEY);
                keygen(gkey, buf);
                out(&zero, sizeof(u8));
                break;
        }
        memset(buf, 0, sizeof(buf));
    }
}
