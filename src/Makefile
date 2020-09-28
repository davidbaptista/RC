CC = gcc
CLFAGS =
SOURCES = pd.c
EXECUTABLES = pd

all: pd.c
	$(CC) $(CLFAGS) $(SOURCES) -o $(EXECUTABLES)

run:
	./pd 84.90.92.172 -d 57002 -n 193.136.138.142 -p 58002

clean:
	-rm $(EXECUTABLES)