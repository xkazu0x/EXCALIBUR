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

struct Playing_Sound_List {
    Sound_ID id;
    f32 volume[2];
    s32 samples_played;
    Playing_Sound_List *next;
};

struct Game_State {
    b32 is_initialized;
    
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

    Playing_Sound_List *first_playing_sound;
    Playing_Sound_List *first_free_playing_sound;
};

struct Memory_Task {
    b32 is_being_used;
    Arena arena;
    Temp_Memory memory_flush;
};

struct Transient_State {
    b32 is_initialized;
    Arena arena;
    
    OS_Work_Queue *high_priority_queue;
    OS_Work_Queue *low_priority_queue;
    Memory_Task tasks[4];
    
    Asset_Manager *asset_manager;
    
    u32 ground_buffer_count;
    Ground_Buffer *ground_buffers;
    
    u32 env_map_width;
    u32 env_map_height;
    // NOTE(xkazu0x): 0 is bottom, 1 is middle, 2 is top
    Environment_Map env_maps[3];
};

internal Low_Entity *
get_low_entity(Game_State *game_state, u32 index) {
    Low_Entity *result = 0;
    if ((index > 0) && (index < game_state->low_entity_count)) {
        result = game_state->low_entities + index;
    }
    return(result);
}

internal Memory_Task *
begin_memory_task(Transient_State *tran_state) {
    Memory_Task *result = 0;

    for (u32 task_index = 0;
         task_index < array_count(tran_state->tasks);
         ++task_index) {
        Memory_Task *task = tran_state->tasks + task_index;
        if (!task->is_being_used) {
            result = task;
            task->is_being_used = true;
            task->memory_flush = begin_temp_memory(&task->arena);
            break;
        }
    }

    return(result);
}

inline void
end_memory_task(Memory_Task *task) {
    end_temp_memory(task->memory_flush);
    complete_previous_write_before_future_write;
    task->is_being_used = false;
}

////////////////////////////////
// NOTE(xkazu0x): Global variables from operating system layer

#if EXCALIBUR_INTERNAL
OS_Memory *debug_global_memory;
#endif

global Debug_OS_Read_File *debug_os_read_file;
global OS_Work_Queue_Add_Entry *os_work_queue_add_entry;
global OS_Work_Queue_Complete *os_work_queue_complete;

#endif // EXCALIBUR_GAME_H
