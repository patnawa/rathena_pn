// Isolated reproduction driver; generated header/functions are exact source excerpts.
#include "reviewed_reputation.hpp"
#include <common/malloc.hpp>
#include <common/showmsg.hpp>
#include <nlohmann/json.hpp>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

using json=nlohmann::json;
char db_path[12]="db";
static char test_name[]="reputation-database-reproduction";
char* SERVER_NAME=test_name;
// Provenance strings used only by the real allocator's leak logger.
const char* get_git_hash(){return "isolated-source-reproduction";}
const char* get_svn_revision(){return "";}
static unsigned assertions=0, cases=0;
static std::vector<std::string> errors;
static bool expected_errors=false;
static json findings=json::array();
void require(bool value,const std::string& what){++assertions;if(!value)throw std::runtime_error(what);}
int32 _vShowMessage(msg_type flag,const char* format,va_list ap){
    char text[8192];vsnprintf(text,sizeof(text),format,ap);
    if(flag==MSG_ERROR||flag==MSG_WARNING||flag==MSG_FATALERROR){
        errors.emplace_back(text);
        if(!expected_errors||std::string(text).find("Memory manager:")!=std::string::npos){std::cerr<<"UNEXPECTED_NATIVE_DIAGNOSTIC: "<<text;std::exit(90);}
    }else if(std::string(text).find("Memory manager: No memory leaks found.")!=std::string::npos)std::cout<<text;
    return 0;
}
#define LOG_FN(name,level) void name(const char* f,...){va_list ap;va_start(ap,f);_vShowMessage(level,f,ap);va_end(ap);}
LOG_FN(ShowMessage,MSG_NONE) LOG_FN(ShowStatus,MSG_STATUS) LOG_FN(ShowInfo,MSG_INFORMATION)
LOG_FN(ShowError,MSG_ERROR) LOG_FN(ShowWarning,MSG_WARNING) LOG_FN(ShowFatalError,MSG_FATALERROR)

