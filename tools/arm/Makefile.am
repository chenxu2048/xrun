all: arm_calls_header arm_calls_src

SRCDIR = $(top_srcdir)/src/xrun/calls/arm
TBLDIR = $(srcdir)
HDRDIR = $(top_srcdir)/include/xrun/calls/arm

arm_calls_header:
	echo "Making calls.h for arm."
	sh mkhdr.sh $(TBLDIR)/arm.tbl $(HDRDIR)/calls_arm.h

arm_calls_src:
	echo "Making calls.c for arm."
	sh mksrc.sh $(TBLDIR)/arm.tbl $(SRCDIR)/calls.c
