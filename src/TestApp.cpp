#include "ContextExtractor.h"
#include <iostream>
#include <windows.h>

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    ContextExtractor extractor;
    
    std::wstring path;
    int len = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, NULL, 0);
    if (len > 0) {
        wchar_t* buf = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, buf, len);
        path = buf;
        delete[] buf;
    }
    
    std::wstring newName = extractor.GetNewNameForFile(path);
    std::wcout << L"New name: " << newName << std::endl;
    return 0;
}
