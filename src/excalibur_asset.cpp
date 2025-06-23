#pragma pack(push, 1)
struct Bitmap_Header {
    u16 file_type;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 bitmap_offset;
    u32 size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 size_of_bitmap;
    s32 horz_resolution;
    s32 vert_resolution;
    u32 colors_used;
    u32 colors_important;

    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
};
#pragma pack(pop)

internal Vec2
top_down_align(Bitmap *bitmap, Vec2 align) {
    //align.y = (f32)(bitmap->height - 1) - align.y;
    align.y = (f32)bitmap->height - align.y;
    
    align.x = safe_ratio0(align.x, (f32)bitmap->width);
    align.y = safe_ratio0(align.y, (f32)bitmap->height);
    
    return(align);
}

internal void
set_bitmap_align(Bitmap *bitmap, Vec2 align) {
    align = top_down_align(bitmap, align);
    bitmap->align_percentage = align;
}

internal Bitmap
debug_load_bitmap(char *filename, s32 align_x, s32 align_y) {
    Bitmap result = {};
    
    Debug_OS_File file = debug_os_read_file(filename);
    if (file.size != 0) {
        Bitmap_Header *header = (Bitmap_Header *)file.data;
        u32 *pixels = (u32 *)((u8 *)file.data + header->bitmap_offset);
        
        result.memory = pixels;
        result.width = header->width;
        result.height = header->height;
        result.align_percentage = top_down_align(&result, make_vec2((f32)align_x, (f32)align_y));
        result.width_over_height = safe_ratio0((f32)result.width, (f32)result.height);
        
        assert(result.height >= 0);
        assert(header->compression == 3);
        
        // NOTE(xkazu0x): byte order in memory is determined bu the header itself,
        // so we have to read out the masks and convert the pixels ourselves.
        u32 red_mask = header->red_mask;
        u32 green_mask = header->green_mask;
        u32 blue_mask = header->blue_mask;
        u32 alpha_mask = ~(red_mask | green_mask | blue_mask);
        
        Bit_Scan red_scan = find_least_significant_set_bit(red_mask);
        Bit_Scan green_scan = find_least_significant_set_bit(green_mask);
        Bit_Scan blue_scan = find_least_significant_set_bit(blue_mask);
        Bit_Scan alpha_scan = find_least_significant_set_bit(alpha_mask);

        assert(red_scan.found);
        assert(green_scan.found);
        assert(blue_scan.found);
        assert(alpha_scan.found);

        s32 red_shift_down   = (s32)red_scan.index;
        s32 green_shift_down = (s32)green_scan.index;
        s32 blue_shift_down  = (s32)blue_scan.index;
        s32 alpha_shift_down = (s32)alpha_scan.index;

        u32 *source_dest = pixels;
        for (s32 y = 0;
             y < header->height;
             ++y) {
            for (s32 x = 0;
                 x < header->width;
                 ++x) {
                u32 color = *source_dest;

                Vec4 texel = make_vec4((f32)((color & red_mask)   >> red_shift_down),
                                       (f32)((color & green_mask) >> green_shift_down),
                                       (f32)((color & blue_mask)  >> blue_shift_down),
                                       (f32)((color & alpha_mask) >> alpha_shift_down));
                
                texel = srgb255_to_linear1(texel);
                texel.rgb *= texel.a;
                texel = linear1_to_srgb255(texel);
                
                *source_dest++ = (((u32)(texel.a + 0.5f) << 24) |
                                  ((u32)(texel.r + 0.5f) << 16) |
                                  ((u32)(texel.g + 0.5f) << 8) |
                                  ((u32)(texel.b + 0.5f) << 0));
            }
        }
    }

    result.pitch = result.width*BYTES_PER_PIXEL;

#if 0
    result.memory = (u8 *)result.memory + result.pitch*(result.height - 1);
    result.pitch = -result.pitch;
#endif
    
    return(result);
}

internal Bitmap
debug_load_bitmap(char *filename) {
    Bitmap result = debug_load_bitmap(filename, 0, 0);
    result.align_percentage = make_vec2(0.5f);
    return(result);
}

