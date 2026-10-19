#include "prx/libc/include/HeapDiagnostics.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

extern "C" {
void APS5_VABI sceLibcInternalBacktraceForGame_nid_postfix(const char* heapName);
void APS5_VABI sceLibcInternalHeapErrorReportForGame_nid_postfix(void* heap, void* block, std::uint32_t error);
void APS5_VABI LibcHeapGetTraceInfo_nid_postfix(LibcHeapInfo* info);
}

static void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stdout, "%s\n", message);
    std::abort();
}

int main() {
    const auto path = std::filesystem::temp_directory_path() / ("anyps5-heap-report-" + std::to_string(std::rand()) + ".txt");
    Require(std::freopen(path.string().c_str(), "w", stderr) != nullptr, "stderr redirection failed");

    char heapName[] = "SceLibcHeap";
    sceLibcInternalBacktraceForGame_nid_postfix(heapName);
    sceLibcInternalBacktraceForGame_nid_postfix(nullptr);
    int heap = 0;
    int block = 0;
    sceLibcInternalHeapErrorReportForGame_nid_postfix(&heap, &block, 0x1234u);
    std::fflush(stderr);
    std::fclose(stderr);

    std::ifstream file(path);
    std::stringstream text;
    text << file.rdbuf();
    file.close();
    std::filesystem::remove(path);
    const std::string report = text.str();
    Require(report.find("[SceLibcHeap] backtrace requested from guest 0x") != std::string::npos, "heap name not reported");
    Require(report.find("[] backtrace requested from guest 0x") != std::string::npos, "null heap name not reported");
    Require(report.find("[libc] heap error 0x1234: heap ") != std::string::npos, "heap error not reported");
    Require(std::string(heapName) == "SceLibcHeap", "heap name modified");

    bool threwNull = false;
    try {
        LibcHeapTraceInfo_nid_no_patch(nullptr);
    } catch (const std::invalid_argument&) {
        threwNull = true;
    }
    Require(threwNull, "null info did not throw");

    bool threwSize = false;
    LibcHeapInfo badInfo{};
    badInfo.size = sizeof(LibcHeapInfo) - 1;
    try {
        LibcHeapTraceInfo_nid_no_patch(&badInfo);
    } catch (const std::invalid_argument&) {
        threwSize = true;
    }
    Require(threwSize, "invalid size did not throw");

    LibcHeapInfo info{};
    info.size = sizeof(LibcHeapInfo);
    LibcHeapGetTraceInfo_nid_postfix(&info);
    Require(info.unknown2 == 0, "unknown2 not zeroed");
    Require(info.mspace_atomic_id_mask != nullptr, "null mspace_atomic_id_mask");
    Require(info.mstate_table != nullptr, "null mstate_table");

    info.mstate_table[0] = 0x1000ull;
    for (std::uint64_t bit = 0; bit < 64; ++bit) {
        const std::uint64_t slot = bit + 1;
        info.mstate_table[slot] = 0x2000ull + slot;
        *info.mspace_atomic_id_mask |= (1ull << bit);
    }

    Require(*info.mspace_atomic_id_mask == ~0ull, "mask incomplete");
    Require(info.mstate_table[0] == 0x1000ull, "base mstate modified");
    for (std::size_t i = 1; i < kLibcHeapMstateTableCapacity; ++i) {
        Require(info.mstate_table[i] == 0x2000ull + i, "mstate slot mismatch");
    }

    *info.mspace_atomic_id_mask = 0;
    for (std::size_t i = 0; i < kLibcHeapMstateTableCapacity; ++i) {
        info.mstate_table[i] = 0;
    }
}
