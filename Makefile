all: sfhttpd

clean:
	rm sfhttpd

sfhttpd: main.c
	gcc -W -Wall -Wextra -pedantic -Wno-unused-parameter \
	  -O4 -fno-strict-aliasing \
	  main.c -o sfhttpd
