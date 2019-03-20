#ifndef XR_UTILS_H
#define XR_UTILS_H

#include <stdlib.h>
#define _XR_NEW(type) ((type *)malloc(sizeof(type)))

#define _XR_CALLP(_this, _func, ...) ((_this)->_func((_this), __VA_ARGS__))

#define _XR_CALL(obj, _func, ...) _XR_CALLP(&obj, _func, __VA_ARGS__))

#endif
