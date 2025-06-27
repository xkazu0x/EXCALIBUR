#ifndef EXCALIBUR_ASSET_H
#define EXCALIBUR_ASSET_H

////////////////////////////////
// NOTE(xkazu0x): Asset types

enum Asset_State {
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked,
};

struct Asset_Slot {
    Asset_State state;
    union {
        Bitmap *bitmap;
        Sound *sound;
    };
};

enum Asset_Tag_ID {
    AssetTag_FacingDirection, // NOTE(xkazu0x): angle in radians off of due right

    AssetTag_Count,
};

enum Asset_Type_ID {
    AssetType_Null,
    
    AssetType_Wall,
    AssetType_Stairwell,
    AssetType_Shadow,
    AssetType_Bat,

    AssetType_Grass,
    AssetType_Stone,
    AssetType_Flower,
    
    AssetType_Skull,
    
    AssetType_Count,
};

struct Asset_Tag {
    u32 id;
    f32 value;
};

struct Asset_Type {
    u32 first_asset_index;
    u32 one_past_last_asset_index;
};

struct Asset {
    u32 first_tag_index;
    u32 one_past_last_tag_index;
    u32 slot_id;
};

struct Asset_Vector {
    f32 e[AssetTag_Count];
};

struct Asset_Bitmap_Info {
    char *filename;
    Vec2 align_percentage;
};

struct Asset_Sound_Info {
    char *filename;
};

struct Asset_Manager {
    struct Transient_State *tran_state;
    Arena arena;

    u32 bitmap_count;
    Asset_Bitmap_Info *bitmap_infos;
    Asset_Slot *bitmaps;

    u32 sound_count;
    Asset_Sound_Info *sound_infos;
    Asset_Slot *sounds;

    u32 tag_count;
    Asset_Tag *tags;

    u32 asset_count;
    Asset *assets;
    
    Asset_Type asset_types[AssetType_Count];
    f32 tag_range[AssetTag_Count];
    
    // TODO(xkazu0x): these should go away once we actually load a asset pack file
    u32 debug_used_bitmap_count;
    u32 debug_used_asset_count;
    u32 debug_used_tag_count;
    Asset_Type *debug_asset_type;
    Asset *debug_asset;
};

struct Bitmap_ID {
    u32 value;
};

struct Sound_ID {
    u32 value;
};

#endif // EXCALIBUR_ASSET_H
