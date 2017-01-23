CXX=g++
CFLAGS=-fPIC -c -I. -I/usr/include -I/usr/include/pgsql -I/usr/local/lib
LDFLAGS=-shared -L. -L/usr/local/lib -lpq

LIBEXT=so

OSNAME := $(shell uname -s)
ifeq ($(OSNAME), Darwin)
CFLAGS:=$(CFLAGS) -I/Applications/Postgres.app/Contents/Versions/9.6/include
LDFLAGS:=$(LDFLAGS) -L/Applications/Postgres.app/Contents/Versions/9.6/lib
LIBEXT=dylib
endif

all: libpgci.o libpgci.$(LIBEXT)

libpgci.$(LIBEXT): libpgci.o
	$(CXX) $(LDFLAGS) libpgci.o -o $@

libpgci.o: pgci.cpp
	$(CXX) $(CFLAGS) -o $@ -c pgci.cpp

clean:
	@echo "Cleaning up source directory"
	rm -f *.o *.$(LIBEXT) *~ *.*~ sample/*.*~

install: libpgci.$(LIBEXT)
	@echo "Installing libpgci to /usr/local"
	cp libpgci.h libpgci.$(LIBEXT) /usr/local/lib

uninstall:
	@echo "Uninstalling libpgci from /usr/local"
	rm -f /usr/local/lib/libpgci.*
