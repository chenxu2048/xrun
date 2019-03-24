#ifndef XR_CALLS_H
#define XR_CALLS_H

#include "config.h"
#define XR_ARCH_X86_64

#define XR_CALLS_CONVERT(name, compat) xr_calls_convert_impl(name, compat)
#define XR_CALLS_NAME(scno, compat) xr_calls_name_impl(scno, compat)
static int xr_calls_convert_impl(const char* name, int compat);
static const char* const xr_calls_name_impl(long scno, int compat);

#if defined(XR_ARCH_X86_IA32) || defined(XR_ARCH_X86_64)
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
