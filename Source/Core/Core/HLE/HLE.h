// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <string>

#include "Common/CommonTypes.h"

namespace HLE
{
enum
{
	HLE_HOOK_START = 0,    // Hook the beginning of the function and execute the function afterwards
	HLE_HOOK_REPLACE = 1,    // Replace the function with the HLE version
	HLE_HOOK_NONE = 2,    // Do not hook the function
};

enum
{
	HLE_TYPE_GENERIC = 0,    // Miscellaneous function
	HLE_TYPE_DEBUG = 1,    // Debug output function
};

void PatchFunctions();

void Patch(u32 pc, const char *func_name);
u32 UnPatch(const std::string& patchName);
void Execute(u32 _CurrentPC, u32 _Instruction);

u32 GetFunctionIndex(u32 em_address);

#ifdef __APPLE__
// Returns the HLE function index if the address matches the function start
u32 GetFirstFunctionIndex(u32 address);
#endif

int GetFunctionTypeByIndex(u32 index);
int GetFunctionFlagsByIndex(u32 index);

bool IsEnabled(int flags);
}
