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

#ifndef AES_H
#define AES_H

#include <echidna/types.h>

void aes_context(u8 *key);
void aes_encrypt(u8 *block);
void aes_decrypt(u8 *block);

#endif
