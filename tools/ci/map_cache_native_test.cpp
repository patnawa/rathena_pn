// Include the real implementation to exercise its private file-loading path.
// This is not a copied parser. Normal map-server main is never invoked.
#define main unused_map_server_main
#include "map_source.cpp"
#undef main

#include <cerrno>
#include <cstdarg>
#include <fstream>
#include <iterator>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <unistd.h>
#include <zlib.h>

namespace {
unsigned assertions=0, failures=0, cases=0, errors=0, records=0;
std::string current;
bool expected_file_error=false;
unsigned file_errors=0;
void check(bool ok,const char* message) {
    ++assertions;
    if (!ok) { ++failures; std::fprintf(stderr,"CACHE FAIL [%s]: %s\n",current.c_str(),message); }
}
void require(bool ok,const char* message) {
    if (!ok) { std::fprintf(stderr,"CACHE BOUNDARY: %s\n",message); std::exit(2); }
}
void deny_network() {
    sock_filter rules[]={
        BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(seccomp_data,nr)),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_socket,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_connect,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_bind,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_listen,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)};
    sock_fprog filter{static_cast<unsigned short>(std::size(rules)),rules};
    require(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)==0 && prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&filter)==0,"network denied");
    for (long call : {__NR_socket,__NR_connect,__NR_bind,__NR_listen})
        check(syscall(call,0,0,0)==-1 && errno==EPERM,"socket/connect/bind/listen denied");
}
template<class T> void append(std::vector<char>& bytes,const T& value) {
    const auto* p=reinterpret_cast<const char*>(&value); bytes.insert(bytes.end(),p,p+sizeof(value));
}
template<class T> void overwrite(std::vector<char>& bytes,size_t offset,const T& value) {
    require(offset+sizeof(value)<=bytes.size(),"fixture overwrite bounded"); std::memcpy(bytes.data()+offset,&value,sizeof(value));
}
std::vector<char> make_cache(const std::vector<unsigned char>& cells,int16 x=3,int16 y=3,const char* name="testmap") {
    unsigned long size=compressBound(cells.size()); std::vector<char> compressed(size);
    require(encode_zip(compressed.data(),&size,cells.data(),cells.size())==Z_OK,"fixture compression");
    compressed.resize(size);
    map_cache_main_header header{}; header.map_count=1; header.file_size=sizeof(header)+sizeof(map_cache_map_info)+size;
    map_cache_map_info info{}; std::strncpy(info.name,name,sizeof(info.name)-1); info.xs=x; info.ys=y; info.len=size;
    std::vector<char> bytes; append(bytes,header); append(bytes,info); bytes.insert(bytes.end(),compressed.begin(),compressed.end()); return bytes;
}
int read_cache(map_data& m,const char* bytes,size_t size,char* decoded,size_t capacity) {
#ifdef MAPCACHE_LEGACY_API
    return map_readfromcache(&m,const_cast<char*>(bytes),decoded);
#else
    return map_readfromcache(&m,bytes,size,decoded,capacity);
#endif
}
bool load_file(FILE* fp,std::vector<char>& output) {
#ifdef MAPCACHE_LEGACY_API
    char* bytes=map_init_mapcache(fp); if(!bytes)return false;
    std::fseek(fp,0,SEEK_END); const auto size=std::ftell(fp); output.assign(bytes,bytes+size); aFree(bytes); return true;
#else
    return map_init_mapcache(fp,output);
#endif
}
void rejected(const std::string& name,const std::vector<char>& bytes,size_t size=SIZE_MAX,size_t capacity=MAX_MAP_SIZE) {
    ++cases; current=name;
    map_data m{}; std::strcpy(m.name,"testmap"); m.xs=71; m.ys=73;
    std::vector<char> decoded(MAX_MAP_SIZE,0x5a);
    check(read_cache(m,bytes.data(),size==SIZE_MAX?bytes.size():size,decoded.data(),capacity)==0,"malformed cache rejected");
    check(m.xs==71 && m.ys==73 && m.cell==nullptr,"failure preserves map dimensions and allocation");
}
void synthetic() {
    auto valid=make_cache({0,1,2,3,4,5,6,0,3});
    for(size_t length=0;length<valid.size();++length)
        rejected("every-truncation-"+std::to_string(length),std::vector<char>(valid.begin(),valid.begin()+length));
    for(int32 length : {-2147483647-1,-1,0,2147483647}) { auto bytes=valid; overwrite(bytes,24,length); rejected("invalid-length-"+std::to_string(length),bytes); }
    for(int16 dimension : {-32768,-1,0,32767}) { auto bytes=valid; overwrite(bytes,20,dimension); rejected("invalid-width-"+std::to_string(dimension),bytes); overwrite(bytes,20,int16{3}); overwrite(bytes,22,dimension); rejected("invalid-height-"+std::to_string(dimension),bytes); }
    auto changed=valid; overwrite(changed,4,uint16{2}); rejected("count-beyond-file",changed);
    changed=valid; std::memset(changed.data()+8,'x',12); rejected("unterminated-record-name",changed);
    changed=valid; changed[8]=0; rejected("empty-record-name",changed);
    changed=valid; changed.back()^=1; rejected("bad-zlib-checksum",changed);
    changed=valid; changed[28]=0; rejected("bad-zlib-header",changed);
    changed=valid; changed.resize(changed.size()-1); overwrite(changed,24,static_cast<int32>(changed.size()-28)); rejected("truncated-zlib-stream-with-bounded-length",changed);
    rejected("decoded-too-short",make_cache({0,0,0,0,0,0,0,0}));
    rejected("decoded-too-long",make_cache({0,0,0,0,0,0,0,0,0,0}));
    rejected("insufficient-decode-capacity",valid,SIZE_MAX,8);
    rejected("zero-decode-capacity",valid,SIZE_MAX,0);
    rejected("unknown-gat-value",make_cache({0,0,0,0,0,0,0,0,255}));
    changed=valid; changed.push_back(0); rejected("undeclared-trailing-record-data",changed);
    changed=valid; overwrite(changed,4,uint16{0}); rejected("zero-record-count-with-extra-data",changed);
    // Two valid records, deliberately unaligned second header, exact geometry.
    auto first=make_cache({0,1,2,3,4,5,6,0,3},3,3,"first");
    while(first.size()%4==0) { first=make_cache(std::vector<unsigned char>(first.size(),0),1,first.size(),"first"); }
    auto two=first; two.insert(two.end(),valid.begin()+8,valid.end()); overwrite(two,4,uint16{2}); overwrite(two,0,static_cast<uint32>(two.size()));
    changed=two; overwrite(changed,24,int32{-1}); rejected("malformed-skipped-record-before-target",changed);
    // The selected record also cannot hide a malformed later record.
    auto tail=valid; tail.insert(tail.end(),first.begin()+8,first.end()); overwrite(tail,4,uint16{2});
    overwrite(tail,valid.size()+16,int32{-1}); rejected("malformed-record-after-target",tail);
    for(size_t displacement=0;displacement<4;++displacement) {
        ++cases; current="unaligned-valid-"+std::to_string(displacement);
        std::vector<char> storage(displacement); storage.insert(storage.end(),two.begin(),two.end());
        map_data m{}; std::strcpy(m.name,"testmap"); char decoded[MAX_MAP_SIZE];
        check(read_cache(m,storage.data()+displacement,two.size(),decoded,sizeof(decoded))==1,"unaligned header/record accepted safely");
        check(m.xs==3 && m.ys==3 && m.cell,"valid geometry committed");
        if(m.cell) { for(int i=0;i<9;++i) { check(m.cell[i].walkable==(i!=1 && i!=5),"all supported GAT walkability preserved"); } aFree(m.cell); }
    }
    ++cases; current="populated-map"; map_data populated{}; std::strcpy(populated.name,"testmap"); populated.xs=71; populated.ys=73;
    CREATE(populated.cell,mapcell,1); auto* original=populated.cell; char decoded[MAX_MAP_SIZE];
    check(read_cache(populated,valid.data(),valid.size(),decoded,sizeof(decoded))==0,"populated map rejected without replacement");
    check(populated.cell==original && populated.xs==71 && populated.ys==73,"existing allocation and dimensions preserved"); aFree(original);
    ++cases; current="absent-map"; map_data absent{}; std::strcpy(absent.name,"missing");
    check(read_cache(absent,valid.data(),valid.size(),decoded,sizeof(decoded))==0 && !absent.cell,"absent map rejected");
    for (uint32 historical_size : {0u,1u,8u,0xffffffffu}) {
        ++cases; current="stale-legacy-size-"+std::to_string(historical_size); changed=valid; overwrite(changed,0,historical_size);
        map_data m{}; std::strcpy(m.name,"testmap");
        check(read_cache(m,changed.data(),changed.size(),decoded,sizeof(decoded))==1,"actual supplied bytes, not legacy file_size, control parsing");
        if(m.cell) aFree(m.cell);
    }
    for (auto dimensions : {std::pair<int16,int16>{1,1},{512,512},{513,512}}) {
        ++cases; current="cell-limit-"+std::to_string(dimensions.first)+"x"+std::to_string(dimensions.second);
        auto cells=make_cache(std::vector<unsigned char>(size_t(dimensions.first)*dimensions.second,0),dimensions.first,dimensions.second);
        map_data m{}; std::strcpy(m.name,"testmap"); const bool allowed=size_t(dimensions.first)*dimensions.second<=MAX_MAP_SIZE;
        check(read_cache(m,cells.data(),cells.size(),decoded,sizeof(decoded))==allowed,"exact native maximum cell count enforced");
        check(allowed ? m.cell && m.xs==dimensions.first && m.ys==dimensions.second : !m.cell && m.xs==0 && m.ys==0,"limit case has atomic output");
        if(m.cell) aFree(m.cell);
    }
#ifndef MAPCACHE_LEGACY_API
    ++cases; current="invalid-arguments";
    check(map_readfromcache(nullptr,valid.data(),valid.size(),decoded,sizeof(decoded))==0,"null map rejected");
    check(map_readfromcache(&absent,nullptr,valid.size(),decoded,sizeof(decoded))==0,"null cache rejected");
    check(map_readfromcache(&absent,valid.data(),valid.size(),nullptr,sizeof(decoded))==0,"null output rejected");
    std::memset(absent.name,'x',sizeof(absent.name));
    check(map_readfromcache(&absent,valid.data(),valid.size(),decoded,sizeof(decoded))==0,"unterminated target name rejected");
#endif
}

