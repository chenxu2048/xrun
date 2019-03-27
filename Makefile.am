SUBDIRS=tools/x86 tools/arm src

if DEBUG
.PHONY: debug
debug:
	gdb -ix $(top_srcdir)/debug/xrunc.gdb
endif
