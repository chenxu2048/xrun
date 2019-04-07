tui enable
set $SHOWSTACK = 1

file ./src/xrunc/xrunc

define reload
  make -j
  file ./src/xrunc/xrunc
end