// Real libc FILE streams with fault-injecting I/O callbacks, not parser stubs.
// They make failed seek/ftell/read paths deterministic without editing a file.
struct Cookie {
    std::vector<char> data;
    off64_t position=0, advertised=8;
    int mode=0;
    unsigned failed=0, closed=0;
};
ssize_t cookie_read(void* raw,char* destination,size_t size) {
    auto& c=*static_cast<Cookie*>(raw);
    if(c.mode==5) { ++c.failed; errno=EIO; return -1; }
    const size_t available=c.position>=static_cast<off64_t>(c.data.size()) ? 0 : c.data.size()-c.position;
    const size_t copied=std::min(available,size);
    if(copied) std::memcpy(destination,c.data.data()+c.position,copied);
    c.position+=copied; return copied;
}
int cookie_seek(void* raw,off64_t* offset,int origin) {
    auto& c=*static_cast<Cookie*>(raw);
    if((c.mode==1 && origin==SEEK_END) || (c.mode==2 && origin==SEEK_CUR) || (c.mode==3 && origin==SEEK_SET)) {
        ++c.failed; errno=EIO; return -1;
    }
    const off64_t target=*offset+(origin==SEEK_END ? c.advertised : origin==SEEK_CUR ? c.position : 0);
    if(target<0) { errno=EINVAL; return -1; }
    c.position=target; *offset=target; return 0;
}
int cookie_close(void* raw) { ++static_cast<Cookie*>(raw)->closed; return 0; }
void file_failures() {
#ifndef MAPCACHE_LEGACY_API
    for(int mode=0;mode<=7;++mode) {
        ++cases; current="stdio-fault-"+std::to_string(mode);
        Cookie cookie; cookie.mode=mode; cookie.data.resize(8);
        if(mode==0) cookie.advertised=0; // empty file
        if(mode==4) cookie.data.resize(7); // actual EOF before reported size
        if(mode==6) cookie.advertised=7; // truncated main header
        cookie_io_functions_t io{}; io.read=cookie_read; io.seek=cookie_seek; io.close=cookie_close;
        FILE* fp=fopencookie(&cookie,"rb",io); require(fp,"fault stream created");
        require(std::setvbuf(fp,nullptr,_IONBF,0)==0,"unbuffered fault stream");
        std::vector<char> output={'k','e','e','p'}; const auto before=output;
        expected_file_error=mode!=7; file_errors=0; const bool loaded=load_file(fp,output); expected_file_error=false;
        if(mode==7) {
            check(loaded && output==cookie.data && file_errors==0,"valid minimum empty-cache file loads");
        } else {
            check(!loaded && output==before,"failed file operation preserves existing output vector");
            check(file_errors==1,"one explicitly expected file-loading error recorded");
            if(mode==1 || mode==2 || mode==3 || mode==5) check(cookie.failed>0,"intended stdio failure actually reached");
        }
        std::fclose(fp); check(cookie.closed==1,"caller closes stream exactly once after success or failure");
    }
    ++cases; current="null-file"; std::vector<char> output={'k'};
    expected_file_error=true; file_errors=0; check(!load_file(nullptr,output),"null FILE rejected"); expected_file_error=false;
    check(output==std::vector<char>{'k'} && file_errors==1,"null FILE preserves output and reports error");
    std::printf("CACHE_STDIO expected_error_cases=8; real fopencookie seek/ftell/read failures exercised\n");
#endif
}
template<class T> T read_oracle(std::ifstream& input) { T value{}; input.read(reinterpret_cast<char*>(&value),sizeof(value)); require(input.good(),"complete independent geometry oracle"); return value; }
void actual_caches(const char* oracle_path) {
    std::ifstream oracle(oracle_path,std::ios::binary); require(oracle.good(),"independent oracle exists");
    for(const char* path : {"db/import/map_cache.dat","db/re/map_cache.dat","db/map_cache.dat"}) {
        FILE* fp=std::fopen(path,"rb"); require(fp,"actual cache opened read-only"); std::vector<char> bytes;
        require(load_file(fp,bytes),"actual private file loader succeeds"); std::fclose(fp);
        const uint32 count=read_oracle<uint32>(oracle);
        for(uint32 i=0;i<count;++i) {
            ++cases; ++records; map_data m{}; oracle.read(m.name,12); require(oracle.good(),"oracle name");
            current=std::string(path)+":"+m.name;
            const auto width=read_oracle<int16>(oracle),height=read_oracle<int16>(oracle); const auto length=read_oracle<uint32>(oracle);
            std::vector<unsigned char> expected(length); oracle.read(reinterpret_cast<char*>(expected.data()),length); require(oracle.good(),"oracle geometry");
            char decoded[MAX_MAP_SIZE];
            check(read_cache(m,bytes.data(),bytes.size(),decoded,sizeof(decoded))==1,"every real record loads from its full original cache");
            check(m.xs==width && m.ys==height && m.cell,"exact original dimensions");
            if(m.cell) {
                bool equal=true;
                for(size_t xy=0;xy<length;++xy) {
                    const auto gat=expected[xy]; const auto& cell=m.cell[xy];
                    equal &= cell.walkable==(gat!=1 && gat!=5) && cell.shootable==(gat!=1) && cell.water==(gat==3);
                }
                check(equal,"all decoded walkable/shootable/water fields equal independent Python zlib oracle");
                check(std::memcmp(decoded,expected.data(),length)==0,"all decoded GAT bytes preserved exactly");
                aFree(m.cell);
            }
        }
        std::printf("CACHE_ACTUAL file=%s records=%u bytes=%zu\n",path,count,bytes.size());
    }
    check(oracle.peek()==EOF,"independent oracle consumed exactly");
}
}
extern "C" void error_message(const char*,...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void error_message(const char* format,...) {
    if(expected_file_error && std::strncmp(format,"map_init_mapcache:",18)==0) { ++file_errors; return; }
    ++errors; va_list args; va_start(args,format); std::vfprintf(stderr,format,args); va_end(args);
}
extern "C" void warning_message(const char*,...) asm("__wrap__Z11ShowWarningPKcz");
extern "C" void warning_message(const char* format,...) { ++errors; va_list args; va_start(args,format); std::vfprintf(stderr,format,args); va_end(args); }
int main(int argc,char** argv) {
    require(argc==2,"oracle argument"); static char name[]="map-cache-native-test"; SERVER_NAME=name;
    deny_network(); malloc_init(); actual_caches(argv[1]); synthetic(); file_failures();
    check(errors==0,"no unexpected native error/warning diagnostics"); malloc_final();
    std::printf("MAP_CACHE_NATIVE_RESULT cases=%u records=%u assertions=%u failures=%u errors=%u\n",cases,records,assertions,failures,errors);
    return failures || errors ? 1 : 0;
}
