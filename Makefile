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
	rm -f nissy

nissy: clean
	mkdir -p tables
	${CC} ${CFLAGS} -o nissy src/*.c

debug:
	${CC} ${DBFLAGS} -o nissy src/*.c

.PHONY: all clean debug

