# This file is part of Echidna.

# Echidna is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 3.

# Echidna is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with Echidna. If not, see <https://www.gnu.org/licenses/>.

CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra -DDEBUG=1
CFLAGS += -Iinclude

AVR_FLAGS ?= -V -F -c avrisp -b 19200 -p m328p
AVR_CFLAGS ?= -Os -std=gnu99 -mmcu=atmega328p -DF_CPU=16000000UL
AVR_CFLAGS += -Wall -Wextra -Wno-main -Iinclude
DEVICE ?= /dev/ttyUSB0

CLIENT := build/client/libechidna.a build/client/libechidna.so
HOST   := build/host/main.hex

DESTDIR ?= /usr/local

.PHONY: all clean install uninstall flash
all: $(CLIENT) $(HOST)
clean:
	@rm -rf build
	@printf '- Removing build\n'
install: $(CLIENT)
	@printf '- Copying headers into %s/include\n' $(DESTDIR)
	@cp -r include/echidna $(DESTDIR)/include
	@printf '- Copying libraries into %s/lib\n' $(DESTDIR)
	@cp -r $(CLIENT) $(DESTDIR)/lib
uninstall:
	@printf '- Deleting headers from %s/include\n' $(DESTDIR)
	@rm -rf "$(DESTDIR)/include/echidna"
	@printf '- Deleting libraries from %s/lib\n' $(DESTDIR)
	@cd "$(DESTDIR)/lib" && rm -rf $(notdir $(CLIENT))
flash: $(HOST)
	@printf '- Flashing main.hex\n'
	@sudo avrdude $(AVR_FLAGS) -P $(DEVICE) -U flash:w:$<

build build/client build/host:
	@mkdir -p $@
build/client/%.o: src/%.c | build/client
	@printf '- Compiling %s\n' $(notdir $@)
	@$(CC) $(CFLAGS) -c $^ -o $@
build/host/%.o: src/%.c | build/host
	@printf '- Cross-compiling %s\n' $(notdir $@)
	@avr-gcc $(AVR_CFLAGS) -c $^ -o $@

CLIENT_OBJ := client.o
CLIENT_OBJ := $(addprefix build/client/,$(CLIENT_OBJ))
build/client/libechidna.a: $(CLIENT_OBJ) | build/client
	@printf '- Archiving %s\n' $(notdir $@)
	@ar cr $@ $^
	@ranlib $@
build/client/libechidna.so: build/client/client.o | build/client
	@printf '- Creating %s\n' $(notdir $@)
	@$(CC) $(CFLAGS) -fPIC -shared -o $@ $^

build/host/main.bin: build/host/aes.o src/host.c | build/host
	@$(eval GKEY := $(shell head -c 32 /dev/urandom | xxd -i))
	@printf '- Cross-compiling %s\n' $(notdir $@)
	@avr-gcc -DGKEY="$(GKEY)" $(AVR_CFLAGS) $^ -o $@
build/host/main.hex: build/host/main.bin | build/host
	@printf '- Translating %s\n' $(notdir $@)
	@avr-objcopy -O ihex -R .eeprom $< $@
