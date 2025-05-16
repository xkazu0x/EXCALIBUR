#ifndef EXCALIBUR_ENTITY_H
#define EXCALIBUR_ENTITY_H

#define INVALID_POS make_vec3(100000.0f)

inline b32
is_entity_flag_set(Sim_Entity *entity, u32 flag) {
    b32 result = entity->flags & flag;
    return(result);
}

inline void
add_entity_flags(Sim_Entity *entity, u32 flag) {
    entity->flags |= flag;
}

inline void
clear_entity_flags(Sim_Entity *entity, u32 flag) {
    entity->flags &= ~(flag);
}

inline void
make_entity_non_spatial(Sim_Entity *entity) {
    add_entity_flags(entity, EntityFlag_NonSpatial);
    entity->pos = INVALID_POS;
}

inline void
make_entity_spatial(Sim_Entity *entity, Vec3 pos, Vec3 d_pos) {
    clear_entity_flags(entity, EntityFlag_NonSpatial);
    entity->pos = pos;
    entity->d_pos = d_pos;
}

internal Vec3
get_entity_ground_point(Sim_Entity *entity, Vec3 for_entity_pos) {
    Vec3 result = for_entity_pos;
    return(result);
}

internal Vec3
get_entity_ground_point(Sim_Entity *entity) {
    Vec3 result = get_entity_ground_point(entity, entity->pos);
    return(result);
}

inline f32
get_stair_ground(Sim_Entity *entity, Vec3 at_ground_point) {
    Assert(entity->type == EntityType_Stairwell);
    Rect2 region_rect = make_rect2_center_dim(entity->pos.xy, entity->walkable_dim);
    Vec2 bary = clamp01(get_barycentric(region_rect, at_ground_point.xy));
    f32 result = entity->pos.z + bary.y*entity->walkable_height;
    return(result);
}

#endif // EXCALIBUR_ENTITY_H
