#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ALIGN_TO(value, alignment) \
  (((value) + ((alignment) - 1)) & ~((alignment) - 1))

typedef struct allocator_buffer {
  struct allocator_buffer *next;
  uint8_t *buffer_end;
  uint8_t *buffer;
} allocator_buffer_t;

typedef struct allocator_block {
  struct allocator_block *next;
} allocator_block_t;

typedef struct allocator {
  size_t allocator_buffer_size;
  size_t allocator_block_size;
  size_t allocator_blocks_per_buffer;

  allocator_buffer_t *buffers;
  allocator_block_t *blocks;
} allocator_t;

allocator_t allocator_init(size_t allocator_block_size,
                           size_t blocks_per_buffer)
{
  if (allocator_block_size < sizeof(allocator_block_t))
    allocator_block_size = sizeof(allocator_block_t);

  allocator_block_size = ALIGN_TO(allocator_block_size, alignof(max_align_t));

  return (allocator_t) { .allocator_block_size = allocator_block_size,
                         .allocator_buffer_size =
                             allocator_block_size * blocks_per_buffer,
                         .allocator_blocks_per_buffer = blocks_per_buffer };
}

int allocator_alloc_buffer(allocator_t *allocator)
{
  uint8_t *buffer = malloc(allocator->allocator_buffer_size);
  if (!buffer)
    return 0;

  allocator_buffer_t *allocator_buffer = malloc(sizeof(allocator_buffer_t));
  if (!allocator_buffer) {
    free(buffer);
    return 0;
  }

  allocator_buffer->buffer = buffer;
  allocator_buffer->buffer_end = buffer + allocator->allocator_buffer_size;
  allocator_buffer->next = allocator->buffers;

  allocator->buffers = allocator_buffer;

  for (int i = 0; i != allocator->allocator_blocks_per_buffer; ++i) {
    allocator_block_t *allocator_block =
        (allocator_block_t *) (allocator_buffer->buffer +
                               i * allocator->allocator_block_size);

    allocator_block->next = allocator->blocks;
    allocator->blocks = allocator_block;
  }

  return 1;
}

allocator_block_t *allocator_alloc(allocator_t *allocator, size_t size)
{
  if (size > allocator->allocator_block_size)
    return NULL;

  if (!allocator->blocks) {
    int is_allocated_succssfully = allocator_alloc_buffer(allocator);
    if (!is_allocated_succssfully)
      return NULL;
  }

  allocator_block_t *allocator_block = allocator->blocks;
  allocator->blocks = allocator->blocks->next;

  return allocator_block;
}

void allocator_free(allocator_t *allocator, allocator_block_t *allocator_block)
{
  if (!allocator_block)
    return;

  int is_already_freed = 0;
  for (allocator_block_t *all_block = allocator->blocks; all_block;
       all_block = all_block->next) {
    if (all_block == allocator_block) {
      is_already_freed = 1;
      break;
    }
  }

  // maybe here is better to report a double free error
  if (is_already_freed) {
    abort();
  }

  uintptr_t allocator_block_ptr = (uintptr_t) allocator_block;
  int is_block_belongs_to_allocator = 0;

  for (allocator_buffer_t *allocator_buffer = allocator->buffers;
       allocator_buffer; allocator_buffer = allocator_buffer->next) {
    uintptr_t buffer_begin_ptr = (uintptr_t) allocator_buffer->buffer;
    uintptr_t buffer_end_ptr = (uintptr_t) allocator_buffer->buffer_end;

    if (allocator_block_ptr >= buffer_begin_ptr &&
        allocator_block_ptr < buffer_end_ptr) {
      size_t offset = allocator_block_ptr - buffer_begin_ptr;
      if (offset % allocator->allocator_block_size == 0) {
        is_block_belongs_to_allocator = 1;
      }
      break;
    }
  }

  // maybe better to say it's just UB
  if (!is_block_belongs_to_allocator) {
    abort();
  }

  allocator_block->next = allocator->blocks;
  allocator->blocks = allocator_block;
}

void allocator_self_free(allocator_t *allocator)
{
  allocator_buffer_t *allocator_buffer = allocator->buffers;
  while (allocator_buffer) {
    allocator_buffer_t *next_allocator_buffer = allocator_buffer->next;
    free(allocator_buffer->buffer);
    free(allocator_buffer);
    allocator_buffer = next_allocator_buffer;
  }

  allocator->buffers = NULL;
  allocator->blocks = NULL;
}

int main()
{
  allocator_t alloc = allocator_init(16, 4);

  allocator_block_t *blocks[4];
  for (int i = 0; i < 4; ++i) {
    blocks[i] = allocator_alloc(&alloc, 16);
    if (!blocks[i]) {
      printf("Allocation %d failed!\n", i);
      return 1;
    } else {
      printf("Allocated block %d at %p\n", i, (void *) blocks[i]);
    }
  }

  allocator_block_t *extra_block = allocator_alloc(&alloc, 16);
  if (!extra_block) {
    printf("Extra allocation failed!\n");
    return 1;
  }
  printf("Allocated extra block at %p (new buffer)\n", (void *) extra_block);

  for (int i = 0; i < 4; ++i) {
    allocator_free(&alloc, blocks[i]);
    printf("Freed block %d at %p\n", i, (void *) blocks[i]);
  }
  allocator_free(&alloc, extra_block);
  printf("Freed extra block at %p\n", (void *) extra_block);

  allocator_self_free(&alloc);
  printf("Allocator cleaned up successfully.\n");

  return EXIT_SUCCESS;
}
