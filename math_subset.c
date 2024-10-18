#define _CRT_SECURE_NO_WARNINGS

#include "libm.h"
#include <immintrin.h>
#include <windows.h>

#undef _CRT_FUNCTIONS_REQUIRED
#define _CRT_FUNCTIONS_REQUIRED 1
#include <fenv.h>

static HMODULE hucrtbase;
typedef int(__cdecl* _HANDLE_MATH_ERROR)(struct _exception*);
static _HANDLE_MATH_ERROR  err_func;

void(__cdecl* ucrtbase___setusermatherr)(_HANDLE_MATH_ERROR func);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void* reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        hucrtbase = LoadLibraryW(L"ucrtbase.dll");
        ucrtbase___setusermatherr = (void*)GetProcAddress(hucrtbase, "__setusermatherr");
        DisableThreadLibraryCalls(hinst);
        break;
    case DLL_PROCESS_DETACH:
        FreeLibrary(hucrtbase);
        break;
    }
    return TRUE;
}

void __cdecl __setusermatherr(_HANDLE_MATH_ERROR func)
{
    err_func = func;
    ucrtbase___setusermatherr(func);
}

double math_error(int type, const char *name, double arg1, double arg2, double retval)
{
    static int err[] = { 0, EDOM, ERANGE, ERANGE };

    if (err_func)
    {
        struct _exception exception = { type, (char*)name, arg1, arg2, retval };

        if (err_func(&exception)) return exception.retval;
    }
    if (type < ARRAYSIZE(err)) errno = err[type];

    return retval;
}

/*********************************************************************
 * Copied from musl: include/math.h
 */
static __inline uint32_t __FLOAT_BITS(float __f)
{
    union { float __f; unsigned __i; } __u;
    __u.__f = __f;
    return __u.__i;
}

/*********************************************************************
 * Copied from musl: include/math.h
 */
static __inline uint64_t __DOUBLE_BITS(double __f)
{
    union { double __f; unsigned long long __i; } __u;
    __u.__f = __f;
    return __u.__i;
}


#undef signbit
/*********************************************************************
 * Copied from musl: include/math.h
 */
#define signbit(x) ( \
	sizeof(x) == sizeof(float) ? (int)((__FLOAT_BITS(x))>>31) : \
	(int)(__DOUBLE_BITS(x)>>63) )

int __cdecl _fdsign(float x)
{
    return signbit(x);
}

int __cdecl _dsign(double x)
{
    return signbit(x);
}

#define return_int_validate(expr, ftype, itype) \
    ftype v = (expr); \
    if (sizeof(itype) == 4 && (v < MININT32 || v > MAXINT32) \
        || sizeof(itype) == 8 && (v < MININT64 || v > MAXINT64)) \
    { \
        *_errno() = EDOM; \
        return 0; \
    } \
    return (itype)v;

#pragma function(lrint)
long __cdecl lrint(double x)
{
    return_int_validate(rint(x), double, long);
}

#pragma function(lrintf)
long __cdecl lrintf(float x)
{
    return_int_validate(rintf(x), float, long);
}

int64_t __cdecl llrint(double x)
{
    return_int_validate(rint(x), double, int64_t);
}

__int64 __cdecl llrintf(float x)
{
    return_int_validate(rintf(x), float, int64_t);
}

/*********************************************************************
 * Copied from musl: src/math/lround.c
 */
long __cdecl lround(double x)
{
    double d = round(x);
    if (d != (double)(long)d) {
        *_errno() = EDOM;
        return 0;
    }
    return d;
}

/*********************************************************************
 * Copied from musl: src/math/lroundf.c
 */
long __cdecl lroundf(float x)
{
    float f = roundf(x);
    if (f != (float)(long)f) {
        *_errno() = EDOM;
        return 0;
    }
    return f;
}

/*********************************************************************
 * Copied from musl: src/math/llround.c
 */
int64_t __cdecl llround(double x)
{
    double d = round(x);
    if (d != (double)(__int64)d) {
        *_errno() = EDOM;
        return 0;
    }
    return d;
}

/*********************************************************************
 * Copied from musl: src/math/llroundf.c
 */
int64_t __cdecl llroundf(float x)
{
    float f = roundf(x);
    if (f != (float)(__int64)f) {
        *_errno() = EDOM;
        return 0;
    }
    return f;
}

#undef isnan
/*********************************************************************
 * Copied from musl: include/math.h
 */
#define isnan(x) ( \
	sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) > 0x7f800000 : \
	(__DOUBLE_BITS(x) & -1ULL>>1) > 0x7ffULL<<52)

int __cdecl _isnan(double x)
{
    return isnan(x);
}

int __cdecl _isnanf(float x)
{
    return isnan(x);
}

#define DECLARE_FUNC(name, ftype, expr) \
ftype __cdecl math_##name(ftype x) \
{ \
    if (expr) \
    { \
        *_errno() = EDOM; \
        feraiseexcept(FE_INVALID); \
        return NAN; \
    } \
    return name(x); \
}

DECLARE_FUNC(acosh, double, x < 1.0)
DECLARE_FUNC(acoshf, float, x < 1.0f)
DECLARE_FUNC(atanh, double, fabs(x) > 1.0)
DECLARE_FUNC(atanhf, float, fabs(x) > 1.0f)

double __cdecl math_sqrt(double x)
{
    __m128d tmp;

    if (signbit(x))
        return math_error(_DOMAIN, "sqrt", x, 0, -NAN);

    tmp = _mm_set_sd(x);
    return _mm_cvtsd_f64(_mm_sqrt_sd(tmp, tmp));
}

float __cdecl math_sqrtf(float x)
{
    __m128 tmp;

    if (signbit(x))
        return math_error(_DOMAIN, "sqrtf", x, 0, -NAN);

    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
}
