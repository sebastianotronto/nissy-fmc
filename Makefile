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
	rm -rf nissy nissy_flutter nissy_flutter_ffi

nissy_flutter:
	flutter create nissy_flutter
	cp -R flutter/* nissy_flutter/

nissy_flutter_ffi: tables
	flutter create --template=plugin_ffi \
		--platforms=linux,android,windows,macos,ios nissy_flutter_ffi
	rm -rf nissy_flutter_ffi/src
	cp -R flutter_ffi/* nissy_flutter_ffi/
	cp src/* nissy_flutter_ffi/src/
	cd nissy_flutter_ffi && \
		flutter pub run ffigen --config ffigen.yaml
	cp tables nissy_flutter_ffi/data/

nissy: clean nissy_flutter nissy_flutter_ffi

debug:
	${CC} ${DBFLAGS} -o nissy cli/*.c src/*.c

cleantables:
	rm -rf tables

tables:
	${CC} ${DBFLAGS} -o buildtables build/*.c src/*.c
	./buildtables
	rm buildtables

.PHONY: all clean cleantables debug
