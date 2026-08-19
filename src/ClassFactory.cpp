#include "ClassFactory.h"
#include "Global.h"
#include "AutoRenameExt.h"
#include <new>

ClassFactory::ClassFactory() : m_cRef(1)
{
    InterlockedIncrement(&g_cRefThisDll);
}

ClassFactory::~ClassFactory()
{
    InterlockedDecrement(&g_cRefThisDll);
}

IFACEMETHODIMP ClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = static_cast<IClassFactory *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

IFACEMETHODIMP_(ULONG) ClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) ClassFactory::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }
    return cRef;
}

IFACEMETHODIMP ClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    AutoRenameExt *pExt = new (std::nothrow) AutoRenameExt();
    if (pExt)
    {
        HRESULT hr = pExt->QueryInterface(riid, ppv);
        pExt->Release();
        return hr;
    }
    return E_OUTOFMEMORY;
}

IFACEMETHODIMP ClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cRefThisDll);
    else
        InterlockedDecrement(&g_cRefThisDll);
    return S_OK;
}
