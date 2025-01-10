#ifndef EXCALIBUR_PLATFORM_H
#define EXCALIBUR_PLATFORM_H

struct platform_context {
    bool32 initialized;
    bool32 quit;
    void *internal_state;
};

struct platform_desc {
    const char *name;
    vec2i screen_size;
};

internal bool32 platform_initialize(platform_context *platform, platform_desc *description);
internal void platform_pull(platform_context *platform);

#endif // EXCALIBUR_PLATFORM_H
