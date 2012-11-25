#!/bin/bash

for f in $(find . -name "${1:-*}"); do
	if head -n 1 $f | grep HTTP >/dev/null; then
		continue
	fi
	rm -f _tmp
	echo -e "HTTP/1.0 200 OK\r" >> _tmp
	echo -e	"Content-Length: $(wc -c $f | sed -e 's/[ 	].*$//;s/^[ 	]*//')\r" >> _tmp
	echo -e	"Content-Type: text/html\r" >> _tmp
	echo -e "\r" >> _tmp
	cat $f >> _tmp
	mv -f _tmp $f
done
