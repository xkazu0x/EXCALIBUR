#ifndef EXCALIBUR_GAME_H
#define EXCALIBUR_GAME_H

// NOTE(xkazu0x):
// EXCALIBUR_INTERNAL:
// > 0 - build for public release
// > 1 - build for develop only
// EXCALIBUR_DEBUG:
// > 0 - not slow code allowed
// > 1 - slow code welcome

struct Low_Entity {
    // TODO(xkazu0x): it's kind of busted that pos's can be invalid here,
    // AND we store whether they would be invalid in the flags field...
    // can we do something better here?
    World_Position pos;
    Sim_Entity sim;
};

struct Controlled_Player {
    u32 entity_index;
    // NOTE(xkazu0x): these are the gamepad requests for simulation
    Vec2 dd_pos;
    Vec2 d_sword;
    f32 d_z;
};

struct Pairwise_Collision_Rule {
    b32 can_collide;
    u32 storage_index_a;
    u32 storage_index_b;

    Pairwise_Collision_Rule *next_in_hash;
};

struct Game_State;
internal void add_collision_rule(Game_State *game_state, u32 storage_index_a, u32 storage_index_b, b32 should_collide);
internal void clear_collision_rules_for(Game_State *game_state, u32 storage_index);

struct Ground_Buffer {
    // NOTE(xkazu0x): An invalid position tells us that
    // this Ground_Buffer has not been filled
    World_Position position; // NOTE(xkazu0x): center of the bitmap
    Bitmap bitmap;
};

struct Game_State {
    Arena world_arena;
    World *world;

    f32 typical_floor_height;
    
    u32 camera_following_entity_index;
    World_Position camera_pos;
    
    Controlled_Player controlled_players[Gamepad_Count];
    
    // TODO(xkazu0x): change the name to "stored entity"
    u32 low_entity_count;
    Low_Entity low_entities[100000];
    
    // TODO(xkazu0x): must be power of two
    Pairwise_Collision_Rule *collision_rule_hash[256];
    Pairwise_Collision_Rule *first_free_collision_rule;

    Sim_Entity_Collision_Volume_Group *null_collision;
    Sim_Entity_Collision_Volume_Group *standard_room_collision;
    Sim_Entity_Collision_Volume_Group *wall_collision;
    Sim_Entity_Collision_Volume_Group *stair_collision;
    Sim_Entity_Collision_Volume_Group *sword_collision;
    Sim_Entity_Collision_Volume_Group *player_collision;
    Sim_Entity_Collision_Volume_Group *monster_collision;
    Sim_Entity_Collision_Volume_Group *familiar_collision;

    f32 time;

    Bitmap test_diffuse;
    Bitmap test_normal;
};

struct Memory_Task {
    b32 is_being_used;
    Arena arena;
    Temp_Memory memory_flush;
};

enum Asset_State {
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked,
};

struct Asset_Slot {
    Asset_State state;
    Bitmap *bitmap;
};

enum Game_Asset_ID {
    GAI_Wall,
    GAI_Stairwell,
    GAI_Shadow,
    GAI_Bat,
    GAI_Sword,
    
    GAI_Count,
};

struct Asset_Tag {
    u32 id;
    f32 value;
};

struct Asset_Bitmap_Info {
    Vec2 align_percentage;
    f32 width_over_height;
    s32 width;
    s32 height;

    u32 first_tag_index;
    u32 tag_count;
};

struct Asset_Group {
    u32 first_tag_index;
    u32 tag_count;
};

struct Game_Assets {
    Debug_OS_Read_File *os_read_file;
    
    struct Transient_State *tran_state;
    Arena arena;
    
    Asset_Slot bitmaps[GAI_Count];

    // NOTE(xkazu0x): Array'd assets
    Bitmap grass[2];
    Bitmap stone[2];
    Bitmap tuft[2];
    Bitmap player[4];
    
    // TODO(xkazu0x): Structured assets
};

internal Bitmap *
get_game_asset_bitmap(Game_Assets *assets, Game_Asset_ID id) {
    Bitmap *result = assets->bitmaps[id].bitmap;
    return(result);
}

struct Transient_State {
    b32 initialized;
    Arena arena;

    u32 ground_buffer_count;
    Ground_Buffer *ground_buffers;
    
    OS_Work_Queue *high_priority_queue;
    OS_Work_Queue *low_priority_queue;
    
    u32 env_map_width;
    u32 env_map_height;
    // NOTE(xkazu0x): 0 is bottom, 1 is middle, 2 is top
    Environment_Map env_maps[3];

    Memory_Task tasks[4];

    Game_Assets assets;
};

internal Low_Entity *
get_low_entity(Game_State *game_state, u32 index) {
    Low_Entity *result = 0;
    if ((index > 0) && (index < game_state->low_entity_count)) {
        result = game_state->low_entities + index;
    }
    return(result);
}

internal void load_asset(Game_Assets *assets, Game_Asset_ID id);

#endif // EXCALIBUR_GAME_H
