#pragma once
#include <string>

class ContextExtractor
{
public:
    ContextExtractor();
    ~ContextExtractor();

    std::wstring GetNewNameForFile(const std::wstring& originalPath);

private:
    std::wstring ExtractTitleFromPropertySystem(const std::wstring& path);
    std::wstring FallbackName(const std::wstring& path);
    std::wstring SanitizeName(const std::wstring& name);
    std::wstring GetExtension(const std::wstring& path);
    std::wstring GetDirectory(const std::wstring& path);
};
