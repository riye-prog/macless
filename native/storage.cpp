#include "storage.hpp"
#include <cstdio>
#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace toolscreen {
std::filesystem::path filePath(const std::string& path){return std::filesystem::path(std::u8string(path.begin(),path.end()));}
std::string pathText(const std::filesystem::path& path){auto value=path.u8string();return std::string(value.begin(),value.end());}
bool replaceFile(const std::string& temporary,const std::string& destination){
#ifdef _WIN32
    return MoveFileExW(filePath(temporary).c_str(),filePath(destination).c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return std::rename(temporary.c_str(),destination.c_str())==0;
#endif
}
}
