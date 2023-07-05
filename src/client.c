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

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <termios.h>

#include <sys/poll.h>
#include <sys/ioctl.h>

#include <echidna/types.h>
#include <echidna/client.h>
#include <echidna/config.h>
#include <echidna/commands.h>

extern int
echidna_open(const char *path, int timeout)
{
    int ret = openat(AT_FDCWD, path, O_RDWR | O_NOCTTY);

    struct termios config = {0};
    if (ret >= 0 && (!isatty(ret) || tcgetattr(ret, &config) < 0))
    {
        close(ret);
        ret = -1;
    }

    /* Configure tty flags, speed, lock it and ping the device */
    cfmakeraw(&config);
    if (ret >= 0 &&
        (cfsetispeed(&config, B115200) < 0 ||
         cfsetospeed(&config, B115200) < 0 ||
         tcsetattr(ret, TCSANOW, &config) < 0 ||
         ioctl(ret, TIOCEXCL, NULL) < 0 ||
         !echidna_ping(ret, 0x7F, timeout)))
    {
        close(ret);
        ret = -1;
    }

    return ret;
}

extern void
echidna_close(int device)
{
    if (device >= 0)
        close(device);
}

extern bool
echidna_sane(int device)
{
    bool ret = false;

    if (device >= 0)
    {
        u8 zero[ECHIDNA_LENGTH_BLOCK] = {0};
        echidna_encrypt(device, zero);
        echidna_decrypt(device, zero);

        ret = true;
        for (int i = 0; i < ECHIDNA_LENGTH_BLOCK; i++)
        {
            if (zero[i] != 0)
            {
                ret = false;
                break;
            }
        }
    }

    return ret;
}

extern bool
echidna_ping(int device, u8 data, int timeout)
{
    bool ret = false;

    if (device >= 0)
    {
        const int attempt_tm = 100;
        timeout = (timeout > 0) ? timeout : INT_MAX;
        timeout = (timeout >= attempt_tm) ? timeout : attempt_tm;

        u8 input = ~data;

        int count = 0, attempts = timeout / attempt_tm;
        for (int i = 0; i < attempts; i++)
        {
            /* Send a ping to the device */
            u8 c = ECHIDNA_CMD_PING;
            write(device, &c, sizeof(c));
            write(device, &data, sizeof(u8));

            /* Try to read the response with a timeout */
            struct pollfd pfd = {.fd = device, .events = POLLIN};
            poll(&pfd, 1, attempt_tm);
            if (pfd.revents & POLLIN)
                read(device, &input, sizeof(u8));
            else
                count++;

            /* Break if succeeded */
            if (input == data)
            {
                ret = true;
                break;
            }
        }

        /* Read pings that may have arrived later */
        int flags = fcntl(device, F_GETFL, 0);
        fcntl(device, F_SETFL, flags | O_NONBLOCK);
        for (int i = 0; i < count; i++)
            read(device, &input, sizeof(u8));
        fcntl(device, F_SETFL, flags);
    }

    return ret;
}

extern bool
echidna_encrypt(int device, u8 *data)
{
    bool ret = false;

    if (device >= 0)
    {
        u8 c = ECHIDNA_CMD_ENCRYPT;
        ret = write(device, &c, sizeof(c)) > 0 &&
              write(device, data, ECHIDNA_LENGTH_BLOCK) > 0 &&
              read(device, data, ECHIDNA_LENGTH_BLOCK) > 0;
    }

    return ret;
}

extern bool
echidna_decrypt(int device, u8 *data)
{
    bool ret = false;

    if (device >= 0)
    {
        u8 c = ECHIDNA_CMD_DECRYPT;
        ret = write(device, &c, sizeof(c)) > 0 &&
              write(device, data, ECHIDNA_LENGTH_BLOCK) > 0 &&
              read(device, data, ECHIDNA_LENGTH_BLOCK) > 0;
    }

    return ret;
}

extern bool
echidna_random(int device, u8 *data)
{
    bool ret = false;

    if (device >= 0)
    {
        u8 c = ECHIDNA_CMD_RANDOM;
        ret = write(device, &c, sizeof(c)) > 0 &&
              read(device, data, ECHIDNA_LENGTH_BLOCK) > 0;
    }

    return ret;
}

extern bool
echidna_keyset(int device, u8 *data)
{
    bool ret = false;

    if (device >= 0)
    {
        u8 c = ECHIDNA_CMD_KEYSET;
        ret = write(device, &c, sizeof(c)) > 0 &&
              write(device, data, ECHIDNA_LENGTH_KEY) > 0 &&
              read(device, &c, sizeof(u8)) > 0;
    }

    return ret;
}
