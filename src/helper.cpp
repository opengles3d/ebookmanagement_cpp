
#include <filesystem>
#include <iostream>
#include <fstream>

#include <windows.h>


#include "../include/helper.h"
#include "../include/SHA256.h"

namespace fs = std::filesystem;

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}

// 将 UTF - 8 编码的 std::u8string 转换为 std::wstring
static std::wstring utf8_to_wstring(const std::u8string& u8str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(u8str.c_str()), -1, nullptr, 0) - 1;//remove '\0'
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(u8str.c_str()), -1, &wstr[0], size_needed);
    return wstr;
}

// 将 std::wstring 转换为 std::string
static std::string wstring_to_string(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr) - 1;//remove '\0'
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
    return str;
}

// 将 UTF-8 编码的 std::u8string 转换为 std::string
static std::string u8string_to_string(const std::u8string& u8str) {
    std::wstring wstr = utf8_to_wstring(u8str);
    return wstring_to_string(wstr);
}


void list(std::string dir, std::function<void(std::string)> processor) {
    if (!fs::exists(dir)) {
        std::cout << "Not exists: " << dir << std::endl;
        return;
    }
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) {
            list(entry.path().string(), processor);
        }
        else {
            try {
                auto u8name = entry.path().generic_u8string();
                std::string name = u8string_to_string(u8name);
                //std::cout << name << std::endl;
                //std::string name = entry.path().generic_string();
                processor(name);
            }
            catch (std::exception& e) {
                std::cout << "Exception:" << e.what() << std::endl;
            }
        }
    }
}

std::string sha256file(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return "";
    }

    SHA256 sha256;
    const int bufSize = 32768;
    char* buffer = new char[bufSize];
    while (file.good()) {
        file.read(buffer, bufSize);
        sha256.update((uint8_t*)buffer, file.gcount());
    }

    std::unique_ptr<uint8_t[]> digest{ sha256.digest() };
    auto r = SHA256::toString(digest.get());
    return r;
}


void renameFile(std::string name, std::string str_toremove) {

    try {

         if (name.find(str_toremove) != std::string::npos) {
              auto newname = name;
              replace(newname, str_toremove, "");
              std::cout << name << std::endl;
              std::cout << newname << std::endl;
              fs::rename(name, newname);
          }
        
    }
    catch (fs::filesystem_error& e) {
        std::cout << e.what() << std::endl;
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

void deleteFile(std::string name) {
    fs::remove(name);
}