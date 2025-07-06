#ifndef EXCALIBUR_MEMORY
#define EXCALIBUR_MEMORY

struct Arena {
    memi reserve_size;
    memi used;
    u8 *base_memory;

    s32 temp_count;
};

internal Arena
make_arena(memi reserve_size, void *base_memory) {
    Arena result;
    result.reserve_size = reserve_size;
    result.used = 0;
    result.base_memory = (u8 *)base_memory;
    result.temp_count = 0;
    return(result);
}

internal memi
get_alignment_offset(Arena *arena, memi alignment) {
    memi result = 0;

    memi arena_ptr = (memi)arena->base_memory + arena->used;
    memi alignment_mask = alignment - 1;

    if (arena_ptr & alignment_mask) {
        result = alignment - (arena_ptr & alignment_mask);
    }
    
    return(result);
}

internal memi
get_arena_size_remaining(Arena *arena, memi alignment = 4) {
    memi result = arena->reserve_size - (arena->used + get_alignment_offset(arena, alignment));
    return(result);
}

internal void *
arena_push_(Arena *arena, memi size_init, memi alignment = 4) {
    memi size = size_init;
    
    memi alignment_offset = get_alignment_offset(arena, alignment);
    size += alignment_offset;
    
    assert((arena->used + size) <= arena->reserve_size);
    void *result = (void *)(arena->base_memory + arena->used + alignment_offset);
    arena->used += size;

    assert(size >= size_init);
    
    return(result);
}

#define push_struct(arena, type, ...) (type *)arena_push_(arena, sizeof(type), ##__VA_ARGS__)
#define push_array(arena, type, count, ...) (type *)arena_push_(arena, sizeof(type)*(count), ##__VA_ARGS__)
#define push_size(arena, size, ...) arena_push_(arena, size, ##__VA_ARGS__)

internal Arena
make_sub_arena(Arena *arena, memi reserve_size, memi alignment = 16) {
    Arena result;
    result.reserve_size = reserve_size;
    result.used = 0;
    result.base_memory = (u8 *)push_size(arena, reserve_size, alignment);
    result.temp_count = 0;
    return(result);
}

struct Temp_Memory {
    Arena *arena;
    memi used;
};

internal Temp_Memory
begin_temp_memory(Arena *arena) {
    Temp_Memory result;
    result.arena = arena;
    result.used = arena->used;
    ++arena->temp_count;
    return(result);
}

internal void
end_temp_memory(Temp_Memory temp) {
    Arena *arena = temp.arena;
    
    assert(arena->used >= temp.used);
    arena->used = temp.used;

    assert(arena->temp_count > 0);
    --arena->temp_count;
}

internal void
check_arena(Arena *arena) {
    assert(arena->temp_count == 0);
}

internal inline void
zero_size(memi size, void *ptr) {
    u8 *byte = (u8 *)ptr;
    while (size--) {
        *byte++ = 0;
    }
}
#define zero_struct(instance) zero_size(sizeof(instance), &(instance))

#endif // EXCALIBUR_MEMORY
