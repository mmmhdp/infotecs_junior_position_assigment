#include <stdint.h>
#include <stdlib.h>

typedef struct allocator_buffer {
  struct allocator_buffer *next;
  uint8_t *buffer;
  uint8_t *buffer_end;
} allocator_buffer_t;

typedef struct allocator_block {
  struct allocator_block* next;
} allocator_block_t;

typedef struct allocator {
  allocator_buffer_t *buffers;
  allocator_block_t *blocks;
} allocator_t;

allocator_t* allocator_init();
allocator_block_t* allocator_alloc(allocator_t* allocator);
void allocator_free(allocator_t* allocator, allocator_block_t* allocator_block);
void allocator_self_free(allocator_t* allocator);

int main()
{
  return EXIT_SUCCESS;
}
