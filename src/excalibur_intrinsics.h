#ifndef EXCALIBUR_INTRINSICS_H
#define EXCALIBUR_INTRINSICS_H

internal s32 round_f32_to_s32(f32 f);
internal u32 round_f32_to_u32(f32 f);

internal s32 truncate_f32_to_s32(f32 f);
internal u32 truncate_f32_to_u32(f32 f);

internal s32 floor_f32_to_s32(f32 f);

internal f32 abs_f32(f32 x);
internal f64 abs_f64(f64 x);

internal f32 sqrt_f32(f32 x);
internal f32 sin_f32(f32 x);
internal f32 cos_f32(f32 x);
internal f32 tan_f32(f32 x);

internal f64 sqrt_f64(f64 x);
internal f64 sin_f64(f64 x);
internal f64 cos_f64(f64 x);
internal f64 tan_f64(f64 x);

#endif // EXCALIBUR_INTRINSICS_H
