#!/bin/sh

IN=$1
SOURCE=$2

CALL_ENTRIES=$(grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | tail -1 | cut -f1)

# calls table mapping call number to name string
grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | (
  echo "#include \"xrun/calls.h\""
  echo
  echo "#ifdef XR_ARCH_ARM"
  echo "/* system call mapping table for arm (both eabi and oabi) */"
  echo "const char *xr_syscall_table_arm[XR_SYSCALL_MAX] = {"
  while read nr abi name entry compat; do
    name_upper=$(echo ${name} | tr 'a-z' 'A-Z')
    echo "  [XR_SYSCALL_${name_upper}] = \"$name\","
  done
  echo "};"
  echo "#endif"
) > "$SOURCE"
