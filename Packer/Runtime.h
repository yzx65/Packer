#pragma once

#ifdef _WIN32
#include "Win32Runtime.h"
#endif

#ifdef _MSC_VER
#include <type_traits> //for std::move
#endif