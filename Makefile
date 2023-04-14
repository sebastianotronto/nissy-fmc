# See LICENSE file for copyright and license details.

VERSION = pre-3.0

PREFIX = /usr/local

CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS   = -std=c99 -pthread -pedantic -Wall -Wextra \
	   -Wno-unused-parameter -O3 ${CPPFLAGS}
DBGFLAGS = -std=c99 -pthread -pedantic -Wall -Wextra \
           -Wno-unused-parameter \
           -g ${CPPFLAGS}

CC = cc

all: nissy

nissy: clean
	${CC} ${CFLAGS} -o nissy src/*.c

debug:
	${CC} ${DBGFLAGS} -o nissy src/*.c

.PHONY: all debug

