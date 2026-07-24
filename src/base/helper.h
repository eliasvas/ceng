#ifndef HELPER_H__
#define HELPER_H__

#include <stdint.h>
#include <assert.h>
#include <stddef.h>

// TODO: We DONT need the c standard library here!

/////////////////////
// Numeric Typedefs
/////////////////////

typedef uint8_t   u8;
typedef int8_t    s8;
typedef uint16_t  u16;
typedef int16_t   s16;
typedef uint32_t  u32;
typedef int32_t   s32;
typedef uint64_t  u64;
typedef int64_t   s64;
typedef float     f32;
typedef double    f64;
typedef int32_t   b32;
typedef char      b8;

static s8  S8_MIN  = (s8)0x80;
static s16 S16_MIN = (s16)0x8000;
static s32 S32_MN  = (s32)0x80000000;
static s64 S64_MIN = (s64)0x8000000000000000llu;
static s8  S8_MAX  = (s8)0x7f;
static s16 S16_MAX = (s16)0x7fff;
static s32 S32_MAX = (s32)0x7fffffff;
static s64 S64_MAX = (s64)0x7fffffffffffffffllu;
static u8  U8_MAX  = (u8)0xff;
static u16 U16_MAX = (u16)0xffff;
static u32 U32_MAX = (u32)0xffffffff;
static u64 U64_MAX = (u64)0xffffffffffffffffllu;

static f64 F64_MAX = 1.7976931348623158e+308;
static f64 F64_MIN_POS = 2.2250738585072014e-308;
static f32 F32_MAX = 3.402823466e+38F;
static f32 F32_MIN_POS = 1.175494351e-38F;

///////////////////
// Helpful Macros
///////////////////

// FIXME: Why are all these macros not ALL_CAPS?
#define KB(val) ((val)*1024LL)
#define MB(val) ((KB(val))*1024LL)
#define GB(val) ((MB(val))*1024LL)
#define TB(val) ((GB(val))*1024LL)
#define align_pow2(val, align) (((val) + ((align) - 1)) & ~(((val) - (val)) + (align) - 1))
#define align2(val) align_pow2(val,2)
#define align4(val) align_pow2(val,4)
#define align8(val) align_pow2(val,8)
#define align16(val) align_pow2(val,16)
#define align32(val) align_pow2(val,32)
#define align64(val) align_pow2(val,64)
#define equalf(a, b, epsilon) (fabs(b - a) <= epsilon)
#define absolute(a) ((a < 0) ? (-a) : (a))
#define maximum(a, b) ((a) > (b) ? (a) : (b))
#define minimum(a, b) ((a) < (b) ? (a) : (b))
#define step(threshold, value) ((value) < (threshold) ? 0 : 1)
#define clamp(x, a, b)  (maximum(a, minimum(x, b)))
#define lerp(a, b, x) ((b)*(x) + (a)*(1-(x)))
#define is_pow2(x) ((x & (x - 1)) == 0)
#define is_digit(x) (x >= '0' && x <= '9')
#define is_upper(x) (x >= 'A' && x <= 'Z')
#define is_lower(x) (x >= 'a' && x <= 'z')
#define array_count(a) (sizeof(a) / sizeof((a)[0]))
#define signof(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define each_enumv(type, upper, it) type it = (type)0; it < upper##_COUNT; it = (type)(it+1)
#define MATH_PI 3.14159265358979323846

// FIXME: This are.. defined in windows.h as.. they should be..
// :| :| :| :| :| :| :| :| :| :| :| :| |: |: |: |:| :| :| :| :| :|
#define RAD2DEG(X) (180 * X / MATH_PI)
#define DEG2RAD(X) (X * MATH_PI / 180)

#define UINT_FROM_PTR(ptr) ((u64)ptr)
#define PTR_FROM_UINT(num) ((char*)num)

