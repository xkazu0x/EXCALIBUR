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

#endif // EXCALIBUR_ENTITY_H
