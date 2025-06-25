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

#if 0
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
#endif

internal Bitmap
debug_load_bitmap(char *filename, Vec2 align_percentage = make_vec2(0.5f)) {
    Bitmap result = {};
    
    Debug_OS_File file = debug_os_read_file(filename);
    if (file.size != 0) {
        Bitmap_Header *header = (Bitmap_Header *)file.data;
        u32 *pixels = (u32 *)((u8 *)file.data + header->bitmap_offset);
        
        result.memory = pixels;
        result.width = header->width;
        result.height = header->height;
        result.align_percentage = align_percentage;
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

//
//
//

internal Bitmap_ID
debug_asset_add_bitmap_info(Asset_Manager *manager, char *filename, Vec2 align_percentage)  {
    assert(manager->debug_used_bitmap_count < manager->bitmap_count);
    
    Bitmap_ID id = {manager->debug_used_bitmap_count++};
    
    Asset_Bitmap_Info *info = manager->bitmap_infos + id.value;
    info->filename = filename;
    info->align_percentage = align_percentage;
    
    return(id);
}

internal void
begin_asset_type(Asset_Manager *manager, Asset_Type_ID type_id) {
    assert(manager->debug_asset_type == 0);
    
    manager->debug_asset_type = manager->asset_types + type_id;
    manager->debug_asset_type->first_asset_index = manager->debug_used_asset_count;
    manager->debug_asset_type->one_past_last_asset_index = manager->debug_asset_type->first_asset_index;
}

internal void
asset_add_bitmap(Asset_Manager *manager, char *filename, Vec2 align_percentage = make_vec2(0.5f)) {
    assert(manager->debug_asset_type);
    assert(manager->debug_asset_type->one_past_last_asset_index < manager->asset_count);
    
    Asset *asset = manager->assets + manager->debug_asset_type->one_past_last_asset_index++;
    asset->first_tag_index = manager->debug_used_tag_count;
    asset->one_past_last_tag_index = asset->first_tag_index;
    asset->slot_id = debug_asset_add_bitmap_info(manager, filename, align_percentage).value;
    
    manager->debug_asset = asset;
}

internal void
asset_add_tag(Asset_Manager *manager, Asset_Tag_ID id, f32 value) {
    assert(manager->debug_asset);

    ++manager->debug_asset->one_past_last_tag_index;
    Asset_Tag *tag = manager->tags + manager->debug_used_tag_count++;
    tag->id = id;
    tag->value = value;
}

internal void
end_asset_type(Asset_Manager *manager) {
    assert(manager->debug_asset_type);
    
    manager->debug_used_asset_count = manager->debug_asset_type->one_past_last_asset_index;
    manager->debug_asset_type = 0;
    manager->debug_asset = 0;
}

internal Asset_Manager *
asset_manager_alloc(Arena *arena, memi size, Transient_State *tran_state) {
    Asset_Manager *manager = push_struct(arena, Asset_Manager);
    manager->tran_state = tran_state;
    manager->arena = make_sub_arena(arena, size);
    
    manager->bitmap_count = 256*AssetType_Count;
    manager->bitmap_infos = push_array(arena, Asset_Bitmap_Info, manager->bitmap_count);
    manager->bitmaps = push_array(arena, Asset_Slot, manager->bitmap_count);
    
    manager->sound_count = 1;
    manager->sounds = push_array(arena, Asset_Slot, manager->sound_count);
    
    manager->asset_count = manager->bitmap_count + manager->sound_count;
    manager->assets = push_array(arena, Asset, manager->asset_count);

    manager->tag_count = 1024*AssetType_Count;
    manager->tags = push_array(arena, Asset_Tag, manager->tag_count);
    
    manager->debug_used_bitmap_count = 1;
    manager->debug_used_asset_count = 1;
    
    begin_asset_type(manager, AssetType_Wall);
    asset_add_bitmap(manager, "../res/wall.bmp");
    end_asset_type(manager);

    begin_asset_type(manager, AssetType_Stairwell);
    asset_add_bitmap(manager, "../res/stair.bmp");
    end_asset_type(manager);

    begin_asset_type(manager, AssetType_Shadow);
    asset_add_bitmap(manager, "../res/shadow.bmp");
    end_asset_type(manager);

    begin_asset_type(manager, AssetType_Bat);
    asset_add_bitmap(manager, "../res/bat.bmp");
    end_asset_type(manager);
    
    begin_asset_type(manager, AssetType_Grass);
    asset_add_bitmap(manager, "../res/grass0.bmp");
    asset_add_bitmap(manager, "../res/grass1.bmp");
    end_asset_type(manager);

    begin_asset_type(manager, AssetType_Stone);
    asset_add_bitmap(manager, "../res/stone0.bmp");
    asset_add_bitmap(manager, "../res/stone1.bmp");
    end_asset_type(manager);
    
    begin_asset_type(manager, AssetType_Flower);
    asset_add_bitmap(manager, "../res/flower0.bmp");
    asset_add_bitmap(manager, "../res/flower1.bmp");
    end_asset_type(manager);

    f32 angle_right = 0.0f*pi32;
    f32 angle_back = 0.5f*pi32;
    f32 angle_left = 1.0f*pi32;
    f32 angle_front = 1.5f*pi32;
    
    begin_asset_type(manager, AssetType_Skull);
    asset_add_bitmap(manager, "../res/skull_right.bmp");
    asset_add_tag(manager, AssetTag_FacingDirection, angle_right);
    
    asset_add_bitmap(manager, "../res/skull_back.bmp");
    asset_add_tag(manager, AssetTag_FacingDirection, angle_back);
    
    asset_add_bitmap(manager, "../res/skull_left.bmp");
    asset_add_tag(manager, AssetTag_FacingDirection, angle_left);
    
    asset_add_bitmap(manager, "../res/skull_front.bmp");
    asset_add_tag(manager, AssetTag_FacingDirection, angle_front);
    end_asset_type(manager);
    
    return(manager);
}

//
//
//
internal Bitmap_ID
asset_best_match(Asset_Manager *manager, Asset_Type_ID type_id, Asset_Vector *match_vector, Asset_Vector *weight_vector) {
    Bitmap_ID result = {};

    f32 best_diff = f32_max;
    
    Asset_Type *type = manager->asset_types + type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        for (u32 asset_index = type->first_asset_index;
             asset_index < type->one_past_last_asset_index;
             ++asset_index) {
            Asset *asset = manager->assets + asset_index;
            
            f32 total_weighted_diff = 0.0f;
            for (u32 tag_index = asset->first_tag_index;
                 tag_index < asset->one_past_last_tag_index;
                 ++tag_index) {
                Asset_Tag *tag = manager->tags + tag_index;
                f32 difference = match_vector->e[tag->id] - tag->value;
                f32 weighted = weight_vector->e[tag->id]*abs_f32(difference);
                total_weighted_diff += weighted;
            }

            if (best_diff > total_weighted_diff) {
                best_diff = total_weighted_diff;
                result.value = asset->slot_id;
            }
        }
    }
    
    return(result);
}

internal Bitmap_ID
random_asset_from(Asset_Manager *manager, Asset_Type_ID type_id, Random_Series *series) {
    Bitmap_ID result = {};
    
    Asset_Type *type = manager->asset_types + type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        u32 count = type->one_past_last_asset_index - type->first_asset_index;
        u32 choice = random_choice(series, count);
        
        Asset *asset = manager->assets + type->first_asset_index + choice;
        result.value = asset->slot_id;
    }
    
    return(result);
}

