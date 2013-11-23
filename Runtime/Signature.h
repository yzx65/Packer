#pragma once

#ifdef __GNUC__
#include <endian.h>
#endif

#if defined(__LITTLE_ENDIAN) || defined(_M_IX86) || defined(_M_AMD64)
#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00
#else
#define IMAGE_DOS_SIGNATURE                 0x4D5A      // MZ
#define IMAGE_NT_SIGNATURE                  0x50450000  // PE00
#endif