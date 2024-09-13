
CC=gcc
CFLAGS=-Wall -Wextra -std=c99


TARGETS = sfl


build: $(TARGETS)

sfl: sfl.c
	$(CC) $(CFLAGS) sfl.c -lm -o sfl


run_sfl: sfl
	./sfl


pack:
	zip -FSr 314CA_CucuViorel-Cosmin_Tema1.zip README Makefile *.c *.h


clean:
	rm -f $(TARGETS)
