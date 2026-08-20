#include "AutoRenameExt.h"
#include "Global.h"
#include "ContextExtractor.h"
#include <strsafe.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <shellapi.h>
#include <shlwapi.h>

extern long g_cRefThisDll;

AutoRenameExt::AutoRenameExt() : m_cRef(1)
{
    InterlockedIncrement(&g_cRefThisDll);
}

AutoRenameExt::~AutoRenameExt()
{
    InterlockedDecrement(&g_cRefThisDll);
}

IFACEMETHODIMP AutoRenameExt::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppv = static_cast<IShellExtInit *>(this);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = static_cast<IContextMenu *>(this);
    }
    else if (IsEqualIID(riid, IID_IExplorerCommand))
    {
        *ppv = static_cast<IExplorerCommand *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

IFACEMETHODIMP_(ULONG) AutoRenameExt::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) AutoRenameExt::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return cRef;
}

IFACEMETHODIMP AutoRenameExt::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
    if (NULL == pDataObj)
        return E_INVALIDARG;

    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stm;

    if (SUCCEEDED(pDataObj->GetData(&fe, &stm)))
    {
        HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
        if (hDrop != NULL)
        {
            UINT nFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
            for (UINT i = 0; i < nFiles; i++)
            {
                wchar_t szFile[MAX_PATH];
                if (DragQueryFileW(hDrop, i, szFile, ARRAYSIZE(szFile)))
                {
                    DWORD attrs = GetFileAttributesW(szFile);
                    if (attrs != INVALID_FILE_ATTRIBUTES)
                    {
                        if (!(attrs & FILE_ATTRIBUTE_HIDDEN) && 
                            !(attrs & FILE_ATTRIBUTE_SYSTEM) &&
                            !(attrs & FILE_ATTRIBUTE_READONLY) &&
                            !(attrs & FILE_ATTRIBUTE_OFFLINE) &&
                            !(attrs & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS))
                        {
                            m_selectedFiles.push_back(szFile);
                        }
                    }
                }
            }
            GlobalUnlock(stm.hGlobal);
        }
        ReleaseStgMedium(&stm);
    }

    return m_selectedFiles.size() > 0 ? S_OK : E_INVALIDARG;
}

IFACEMETHODIMP AutoRenameExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (uFlags & CMF_DEFAULTONLY)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));

    InsertMenuW(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst + 1, L"Auto Rename File(s)");

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(2));
}

IFACEMETHODIMP AutoRenameExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    if (HIWORD(pici->lpVerb))
        return E_INVALIDARG;

    if (LOWORD(pici->lpVerb) == 1)
    {
        this->ProcessFiles();
    }
    return S_OK;
}

IFACEMETHODIMP AutoRenameExt::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

// IExplorerCommand implementation
IFACEMETHODIMP AutoRenameExt::GetTitle(IShellItemArray *psiItemArray, LPWSTR *ppszName)
{
    return SHStrDupW(L"Auto Rename File(s)", ppszName);
}

IFACEMETHODIMP AutoRenameExt::GetIcon(IShellItemArray *psiItemArray, LPWSTR *ppszIcon)
{
    return SHStrDupW(L"imageres.dll,-5322", ppszIcon); // generic rename icon
}

