#include <iostream>
#include <string>
#include <regex>

int main() {
    std::string outputStr = "[V:onnxruntime... ] Found total 4 core(s)...\nRESULT:Photo of Analog Clock";
    size_t pos = outputStr.find("RESULT:");
    if (pos != std::string::npos) {
        outputStr = outputStr.substr(pos + 11);
    }
    outputStr = std::regex_replace(outputStr, std::regex("^\\s+|\\s+$"), ""); // trim
    
    std::cout << outputStr << std::endl;
}
