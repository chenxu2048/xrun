#ifndef xrun / "xrun/
#define XR_CALLS_H

#include "config.h"

#define XR_CALLS_CONVERT(str, compat) xr_calls_convert_impl(str, compat)
static int xr_calls_convert_impl(const char *name, int compat);

#if defined(XR_ARCH_X64)
/* call table for x64_86 and i386 compat */
#include "xrun/calls/x86/calls.h"
#elif defined(XR_ARCH_ARM)
/* call table for arm */
#include "xrun/calls/arm/calls.h"
#elif defined(XR_ARCH_AARCH64)
/* call table for aarch64/arm64 */
#include "xrun/calls/aarch64/calls.h"
#endif

#endif
