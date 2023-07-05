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

#ifndef ECHIDNA_CLIENT_H
#define ECHIDNA_CLIENT_H

#include <echidna/types.h>

int echidna_open(const char *path, int timeout);
void echidna_close(int device);
bool echidna_sane(int device);
bool echidna_ping(int device, u8 data, int timeout);
bool echidna_encrypt(int device, u8 *data);
bool echidna_decrypt(int device, u8 *data);
bool echidna_random(int device, u8 *data);
bool echidna_keyset(int device, u8 *data);

#endif
