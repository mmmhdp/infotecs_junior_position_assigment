#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#define TRUE 1
#define FALSE 0

#ifndef ALLOCATOR_15_BLOCK_PER_BUFFER
  #define ALLOCATOR_15_BLOCK_PER_BUFFER 4
#endif

#ifndef ALLOCATOR_180_BLOCK_PER_BUFFER
  #define ALLOCATOR_180_BLOCK_PER_BUFFER 1
#endif

#if !defined(NAIVE_BOOTSTRAP) && !defined(POSIX_BOOTSTRAP)
#if defined(__unix__) || defined(__APPLE__)
#define POSIX_BOOTSTRAP
#else
#define NAIVE_BOOTSTRAP
#endif
#endif

#if defined(NAIVE_BOOTSTRAP) && defined(POSIX_BOOTSTRAP)
#error "Define only one of NAIVE_BOOTSTRAP or POSIX_BOOTSTRAP"
#endif

#if !defined(NAIVE_BOOTSTRAP) && !defined(POSIX_BOOTSTRAP)
#error \
    "Seems like you have to provide a boostrap-family functions implementation by yourself"
#endif

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

/**
 * \internal
 * @brief Allocates raw memory for the allocator backend.
 *
 * This function provides the underlying memory for the custom allocator.
 * It may use `malloc` (NAIVE_BOOTSTRAP) or `mmap` (POSIX_BOOTSTRAP),
 * depending on compile-time configuration.
 *
 * @param size The number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
static void *bootstrap_allocator(size_t size);

/**
 * \internal
 * @brief Frees memory previously allocated by bootstrap_allocator.
 *
 * Corresponds to the underlying memory allocation method used:
 * - `free` for NAIVE_BOOTSTRAP
 * - `munmap` for POSIX_BOOTSTRAP
 *
 * @param ptr Pointer to memory to free.
 * @param size Size of the memory block.
 */
static void bootstrap_free(void *ptr, size_t size);

/**
 * \internal
 * @brief Initializes an allocator structure.
 *
 * Sets up sizes and counters for blocks and buffers, but does not allocate
 * memory yet.
 *
 * @param allocator_block_size Size of each block in bytes.
 * @param blocks_per_buffer Number of blocks per buffer.
 * @return Initialized allocator_t structure.
 */
static allocator_t allocator_init(size_t allocator_block_size,
                                  size_t blocks_per_buffer);

/**
 * \internal
 * @brief Allocates a new buffer and splits it into blocks for the allocator.
 *
 * Adds all blocks from the new buffer to the free list of the allocator.
 *
 * @param allocator Pointer to allocator structure.
 * @return TRUE on success, FALSE on allocation failure.
 */
static int allocator_alloc_buffer(allocator_t *allocator);

/**
 * \internal
 * @brief Allocates a block of memory from the allocator.
 *
 * If no free blocks exist, a new buffer is allocated automatically.
 *
 * @param allocator Pointer to allocator structure.
 * @param size Requested block size in bytes (must be <= allocator_block_size).
 * @return Pointer to allocated block, or NULL on failure.
 */
static allocator_block_t *allocator_alloc(allocator_t *allocator, size_t size);

/**
 * \internal
 * @brief Returns a block to the allocator's free list.
 *
 * Checks that the block belongs to this allocator before freeing.
 *
 * @param allocator Pointer to allocator structure.
 * @param allocator_block Block to free.
 * @return TRUE if the block belongs to this allocator and was freed, FALSE
 * otherwise.
 */
static int allocator_free(allocator_t *allocator,
                          allocator_block_t *allocator_block);

/**
 * \internal
 * @brief Frees all buffers and resets the allocator.
 *
 * Cleans up all memory used by this allocator, including buffers and internal
 * bookkeeping.
 *
 * @param allocator Pointer to allocator structure.
 */
static void allocator_self_free(allocator_t *allocator);

/**
 * \internal
 * @brief Initializes the global allocators (15 and 180 byte blocks).
 *
 * Allocators are created once on first call.
 *
 * @return TRUE if initialization succeeded or was already done, FALSE on
 * failure.
 */
static int my_malloc_prep_allocators(void);

/**
 * \internal
 * @brief Allocates memory of size 15 or 180 bytes.
 *
 * Uses the corresponding custom allocator. Returns NULL if unsupported size is
 * requested.
 *
 * @param size Number of bytes to allocate (must be 15 or 180).
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *my_malloc(size_t size);

/**
 * @brief Frees memory previously allocated by my_malloc.
 *
 * Checks both global allocators. Aborts if pointer does not belong to either
 * allocator.
 *
 * @param ptr Pointer to memory to free. If NULL, does nothing.
 */
