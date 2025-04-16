#define TILE_CHUNK_SAFE_MARGIN 256

inline tile_chunk_t *
get_tile_chunk(tile_map_t *tile_map, u32 tile_chunk_x, u32 tile_chunk_y, u32 tile_chunk_z,
               memory_arena_t *arena = 0) {
    EX_ASSERT(tile_chunk_x > TILE_CHUNK_SAFE_MARGIN);
    EX_ASSERT(tile_chunk_y > TILE_CHUNK_SAFE_MARGIN);
    EX_ASSERT(tile_chunk_z > TILE_CHUNK_SAFE_MARGIN);

    EX_ASSERT(tile_chunk_x < (u32_max - TILE_CHUNK_SAFE_MARGIN));
    EX_ASSERT(tile_chunk_y < (u32_max - TILE_CHUNK_SAFE_MARGIN));
    EX_ASSERT(tile_chunk_z < (u32_max - TILE_CHUNK_SAFE_MARGIN));
    
    // TODO(xkazu0x): BETTER HASH FUNCTION!!!!
    u32 hash_value = 19*tile_chunk_x + 7*tile_chunk_y + 3*tile_chunk_z;
    u32 hash_slot = hash_value & (EX_ARRAY_COUNT(tile_map->tile_chunk_hash) - 1);
    EX_ASSERT(hash_slot < EX_ARRAY_COUNT(tile_map->tile_chunk_hash));
    
    tile_chunk_t *chunk = tile_map->tile_chunk_hash + hash_slot;
    do {
        if ((tile_chunk_x == chunk->tile_chunk_x) &&
            (tile_chunk_y == chunk->tile_chunk_y) &&
            (tile_chunk_z == chunk->tile_chunk_z)) {
            break;
        }

        if (arena && (chunk->tile_chunk_x != 0) && (!chunk->next_in_hash)) {
            chunk->next_in_hash = (tile_chunk_t *)memory_arena_push(arena, sizeof(tile_chunk_t));
            chunk->tile_chunk_x = 0;
            chunk = chunk->next_in_hash;
        }

        if (arena && (chunk->tile_chunk_x == 0)) {
            u32 tile_count = tile_map->chunk_dim*tile_map->chunk_dim;

            chunk->tile_chunk_x = tile_chunk_x;
            chunk->tile_chunk_y = tile_chunk_y;
            chunk->tile_chunk_z = tile_chunk_z;
            
            chunk->tiles = (u32 *)memory_arena_push(arena, tile_count*sizeof(u32));
            // TODO(xkazu0x): do we want to always initialize?
            for (u32 tile_index = 0; tile_index < tile_count; tile_index++) {
                chunk->tiles[tile_index] = 1;
            }

            chunk->next_in_hash = 0;
            break;
        }
        
        chunk = chunk->next_in_hash;
    } while(chunk);
    return(chunk);
}

inline tile_chunk_position_t
get_tile_chunk_position(tile_map_t *tile_map, u32 tile_x, u32 tile_y, u32 tile_z) {
    tile_chunk_position_t result;
    result.tile_chunk_x = tile_x >> tile_map->chunk_shift;
    result.tile_chunk_y = tile_y >> tile_map->chunk_shift;
    result.tile_chunk_z = tile_z;
    result.tile_x = tile_x & tile_map->chunk_mask;
    result.tile_y = tile_y & tile_map->chunk_mask;
    return(result);
}

inline u32
get_tile_value_unchecked(tile_map_t *tile_map, tile_chunk_t *tile_chunk, u32 tile_x, u32 tile_y) {
    EX_ASSERT(tile_chunk);
    EX_ASSERT(tile_x < tile_map->chunk_dim);
    EX_ASSERT(tile_y < tile_map->chunk_dim);
    
    u32 result = tile_chunk->tiles[tile_y*tile_map->chunk_dim + tile_x];
    return(result);
}

inline b32
get_tile_chunk_tile_value(tile_map_t *tile_map, tile_chunk_t *tile_chunk, u32 tile_x, u32 tile_y) {
    u32 result = 0;
    if (tile_chunk && tile_chunk->tiles) {    
        result = get_tile_value_unchecked(tile_map, tile_chunk, tile_x, tile_y);
    }
    return(result);
}

internal b32
get_tile_map_tile_value(tile_map_t *tile_map, u32 tile_x, u32 tile_y, u32 tile_z) {
    tile_chunk_position_t chunk_pos = get_tile_chunk_position(tile_map, tile_x, tile_y, tile_z);
    tile_chunk_t *tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);
    u32 result = get_tile_chunk_tile_value(tile_map, tile_chunk, chunk_pos.tile_x, chunk_pos.tile_y);
    return(result);
}

internal b32
get_tile_map_tile_value(tile_map_t *tile_map, tile_map_position_t tile_map_pos) {
    u32 result = get_tile_map_tile_value(tile_map, tile_map_pos.tile_x, tile_map_pos.tile_y, tile_map_pos.tile_z);
    return(result);
}

