inline void
recanonicalize_coord(tile_map_t *tile_map, u32 *tile_index, f32 *tile_rel) {
    // TODO(xkazu0x): floorf does not fucking work
    s32 offset = round_f32_to_s32(*tile_rel / tile_map->tile_size_in_meters);
    *tile_index += offset;
    *tile_rel -= offset*tile_map->tile_size_in_meters;

    EX_ASSERT(*tile_rel >= -0.5f*tile_map->tile_size_in_meters);
    EX_ASSERT(*tile_rel <= 0.5f*tile_map->tile_size_in_meters);
}

inline tile_map_position_t
recanonicalize_position(tile_map_t *tile_map, tile_map_position_t pos) {
    tile_map_position_t result = pos;
    recanonicalize_coord(tile_map, &result.tile_x, &result.tile_rel_x);
    recanonicalize_coord(tile_map, &result.tile_y, &result.tile_rel_y);
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

inline void
set_tile_value_unchecked(tile_map_t *tile_map, tile_chunk_t *tile_chunk, u32 tile_x, u32 tile_y, u32 tile_value) {
    EX_ASSERT(tile_chunk);
    EX_ASSERT(tile_x < tile_map->chunk_dim);
    EX_ASSERT(tile_y < tile_map->chunk_dim);
    
    tile_chunk->tiles[tile_y*tile_map->chunk_dim + tile_x] = tile_value;
}

inline b32
get_tile_chunk_tile_value(tile_map_t *tile_map, tile_chunk_t *tile_chunk, u32 tile_x, u32 tile_y) {
    u32 result = 0;
    if (tile_chunk) {    
        result = get_tile_value_unchecked(tile_map, tile_chunk, tile_x, tile_y);
    }
    return(result);
}

inline void
set_tile_chunk_tile_value(tile_map_t *tile_map, tile_chunk_t *tile_chunk, u32 tile_x, u32 tile_y, u32 tile_value) {
    u32 result = 0;
    if (tile_chunk) {    
        set_tile_value_unchecked(tile_map, tile_chunk, tile_x, tile_y, tile_value);
    }
}

inline tile_chunk_t *
get_tile_chunk(tile_map_t *tile_map, u32 tile_chunk_x, u32 tile_chunk_y) {
    tile_chunk_t *result = 0;
    if ((tile_chunk_x < tile_map->tile_chunk_count_x) &&
        (tile_chunk_y < tile_map->tile_chunk_count_y)) {
        result = &tile_map->tile_chunks[tile_chunk_y*tile_map->tile_chunk_count_x + tile_chunk_x];
    }
    return(result);
}

inline tile_chunk_position_t
get_tile_chunk_position(tile_map_t *tile_map, u32 abs_tile_x, u32 abs_tile_y) {
    tile_chunk_position_t result;
    result.tile_chunk_x = abs_tile_x >> tile_map->chunk_shift;
    result.tile_chunk_y = abs_tile_y >> tile_map->chunk_shift;
    result.tile_x = abs_tile_x & tile_map->chunk_mask;
    result.tile_y = abs_tile_y & tile_map->chunk_mask;
    return(result);
}

internal b32
get_tile_map_tile_value(tile_map_t *tile_map, u32 tile_x, u32 tile_y) {
    tile_chunk_position_t chunk_pos = get_tile_chunk_position(tile_map, tile_x, tile_y);
    tile_chunk_t *tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);
    u32 result = get_tile_chunk_tile_value(tile_map, tile_chunk, chunk_pos.tile_x, chunk_pos.tile_y);
    return(result);
}

internal b32
is_tile_map_point_empty(tile_map_t *tile_map, tile_map_position_t tile_map_pos) {
    u32 tile_value = get_tile_map_tile_value(tile_map, tile_map_pos.tile_x, tile_map_pos.tile_y);
    b32 result = (tile_value == 0);
    return(result);
}

internal void
set_tile_map_tile_value(memory_arena_t *arena, tile_map_t *tile_map, u32 tile_x, u32 tile_y, u32 tile_value) {
    tile_chunk_position_t chunk_pos = get_tile_chunk_position(tile_map, tile_x, tile_y);
    tile_chunk_t *tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);
    
    // TODO(xkazu0x): on-demand tile chunk creation
    EX_ASSERT(tile_chunk);
    set_tile_chunk_tile_value(tile_map, tile_chunk, chunk_pos.tile_x, chunk_pos.tile_y, tile_value);
}
