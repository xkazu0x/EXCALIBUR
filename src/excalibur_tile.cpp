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

inline tile_chunk_t *
get_tile_chunk(tile_map_t *tile_map, u32 tile_chunk_x, u32 tile_chunk_y, u32 tile_chunk_z) {
    tile_chunk_t *result = 0;
    if ((tile_chunk_x < tile_map->tile_chunk_count_x) &&
        (tile_chunk_y < tile_map->tile_chunk_count_y) &&
        (tile_chunk_z < tile_map->tile_chunk_count_z)) {
        result = &tile_map->tile_chunks[tile_chunk_z*tile_map->tile_chunk_count_y*tile_map->tile_chunk_count_x +
                                        tile_chunk_y*tile_map->tile_chunk_count_x +
                                        tile_chunk_x];
    }
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
    tile_chunk_t *tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);
    
    EX_ASSERT(tile_chunk);
    if (!tile_chunk->tiles) {
        u32 tile_count = tile_map->chunk_dim*tile_map->chunk_dim;
        tile_chunk->tiles = (u32 *)memory_arena_push(arena, tile_count*sizeof(u32));
        for (u32 tile_index = 0; tile_index < tile_count; tile_index++) {
            tile_chunk->tiles[tile_index] = 1;
        }
    }
    
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
