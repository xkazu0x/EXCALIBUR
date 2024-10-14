#include "sxmemory.h"
#include "sxlogger.h"

#include <memory>
#include <string.h>
#include <stdio.h>

struct memory_stats {
	uint64_t total_allocated;
	uint64_t tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char *memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
	"UNKNOWN      ",
	"ARRAY        ",
	"DYNAMIC ARRAY",
	"PLATFORM     ",
	"RENDERER     "
};

static struct memory_stats stats;

void sxmemory::initialize() {
	memset(&stats, 0, sizeof(memory_stats));
}

void sxmemory::shutdown() {
}

void *sxmemory::allocate(uint64_t size, memory_tag tag) {
	if (tag == MEMORY_TAG_UNKNOWN) {
		SXWARN("SXMEMORY ALLOCATE CALLED USING MEMORY_TAG_UNKNOWN");
	}
	stats.total_allocated += size;
	stats.tagged_allocations[tag] += size;
	// TODO: memory aligment
	void *block = malloc(size);
	memset(block, 0, sizeof(size));
	return block;
}

void sxmemory::free_memory(void *block, uint64_t size, memory_tag tag) {
	if (tag == MEMORY_TAG_UNKNOWN) {
		SXWARN("SXMEMORY FREE CALLED USING MEMORY_TAG_UNKNOWN");
	}
	stats.total_allocated -= size;
	stats.tagged_allocations[tag] -= size;
	// TODO: memory aligment
	free(block);
}

void *sxmemory::zero_memory(void *block, uint64_t size) {
	return memset(&block, 0, sizeof(size));
}

void *sxmemory::copy_memory(void *dest, const void *source, uint64_t size) {
	return memcpy(dest, source, size);
}

void *sxmemory::set_memory(void *dest, int32_t value, uint64_t size) {
	return memset(dest, value, size);
}

char *sxmemory::get_memory_usage_str() {
	const uint64_t kib = 1024;
	const uint64_t mib = 1024 * kib;
	const uint64_t gib = 1024 * mib;

	char buffer[1024] = "SYSTEM MEMORY USE (TAGGED):\n";
	uint64_t offset = strlen(buffer);
	for (uint32_t i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
		char unit[4] = "XiB";
		float amount = 1.0f;
		if (stats.tagged_allocations[i] >= gib) {
			unit[0] = 'G';
			amount = stats.tagged_allocations[i] / static_cast<float>(gib);
		} else if (stats.tagged_allocations[i] >= mib) {
			unit[0] = 'M';
			amount = stats.tagged_allocations[i] / static_cast<float>(mib);
		} else if (stats.tagged_allocations[i] >= kib) {
			unit[0] = 'K';
			amount = stats.tagged_allocations[i] / static_cast<float>(kib);
		} else {
			unit[0] = 'B';
			unit[1] = 0;
			amount = static_cast<float>(stats.tagged_allocations[i]);
		}
		offset += snprintf(buffer + offset, 1024, "  %s\t: %.2f%s\n", memory_tag_strings[i], amount, unit);
	}
	char *out_string = _strdup(buffer);
	return out_string;
}