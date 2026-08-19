#include "ContextExtractor.h"
#include <windows.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <regex>
#include <string>
#include <iostream>
#include <unordered_set>
#include <shlobj.h>

ContextExtractor::ContextExtractor()
{
    // CoInitialize should be handled by the calling thread, 
    // but COM apartments in background threads might need it.
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

ContextExtractor::~ContextExtractor()
{
    CoUninitialize();
}

std::wstring ContextExtractor::GetExtension(const std::wstring& path)
{
    std::wstring safePath = path;
    // Web sanitization: strip ?v=...
    size_t qPos = safePath.find(L"?");
    if (qPos != std::wstring::npos)
        safePath = safePath.substr(0, qPos);
        
    // Clean .crdownload
    size_t crdPos = safePath.find(L".crdownload");
    if (crdPos != std::wstring::npos)
        safePath = safePath.substr(0, crdPos);

    size_t slashPos = path.find_last_of(L"/\\");
    size_t dotPos = path.find_last_of(L".");
    
    // Only extract extension if the dot appears AFTER the last folder slash
    if (dotPos != std::wstring::npos && (slashPos == std::wstring::npos || dotPos > slashPos))
    {
        return path.substr(dotPos);
    }
    return L"";
}

std::wstring ContextExtractor::GetDirectory(const std::wstring& path)
{
    size_t slashPos = path.find_last_of(L"\\/");
    if (slashPos != std::wstring::npos)
        return path.substr(0, slashPos + 1);
    return L"";
}

std::wstring ContextExtractor::SanitizeName(const std::wstring& name)
{
    std::wstring safe = name;
    // Remove invalid characters for Windows filenames, plus user-requested symbols (curly braces, brackets, parentheses)
    std::wregex invalidChars(L"[<>:\"/\\\\|?*\\x00-\\x1F\\{\\}\\(\\)\\[\\]]");
    safe = std::regex_replace(safe, invalidChars, L"_");
    
    // Trim spaces
    size_t first = safe.find_first_not_of(L" \t\n\r");
    if (first == std::wstring::npos) return L"";
    size_t last = safe.find_last_not_of(L" \t\n\r");
    safe = safe.substr(first, (last - first + 1));

    // Apply Case Enforcement from INI
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath)))
    {
        std::wstring iniPath = std::wstring(appDataPath) + L"\\AutoRename.ini";
        wchar_t caseRule[256];
        GetPrivateProfileStringW(L"Rules", L"Case", L"none", caseRule, 256, iniPath.c_str());
        std::wstring rule(caseRule);
        if (rule == L"lower")
        {
            for (auto& c : safe) c = towlower(c);
        }
        else if (rule == L"upper")
        {
            for (auto& c : safe) c = towupper(c);
        }
    }
    
    // Truncate to prevent MAX_PATH overflow (Windows limits paths to 260 chars)
    // 120 characters is a safe upper bound for a filename base
    if (safe.length() > 120)
    {
        safe = safe.substr(0, 120);
        // Re-trim in case we cut at a space
        size_t last = safe.find_last_not_of(L" \t\n\r");
        if (last != std::wstring::npos) safe.erase(last + 1);
    }

    return safe;
}

