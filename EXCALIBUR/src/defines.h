#pragma once

#ifdef _WIN32
#define EXCALIBUR_PLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on windows"
#endif
#endif

#define SXCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;