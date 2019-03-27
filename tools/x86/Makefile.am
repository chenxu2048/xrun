SRCDIR = $(top_srcdir)/src/xrun/calls/x86
TBLDIR = $(srcdir)
HDRDIR = $(top_srcdir)/include/xrun/calls/x86

MKHDR = $(srcdir)/mkhdr.sh
MKSRC = $(srcdir)/mksrc.sh

all: $(HDRDIR)/calls_64.h $(SRCDIR)/calls_64.c $(HDRDIR)/calls_32.h $(SRCDIR)/calls_32.c

$(HDRDIR)/calls_64.h: $(MKHDR) $(TBLDIR)/x64.tbl
	echo "Making calls.h for x64."
	sh $(MKHDR) $(TBLDIR)/x64.tbl $(HDRDIR)/calls_64.h

$(SRCDIR)/calls_64.c: $(MKSRC) $(TBLDIR)/x64.tbl
	echo "Making calls.c for x64."
	sh $(MKSRC) $(TBLDIR)/x64.tbl $(SRCDIR)/calls_64.c


$(HDRDIR)/calls_32.h: $(MKHDR) $(TBLDIR)/x86.tbl
	echo "Making calls.h for ia32."
	sh $(MKHDR) $(TBLDIR)/x86.tbl $(HDRDIR)/calls_32.h

$(SRCDIR)/calls_32.c: $(MKSRC) $(TBLDIR)/x86.tbl
	echo "Making calls.c for ia32."
	sh $(MKSRC) $(TBLDIR)/x86.tbl $(SRCDIR)/calls_32.c

.PHONY: clean
clean:
	rm -f $(HDRDIR)/calls_64.h $(SRCDIR)/calls_64.c $(HDRDIR)/calls_32.h $(SRCDIR)/calls_32.c
