#include <stdint.h>

typedef uint32_t u32;
typedef int32_t s32;
typedef uint8_t u8;
typedef float f32;
typedef double f64;

typedef enum{ false=0, true=!false } bool;
typedef struct{ f32 x, y; } v2f;
typedef union{ struct{ s32 Row, Column; }; struct{ s32 x, y; }; } v2s;

v2s V2s(s32 x, s32 y){ return (v2s){ .x = x, .y = y }; }
v2f V2f(f32 x, f32 y){ return (v2f){x, y}; }

#define SWAP(a, b, t) {t tmp = (a); (a) = (b); (b) = tmp;}
#define ABS(a) ((a) > 0? (a): (-(a)))
#define MAX(a, b) ((a) > (b)? (a): (b))
#define MIN(a, b) ((a) < (b)? (a): (b))
#define CLAMP(x, a, b) MAX(a, MIN(x, b))
#define SIGN(a) (a >= 0? 1: -1)