internal Bitmap_ID
first_asset_from(Asset_Manager *manager, Asset_Type_ID type_id) {
    Bitmap_ID result = {};
    
    Asset_Type *type = manager->asset_types + type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        Asset *asset = manager->assets + type->first_asset_index;
        result.value = asset->slot_id;
    }
    
    return(result);
}

internal Bitmap *
asset_get_bitmap(Asset_Manager *manager, Bitmap_ID id) {
    Bitmap *result = manager->bitmaps[id.value].bitmap;
    return(result);
}

struct Load_Bitmap_Work {
    Memory_Task *memory_task;
    Asset_Manager *manager;
    Bitmap_ID id;
    Bitmap *bitmap;
    Asset_State final_state;
};

internal
OS_WORK_QUEUE_CALLBACK(load_bitmap_work) {
    Load_Bitmap_Work *work = (Load_Bitmap_Work *)data;
    Asset_Bitmap_Info *info = work->manager->bitmap_infos + work->id.value;
    *work->bitmap = debug_load_bitmap(info->filename, info->align_percentage);

    complete_previous_write_before_future_write;

    work->manager->bitmaps[work->id.value].bitmap = work->bitmap;
    work->manager->bitmaps[work->id.value].state = work->final_state;
    
    end_memory_task(work->memory_task);
}

internal void
asset_load_bitmap(Asset_Manager *manager, Bitmap_ID id) {
    if (id.value && (atomic_compare_exchange_u32((u32 *)&manager->bitmaps[id.value].state, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded)) {
        Memory_Task *memory_task = begin_memory_task(manager->tran_state);
        if (memory_task) {
            Load_Bitmap_Work *work = push_struct(&memory_task->arena, Load_Bitmap_Work);
            work->memory_task = memory_task;
            work->manager = manager;
            work->id = id;
            work->bitmap = push_struct(&manager->arena, Bitmap);
            work->final_state = AssetState_Loaded;

            os_work_queue_add_entry(manager->tran_state->low_priority_queue, load_bitmap_work, work);
        }
    }
}