internal Game_Assets *
alloc_game_assets(Arena *arena, memi size, Transient_State *tran_state) {
    Game_Assets *result = push_struct(arena, Game_Assets);
    result->arena = make_sub_arena(arena, size);
    result->tran_state = tran_state;

    result->bitmap_count = AssetType_Count;
    result->bitmaps = push_array(arena, Asset_Slot, result->bitmap_count);
    
    result->sound_count = 1;
    result->sounds = push_array(arena, Asset_Slot, result->sound_count);

    result->tag_count = 0;
    result->tags = 0;
    
    result->asset_count = result->bitmap_count;
    result->assets = push_array(arena, Asset, result->asset_count);

    for (u32 asset_type_id = 0;
         asset_type_id < AssetType_Count;
         ++asset_type_id) {
        Asset_Type *type = result->asset_types + asset_type_id;
        type->first_asset_index = asset_type_id;
        type->one_past_last_asset_index = asset_type_id + 1;

        Asset *asset = result->assets + type->first_asset_index;
        asset->first_tag_index = 0;
        asset->one_past_last_tag_index = 0;
        asset->slot_id = type->first_asset_index;
    }
    
    result->grass[0]  = debug_load_bitmap("../res/grass00.bmp");
    result->grass[1]  = debug_load_bitmap("../res/grass01.bmp");
    result->stone[0]  = debug_load_bitmap("../res/stone00.bmp");
    result->stone[1]  = debug_load_bitmap("../res/stone01.bmp");
    result->tuft[0]   = debug_load_bitmap("../res/tuft0.bmp");
    result->tuft[1]   = debug_load_bitmap("../res/tuft1.bmp");
    result->player[0] = debug_load_bitmap("../res/skull_back.bmp");
    result->player[1] = debug_load_bitmap("../res/skull_right.bmp");
    result->player[2] = debug_load_bitmap("../res/skull_front.bmp");
    result->player[3] = debug_load_bitmap("../res/skull_left.bmp");
    
    return(result);
}

internal Bitmap_ID
get_first_bitmap_id(Game_Assets *assets, Asset_Type_ID type_id) {
    Bitmap_ID result = {};
    
    Asset_Type *type = assets->asset_types + type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        Asset *asset = assets->assets + type->first_asset_index;
        result.value = asset->slot_id;
    }
    
    return(result);
}

internal Bitmap *
get_bitmap(Game_Assets *assets, Bitmap_ID id) {
    Bitmap *result = assets->bitmaps[id.value].bitmap;
    return(result);
}

struct Load_Bitmap_Work {
    Memory_Task *memory_task;
    Game_Assets *assets;

    char *filename;
    Bitmap_ID id;
    Bitmap *bitmap;

    b32 has_alignment;
    s32 align_x;
    s32 align_y;

    Asset_State final_state;
};

internal
OS_WORK_QUEUE_CALLBACK(load_bitmap_work) {
    Load_Bitmap_Work *work = (Load_Bitmap_Work *)data;
    
    if (work->has_alignment) {
        *work->bitmap = debug_load_bitmap(work->filename, work->align_x, work->align_y);
    } else {
        *work->bitmap = debug_load_bitmap(work->filename);
    }

    complete_previous_write_before_future_write;

    work->assets->bitmaps[work->id.value].bitmap = work->bitmap;
    work->assets->bitmaps[work->id.value].state = work->final_state;
    
    end_memory_task(work->memory_task);
}

internal void
load_bitmap(Game_Assets *assets, Bitmap_ID id) {
    if (id.value && (atomic_compare_exchange_u32((u32 *)&assets->bitmaps[id.value].state, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded)) {
        Memory_Task *memory_task = begin_memory_task(assets->tran_state);
        if (memory_task) {
            Load_Bitmap_Work *work = push_struct(&memory_task->arena, Load_Bitmap_Work);
            work->memory_task = memory_task;
            work->assets = assets;
            work->id = id;
            work->filename = "";
            work->bitmap = push_struct(&assets->arena, Bitmap);
            work->has_alignment = false;
            work->final_state = AssetState_Loaded;
            
            switch (id.value) {
                case AssetType_Wall: {
                    work->filename = "../res/wall.bmp";
                } break;

                case AssetType_Stairwell: {
                    work->filename = "../res/stair.bmp";
                } break;

                case AssetType_Shadow: {
                    work->filename = "../res/shadow.bmp";
                } break;

                case AssetType_Bat: {
                    work->filename = "../res/bat.bmp";
                } break;

                case AssetType_Sword: {
                    work->filename = "../res/shadow.bmp";
                } break;
            }

            os_work_queue_add_entry(assets->tran_state->low_priority_queue, load_bitmap_work, work);
        }
    }
}

// TODO(xkazu0x): load_sound();