std::wstring ContextExtractor::ExtractTitleFromPropertySystem(const std::wstring& path)
{
    std::wstring title = L"";
    IShellItem2* pItem = NULL;
    
    if (SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&pItem))))
    {
        PROPVARIANT propVar;
        PropVariantInit(&propVar);
        
        // Try System.Title
        if (SUCCEEDED(pItem->GetProperty(PKEY_Title, &propVar)) && propVar.vt == VT_LPWSTR && propVar.pwszVal != NULL && wcslen(propVar.pwszVal) > 0)
        {
            title = propVar.pwszVal;
        }
        else 
        {
            // Fallback 1: System.Photo.DateTaken
            PropVariantClear(&propVar);
            if (SUCCEEDED(pItem->GetProperty(PKEY_Photo_DateTaken, &propVar)) && propVar.vt != VT_EMPTY)
            {
                wchar_t szDate[100];
                if (SUCCEEDED(PropVariantToString(propVar, szDate, ARRAYSIZE(szDate))) && wcslen(szDate) > 0)
                {
                    title = L"Photo_";
                    title += szDate;
                }
            }
            else
            {
                // Fallback 2: System.Media.DateEncoded (Video/Audio)
                PropVariantClear(&propVar);
                if (SUCCEEDED(pItem->GetProperty(PKEY_Media_DateEncoded, &propVar)) && propVar.vt != VT_EMPTY)
                {
                    wchar_t szDate[100];
                    if (SUCCEEDED(PropVariantToString(propVar, szDate, ARRAYSIZE(szDate))) && wcslen(szDate) > 0)
                    {
                        title = L"Media_";
                        title += szDate;
                    }
                }
            }
        }
        
        PropVariantClear(&propVar);
        pItem->Release();
    }
    
    // Validate Property System result: if it's just spaces or invalid chars, clear it to trigger text fallback
    if (!title.empty())
    {
        std::wstring testTitle = SanitizeName(title);
        if (testTitle.empty() || testTitle.find_first_not_of(L"_") == std::wstring::npos)
        {
            title = L"";
        }
    }
    
    // Text-based Fallback Extractors
    if (title.empty())
    {
        std::wstring ext = GetExtension(path);
        std::wstring extLower = ext;
        for (auto& c : extLower) c = towlower(c);

        HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            char buf[16384] = {0}; // Increased to 16KB to catch headers further down
            DWORD bytesRead;
            if (ReadFile(hFile, buf, sizeof(buf)-1, &bytesRead, NULL))
            {
                std::string content(buf);
                std::string matchedTitle = "";

                // Explicitly strip UTF-8 BOM if present at the absolute start
                if (content.size() >= 3 && (unsigned char)content[0] == 0xEF && (unsigned char)content[1] == 0xBB && (unsigned char)content[2] == 0xBF) {
                    content = content.substr(3);
                }

                if (extLower == L".htm" || extLower == L".html" || extLower == L".xml" || extLower == L".svg")
                {
                    std::smatch match;
                    std::regex titleRegex("<title[^>]*>([^<]*)</title>", std::regex_constants::icase);
                    if (std::regex_search(content, match, titleRegex) && match.size() > 1)
                        matchedTitle = match[1].str();
                }
                
                if (matchedTitle.empty() && (extLower == L".md" || extLower == L".mdx"))
                {
                    std::smatch match;
                    std::regex mdRegex("(?:^|\\n)#\\s+([^\\n\\r]+)");
                    if (std::regex_search(content, match, mdRegex) && match.size() > 1)
                        matchedTitle = match[1].str();
                    else 
                    {
                        std::regex fmRegex("(?:^|\\n)(?:name|title)\\s*:\\s*([^\\n\\r]+)", std::regex_constants::icase);
                        if (std::regex_search(content, match, fmRegex) && match.size() > 1)
                            matchedTitle = match[1].str();
                    }
                }
                
                if (matchedTitle.empty() && (extLower == L".json" || extLower == L".jsonc"))
                {
                    std::smatch match;
                    std::regex jsonRegex("\"(?:name|title|id|project_id)\"\\s*:\\s*\"([^\"]+)\"", std::regex_constants::icase);
                    if (std::regex_search(content, match, jsonRegex) && match.size() > 1)
                        matchedTitle = match[1].str();
                }
                
                if (matchedTitle.empty() && (extLower == L".csv" || extLower == L".tsv"))
                {
                    size_t newlinePos = content.find_first_of("\r\n");
                    if (newlinePos != std::string::npos)
                        matchedTitle = content.substr(0, newlinePos);
                }
                
                if (matchedTitle.empty())
                {
                    static const std::unordered_set<std::wstring> devExts = {
                        L".txt", L".py", L".js", L".ts", L".java", L".c", L".cpp", L".h", L".hpp", L".cs", L".rb", L".go", 
                        L".php", L".swift", L".kt", L".rs", L".dart", L".scala", L".groovy", L".lua", L".pl", L".pm", L".sh", 
                        L".bat", L".cmd", L".ps1", L".vbs", L".sql", L".yaml", L".yml", L".ini", L".cfg", L".conf", L".env", 
                        L".log", L".css", L".scss", L".sass", L".less", L".vue", L".jsx", L".tsx", L".svelte", L".graphql", 
                        L".toml", L".properties", L".gradle", L".asm", L".s", L".m", L".mm", L".r", L".jl", L".nim", L".zig", 
                        L".v", L".fs", L".fsi", L".fsx", L".ml", L".mli", L".erl", L".hrl", L".ex", L".exs", L".clj", L".cljs", 
                        L".edn", L".el", L".lisp", L".lsp", L".scm", L".rkt", L".awk", L".sed", L".tex", L".sty", L".cls", 
                        L".bib", L".rst", L".adoc", L".asciidoc", L".rtf", L".srt", L".vtt", L".cue", L".pls", L".m3u", 
                        L".m3u8", L".nfo", L".diz", L".me", L".1st", L".bak", L".old", L".orig", L".patch", L".diff", L".po", 
                        L".pot", L".xlf", L".xliff", L".resx", L".str", L".strings", L".tf", L".tfvars", L".hcl", L".nomad", 
                        L".sln", L".csproj", L".vbproj", L".fsproj", L".vcxproj", L".mk", L".mak", L".dsp", L".dsw", L".pro", 
                        L".pri", L".gyp", L".gypi", L".bp", L".bazel", L".bzl",
                        L".htm", L".html", L".xml", L".svg", L".md", L".mdx", L".json", L".jsonc", L".csv", L".tsv",
                        L".twb", L".twbx", L".pbix", L".pbit", L".qvw", L".qvf", L".yxmd", L".yxmc", L".dmn", L".bpmn", 
                        L".drawio", L".vsdx", L".mpp", L".dtsx", L".ipynb"
                    };

                    if (devExts.count(extLower) > 0)
                    {
                        // Universal line-by-line fallback
                        size_t pos = 0;
                        while (pos < content.length())
                        {
                            size_t nextPos = content.find_first_of("\n\r", pos);
                            std::string line = (nextPos == std::string::npos) ? content.substr(pos) : content.substr(pos, nextPos - pos);
                            
                            // Strip comments at the start of the line
                            std::regex commentRegex("^(?:\\s*(?:\\/\\/|#|\\/\\*|<!--|--|///|rem\\s|'|\\*)\\s*)?", std::regex_constants::icase);
                            line = std::regex_replace(line, commentRegex, "");
                            
                            // Trim whitespace
                            size_t first = line.find_first_not_of(" \t");
                            if (first != std::string::npos) line = line.substr(first);
                            else line = "";
                            
                            size_t last = line.find_last_not_of(" \t");
                            if (last != std::string::npos) line = line.substr(0, last + 1);
                            
                            // Skip pure symbol lines (e.g. -----, ====)
                            std::regex symbolRegex("^[-=_*]+$");
                            if (std::regex_match(line, symbolRegex)) {
                                pos = (nextPos == std::string::npos) ? content.length() : nextPos + 1;
                                continue;
                            }
                            
                            // Skip common coding keywords or HTML tags
                            std::regex keywordRegex("^(?:using|import|include|package|namespace|def|class|public|private|var|const|let)\\b|^<[/a-zA-Z]", std::regex_constants::icase);
                            if (std::regex_search(line, keywordRegex)) {
                                pos = (nextPos == std::string::npos) ? content.length() : nextPos + 1;
                                continue;
                            }
                            
                            // Strip "File:" prefix if present
                            if (line.size() > 5 && (line.substr(0, 5) == "File:" || line.substr(0, 5) == "file:")) {
                                line = line.substr(5);
                                first = line.find_first_not_of(" \t");
                                if (first != std::string::npos) line = line.substr(first);
                            }
                            
                            // Does it have at least 3 letters?
                            std::regex alphaRegex("[a-zA-Z]{3,}");
                            if (std::regex_search(line, alphaRegex))
                            {
                                // Remove everything except alphanumerics, spaces, dots, hyphens, underscores
                                std::regex cleanRegex("[^a-zA-Z0-9 _.-]");
                                line = std::regex_replace(line, cleanRegex, "");
                                
                                first = line.find_first_not_of(" \t");
                                if (first != std::string::npos) line = line.substr(first);
                                last = line.find_last_not_of(" \t");
                                if (last != std::string::npos) line = line.substr(0, last + 1);
                                
                                if (line.length() >= 3)
                                {
                                    matchedTitle = line;
                                    break;
                                }
                            }
                            
                            if (nextPos == std::string::npos) break;
                            pos = nextPos + 1;
                            
                            // Don't scan more than 50 lines to keep performance high
                            if (pos > 4000) break;
                        }
                    }
                    else if (extLower == L".pdf" || extLower == L".docx" || extLower == L".png" || extLower == L".jpg" || extLower == L".jpeg" || extLower == L".bmp" || extLower == L".gif" || extLower == L".tiff" || extLower == L".webp")
                    {
                        // Advanced fallback: Extract visual text from PDF/DOCX/Images using dynamic Python
                        wchar_t dllPath[MAX_PATH];
                        if (GetModuleFileNameW(GetModuleHandleW(L"AutoRename.dll"), dllPath, MAX_PATH))
                        {
                            std::wstring exePath = std::wstring(dllPath);
                            // Navigate from build/Release/AutoRename.dll to src/
                            std::wstring scriptName;
                            if (extLower == L".pdf") scriptName = L"pdf_ext.py";
                            else if (extLower == L".docx") scriptName = L"docx_ext.py";
                            else scriptName = L"img_ext.py";
                            
                            exePath = exePath.substr(0, exePath.find_last_of(L"\\/")) + L"\\..\\..\\src\\" + scriptName;
                            
                            std::wstring cmdLine = L"cmd.exe /c python \"" + exePath + L"\" \"" + path + L"\"";
                            
                            SECURITY_ATTRIBUTES sa;
                            sa.nLength = sizeof(sa);
                            sa.bInheritHandle = TRUE;
                            sa.lpSecurityDescriptor = NULL;
                            
                            HANDLE hRead, hWrite;
                            if (CreatePipe(&hRead, &hWrite, &sa, 0))
                            {
                                SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
                                
                                STARTUPINFOW si = {sizeof(si)};
                                si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                                si.hStdOutput = hWrite;
                                si.hStdError = hWrite;
                                si.wShowWindow = SW_HIDE;
                                
                                PROCESS_INFORMATION pi;
                                std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
                                cmdBuf.push_back(L'\0');
                                
                                if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
                                {
                                    CloseHandle(hWrite);
                                    char pipeBuf[1024];
                                    DWORD bytesRead;
                                    std::string output;
                                    while (ReadFile(hRead, pipeBuf, sizeof(pipeBuf) - 1, &bytesRead, NULL) && bytesRead > 0)
                                    {
                                        pipeBuf[bytesRead] = '\0';
                                        output += pipeBuf;
                                    }
                                    CloseHandle(hRead);
                                    DWORD waitRes = WaitForSingleObject(pi.hProcess, 5000);
                                    if (waitRes == WAIT_TIMEOUT) {
                                        TerminateProcess(pi.hProcess, 1);
                                    }
                                    CloseHandle(pi.hProcess);
                                    CloseHandle(pi.hThread);
                                    
                                    if (!output.empty())
                                    {
                                        size_t pos = output.find("$$RESULT$$:");
                                        if (pos != std::string::npos) {
                                            output = output.substr(pos + 11);
                                        }
                                        size_t end = output.find_last_not_of("\r\n");
                                        if (end != std::string::npos) output = output.substr(0, end + 1);
                                        matchedTitle = output;
                                    }
                                }
                                else
                                {
                                    CloseHandle(hWrite);
                                    CloseHandle(hRead);
                                }
                            }
                        }
                    }
                }

                if (!matchedTitle.empty())
                {
                    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, matchedTitle.c_str(), -1, NULL, 0);
                    if (wchars_num > 0)
                    {
                        wchar_t* wstr = new wchar_t[wchars_num];
                        MultiByteToWideChar(CP_UTF8, 0, matchedTitle.c_str(), -1, wstr, wchars_num);
                        title = wstr;
                        delete[] wstr;
                    }
                }
            }
            CloseHandle(hFile);
        }
    }
    
    return title;
}

