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
	./pd 193.136.128.108 -d 57002 -n 193.136.138.142 -p 58011

run-user:
	./user -n 193.136.138.142 -p 58011 -m 193.136.138.142 -q 59000

clean:
	-rm $(EXECUTABLES)
