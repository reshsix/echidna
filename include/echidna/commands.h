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

#ifndef ECHIDNA_COMMANDS_H
#define ECHIDNA_COMMANDS_H

enum echidna_command
{
    ECHIDNA_CMD_NOOP,
    ECHIDNA_CMD_PING,
    ECHIDNA_CMD_ENCRYPT,
    ECHIDNA_CMD_DECRYPT,
    ECHIDNA_CMD_RANDOM,
    ECHIDNA_CMD_KEYSET
};

#endif
