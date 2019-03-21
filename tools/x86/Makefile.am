all: x64_calls_header x64_calls_src ia32_calls_header x64_calls_src

SRCDIR = $(top_srcdir)/src/xrun/calls/x86
TBLDIR = $(srcdir)
HDRDIR = $(top_srcdir)/include/xrun/calls/x86

x64_calls_header:
	echo "Making calls.h for x64."
	sh mkhdr.sh $(TBLDIR)/x64.tbl $(SRCDIR)/calls_64.h

x64_calls_src:
	echo "Making calls.c for x64."
	sh mksrc.sh $(TBLDIR)/x64.tbl $(HDRDIR)/calls_64.c


ia32_calls_header:
	echo "Making calls.h for ia32."
	sh mkhdr.sh $(TBLDIR)/x86.tbl $(SRCDIR)/calls_32.h

ia32_calls_src:
	echo "Making calls.c for ia32."
	sh mksrc.sh $(TBLDIR)/x86.tbl $(HDRDIR)/calls_32.c
