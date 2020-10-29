CC = gcc
CLFAGS = -Wall
EXECUTABLES = pd user fs

all: pd user fs

pd: pd.c
	gcc -Wall -o pd pd.c

user: user.c
	gcc -Wall -o user user.c

fs: fs.c
	gcc -Wall -o fs fs.c

run-pd:
	./pd 193.136.128.108 -d 57002 -n 193.136.138.142 -p 58011

run-user:
	./user -n 193.136.138.142 -p 58011 -m 193.136.128.109 -q 59002

run-fs:
	./fs -q 59002 -n 193.136.138.142 -p 58011 -v

clean:
	-rm $(EXECUTABLES)