static f64 pwr(double b, int e) {
  f64 p = 1;
  for (int i = 0; i < absolute(e); i++) p = p * b;
  if (e < 0) return 1 / p;
  return p;
}

//////////////////////////////
// Platform Specific Defines
//////////////////////////////

#if !defined(ENABLE_ASSERT)
    #define ENABLE_ASSERT 0
#endif

#if !defined(ENABLE_ASAN)
    #define ENABLE_ASAN 0
#endif

#if (ENABLE_ASAN)
    #include <sanitizer/asan_interface.h>
#endif

#if ENABLE_ASAN
#define AsanPoison(p,z)   __asan_poison_memory_region((p),(z))
#define AsanUnpoison(p,z) __asan_unpoison_memory_region((p),(z))
#else
    #define AsanPoison(p,z)
    #define AsanUnpoison(p,z)
#endif

#if defined(__clang__)
    #define COMPILER_CLANG 1
    #if defined(_WIN32)
        #define OS_WINDOWS 1
    #elif defined(__gnu_linux__) || defined(__EMSCRIPTEN__)
        #define OS_LINUX 1
    #elif defined(__APPLE__) && defined(__MACH__)
        #define OS_MAC 1
    #else
        #error OS Detection failed!
    #endif
    #if defined(__amd64__)
        #define ARCH_X64 1
    #elif defined(__wasm64__)
        #define ARCH_WASM64 1
    #elif defined(__wasm32__)
        #define ARCH_WASM32 1
    #elif defined(__i386__)
        #define ARCH_X86 1
    #elif defined(__arm__)
        #define ARCH_ARM 1
    #elif defined(__aarch64__)
        #define ARCH_ARM64 1
    #else
        #error ARCH detection failed!
    #endif
#elif defined(_MSC_VER)
    #define COMPILER_CL 1
    #if defined(_WIN32)
        #define OS_WINDOWS 1
    #else
        #error OS detection failed!
    #endif
    #if defined(_M_AMD64)
        #define ARCH_X64 1
    #elif defined(_M_I86)
        #define ARCH_X86 1
    #elif defined(_M_ARM)
        #define ARCH_ARM 1
    #else
        #error ARCH detection failed!
    #endif
#elif defined(__GNUC__)
    #define COMPILER_GCC 1
    #if defined(_WIN32)
        #define OS_WINDOWS 1
    #elif defined(__gnu_linux__)
        #define OS_LINUX 1
    #elif defined(__APPLE__) && defined(__MACH__)
        #define OS_MAC 1
    #else
        #error OS detection failed!
    #endif
    #if defined(__amd64__)
        #define ARCH_X64 1
    #elif defined(__i386__)
        #define ARCH_X86 1
    #elif defined(__arm__)
        #define ARCH_ARM 1
    #elif defined(__aarch64__)
        #define ARCH_ARM64 1
    #else
        #error ARCH detection failed!
    #endif

#else
#error Couldnt find anything about this context
#endif

#if !defined(COMPILER_CL)
    #define COMPILER_CL 0
#endif
#if !defined(COMPILER_CLANG)
    #define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
    #define COMPILER_GCC 0
#endif
#if !defined(OS_WINDOWS)
    #define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
    #define OS_LINUX 0
#endif
#if !defined(OS_MAC)
    #define OS_MAC 0
#endif
#if !defined(ARCH_X64)
    #define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
    #define ARCH_X86 0
#endif
#if !defined(ARCH_ARM)
    #define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
    #define ARCH_ARM64 0
#endif
#if !defined(ARCH_WASM64)
    #define ARCH_WASM64 0
#endif
#if !defined(ARCH_WASM32)
    #define ARCH_WASM32 0
#endif

#if defined(__cplusplus)
    #define LANG_CXX 1
#else
    #define LANG_C 1
#endif
#if ARCH_X64 || ARCH_ARM64 || ARCH_WASM64
    #define ARCH_ADDRSIZE 64
