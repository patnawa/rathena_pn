"""Real weekly/reward script VM: alternate objectives, daily credit and replay fences.

Uses the Alice encounter harness, native script control/registries/instance arrays,
explicit world/clock/UI doubles. Durable material delivery is independently
proved by point_shop_native_test and point_asset_sql_cases.
"""
import argparse,json
from pathlib import Path
import alice_test as alice

CASES={
 'credit':'{ PNTestCredit=callfunc("PN_RewardClearCredit",PNTestTrack); end; }',
 'reset':'{ callfunc "PN_WeeklyReset"; end; }',
 'hunt':'{ callfunc "PN_WeeklyHunt",@huntmap$,@summoned; end; }',
 'explore':'{ callfunc "PN_WeeklyExplore",@exploremap$; end; }',
 'boss':'{ callfunc "PN_WeeklyBossComplete",@party; end; }',
 'board':'{ callfunc "PN_WeeklyBoard"; end; }',
 'reward':'{ callfunc "PN_RewardProgress"; PNTestAfterShop=1; end; }',
}
EXTRA=r'''
 auto reg=[&](const char* key){return registries[players[0]->id][key];};
 auto set=[&](const char* key,int64 value){registries[players[0]->id][key]=value;};
 auto script=[&](const std::string& text){auto* c=parse_script(text.c_str(),"weekly reward fixture",1,0);boundary(c!=nullptr,"fixture parses");run_script(c,0,players[0]->id,NPC);boundary(!players[0]->st,"fixture must end synchronously");script_free_code(c);};
 setup(1);clock_now=345600+3000LL*604800;set("PNWeeklyActive",1);finish("weekly-reset");
 auto week=reg("PNWeeklyWeek");check(week==3000,"Monday boundary computed in UTC");
 for(int i=0;i<25;++i)script("{callfunc \"PN_WeeklyHunt\",\"pay_fild01\",0;end;}");
 check(reg("PNWeeklyRegion")==25&&reg("PNWeeklyKills")==25,"rotating Payon route actually counts eligible field hunts");
 script("{callfunc \"PN_WeeklyHunt\",\"pay_fild01\",990001;end;}");
 script("{callfunc \"PN_WeeklyHunt\",\"pn_train\",0;end;}");
 check(reg("PNWeeklyKills")==25,"summoned slaves and training maps rejected");
 for(const char* city:{"payon","payon","alberta"})script(std::string("{callfunc \"PN_WeeklyExplore\",\"")+city+"\";end;}");
 check(reg("PNWeeklyExplore")==3,"repeated city visit cannot count as third destination");
 script("{callfunc \"PN_WeeklyExplore\",\"izlude\";callfunc \"PN_WeeklyBossComplete\",0;end;}");
 check(reg("PNWeeklyExplore")==7&&reg("PNWeeklyBoss")==1&&!reg("PNWeeklyParty"),"exploration and solo boss are independent alternatives");
 choices={1};finish("weekly-board");check(reg("PNWeeklyClaimed")==1&&reg("PNWeeklyStamps")==1,"three alternate objectives award cosmetic weekly stamp");
 choices.clear();finish("weekly-board");check(reg("PNWeeklyStamps")==1,"alternate stamp claim cannot repeat");
 clock_now+=604800;finish("weekly-reset");check(reg("PNWeeklyRegion")==0&&reg("PNWeeklyExplore")==0&&reg("PNWeeklyBoss")==0&&reg("PNWeeklyStamps")==1,"Monday clears alternate objectives and retains lifetime stamp");
 script("{callfunc \"PN_WeeklyHunt\",\"pay_fild01\",0;callfunc \"PN_WeeklyHunt\",\"gef_fild01\",0;end;}");
 check(reg("PNWeeklyRegion")==1,"next week rotates hunt to Geffen");
 script("{callfunc \"PN_WeeklyBossComplete\",1;end;}");check(reg("PNWeeklyParty")==1,"party clear earns optional social objective");
 // Credit requires the actual attached instance, authenticated roster, clear,
 // original reward guard and owning party. An ordinary field boss cannot earn.
 setup(1);clock_now=1900000000;set("PNTestTrack",1);finish("weekly-credit");check(reg("PNTestCredit")==0&&!reg("PNRewardAlicePoints"),"incomplete encounter has no guarantee credit");
 stage("'alice_complete",1);iv("'alice_eligible",100,1);iv("'alice_claimed",100,1);locations[players[0]->id].map="prontera";
 finish("weekly-credit");check(!reg("PNRewardAlicePoints"),"outside authentic boss map refused");
 locations[players[0]->id].map="1@alice_mad";players[0]->status.party_id=99;finish("weekly-credit");check(!reg("PNRewardAlicePoints"),"foreign party refused");
 players[0]->status.party_id=17;finish("weekly-credit");check(reg("PNTestCredit")==1&&reg("PNRewardAlicePoints")==1&&reg("PNRewardAliceClears")==1,"real eligible reward grants one persistent clear credit");
 auto saved=registries[players[0]->id];finish("weekly-credit");check(reg("PNTestCredit")==0&&reg("PNRewardAlicePoints")==1,"same encounter callback replay refused");
 iv("'pn_alice_credit",100,0);finish("weekly-credit");check(reg("PNRewardAlicePoints")==1,"new instance on same daily admission still refuses credit");
 clock_now+=86400;iv("'pn_alice_credit",100,1);finish("weekly-credit");check(reg("PNRewardAlicePoints")==1,"old cleared instance cannot pay again on next day");
 iv("'pn_alice_credit",100,0);finish("weekly-credit");check(reg("PNRewardAlicePoints")==2,"fresh next-day eligible clear progresses");
 registries[players[0]->id]=saved;finish("weekly-credit");check(reg("PNRewardAlicePoints")==1,"rehydrated saved character stays replay fenced by instance");
 // Bioresearch Story lacks the final boss and is deliberately excluded.
 set("PNTestTrack",2);locations[players[0]->id].map="1@gol2";stage("'bio_party",17);stage("'bio_zone",9);iv("'bio_eligible",100,1);iv("'bio_claimed",100,1);stage("'bio_mode",1);
 finish("weekly-credit");check(!reg("PNRewardBioPoints"),"Story record cannot mint boss credit");
 stage("'bio_mode",2);finish("weekly-credit");check(!reg("PNRewardBioPoints"),"Battle record without boss start refused");
 stage("'bio_boss_started",1);finish("weekly-credit");check(reg("PNRewardBioPoints")==1&&reg("PNRewardAlicePoints")==1,"Battle boss has its own character track");
 finish("weekly-credit");check(reg("PNRewardBioPoints")==1,"Battle credit replay refused");
 // The real VM fences every script before its first instruction while any
 // durable purchase is pending, even when that purchase uses a different key.
 setup(1);clock_now=1900000000;set("PNTestTrack",1);stage("'alice_complete",1);iv("'alice_eligible",100,1);iv("'alice_claimed",100,1);
 auto receipt=std::make_shared<pn_shop::Commit>();receipt->point.scope=pn_shop::CharacterPoint;std::strcpy(receipt->point.key,"DifferentPoints");
 players[0]->shop_commit.request=receipt;players[0]->shop_commit.pending=true;
 finish("weekly-credit");check(!reg("PNRewardAliceDay")&&!reg("PNRewardAlicePoints")&&!iv("'pn_alice_credit",100),"pending foreign purchase defers day, points and instance marker together");
 players[0]->shop_commit.pending=false;do_timer(gettick()+200);
 check(reg("PNRewardAliceDay")>0&&reg("PNRewardAlicePoints")==1&&iv("'pn_alice_credit",100)==1,"settled receipt resumes exactly one entire clear credit");
 players[0]->shop_commit={};receipt.reset();
 choices={1,1};finish("weekly-reward");check(!reg("PNTestAfterShop"),"Alice shop opening ends caller before next dialog");
 choices={2,1};finish("weekly-reward");check(!reg("PNTestAfterShop"),"Bio shop opening ends caller before next dialog");
 std::printf("WEEKLY_REWARDS_VM_OK alternate objectives, rotation, preserved stamps, authentic clears, relog and replay fences\n");
'''

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path);p.add_argument('--prepare-only',action='store_true');a=p.parse_args()
 original=alice.fixtures
 def fixtures(build,pre_fix=False):
  digest=original(build,pre_fix)
  header=build/'episode_cases.inc';source=header.read_text();entries=[]
  for name,body in CASES.items():
   path='weekly_'+name+'.script';(build/path).write_text(body);entries.append('{'+','.join([json.dumps('weekly-'+name),json.dumps(path),'""','0','false'])+'},')
  header.write_text(source.replace('};\nstruct Recipe', '\n'.join(entries)+'\n};\nstruct Recipe',1))
  return digest+source
 alice.fixtures=fixtures
 # Extended registry/array objectives initialize the player's array metadata;
 # the production session owns it, so this isolated fixture must free it too.
 cleanup='for(auto& p:players){if(p->regs.arrays){p->regs.arrays->destroy(p->regs.arrays,script_free_array_db);p->regs.arrays=nullptr;}if(p->regs.vars){script_free_vars(p->regs.vars);p->regs.vars=nullptr;}}'
 alice.MAIN=alice.MAIN.replace(' check(errors==0,"no native script or quest errors")',EXTRA+'\n '+cleanup+'\n check(errors==0,"no native script or quest errors")',1)
 # Rendering is an explicit boundary, matching the existing cosmetic harness.
 alice.WORLD=alice.WORLD.replace('    if(command=="select")','    if(command=="specialeffect2") {}\n    else if(command=="callshop") {check(!strcmp(script_getstr(st,2),"PN Alice Certainty")||!strcmp(script_getstr(st,2),"PN Bio Certainty"),"only configured certainty shops open");}\n    else if(command=="select")',1)
 import sys
 args=[sys.argv[0]]
 if a.build_dir:args+=['--build-dir',str(a.build_dir)]
 if a.prepare_only:args+=['--prepare-only']
 sys.argv=args
 # Alice's main defines the explicit builtin list. Inject effect as a known
 # boundary by adding it to the native installer input before main composes it.
 alice.native.CPP=alice.native.CPP.replace('"getexp", "callfunc"};','"specialeffect2", "callshop", "getexp", "callfunc"};',1)
 alice.main()
if __name__=='__main__':main()
