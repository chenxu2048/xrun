all: arm_calls_header arm_calls_src

arm_calls_header:
	echo "Making calls.h for arm."
	sh mkhdr.sh arm.tbl $(top_srcdir)/include/xrun/calls/arm/calls_arm.h

arm_calls_src:
	echo "Making calls.c for arm."
	sh mksrc.sh arm.tbl $(top_srcdir)/src/xrun/calls/arm/calls.c
