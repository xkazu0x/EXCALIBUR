#pragma once

#include "stdint.h"

enum memory_tag {
	MEMORY_TAG_UNKNOWN,
	MEMORY_TAG_ARRAY,
	MEMORY_TAG_DYNAMIC_ARRAY,
	MEMORY_TAG_PLATFORM,
	MEMORY_TAG_RENDERER,
	MEMORY_TAG_MAX_TAGS
};

namespace sxmemory {
	void initialize();
	void shutdown();
	void *allocate(uint64_t size, memory_tag tag);
	void free_memory(void *block, uint64_t size, memory_tag tag);
	void *zero_memory(void *block, uint64_t size);
	void *copy_memory(void *dest, const void *source, uint64_t size);
	void *set_memory(void *dest, int32_t value, uint64_t size);
	char *get_memory_usage_str();
}