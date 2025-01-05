#ifndef EX_PLATFORM_H
#define EX_PLATFORM_H

///////////////////////////////////////
// NOTE(xkazu0x): platform types

struct window_context {
    bool8 fullscreen;
    const char *title;
    vec2i position;
    vec2i size;
};

struct platform_context {
    bool8 quit;
    window_context window;
};

//////////////////////////////////
// NOTE(xkazu0x): window functions

function bool8 platform_window_create(platform_context *platform);
function void platform_window_pull(platform_context *platform);
function void platform_window_toggle_fullscreen(platform_context *platform);

#endif // EX_PLATFORM_H
