#include "Global.h"
#include "ClassFactory.h"
#include <shlobj.h>

long g_cRefThisDll = 0;
HINSTANCE g_hInst = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
        break;
    }
    return TRUE;
}

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
    if (IsEqualIID(rclsid, CLSID_AutoRenameExt))
    {
        ClassFactory *pcf = new ClassFactory();
        return pcf->QueryInterface(riid, ppvOut);
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

// Helper to write string to registry
HRESULT SetHKCRRegistryKeyAndValue(PCWSTR pszSubKey, PCWSTR pszValueName, PCWSTR pszData)
{
    HKEY hKey = NULL;
    HRESULT hr = HRESULT_FROM_WIN32(RegCreateKeyExW(HKEY_CLASSES_ROOT, pszSubKey, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL));
    if (SUCCEEDED(hr))
    {
        if (pszData != NULL)
        {
            DWORD cbData = lstrlenW(pszData) * sizeof(*pszData);
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, pszValueName, 0, REG_SZ, 
                (const BYTE*)pszData, cbData));
        }
        RegCloseKey(hKey);
    }
    return hr;
}

STDAPI DllRegisterServer(void)
{
    // Register shell extension
    wchar_t szModule[MAX_PATH];
    if (GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule)) == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    // Convert GUID to string
    wchar_t szCLSID[MAX_PATH];
    StringFromGUID2(CLSID_AutoRenameExt, szCLSID, ARRAYSIZE(szCLSID));

    wchar_t szSubkey[MAX_PATH];
    
    // Create HKCR\CLSID\{GUID}
    wsprintfW(szSubkey, L"CLSID\\%s", szCLSID);
    SetHKCRRegistryKeyAndValue(szSubkey, NULL, L"AutoRenameExt Class");

    // Create HKCR\CLSID\{GUID}\InprocServer32
    wsprintfW(szSubkey, L"CLSID\\%s\\InprocServer32", szCLSID);
    SetHKCRRegistryKeyAndValue(szSubkey, NULL, szModule);
    SetHKCRRegistryKeyAndValue(szSubkey, L"ThreadingModel", L"Apartment");

    // Register under AllFileSystemObjects for IContextMenu (Classic)
    SetHKCRRegistryKeyAndValue(L"AllFileSystemObjects\\shellex\\ContextMenuHandlers\\AutoRenameExt", NULL, szCLSID);

    // Register for Files (Windows 11 Modern UI)
    SetHKCRRegistryKeyAndValue(L"*\\shell\\AutoRenameExt", L"ExplorerCommandHandler", szCLSID);
    SetHKCRRegistryKeyAndValue(L"*\\shell\\AutoRenameExt", L"Icon", L"imageres.dll,-5322");
    SetHKCRRegistryKeyAndValue(L"*\\shell\\AutoRenameExt", L"MUIVerb", L"Auto Rename File(s)");

    // Register for Directories (Windows 11 Modern UI)
    SetHKCRRegistryKeyAndValue(L"Directory\\shell\\AutoRenameExt", L"ExplorerCommandHandler", szCLSID);
    SetHKCRRegistryKeyAndValue(L"Directory\\shell\\AutoRenameExt", L"Icon", L"imageres.dll,-5322");
    SetHKCRRegistryKeyAndValue(L"Directory\\shell\\AutoRenameExt", L"MUIVerb", L"Auto Rename File(s)");

    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    wchar_t szCLSID[MAX_PATH];
    StringFromGUID2(CLSID_AutoRenameExt, szCLSID, ARRAYSIZE(szCLSID));
    wchar_t szSubkey[MAX_PATH];

    // Remove ContextMenuHandlers (Classic)
    RegDeleteTreeW(HKEY_CLASSES_ROOT, L"AllFileSystemObjects\\shellex\\ContextMenuHandlers\\AutoRenameExt");

    // Remove shell handler (Windows 11 Modern UI)
    RegDeleteTreeW(HKEY_CLASSES_ROOT, L"*\\shell\\AutoRenameExt");
    RegDeleteTreeW(HKEY_CLASSES_ROOT, L"Directory\\shell\\AutoRenameExt");

    // Remove CLSID
    wsprintfW(szSubkey, L"CLSID\\%s", szCLSID);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, szSubkey);

    return S_OK;
}
