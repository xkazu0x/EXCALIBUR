#include "base/ex_base.h"
#include "base/ex_base.cpp"

#include "platform/ex_platform.h"
#include "platform/ex_platform.cpp"

global platform_context platform;

int main(void) {
    EXINFO("-+=+EXCALIBUR : CONTEXT+=+-");
    EXINFO("\t> OPERATING SYSTEM: %s", string_from_operating_system(operating_system_from_context()));
    EXINFO("\t> ARCHITECTURE: %s", string_from_architecture(architecture_from_context()));
    platform.window.title = "EXCALIBUR";
    platform.window.size.x = 960;
    platform.window.size.y = 540;
    if (!platform_window_create(&platform)) return(1);
    EXINFO("-+=+EXCALIBUR : INITIALIZED+=+-");
    while (!platform.quit) {
        platform_window_pull(&platform);
    }
    EXINFO("-+=+EXCALIBUR : SHUTTING-DOWN+=+-");
    return(0);
}
