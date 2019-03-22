#!/bin/sh

IN=$1
HEADER=$2

CALL_ENTRIES=$(grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | tail -1 | cut -f1)
CALL_ENTRIES=$(expr $CALL_ENTRIES + 1)

ABI=$(grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | tail -1 | cut -f2)

# marco define
grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | (
  if [ "$ABI" = "i386" ]; then
    echo "#ifndef _XR_CALLS_IA32_H"
    echo "#define _XR_CALLS_IA32_H"
  else
    echo "#ifndef _XR_CALLS_X64_H"
    echo "#define _XR_CALLS_X64_H"
  fi
  echo "#ifndef XR_SYSCALL_MAX"
  echo "#define XR_SYSCALL_MAX ${CALL_ENTRIES}"
  echo "#endif"
  echo
  if [ "$ABI" = "i386" ]; then
    echo "#define XR_IA32_SYSCALL_MAX ${CALL_ENTRIES}"
    echo "extern const char *xr_syscall_table_ia32[XR_IA32_SYSCALL_MAX];"
    echo "extern const int xr_syscall_table_x86_to_x64[XR_IA32_SYSCALL_MAX];"
  else
    echo "#define XR_X64_SYSCALL_MAX ${CALL_ENTRIES}"
    echo "extern const char *xr_syscall_table_x64[XR_SYSCALL_MAX];"
  fi
  echo
  echo "/* syscall macros */"
  while read nr abi name entry compat; do
    # generate syscall number macro
    name_upper=$(echo ${name} | tr 'a-z' 'A-Z')
    if [ "$abi" = "i386" ]; then
      echo
      echo "#ifndef XR_SYSCALL_${name_upper}"
      echo "#define XR_SYSCALL_${name_upper} ${nr}"
      echo "#endif"
      echo "#define XR_IA32_SYSCALL_${name_upper} ${nr}"
    elif [ "$abi" = "x32" ]; then
      echo "#define XR_X32_SYSCALL_${name_upper} ${nr}"
    else
      echo "#define XR_SYSCALL_${name_upper} ${nr}"
    fi
  done
  echo "#endif"
) > "$HEADER"