void deny_network(){
    sock_filter filter[]={BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(seccomp_data,nr)),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_socket,4,0),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_connect,3,0),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_bind,2,0),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_listen,1,0),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)};
    sock_fprog program{static_cast<unsigned short>(sizeof(filter)/sizeof(*filter)),filter};
    require(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)==0,"no-new-privileges");
    require(prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program)==0,"kernel network denial");
    require(socket(AF_INET,SOCK_STREAM,0)==-1&&errno==EPERM,"socket actually denied");
}
std::string read(const std::string& path){std::ifstream f(path,std::ios::binary);require(f.good(),"required fixture: "+path);return {std::istreambuf_iterator<char>(f),{}};}
json snapshot(const std::shared_ptr<s_reputation>& r){
    if(!r)return nullptr;
    json result={{"Id",r->id},{"Name",r->name},{"Variable",r->variable},{"Minimum",r->minimum},{"Maximum",r->maximum}};
#ifdef MAP_GENERATOR
    result["Visibility"]=r->visibility;
#endif
    return result;
}
uint64 parse(ReputationDatabase& db,const std::string& text,bool reject=false,const std::string& diagnostic=""){
    ++cases;auto tree=ryml::parse_in_arena(ryml::to_csubstr(text));errors.clear();expected_errors=reject;
    auto result=db.parseBodyNode(tree.rootref());expected_errors=false;
    if(reject){
        require(result==0,"actual parser rejects invalid row");
        require(!errors.empty(),"native rejection diagnostic exists");
        bool found=false;for(auto& error:errors){if(error.find(diagnostic)!=std::string::npos)found=true;
            require(error.find(diagnostic)!=std::string::npos||error.find("Occurred in file")!=std::string::npos||error.find("At least one mandatory node")!=std::string::npos,"only exact scoped rejection diagnostics");}
        require(found,"expected specific native diagnostic: "+diagnostic);
    }else require(result==1&&errors.empty(),"actual parser accepts reviewed row cleanly");
    return result;
}
std::string row(int id=100){return "Id: "+std::to_string(id)+"\nName: Original\nVariable: RepOriginal\nMinimum: -100\nMaximum: 100\n";}
void note(const std::string& issue,const json& evidence){findings.push_back({{"classification","CONFIRMED_CURRENT_BEHAVIOR"},{"issue",issue},{"evidence",evidence}});}
void current_database(){
    require(reputation_db.load(),"actual full ordered reputation import load");
    require(reputation_db.size()==13,"all thirteen current effective definitions");
    auto expected=json::parse(read("expected.json"));
    for(auto& entry:expected){auto r=reputation_db.find(entry["Id"].get<int64>());require(r!=nullptr,"current ID loaded");
        for(auto key:{"Id","Name","Variable","Minimum","Maximum"})require(snapshot(r)[key]==entry[key],std::string("exact current field ")+key);
        require(r->minimum<=r->maximum&&r->minimum!=INT64_MIN&&r->maximum!=INT64_MIN,"current bounds unaffected by synthetic defects");
#ifdef MAP_GENERATOR
        int visibility=entry.value("Visibility",std::string("Always"))=="Always"?0:entry["Visibility"]=="Never"?1:2;
        require(r->visibility==visibility,"native ordered generator visibility overrides");
#endif
    }
    ++cases;
}
void parser_cases(){
    ReputationDatabase db;
    parse(db,"Id: 100\nName: Default\nVariable: RepDefault\nMinimum: -5\n");
    require(db.find(100)->maximum==INT64_MIN,"reproduce omitted Maximum actual INT64_MIN");
    note("omitted_Maximum_uses_wrong_documented_default",snapshot(db.find(100)));
    parse(db,"Id: 101\nName: Default\nVariable: RepDefault\n");
    require(db.find(101)->minimum==INT64_MIN&&db.find(101)->maximum==INT64_MIN,"both omitted bounds current behavior");
    parse(db,"Id: 102\nName: Default\nVariable: RepDefault\nMaximum: 5\n");
    require(db.find(102)->minimum==INT64_MIN&&db.find(102)->maximum==5,"omitted Minimum documented default");
    parse(db,"Id: 103\nName: MissingVariable\nMinimum: -1\nMaximum: 1\n",true,"Missing mandatory node \"Variable\"");
    require(!db.find(103),"rejected new missing Variable never inserted");
    note("documented_Variable_default_not_implemented",{{"Id",103},{"return",0},{"inserted",false}});
    parse(db,row(110));auto original=db.find(110);auto before=snapshot(original);
    parse(db,"Id: 110\nName: Renamed\n");
    require(db.find(110)==original&&original->name=="Renamed"&&original->variable=="RepOriginal"&&original->minimum==-100&&original->maximum==100,"valid Name-only override preserves all omitted fields");
    parse(db,"Id: 110\nMinimum: -50\n");parse(db,"Id: 110\nMaximum: 50\n");
    require(original->minimum==-50&&original->maximum==50,"valid independent bound overrides");
    parse(db,"Id: 110\nName: PartiallyChanged\nVariable: RepChanged\nMinimum: -25\nMaximum: invalid\n",true,"Node \"Maximum\" cannot be parsed");
    require(db.find(110)==original&&original->name=="PartiallyChanged"&&original->variable=="RepChanged"&&original->minimum==-25&&original->maximum==50,"reproduce rejection leaves earlier mutations in existing object");
    note("rejected_existing_override_is_not_atomic",{{"before",before},{"after",snapshot(original)},{"return",0}});
    parse(db,"Id: 110\nName: ChangedBeforeVariableRejection\nVariable: abcdefghijklmnopqrstuvwxyz1234567XX\n",true,"exceeds maximum length 32");
    require(original->name=="ChangedBeforeVariableRejection"&&original->variable=="RepChanged","Name changed before invalid Variable refusal");
    parse(db,"Id: 120\nName: Equal\nVariable: RepEqual\nMinimum: 5\nMaximum: 5\n");
    parse(db,"Id: 121\nName: Inverted\nVariable: RepInverted\nMinimum: 5\nMaximum: 4\n");
    require(db.find(121)->minimum>db.find(121)->maximum,"reproduce inverted bounds accepted");
    note("inverted_bounds_accepted",snapshot(db.find(121)));
    parse(db,"Id: 122\nName: Invalid\nVariable: RepInvalid\nMaximum: invalid\n",true,"Node \"Maximum\" cannot be parsed");
    require(!db.find(122),"rejected new invalid bound never inserted");
    for(auto value:{"9223372036854775808","-9223372036854775809"}){
        parse(db,"Id: 123\nName: Overflow\nVariable: RepOverflow\nMaximum: "+std::string(value)+"\n",true,"Node \"Maximum\" cannot be parsed");
        require(!db.find(123),"out-of-int64 new row not inserted");
    }
#ifdef MAP_GENERATOR
    parse(db,row(130));auto v=db.find(130);require(v->visibility==s_reputation::ALWAYS,"native default visibility");
    parse(db,"Id: 130\nVisibility: Exist\n");require(v->minimum==-100&&v->maximum==100&&v->visibility==s_reputation::EXIST,"visibility-only override preserves bounds");
    parse(db,"Id: 130\nName: ChangedBeforeVisibilityRejection\nMaximum: 90\nVisibility: Invalid\n",true,"Visibility \"Invalid\" unknown");
    require(v->name=="ChangedBeforeVisibilityRejection"&&v->maximum==90&&v->visibility==s_reputation::EXIST,"generator rejection also leaves prior changes");
    note("rejected_generator_visibility_override_is_not_atomic",snapshot(v));
#endif
}
void pc_reputation_generate();
int main(int argc,char** argv){
    try{
        require(argc==2,"explicit isolated test mode");deny_network();std::filesystem::create_directories("log");malloc_init();do_init_database();
        std::string mode=argv[1];
#ifdef MAP_GENERATOR
        if(mode=="minimum_ub"||mode=="maximum_ub"){
            parse(reputation_db,"Id: 999\nName: Overflow\nVariable: RepOverflow\nMinimum: "+std::string(mode=="minimum_ub"?"-9223372036854775808":"-1")+"\nMaximum: "+std::string(mode=="maximum_ub"?"-9223372036854775808":"1")+"\n");
            std::cout<<"EXPECT_GENERATOR_"<<mode<<std::endl;pc_reputation_generate();
            throw std::runtime_error("expected UBSan generator failure did not occur");
        }
#endif
        require(mode=="current","known nonfailing reproduction mode");current_database();parser_cases();
#ifdef MAP_GENERATOR
        require(reputationgroup_db.load(),"native ordered reputation group load");
        std::filesystem::create_directories("generated/clientside/data/contentdata");
        pc_reputation_generate();
        auto bytes=read("generated/clientside/data/contentdata/reputeinfodata.bson");
        auto decoded=json::from_bson(bytes.begin(),bytes.end());require(decoded["reputeInfo"].size()==13,"actual generator BSON contains current13 entries");
        for(auto& p:reputation_db){auto node=decoded["reputeInfo"][std::to_string(p.first)];require(node["Name"]==p.second->name&&node["MaxPoint_Negative"]==std::abs(p.second->minimum)&&node["MaxPoint_Positive"]==std::abs(p.second->maximum),"actual current BSON roundtrip fields");}
        reputation_db.clear();parse(reputation_db,"Id: 999\nName: Representable\nVariable: RepMax\nMinimum: -9223372036854775807\nMaximum: 9223372036854775807\n");
        pc_reputation_generate();bytes=read("generated/clientside/data/contentdata/reputeinfodata.bson");decoded=json::from_bson(bytes.begin(),bytes.end());
        require(decoded["reputeInfo"]["999"]["MaxPoint_Negative"]==INT64_MAX&&decoded["reputeInfo"]["999"]["MaxPoint_Positive"]==INT64_MAX,"actual largest representable magnitude BSON roundtrip");
        bool rejected=false;try{json unsupported={{"magnitude",uint64(1)<<63}};auto unused=json::to_bson(unsupported);(void)unused;}catch(const json::out_of_range& error){require(error.id==407&&std::string(error.what()).find("9223372036854775808 cannot be represented by BSON")!=std::string::npos,"exact bundled BSON unsigned overflow diagnostic");rejected=true;}
        require(rejected,"bundled BSON rejects unsigned 2^63");note("unsigned_INT64_MIN_magnitude_not_BSON_representable",{{"exception_id",407},{"magnitude","9223372036854775808"}});
#endif
        reputation_db.clear();reputationgroup_db.clear();malloc_final();
        std::cout<<"REPUTATION_CURRENT_REPRODUCTION_COMPLETE "<<json({{"cases",cases},{"assertions",assertions},{"findings",findings}}).dump()<<std::endl;
        return 0;
    }catch(const std::exception& error){std::cerr<<"REPRODUCTION_FAILURE: "<<error.what()<<std::endl;return 1;}
}
