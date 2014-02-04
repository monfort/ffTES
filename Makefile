#!/usr/bin/make
CC = gcc
EXEC = DecAndCalcTES
all: $(EXEC) demo

$(EXEC): main.c decoder.c
	${CC} -O2 -D__STDC_CONSTANT_MACROS $^ -lm -lc -lz -lavformat -lavcodec -lavutil -o $@

demo: demo.c 

clean:
	rm -rf *o demo $(EXEC)
