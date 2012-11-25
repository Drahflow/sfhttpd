all: sfhttpd

clean:
	rm sfhttpd

sfhttpd: main.c
	gcc -W -Wall -O4 main.c -o sfhttpd
