all: x64_calls_header x64_calls_src ia32_calls_header x64_calls_src

SRCDIR=$(top_srcdir)/src/xrun/calls/x86
HDRDIR=$(top_srcdir)/include/xrun/calls/x86

x64_calls_header:
	echo "Making calls.h for x64."
	sh mkhdr.sh x64.tbl $(SRCDIR)/calls_64.h

x64_calls_src:
	echo "Making calls.c for x64."
	sh mksrc.sh x64.tbl $(HDRDIR)/calls_64.c


ia32_calls_header:
	echo "Making calls.h for ia32."
	sh mkhdr.sh x86.tbl $(SRCDIR)/calls_32.h

x64_calls_src:
	echo "Making calls.c for ia32."
	sh mksrc.sh x86.tbl $(HDRDIR)/calls_32.c
