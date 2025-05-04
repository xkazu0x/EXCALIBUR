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
    vec3 result = entity->pos + make_vec3(0.0f, 0.0f, -0.5f*entity->dim.z);
    return(result);
}

inline f32
get_stair_ground(sim_entity_t *entity, vec3 at_ground_point) {
    ASSERT(entity->type == ENTITY_TYPE_STAIRWELL);
    rect3 region_rect = make_rect3_center_dim(entity->pos, entity->dim);
    vec3 bary = clamp01(get_barycentric(region_rect, at_ground_point));
    f32 result = region_rect.min.z + bary.y*entity->walkable_height;
    return(result);
}

#endif // EXCALIBUR_ENTITY_H
