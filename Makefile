CC      ?= gcc
CFLAGS  := -Wall -Wextra -Werror -std=c11 -O2 -Iinclude
LDFLAGS :=

SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:.c=.o)
BIN     := pciex

PREFIX  ?= /usr/local

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(BIN)

install: $(BIN)
	install -Dm755 $(BIN) $(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(PREFIX)/bin/$(BIN)
