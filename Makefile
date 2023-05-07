# See LICENSE file for copyright and license details.

VERSION = pre-3.0

PREFIX = /usr/local

CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Wextra \
         -Wno-unused-parameter -Wno-unused-function \
	 -O3 ${CPPFLAGS}
DBFLAGS = -std=c99 -pedantic -Wall -Wextra \
          -Wno-unused-parameter -Wno-unused-function \
	  -fsanitize=address -fsanitize=undefined \
	  -g3 ${CPPFLAGS}

CC = clang

all: nissy

clean:
	rm -rf nissy

nissy: clean
	${CC} ${CFLAGS} -o nissy src/*.c

debug:
	${CC} ${DBFLAGS} -o nissy cli/*.c src/*.c

cleantables:
	rm -rf tables

tables: cleantables
	${CC} ${DBFLAGS} -o buildtables build/*.c src/*.c
	./buildtables
	rm buildtables

.PHONY: all clean cleantables debug