inline void
set_tile_value_unchecked(tile_map_t *tile_map, tile_chunk_t *tile_chunk, u32 tile_x, u32 tile_y, u32 tile_value) {
    EX_ASSERT(tile_chunk);
    EX_ASSERT(tile_x < tile_map->chunk_dim);
    EX_ASSERT(tile_y < tile_map->chunk_dim);
    
    tile_chunk->tiles[tile_y*tile_map->chunk_dim + tile_x] = tile_value;
}

inline void
set_tile_chunk_tile_value(tile_map_t *tile_map, tile_chunk_t *tile_chunk, u32 tile_x, u32 tile_y, u32 tile_value) {
    if (tile_chunk) {
        set_tile_value_unchecked(tile_map, tile_chunk, tile_x, tile_y, tile_value);
    }
}

internal void
set_tile_map_tile_value(memory_arena_t *arena, tile_map_t *tile_map,
                        u32 tile_x, u32 tile_y, u32 tile_z,
                        u32 tile_value) {
    tile_chunk_position_t chunk_pos = get_tile_chunk_position(tile_map, tile_x, tile_y, tile_z);
    tile_chunk_t *tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z, arena);
    set_tile_chunk_tile_value(tile_map, tile_chunk, chunk_pos.tile_x, chunk_pos.tile_y, tile_value);
}

internal b32
is_tile_value_empty(u32 tile_value) {
    b32 result = ((tile_value == 1) ||
                  (tile_value == 3) ||
                  (tile_value == 4));
    return(result);
}

internal b32
is_tile_map_point_empty(tile_map_t *tile_map, tile_map_position_t tile_map_pos) {
    u32 tile_value = get_tile_map_tile_value(tile_map, tile_map_pos.tile_x, tile_map_pos.tile_y, tile_map_pos.tile_z);
    b32 result = is_tile_value_empty(tile_value);
    return(result);
}

internal void
tile_map_initialize(tile_map_t *tile_map, f32 tile_size_in_meters) {
    tile_map->chunk_shift = 4;
    tile_map->chunk_mask = (1 << tile_map->chunk_shift) - 1;
    tile_map->chunk_dim = (1 << tile_map->chunk_shift);
    tile_map->tile_size_in_meters = tile_size_in_meters;

    for (u32 tile_chunk_index = 0;
         tile_chunk_index < EX_ARRAY_COUNT(tile_map->tile_chunk_hash);
         tile_chunk_index++) {
        tile_map->tile_chunk_hash[tile_chunk_index].tile_chunk_x = 0;;
    }
}

/////////////////
// TODO(xkazu0x):

inline void
recanonicalize_coord(tile_map_t *tile_map, u32 *tile_index, f32 *tile_rel) {
    s32 offset = round_f32_to_s32(*tile_rel / tile_map->tile_size_in_meters);
    *tile_index += offset;
    *tile_rel -= offset*tile_map->tile_size_in_meters;

    EX_ASSERT(*tile_rel > -0.5f*tile_map->tile_size_in_meters);
    EX_ASSERT(*tile_rel < 0.5f*tile_map->tile_size_in_meters);
}

inline tile_map_position_t
map_into_tile_space(tile_map_t *tile_map, tile_map_position_t base_pos, vec2f offset) {
    tile_map_position_t result = base_pos;
    result.tile_offset_ += offset;
    recanonicalize_coord(tile_map, &result.tile_x, &result.tile_offset_.x);
    recanonicalize_coord(tile_map, &result.tile_y, &result.tile_offset_.y);
    return(result);
}

internal b32
are_on_the_same_tile(tile_map_position_t *a, tile_map_position_t *b) {
    b32 result = ((a->tile_x == b->tile_x) &&
                  (a->tile_y == b->tile_y) &&
                  (a->tile_z == b->tile_z));
    return(result);
}

inline vec3f
subtract(tile_map_t *tile_map, tile_map_position_t *a, tile_map_position_t *b) {
    vec3f result = {};

    f32 tile_dx = (f32)a->tile_x - (f32)b->tile_x;
    f32 tile_dy = (f32)a->tile_y - (f32)b->tile_y;
    f32 tile_dz = (f32)a->tile_z - (f32)b->tile_z;
    
    result.x = tile_map->tile_size_in_meters*tile_dx + (a->tile_offset_.x - b->tile_offset_.x);
    result.y = tile_map->tile_size_in_meters*tile_dy + (a->tile_offset_.y - b->tile_offset_.y);
    result.z = tile_map->tile_size_in_meters*tile_dz;

    return(result);
}

inline tile_map_position_t
centered_tile_point(u32 tile_x, u32 tile_y, u32 tile_z) {
    tile_map_position_t result = {};
    result.tile_x = tile_x;
    result.tile_y = tile_y;
    result.tile_z = tile_z;
    return(result);
}