#else
    #define ARCH_ADDRSIZE 32
#endif
#if (OS_WINDOWS)
    #include <windows.h>
#endif

#define INLINE static inline
#define INTERNAL static
#define global_var static

//////////////////////////////
// Linked-List Macros
//////////////////////////////

#define sll_stack_push_N(f,n,next) ((n)->next=(f), (f)=(n))
#define sll_stack_pop_N(f,next) ((f==0)?((f)=(f)):((f)=(f)->next))
#define sll_stack_push(f,n) sll_stack_push_N(f,n,next)
#define sll_stack_pop(f) sll_stack_pop_N(f,next)

#define sll_queue_push_N(f,l,n,next) (((f)==0)?((f)=(l)=(n)):((l)->next=(n),(l)=(n),(n)->next=0))
#define sll_queue_pop_N(f,l,next) (((f)==(l))?((f)=(l)=0):((f)=(f)->next))
#define sll_queue_push(f,l,n) sll_queue_push_N(f,l,n,next)
#define sll_queue_pop(f,l) sll_queue_pop_N(f,l,next)

#define check_zero(z,p) ((p) == 0 || (p) == z)
#define set_zero(z,p) ((p) = z)
#define dll_insert_NPZ(z,f,l,p,n,next,prev) (check_zero(z,f) ? ((f) = (l) = (n), set_zero(z,(n)->next), set_zero(z,(n)->prev)) : check_zero(z,p) ? ((n)->next = (f), (f)->prev = (n), (f) = (n), set_zero(z,(n)->prev)) : ((p)==(l)) ? ((l)->next = (n), (n)->prev = (l), (l) = (n), set_zero(z, (n)->next)) : (((!check_zero(z,p) && check_zero(z,(p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define dll_push_back_NPZ(z,f,l,n,next,prev) dll_insert_NPZ(z,f,l,l,n,next,prev)
#define dll_push_back_NP(f,l,n,next,prev) dll_push_back_NPZ(0,f,l,n,next,prev)
#define dll_push_back(f,l,n) dll_push_back_NP(f,l,n,next,prev)
#define dll_push_front_NPZ(z,f,l,n,next,prev) dll_insert_NPZ(z,l,f,f,n,prev,next)
#define dll_push_front_NP(f,l,n,next,prev) dll_push_front_NPZ(0,f,l,n,next,prev)
#define dll_push_front(f,l,n) dll_push_back_NP(l,f,n,prev,next)
#define dll_remove_NPZ(z,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)), ((n) == (l) ? (l) = (l)->prev : (0)), (check_zero(z,(n)->prev) ? (0) : ((n)->prev->next = (n)->next)), (check_zero(z,(n)->next) ? (0) : ((n)->next->prev = (n)->prev)))
#define dll_remove_NP(f,l,n,next,prev) dll_remove_NPZ(0,f,l,n,next,prev)
#define dll_remove(f,l,n) dll_remove_NP(f,l,n,next,prev)

//////////////////////////////
// Memory Allocation Helpers
//////////////////////////////

#if (OS_WINDOWS)
    #define M_RESERVE(bytes) VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_NOACCESS)
    #define M_COMMIT(reserved_ptr, bytes) VirtualAlloc(reserved_ptr, bytes, MEM_COMMIT, PAGE_READWRITE)
    #define M_ALLOC(bytes) VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
    #define M_RELEASE(base, bytes) VirtualFree(base, bytes, MEM_RELEASE)
    #define M_ZERO(p, s) (ZeroMemory(p, s))
// TODO: NO cstdlib
    #define M_CMP(a, b, s) memcmp(a, b, s)
#else
    #include <sys/mman.h>
    #include <string.h>
    #define M_RESERVE(bytes) mmap(NULL, bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0)
    #define M_COMMIT(reserved_ptr, bytes) mmap(reserved_ptr, bytes, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define M_ALLOC(bytes) mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define M_RELEASE(base, bytes) munmap(base, bytes)
    #define M_ZERO(p, s) memset(p, 0, s)