void my_free(void *ptr);

static void *bootstrap_allocator(size_t size)
{
  void *ptr;

#ifdef NAIVE_BOOTSTRAP
  ptr = malloc(size);
  if (!ptr)
    return NULL;
#endif

#ifdef POSIX_BOOTSTRAP
  ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
             -1, 0);

  if (ptr == MAP_FAILED)
    return NULL;
#endif

  return ptr;
}

static void bootstrap_free(void *ptr, size_t size)
{
#ifdef NAIVE_BOOTSTRAP
  (void) size;
  free(ptr);
#endif

#ifdef POSIX_BOOTSTRAP
  munmap(ptr, size);
#endif
}

static allocator_t allocator_init(size_t allocator_block_size,
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

static int allocator_alloc_buffer(allocator_t *allocator)
{
  uint8_t *buffer = bootstrap_allocator(allocator->allocator_buffer_size);
  if (!buffer)
    return FALSE;

  allocator_buffer_t *allocator_buffer =
      bootstrap_allocator(sizeof(allocator_buffer_t));
  if (!allocator_buffer) {
    free(buffer);
    return FALSE;
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

  return TRUE;
}

static allocator_block_t *allocator_alloc(allocator_t *allocator, size_t size)
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

static int allocator_free(allocator_t *allocator,
                          allocator_block_t *allocator_block)
{
  int is_block_belongs_to_allocator = FALSE;

  if (!allocator_block) {
    return is_block_belongs_to_allocator;
  }

#ifdef ALLOCATOR_DOUBLE_FREE_AWARE
  int is_already_freed = FALSE;
  for (allocator_block_t *all_block = allocator->blocks; all_block;
       all_block = all_block->next) {
    if (all_block == allocator_block) {
      is_already_freed = TRUE;
      break;
    }
  }

  if (is_already_freed) {
    abort();
  }
#endif

  // better to say it's just UB
  // but design choice with several allocators
  // force to have such a procedure to
  // determinate correct free call
  uintptr_t allocator_block_ptr = (uintptr_t) allocator_block;

  for (allocator_buffer_t *allocator_buffer = allocator->buffers;
       allocator_buffer; allocator_buffer = allocator_buffer->next) {
    uintptr_t buffer_begin_ptr = (uintptr_t) allocator_buffer->buffer;
    uintptr_t buffer_end_ptr = (uintptr_t) allocator_buffer->buffer_end;

    if (allocator_block_ptr >= buffer_begin_ptr &&
        allocator_block_ptr < buffer_end_ptr) {
      size_t offset = allocator_block_ptr - buffer_begin_ptr;
      if (offset % allocator->allocator_block_size == 0) {
        is_block_belongs_to_allocator = TRUE;
      }
      break;
    }
  }

  if (!is_block_belongs_to_allocator) {
    return is_block_belongs_to_allocator;
  }

  allocator_block->next = allocator->blocks;
  allocator->blocks = allocator_block;

  return is_block_belongs_to_allocator;
}

static void allocator_self_free(allocator_t *allocator)
{
  allocator_buffer_t *allocator_buffer = allocator->buffers;
  while (allocator_buffer) {
    allocator_buffer_t *next_allocator_buffer = allocator_buffer->next;

    bootstrap_free(allocator_buffer->buffer, allocator->allocator_buffer_size);
    bootstrap_free(allocator_buffer, sizeof(allocator_buffer_t));

    allocator_buffer = next_allocator_buffer;
  }

  allocator->buffers = NULL;
  allocator->blocks = NULL;
}

allocator_t allocator_15;
allocator_t allocator_180;
int is_allocators_initialized = FALSE;

static int my_malloc_prep_allocators()
{
  if (is_allocators_initialized)
    return is_allocators_initialized;

  allocator_15 = allocator_init(15, ALLOCATOR_15_BLOCK_PER_BUFFER);
  allocator_180 = allocator_init(180, ALLOCATOR_180_BLOCK_PER_BUFFER);
  is_allocators_initialized = TRUE;

  return is_allocators_initialized;
}

void *my_malloc(size_t size)
{
  // part of the requirements for
  // custom implementation
  if (size != 15 && size != 180)
    return NULL;

  if (!is_allocators_initialized)
    my_malloc_prep_allocators();

  void *mem;
  if (size == 15)
    mem = allocator_alloc(&allocator_15, size);
  else
    mem = allocator_alloc(&allocator_180, size);

  return mem;
}

void my_free(void *ptr)
{
  if (!ptr)
    return;

  if (!allocator_free(&allocator_15, ptr) &&
      !allocator_free(&allocator_180, ptr))
    abort();
}
