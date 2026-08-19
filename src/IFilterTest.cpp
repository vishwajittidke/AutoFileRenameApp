#include <iostream>
#include <windows.h>
#include <filter.h>
#include <ntquery.h>

#pragma comment(lib, "ntquery.lib")

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 2) return 1;
    CoInitialize(NULL);
    
    IFilter* pFilter = NULL;
    HRESULT hr = LoadIFilter(argv[1], NULL, (void**)&pFilter);
    if (SUCCEEDED(hr) && pFilter) {
        DWORD flags = 0;
        hr = pFilter->Init(IFILTER_INIT_CANON_PARAGRAPHS | IFILTER_INIT_CANON_HYPHENS | IFILTER_INIT_CANON_SPACES | IFILTER_INIT_APPLY_INDEX_ATTRIBUTES, 0, NULL, &flags);
        if (SUCCEEDED(hr)) {
            STAT_CHUNK stat;
            while (SUCCEEDED(pFilter->GetChunk(&stat))) {
                if (stat.flags & CHUNK_TEXT) {
                    ULONG bufSize = 256;
                    wchar_t buf[256] = {0};
                    if (SUCCEEDED(pFilter->GetText(&bufSize, buf))) {
                        std::wcout << L"Extracted: " << buf << std::endl;
                        break;
                    }
                }
            }
        }
        pFilter->Release();
    } else {
        std::wcout << L"LoadIFilter failed: " << std::hex << hr << std::endl;
    }
    
    CoUninitialize();
    return 0;
}
