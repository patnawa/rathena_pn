// Runtime comparison identity. Verification happens once before server readiness;
// reload paths invalidate it until a verified restart. Lab calls never hash files.
#ifndef PN_RUNTIME_IDENTITY_HPP
#define PN_RUNTIME_IDENTITY_HPP
#include <string>
namespace pn_runtime_identity {
inline std::string verified;
inline void invalidate() { verified.clear(); }
inline const std::string& current() { return verified; }
}
#ifndef _WIN32
#include <openssl/evp.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
namespace pn_runtime_identity {
inline std::string hash_file(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);if(!file)return {};
    EVP_MD_CTX* context=EVP_MD_CTX_new();if(!context)return {};
    bool ok=EVP_DigestInit_ex(context,EVP_sha256(),nullptr)==1;
    char buffer[65536];while(ok && file){file.read(buffer,sizeof(buffer));if(file.gcount())ok=EVP_DigestUpdate(context,buffer,file.gcount())==1;}
    unsigned char digest[EVP_MAX_MD_SIZE];unsigned length=0;
    ok=ok && file.eof() && EVP_DigestFinal_ex(context,digest,&length)==1;
    EVP_MD_CTX_free(context);if(!ok || length!=32)return {};
    const char* hex="0123456789abcdef";std::string value;for(unsigned i=0;i<length;++i){value+=hex[digest[i]>>4];value+=hex[digest[i]&15];}return value;
}
inline bool initialize(const std::filesystem::path& root=".") {
    invalidate();
    try {
        const std::string manifest="conf/import/pn_runtime_identity";
        std::ifstream file(root/manifest);std::string line;
        if(!std::getline(file,line) || line!="pn-runtime-v1")return false;
        if(!std::getline(file,line) || line.size()!=71 || line.substr(0,7)!="source ")return false;
        std::set<std::string> declared;
        while(std::getline(file,line)) {
            if(line.size()<66 || line[64]!=' ')return false;
            const auto hash=line.substr(0,64),name=line.substr(65);std::filesystem::path relative(name);
            if(relative.is_absolute() || name==manifest || name.find('\\')!=std::string::npos || !declared.insert(name).second)return false;
            for(const auto& part:relative)if(part==".." || part==".")return false;
            if(name!="map-server" && name.rfind("db/",0) && name.rfind("npc/",0) && name.rfind("conf/",0))return false;
            if(std::filesystem::is_symlink(root/relative) || hash_file(root/relative)!=hash)return false;
            if(name=="map-server" && hash_file("/proc/self/exe")!=hash)return false;
        }
        if(!file.eof() || !declared.count("map-server"))return false;
        std::set<std::string> actual={"map-server"};
        for(const char* dir:{"db","npc","conf"}) {
            if(!std::filesystem::is_directory(root/dir))return false;
            for(const auto& entry:std::filesystem::recursive_directory_iterator(root/dir)) {
                if(entry.is_symlink())return false;
                if(entry.is_regular_file()) {
                    auto name=std::filesystem::relative(entry.path(),root).generic_string();
                    if(name!=manifest)actual.insert(name);
                }
            }
        }
        if(actual!=declared)return false;
        verified=hash_file(root/manifest);return !verified.empty();
    } catch(...) { invalidate();return false; }
}
}
#else
namespace pn_runtime_identity { inline bool initialize() { invalidate();return false; } }
#endif
#endif
