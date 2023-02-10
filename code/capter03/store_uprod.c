// int类型拓展，可以提供64位的值
#include <inttypes.h>

// 由于这个标准没有提供128位的标准，所以依赖GCC提供的128位整数支持
typedef unsigned __int128 uint128_t;

void store_uprod(uint128_t *dest, uint64_t x, uint64_t y)
{
    *dest = x * (uint128_t)y;
}
