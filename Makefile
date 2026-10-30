CC = cc
CFLAGS = -Wall -Wextra -g

all: Part-1/fileManagement Part-2/processManagement Part-2/writer

Part-1/fileManagement: Part-1/fileManagement.c
	$(CC) $(CFLAGS) -o $@ $<

Part-2/processManagement: Part-2/processManagement.c
	$(CC) $(CFLAGS) -o $@ $<

Part-2/writer: Part-2/writer.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f Part-1/fileManagement Part-2/processManagement Part-2/writer Part-2/newfile.txt

.PHONY: all clean
