#ifndef EXCALIBUR_ENTITY_H
#define EXCALIBUR_ENTITY_H

#define INVALID_POS make_vec3(100000.0f)

inline b32
is_entity_flag_set(sim_entity_t *entity, u32 flag) {
    b32 result = entity->flags & flag;
    return(result);
}

inline void
add_entity_flags(sim_entity_t *entity, u32 flag) {
    entity->flags |= flag;
}

inline void
clear_entity_flags(sim_entity_t *entity, u32 flag) {
    entity->flags &= ~(flag);
}

inline void
make_entity_non_spatial(sim_entity_t *entity) {
    add_entity_flags(entity, ENTITY_FLAG_NON_SPATIAL);
    entity->pos = INVALID_POS;
}

inline void
make_entity_spatial(sim_entity_t *entity, vec3 pos, vec3 d_pos) {
    clear_entity_flags(entity, ENTITY_FLAG_NON_SPATIAL);
    entity->pos = pos;
    entity->d_pos = d_pos;
}

internal vec3
get_entity_ground_point(sim_entity_t *entity) {
    vec3 result = entity->pos;
    return(result);
}

inline f32
get_stair_ground(sim_entity_t *entity, vec3 at_ground_point) {
    ASSERT(entity->type == ENTITY_TYPE_STAIRWELL);
    rect2 region_rect = make_rect2_center_dim(entity->pos.xy, entity->walkable_dim);
    vec2 bary = clamp01(get_barycentric(region_rect, at_ground_point.xy));
    f32 result = entity->pos.z + bary.y*entity->walkable_height;
    return(result);
}

#endif // EXCALIBUR_ENTITY_H
