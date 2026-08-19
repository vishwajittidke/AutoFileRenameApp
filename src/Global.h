#pragma once
#include <windows.h>

// {A1B2C3D4-E5F6-7890-1234-567890ABCDEF}
// Generate a unique GUID for our COM object
static const GUID CLSID_AutoRenameExt = 
{ 0xA1B2C3D4, 0xE5F6, 0x7890, { 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF } };

extern long g_cRefThisDll;
extern HINSTANCE g_hInst;
