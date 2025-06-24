#ifndef EXCALIBUR_ASSET_H
#define EXCALIBUR_ASSET_H

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

enum Asset_Tag_ID {
    AssetTag_Smooth,
    AssetTag_Flat,

    AssetTag_Count,
};

struct Asset_Tag {
    u32 id;
    f32 value;
};

enum Asset_Type_ID {
    AssetType_None,
    
    AssetType_Wall,
    AssetType_Stairwell,
    AssetType_Shadow,
    AssetType_Bat,
    AssetType_Sword,

    AssetType_Grass,
    AssetType_Stone,
    AssetType_Flower,
    
    AssetType_Count,
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

struct Asset_Bitmap_Info {
    char *filename;
    Vec2 align_percentage;
};

struct Asset_Group {
    u32 first_tag_index;
    u32 one_past_last_tag_index;
};

struct Game_Assets {
    Arena arena;
    struct Transient_State *tran_state;
    
    u32 bitmap_count;
    Asset_Slot *bitmaps;
    Asset_Bitmap_Info *bitmap_infos;

    u32 sound_count;
    Asset_Slot *sounds;

    u32 tag_count;
    Asset_Tag *tags;

    u32 asset_count;
    Asset *assets;
    
    Asset_Type asset_types[AssetType_Count];

    // NOTE(xkazu0x): Array'd assets
    // TODO(xkazu0x): Structured assets
    Bitmap player[4];
    
    // TODO(xkazu0x): these should go away once we actually load a asset pack file
    u32 debug_used_bitmap_count;
    u32 debug_used_asset_count;
    Asset_Type *debug_asset_type;
};

struct Bitmap_ID {
    u32 value;
};

struct Sound_ID {
    u32 value;
};

internal Bitmap *get_bitmap(Game_Assets *assets, Bitmap_ID id);

internal void load_bitmap(Game_Assets *assets, Bitmap_ID id);
internal void load_sound(Game_Assets *assets, Sound_ID id);

#endif // EXCALIBUR_ASSET_H
