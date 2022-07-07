CFLAGS += -g -Wall  -ffreestanding
LDFLAGS += -g -ffreestanding
CC = gcc

ALL: login

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -Wall -o $@ $<

%.exe: ece391%.o userutils.o sha256.o
	$(CC) $(LDFLAGS) -o $@ $^

%: %.exe
	../../elfconvert $<
	mv $<.converted ../to_fsdir/$@

clean::
	rm -f *~ *.o

clear: clean
	rm -f *.converted
	rm -f *.exe
	rm -f ../to_fsdir/*
