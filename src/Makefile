CC = gcc
CLFAGS = -Wall
SOURCES = pd.c
EXECUTABLES = pd user

all: pd user

pd: pd.c
	gcc -Wall -o pd pd.c

user: user.c
	gcc -o user user.c

run-pd:
	./pd 194.210.134.223 -d 57002 -n 193.136.138.142 -p 58011

run-user:
	./user -n 194.210.134.223 -p 58011 -m 194.210.134.223 -q 59011

clean:
	-rm $(EXECUTABLES)