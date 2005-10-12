# Makefile for hdf-dods tests
# Note: This Makefile is designed for GNU Make

# Definitions
CC = gcc

HDFDIR = /usr/local/hdf4.2r1
HDFINC = $(HDFDIR)/include
HDFLIB = $(HDFDIR)/lib

PROGS := makean1 makedfp1 makedfr1 makedfr2 makedfr3 makegr1 makegr2 makegr3 \
	makegr4 makegr5 makesds1 makesds2 makesds3 makesds4 makesds5 makesds6 \
	makesds7 makevs1 makevs2 makevs3 makevs4 makevs5

TESTFILES := $(PROGS:make%=test%.hdf)

all: $(PROGS) $(TESTFILES)

clean:
	rm -f $(PROGS) $(TESTFILES) *~

% :: %.c
	$(CC) -o $@ $< -I$(HDFINC) -L$(HDFLIB) -lmfhdf -ldf -lnsl -lz -ljpeg

test%.hdf :: make%
	./$<