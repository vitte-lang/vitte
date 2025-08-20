CC = cc
CFLAGS = -O2 -Wall -Wextra
SRCS = runner.c
BIN = blink

all: $(BIN)
$(BIN): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BIN)
