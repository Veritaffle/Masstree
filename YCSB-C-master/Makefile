CC=g++
CFLAGS=-std=c++11 -g -Wall -pthread -I./
# LDFLAGS= -lpthread -ltbb -lhiredis
LDFLAGS= -L/usr/local/lib/libhiredis.so.0.13
#	TODO broken so in the meantime run export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
LDLIBS= -lpthread -lhiredis
SUBDIRS=core db redis
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
EXEC=ycsbc

all: $(SUBDIRS) $(EXEC)

$(SUBDIRS):
	$(MAKE) -C $@

$(EXEC): $(wildcard *.cc) $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@ $(LDLIBS)

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) $(EXEC)

.PHONY: $(SUBDIRS) $(EXEC)