// TODO: NO cstdlib
    #define M_CMP(a, b, s) memcmp(a, b, s)
#endif

#define M_ZERO_STRUCT(p)  M_ZERO((p), sizeof(*(p)))
#define M_ZERO_ARRAY(a)  M_ZERO((a), sizeof(a))
// TODO: add memcpy and memcmp here please!

// FIXME: NO STD LIBRARY PLEASE
#define M_COPY(dest, src, n) memcpy(dest, src,n);

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#define ASAN_POISON_MEMORY_REGION(addr, size) \
  __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
  __asan_unpoison_memory_region((addr), (size))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#endif

//////////////////////////////
// String stuff
//////////////////////////////

s64 cstr_count(const char *s);
#define MAKE_STR(s) ((buf){s, cstr_count(s)})

static b32 str_cmp(char *l, char *r, s64 size) {
  if (!l || !r) return 0;
  for (s64 idx = 0; idx < size; idx++) {
    if (l[idx] != r[idx]) return 0;
  }
  return 1;
}

static b32 str_find(char *s, char c, s64 size) {
  for (s64 idx = 0; idx < size; idx++) {
    if (s[idx] == c) return 1;
  }
  return 0;
}

static s64 str_to_int(char *s, s64 size) {
  s64 counter = 0;
  for (s64 idx = size-1; idx >= 0; idx-=1) {
    counter += (s[size-idx-1] - '0') * pwr(10,idx);
  }
  return counter;
}

static f64 str_to_float(char *s, s64 size) {
  f64 counter = 0.0;
  s64 dot_idx = -1;
  s64 sign = 1;

  if (size == 0) return 0.0;

  // handle sign
  u64 start = 0;
  if (s[0] == '-') {
    sign = -1;
    start = 1;
  } else if (s[0] == '+') {
    start = 1;
  }

  // find dot index
  for (s64 i = start; i < size; i++) {
    if (s[i] == '.') {
      dot_idx = i;
      break;
    } else if (s[i] == 'e' || s[i] == 'E') {
      // cop out with scientific notation
      return 0.0;
    }
  }

  if (dot_idx == -1) {
    counter = (f64)str_to_int(s + start, size - start);
  } else {
    // integer part
    u32 p = 0;
    for (s64 i = (s64)dot_idx - 1; i >= (s64)start; i-=1, p+=1) {
      counter += (s[i] - '0') * pwr(10, p);
    }

    // fractional part
    p = 1;
    for (s64 i = dot_idx + 1; i < size; i+=1, p+=1) {
      counter += (s[i] - '0') / pwr(10, p);
    }
  }

  return counter * sign;
}

static b32 str_to_bool(char *s, s64 size) {
  return str_cmp(s, "true", size);
}

//////////////////////////////
// Buffer abstraction
//////////////////////////////

typedef struct {
  char *data;
  s64 count;
} buf;

static buf buf_make(char *data, u64 count) {
  return (buf) {
    .data = data,
    .count = count,
  };
}

static b32 buf_eq(buf l, buf r) {
  return str_cmp(l.data, r.data, l.count);
}

static b32 buf_contains(buf buf, char c) {
  return str_find(buf.data, c, buf.count);
}

static s64 buf_to_int(buf b) {
  return str_to_int(b.data, b.count);
}

static f64 buf_to_float(buf b) {
  return str_to_float(b.data, b.count);
}

static b32 buf_to_bool(buf b) {
  return str_to_bool(b.data, b.count);
}

static buf buf_lcut(buf b, buf delim) {
  for (s64 i = 0; i < (b.count - delim.count); i+=1) {
    buf candidate = buf_make(b.data, i);
    if (buf_eq(delim, buf_make(&b.data[i], delim.count))) return candidate;
  }
  return b;
}



#endif
