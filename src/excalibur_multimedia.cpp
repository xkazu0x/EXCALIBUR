////////////////////////////////
// NOTE(xkazu0x): Bitmap functions

internal Bitmap
load_bitmap(char *filename, Vec2 align_percentage) {
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

////////////////////////////////
// NOTE(xkazu0x): Wave functions

internal Riff_Iterator
riff_parse_chunk_at(void *at, void *stop) {
    Riff_Iterator iter;
    iter.at = (u8 *)at;
    iter.stop = (u8 *)stop;
    return(iter);
}

internal b32
riff_is_valid(Riff_Iterator iter) {
    b32 result = (iter.at < iter.stop);
    return(result);
}

internal Riff_Iterator
riff_next_chunk(Riff_Iterator iter) {
    Wave_Chunk *chunk = (Wave_Chunk *)iter.at;
    u32 size = (chunk->size + 1) & ~1;
    iter.at += sizeof(Wave_Chunk) + size;
    return(iter);
}

internal u32
riff_get_chunk_type(Riff_Iterator iter) {
    Wave_Chunk *chunk = (Wave_Chunk *)iter.at;
    u32 result = chunk->id;
    return(result);
}

internal void *
riff_get_chunk_data(Riff_Iterator iter) {
    void *result = (iter.at + sizeof(Wave_Chunk));
    return(result);
}

internal u32
riff_get_chunk_data_size(Riff_Iterator iter) {
    Wave_Chunk *chunk = (Wave_Chunk *)iter.at;
    u32 result = chunk->size;
    return(result);
}

internal Sound
load_wav(char *filename) {
    Sound result = {};
    
    Debug_OS_File file = debug_os_read_file(filename);
    if (file.size != 0) {
        Wave_Header *header = (Wave_Header *)file.data;
        assert(header->riff_id == WaveChunkType_RIFF);
        assert(header->wave_id == WaveChunkType_WAVE);

        u32 channel_count = 0;
        u32 sample_data_size = 0;
        s16 *sample_data = 0;
        for (Riff_Iterator iter = riff_parse_chunk_at(header + 1, (u8 *)(header + 1) + header->size - 4);
             riff_is_valid(iter);
             iter = riff_next_chunk(iter)) {
            switch (riff_get_chunk_type(iter)) {
                case WaveChunkType_fmt: {
                    Wave_Fmt *fmt = (Wave_Fmt *)riff_get_chunk_data(iter);
                    assert(fmt->format_tag == 1); // NOTE(xkazu0x): Only support PCM
                    //assert(fmt->samples_per_sec == 48000);
                    assert(fmt->bits_per_sample == 16);
                    assert(fmt->block_align == (sizeof(s16)*fmt->channels));
                    channel_count = fmt->channels;
                } break;
                    
                case WaveChunkType_data: {
                    sample_data = (s16 *)riff_get_chunk_data(iter);
                    sample_data_size = riff_get_chunk_data_size(iter);
                } break;
            }
        }

        assert(channel_count && sample_data);

        result.channel_count = channel_count;
        result.sample_count = sample_data_size/channel_count*sizeof(s16);
        if (channel_count == 1) {
            result.samples[0] = sample_data;
            result.samples[1] = 0;
        } else if (channel_count == 2) {
            result.samples[0] = sample_data;
            result.samples[1] = sample_data + result.sample_count;

            for (u32 sample_index = 0;
                 sample_index < result.sample_count;
                 ++sample_index) {
                s16 source = sample_data[2*sample_index];
                sample_data[2*sample_index] = sample_data[sample_index];
                sample_data[sample_index] = source;
            }
        } else {
            invalid_path;
        }

        // TODO(xkazu0x): Load right channels!
        result.channel_count = 1;
    }

    return(result);
}
