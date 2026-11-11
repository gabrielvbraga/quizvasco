CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra

quizvasco: quiz/vasco.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f quizvasco quizvasco.exe

.PHONY: clean
