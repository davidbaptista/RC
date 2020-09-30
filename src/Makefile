CC = gcc
CLFAGS =
SOURCES = pd.c
EXECUTABLES = pd

all: pd.c
	$(CC) $(CLFAGS) $(SOURCES) -o $(EXECUTABLES)

run:
	./pd 194.210.134.223 -d 57002 -n 193.136.138.142 -p 58011

clean:
	-rm $(EXECUTABLES)