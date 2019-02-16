#ifndef _XR_TRACERIMPL_H
#define _XR_TRACERIMPL_H

#include "tracer/process.h"
struct xr_tracer_impl_s;
typedef struct xr_tracer_impl_s xr_tracer_impl_t;

typedef struct xr_tracer_s xr_tracer_t;

typedef bool _xr_tracer_step_f(xr_tracer_impl_t *tracer_impl);

typedef int _xr_tracer_trap_f(xr_tracer_impl_t *tracer_impl,
                              xr_thread_t **trapped_thread);

#endif