std::wstring ContextExtractor::FallbackName(const std::wstring& path)
{
    // Basic fallback: just use current timestamp to guarantee uniqueness
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[100];
    wsprintfW(buf, L"File_%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buf);
}

std::wstring ContextExtractor::GetNewNameForFile(const std::wstring& originalPath)
{
    std::wstring ext = GetExtension(originalPath);
    std::wstring dir = GetDirectory(originalPath);
    
    std::wstring baseName = ExtractTitleFromPropertySystem(originalPath);
    
    if (baseName.empty())
    {
        baseName = FallbackName(originalPath);
    }
    
    baseName = SanitizeName(baseName);
    
    // If sanitization resulted in an empty string (or purely invalid chars), force a timestamp fallback
    if (baseName.empty() || baseName.find_first_not_of(L"_") == std::wstring::npos)
    {
        baseName = FallbackName(originalPath);
        baseName = SanitizeName(baseName);
    }
    
    // Construct final path
    std::wstring finalPath = dir + baseName + ext;
    
    // Prevent self-rename which causes file deletion or errors
    if (finalPath == originalPath)
    {
        return L"";
    }
    
    // Handle duplicates (append (1), (2), etc)
    int counter = 1;
    while (GetFileAttributesW(finalPath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        wchar_t buf[256];
        wsprintfW(buf, L"%s (%d)%s", (dir + baseName).c_str(), counter++, ext.c_str());
        finalPath = buf;
    }
    
    return finalPath;
}
