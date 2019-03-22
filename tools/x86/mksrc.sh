#!/bin/sh

IN=$1
SOURCE=$2

CALL_ENTRIES=$(grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | tail -1 | cut -f1)

ABI=$(grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | tail -1 | cut -f2)


grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | (
  echo "#include \"xrun/calls.h\""
  echo
  echo "/* call table mapping */"
  if [ "$ABI" = "i386" ]; then
    echo "#ifdef XR_ARCH_X86_IA32"
    echo "const char *xr_syscall_table_ia32[XR_IA32_SYSCALL_MAX] = {"
  else
    echo "#ifdef XR_ARCH_X86_IA32"
    echo "const char *xr_syscall_table_x64[XR_SYSCALL_MAX] = {"
  fi

  # mapping entries
  while read nr abi name entry compat; do
    name_upper=$(echo ${name} | tr 'a-z' 'A-Z')
    if [ "$abi" = "i386" ]; then
      echo "  [XR_IA32_SYSCALL_${name_upper}] = \"$name\","
    elif [ "$abi" = "x32" ]; then
      echo "  [XR_X32_SYSCALL_${name_upper}] = \"$name\","
    else
      echo "  [XR_SYSCALL_${name_upper}] = \"$name\","
    fi
  done
  echo "};"
) > "$SOURCE"

if [ "$ABI" = "i386" ]; then
  grep -E "^[0-9A-Fa-fXx]+[[:space:]]+" "$IN" | sort -n | (
    echo
    echo "#ifdef define(XR_ARCH_X86_64) && define(XR_ARCH_X86_IA32)"
    echo
    echo "const int xr_syscall_table_x86_to_x64[XR_IA32_SYSCALL_MAX] = {"
    while read nr abi name entry compat; do
      name_upper=$(echo ${name} | tr 'a-z' 'A-Z')
      echo "  [XR_IA32_SYSCALL_${name_upper}] = XR_SYSCALL_${name_upper},"
    done
    echo "};"
    echo "#endif"
  ) >> "$SOURCE"
fi

echo "#endif" >> "$SOURCE"
