#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <vector>
#include <string>

class AutoRenameExt : public IShellExtInit, public IContextMenu, public IExplorerCommand
{
public:
    AutoRenameExt();
    ~AutoRenameExt();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // IShellExtInit
    IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);

    // IContextMenu
    IFACEMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    IFACEMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pReserved, LPSTR pszName, UINT cchMax);

    // IExplorerCommand
    IFACEMETHODIMP GetTitle(IShellItemArray *psiItemArray, LPWSTR *ppszName);
    IFACEMETHODIMP GetIcon(IShellItemArray *psiItemArray, LPWSTR *ppszIcon);
    IFACEMETHODIMP GetToolTip(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip);
    IFACEMETHODIMP GetCanonicalName(GUID *pguidCommandName);
    IFACEMETHODIMP GetState(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE *pCmdState);
    IFACEMETHODIMP Invoke(IShellItemArray *psiItemArray, IBindCtx *pbc);
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS *pFlags);
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand **ppEnum);

private:
    long m_cRef;
    std::vector<std::wstring> m_selectedFiles;

    void ProcessFiles();
    std::wstring GetNewNameForFile(const std::wstring& filePath);
};
