#pragma once

#include <string>
#include <functional>

void list(std::string dir, std::function<void(std::string)> processor);
std::string sha256file(std::string filename);
void renameFile(std::string name, std::string str_toremove);
void deleteFile(std::string name);