IFACEMETHODIMP AutoRenameExt::GetToolTip(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP AutoRenameExt::GetCanonicalName(GUID *pguidCommandName)
{
    *pguidCommandName = CLSID_AutoRenameExt;
    return S_OK;
}

IFACEMETHODIMP AutoRenameExt::GetState(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE *pCmdState)
{
    *pCmdState = ECS_ENABLED;
    return S_OK;
}

IFACEMETHODIMP AutoRenameExt::Invoke(IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    m_selectedFiles.clear();
    DWORD count = 0;
    psiItemArray->GetCount(&count);
    
    for (DWORD i = 0; i < count; i++)
    {
        IShellItem *pItem = NULL;
        if (SUCCEEDED(psiItemArray->GetItemAt(i, &pItem)))
        {
            PWSTR pszPath = NULL;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
            {
                m_selectedFiles.push_back(pszPath);
                CoTaskMemFree(pszPath);
            }
            pItem->Release();
        }
    }

    if (!m_selectedFiles.empty())
    {
        this->ProcessFiles();
    }
    return S_OK;
}

IFACEMETHODIMP AutoRenameExt::GetFlags(EXPCMDFLAGS *pFlags)
{
    *pFlags = ECF_DEFAULT;
    return S_OK;
}

IFACEMETHODIMP AutoRenameExt::EnumSubCommands(IEnumExplorerCommand **ppEnum)
{
    *ppEnum = NULL;
    return E_NOTIMPL;
}

void AutoRenameExt::ProcessFiles()
{
    std::vector<std::wstring> filesCopy = m_selectedFiles;
    std::thread([filesCopy]() {
        // --- 1. Launch Pad (Native Win32 Console) ---
        AllocConsole();
        FILE* fpOut;
        freopen_s(&fpOut, "CONOUT$", "w", stdout);
        SetConsoleTitleW(L"🚀 Auto Rename Launch Pad");
        
        std::wcout << L"=======================================\n";
        std::wcout << L"    LAUNCH PAD: FILE RENAME SYSTEM     \n";
        std::wcout << L"=======================================\n\n";

        ContextExtractor extractor;
        std::vector<std::wstring> generatedNames;
        
        for (const auto& file : filesCopy)
        {
            std::wcout << L"[+] Targeting File: " << file << L"\n";
            std::wcout << L"    Firing rockets ";
            
            // --- 2. Rocket Animation ---
            for(int i=0; i<3; i++) {
                std::wcout << L"🚀";
                std::wcout.flush();
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
            std::wcout << L"\n";

            std::wstring newName = extractor.GetNewNameForFile(file);
            if (!newName.empty() && newName != file)
            {
                // Ensure unique name within the batch and on disk
                std::wstring uniqueName = newName;
                int counter = 1;
                while (true) {
                    bool exists_in_batch = false;
                    for (const auto& n : generatedNames) {
                        if (n == uniqueName) { exists_in_batch = true; break; }
                    }
                    if (!exists_in_batch && GetFileAttributesW(uniqueName.c_str()) == INVALID_FILE_ATTRIBUTES) {
                        break;
                    }
                    
                    std::wcout << L"    [!] Duplicate found! Resolving collision...\n";
                    
                    // Insert (counter) before the extension
                    size_t dotPos = newName.find_last_of(L".");
                    if (dotPos != std::wstring::npos) {
                        uniqueName = newName.substr(0, dotPos) + L" (" + std::to_wstring(counter) + L")" + newName.substr(dotPos);
                    } else {
                        uniqueName = newName + L" (" + std::to_wstring(counter) + L")";
                    }
                    counter++;
                }
                
                generatedNames.push_back(uniqueName);
                std::wcout << L"    [>] Renaming to: " << uniqueName << L"\n\n";
                
                std::wstring fromFile = file + L'\0' + L'\0';
                std::wstring toFile = uniqueName + L'\0' + L'\0';
                
                SHFILEOPSTRUCTW fileOp = {0};
                fileOp.hwnd = NULL;
                fileOp.wFunc = FO_RENAME;
                fileOp.pFrom = fromFile.c_str();
                fileOp.pTo = toFile.c_str();
                fileOp.fFlags = FOF_ALLOWUNDO | FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_RENAMEONCOLLISION;
                
                SHFileOperationW(&fileOp);
            }
        }

        std::wcout << L"\n[+] Processing complete! Closing Launch Pad in 2 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        fclose(fpOut);
        FreeConsole();

        // --- 3. Success Message Box ---
        MessageBoxW(NULL, L"rename of the file is successfully done.", L"Success", MB_OK | MB_ICONINFORMATION);

    }).detach();
}
