#ifndef EXCALIBUR_INTRINSICS_H
#define EXCALIBUR_INTRINSICS_H

internal inline f32 abs_f32(f32 x);
internal inline f32 sqrt_f32(f32 x);
internal inline f32 sin_f32(f32 x);
internal inline f32 cos_f32(f32 x);
internal inline f32 tan_f32(f32 x);

internal inline s32 sign_of(s32 x);

internal inline u32 rotate_left(u32 value, s32 amount);
internal inline u32 rotate_right(u32 value, s32 amount);

internal inline s32 round_f32_to_s32(f32 x);
internal inline u32 round_f32_to_u32(f32 x);

internal inline s32 truncate_f32_to_s32(f32 x);
internal inline u32 truncate_f32_to_u32(f32 x);

internal inline s32 ceil_f32_to_s32(f32 x);
internal inline s32 floor_f32_to_s32(f32 x);

struct bit_scan_result_t {
    b32 found;
    u32 index;
};

internal inline bit_scan_result_t find_least_significant_set_bit(u32 value);

#endif // EXCALIBUR_INTRINSICS_H
