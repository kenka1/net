#include "net.h"

void swap_endian(void *ptr, size_t size)
{
  uint8_t byte;
  uint8_t *bptr = ptr;
  for (size_t i = 0; i < size / 2; i++) {
    byte = bptr[i];
    bptr[i] = bptr[size - i - 1];
    bptr[size - i - 1] = byte;
  }
}
