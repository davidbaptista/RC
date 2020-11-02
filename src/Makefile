CC = gcc
CLFAGS = -Wall
EXECUTABLES = pd user FS/fs AS/as

all: pd user fs as

pd: pd.c
	gcc -Wall -o pd pd.c

user: user.c
	gcc -Wall -o user user.c

fs: FS/fs.c
	gcc -Wall -o FS/fs FS/fs.c

as: AS/as.c
	gcc -o AS/as AS/as.c

run-pd:
	./pd 193.136.128.109 -d 57002 -n 193.136.128.103 -p 58002

run-user:
	./user -n 193.136.128.103 -p 58002 -m 193.136.138.142 -q 59000

run-fs:
	./FS/fs -q 59002 -n 193.136.138.142 -p 58011 -v

run-as:
	./AS/as -p 58002 -v

clean:
	-rm $(EXECUTABLES)
