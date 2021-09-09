#include "hidhacking.h"

#define CACHE_ALIGNMENT 64

static void *alloc_zeroed(size_t alignment, size_t size)
{
   void *result = memalign(alignment, size);
   if (result)
      memset(result, 0, size);

   return result;
}

void *alloc_cachealigned_zeroed(size_t requested_size, size_t *actual_size) {
   size_t size = (requested_size + 0x3f) & ~0x3f;

   if(actual_size) {
      *actual_size = size;
   }
   
   return alloc_zeroed(CACHE_ALIGNMENT, size);
}

void init_cachealigned_buffer(uint32_t min_size, void **out_buf_ptr, uint32_t *actual_size)
{
   *actual_size = (min_size + 0x3f) & ~0x3f;

   *out_buf_ptr = alloc_zeroed(CACHE_ALIGNMENT, *actual_size);
}
