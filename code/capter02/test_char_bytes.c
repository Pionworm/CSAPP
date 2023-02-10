#include <string.h>
#include "./byte_pointer.c"

int main() {
    const char *s = "abcdef";
    show_bytes((byte_pointer) s, strlen(s));
}