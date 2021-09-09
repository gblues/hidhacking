
#include <malloc.h>
#include <unistd.h>
#include <string.h>

void init_cachealigned_buffer(uint32_t min_size, void **out_buf_ptr, uint32_t *actual_size);
void *alloc_cachealigned_zeroed(size_t requested_size, size_t *actual_size);