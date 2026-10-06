#pragma once

#include <array>

struct PatchDetails {
    unsigned virtual_address;
    const char* hex_bytes;
};

inline constexpr std::array<PatchDetails, 3> kPatches = {{
    {
        .virtual_address = 0x004DCCF0,  // lua_ScanDllStart
        .hex_bytes = "B8"
                     "00000000"  // mov eax, 0
                     "C3"        // ret
    },
    {
        .virtual_address = 0x004E5CB0,  // ScanDllStart
        .hex_bytes = "B8"
                     "01000000"  // mov eax, 1
                     "A3"
                     "74B4B600"  // mov isScanDllFinished, eax
                     "68"
                     "E05C4E00"  // push offset aAwesomeWotlkLib_dll
                     "E8"
                     "1C683800"  // call _loadddll
                     "83C4"
                     "04"    // add esp, 4
                     "55"    // push ebp
                     "8BEC"  // mov ebp, esp
                     "E8"
                     "A110F2FF"  // call 0x00406D70
                     "E9"
                     "045BF2FF"                                  // jmp 0x0040B7D8
                     "CCCCCCCCCCCCCCCCCCCCCCCC"                  // int3 x12 (padding)
                     "417765736F6D65576F746C6B4C69622E646C6C00"  // "AwesomeWotlkLib.dll" at 0x004E5CE0
    },
    {
        .virtual_address = 0x0040B7D0,  // StartAddress
        .hex_bytes = "E9"
                     "DBA40D00"  // jmp 0x004E5CB0
                     "909090"    // nop x3 (pads out the 8 bytes replaced)
    }
}};
