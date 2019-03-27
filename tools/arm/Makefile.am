SRCDIR = $(top_srcdir)/src/xrun/calls/arm
TBLDIR = $(srcdir)
HDRDIR = $(top_srcdir)/include/xrun/calls/arm

MKHDR = $(srcdir)/mkhdr.sh
MKSRC = $(srcdir)/mksrc.sh

all: $(HDRDIR)/calls_arm.h $(SRCDIR)/calls.c

$(HDRDIR)/calls_arm.h: $(MKHDR) $(TBLDIR)/arm.tbl
	echo "Making calls.h for arm."
	sh $(MKHDR) $(TBLDIR)/arm.tbl $(HDRDIR)/calls_arm.h

$(SRCDIR)/calls.c: $(MKSRC) $(TBLDIR)/arm.tbl
	echo "Making calls.c for arm."
	sh $(MKSRC) $(TBLDIR)/arm.tbl $(SRCDIR)/calls.c

.PHONY: clean
clean:
	rm -f $(HDRDIR)/calls_arm.h $(SRCDIR)/calls.c
