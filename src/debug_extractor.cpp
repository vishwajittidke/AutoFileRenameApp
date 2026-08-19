#include "ContextExtractor.h"
#include <iostream>

int main() {
    ContextExtractor extractor;
    std::wstring name = extractor.ExtractTitleFromPropertySystem(L"C:\\Users\\Vishwajit\\Downloads\\Flight Logo.png");
    std::wcout << L"EXTRACTED: " << name << std::endl;
    return 0;
}
