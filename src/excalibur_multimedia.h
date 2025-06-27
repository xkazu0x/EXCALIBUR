#ifndef EXCALIBUR_MULTIMEDIA
#define EXCALIBUR_MULTIMEDIA

////////////////////////////////
// NOTE(xkazu0x): Bitmap types

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

#define BITMAP_BYTES_PER_PIXEL 4
struct Bitmap {
    Vec2 align_percentage;
    f32 width_over_height;
    
    s32 width;
    s32 height;
    s32 pitch;
    void *memory;
};

////////////////////////////////
// NOTE(xkazu0x): Bitmap functions

internal Bitmap load_bitmap(char *filename, Vec2 align_percentage = make_vec2(0.5f));

////////////////////////////////
// NOTE(xkazu0x): Wave types

#pragma pack(push, 1)
struct Wave_Header {
    u32 riff_id;
    u32 size;
    u32 wave_id;
};

#define RIFF_CODE(a, b, c, d) (((u32)(a)<<0)|((u32)(b)<<8)|((u32)(c)<<16)|((u32)(d)<<24))
enum Wave_Chunk_Type {
    WaveChunkType_fmt  = RIFF_CODE('f', 'm', 't', ' '),
    WaveChunkType_data = RIFF_CODE('d', 'a', 't', 'a'),
    WaveChunkType_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WaveChunkType_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};

struct Wave_Chunk {
    u32 id;
    u32 size;
};

struct Wave_Fmt {
    u16 format_tag;
    u16 channels;
    u32 samples_per_sec;
    u32 avg_bytes_per_sec;
    u16 block_align;
    u16 bits_per_sample;
    u16 size;
    u16 valid_bits_per_sample;
    u32 channels_mask;
    u8 sub_format[16];
};
#pragma pack(pop)

struct Riff_Iterator {
    u8 *at;
    u8 *stop;
};

struct Sound {
    u32 channel_count;
    u32 sample_count;
    s16 *samples[2];
};

////////////////////////////////
// NOTE(xkazu0x): Wave functions

internal Riff_Iterator riff_parse_chunk_at(void *at, void *stop);
internal b32 riff_is_valid(Riff_Iterator iter);
internal Riff_Iterator riff_next_chunk(Riff_Iterator iter);
internal u32 riff_get_chunk_type(Riff_Iterator iter);
internal void *riff_get_chunk_data(Riff_Iterator iter);
internal u32 riff_get_chunk_data_size(Riff_Iterator iter);

internal Sound load_wav(char *filename);

#endif // EXCALIBUR_MULTIMEDIA
