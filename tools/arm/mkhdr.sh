#!/bin/sh

IN=$1
HEADER=$2

CALL_ENTRIES=$(grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | tail -1 | cut -f1)
CALL_ENTRIES=$(expr $CALL_ENTRIES + 1)

# marco define
grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | (
  echo "#ifndef _XR_CALLS_ARM_H"
  echo "#define _XR_CALLS_ARM_H"
  echo
  echo "#define xr_syscall_table xr_syscall_table_arm"
  echo "#define XR_ARM_SYSCALL_NORMAL ${CALL_ENTRIES}"
  echo "#define XR_ARM_SYSCALL_PRIVATE 8"
  echo "#define XR_SYSCALL_MAX (XR_ARM_SYSCALL_NORMAL + XR_ARM_SYSCALL_PRIVATE)"
  echo "extern const char *xr_syscall_table_arm[XR_SYSCALL_MAX];"
  echo
  echo "#define xr_syscall_arm_to_oabi(nr) ((nr) + 0x900000)"
  echo "#define xr_syscall_arm_from_oabi(nr) ((nr) - 0x900000)"
  echo "#define xr_syscall_arm_private_convert(nr) ((nr & 0xffff) + XR_ARM_SYSCALL_NORMAL)"
  echo
  echo "/* marco define region begin */"
  while read nr abi name entry compat; do
    # generate syscall number macro
    name_upper=$(echo ${name} | tr 'a-z' 'A-Z')
    echo "#define XR_SYSCALL_${name_upper} ${nr}"
  done

  echo "#define XR_SYSCALL_BREAKPOINT (XR_ARM_SYSCALL_NORMAL + 1)"
  echo "#define XR_SYSCALL_CACHEFLUSH (XR_ARM_SYSCALL_NORMAL + 2)"
  echo "#define XR_SYSCALL_USR26 (XR_ARM_SYSCALL_NORMAL + 3)"
  echo "#define XR_SYSCALL_USR32 (XR_ARM_SYSCALL_NORMAL + 4)"
  echo "#define XR_SYSCALL_SET_TLS (XR_ARM_SYSCALL_NORMAL + 5)"
  echo "#define XR_SYSCALL_GET_TLS (XR_ARM_SYSCALL_NORMAL + 6)"
  echo "/* marco define region end */"
  echo "#endif"
) > "$HEADER"


