#include "ex_base.h"
#include "ex_base.cpp"
#include "ex_logger.h"

int main(void) {
    EXINFO("-+=+EXCALIBUR+=+-");
    EXINFO("-+=+EXCALIBUR : CONTEXT+=+-");
    EXINFO("\t> OPERATING SYSTEM: %s", string_from_operating_system(operating_system_from_context()));
    EXINFO("\t> ARCHITECTURE: %s", string_from_architecture(architecture_from_context()));
    return(0);
}
