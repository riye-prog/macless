#pragma once
#include <string>
#include <filesystem>
namespace toolscreen {
std::filesystem::path filePath(const std::string& path);
std::string pathText(const std::filesystem::path& path);
bool replaceFile(const std::string& temporary,const std::string& destination);
}
