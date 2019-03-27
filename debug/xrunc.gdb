tui enable
set $SHOWSTACK = 1

define load
  file ./src/xrunc/xrunc
end

define reload
  !make -j
  load
end

load
