#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_HEAPDIAGNOSTICS_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_HEAPDIAGNOSTICS_HPP

#include "SceTypes.hpp"
#include <cstddef>

constexpr std::size_t kLibcHeapMstateTableCapacity = 65;

extern "C" void LibcHeapTraceInfo_nid_no_patch(LibcHeapInfo* info);

#endif
