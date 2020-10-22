CC = gcc
CLFAGS = -Wall
SOURCES = pd.c
EXECUTABLES = pd user

all: pd user

pd: pd.c
	gcc -Wall -o pd pd.c

user: user.c
	gcc -o user user.c

fs: fs.c
	gcc -o fs fs.c

run-pd:
	./pd 193.136.128.104 -d 57002 -n 193.136.138.142 -p 58011

run-user:
	./user -n 193.136.138.142 -p 58011 -m 193.136.138.142 -q 59000

run-fs:
	./fs -q 59002 -n 193.136.138.142 -p 59000 -v

clean:
	-rm $(EXECUTABLES)
