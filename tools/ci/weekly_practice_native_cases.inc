void weekly_cases(const std::string& dir){
 const std::vector<std::pair<std::string,std::string>> functions={
  {"PN_WeeklyReset","reset"},{"PN_WeeklyLabComplete","lab"},{"PN_GuideMilestones","build"},{"PN_WeeklyBoard","board"}};
 for(auto& f:functions)strdb_put(script_get_userfunc_db(),f.first.c_str(),compile(read(dir+"/weekly-"+f.second+".script"),f.first.c_str()));
 auto* reset=compile("{ callfunc \"PN_WeeklyReset\"; end; }","weekly reset");
 auto* lab=compile("{ callfunc \"PN_WeeklyLabComplete\"; end; }","weekly lab");
 auto* board=compile("{ callfunc \"PN_WeeklyBoard\"; end; }","weekly board");
 auto* build=compile("{ callfunc \"PN_GuideMilestones\"; end; }","weekly build");
 {
  auto sd=recipient();setnum("PNWeeklyKills",50);setnum("PNWeeklyStamps",3);kafra_walk(reset,{});
  int64 week=nums[add_str("PNWeeklyWeek")];check(week>0&&nums[add_str("PNWeeklyKills")]==0,"new week resets progress");
  setnum("PNWeeklyKills",25);setnum("PNWeeklyLab",1);kafra_walk(reset,{});
  check(nums[add_str("PNWeeklyKills")]==25&&nums[add_str("PNWeeklyLab")]==1,"same week preserves progress");
  setnum("PNWeeklyWeek",week-1);setnum("PNWeeklyClaimed",1);kafra_walk(reset,{});
  check(nums[add_str("PNWeeklyKills")]==0&&nums[add_str("PNWeeklyLab")]==0&&nums[add_str("PNWeeklyClaimed")]==0&&nums[add_str("PNWeeklyStamps")]==3,"week rollover preserves lifetime cosmetic record only");
 }
 {
  auto sd=recipient();kafra_walk(lab,{});check(nums[add_str("PNWeeklyLab")]==0,"practice requires opt in");
  kafra_walk(board,{1});check(nums[add_str("PNWeeklyActive")]==1,"actual board opt in");
  kafra_walk(lab,{});check(nums[add_str("PNWeeklyLab")]==1,"completed measurement records objective");
  kafra_walk(build,{3});check(nums[add_str("PNWeeklyBuild")]==1,"actual magic build review records objective");
  setnum("PNWeeklyKills",49);kafra_walk(board,{});check(nums[add_str("PNWeeklyStamps")]==0,"incomplete hunt cannot claim");
  setnum("PNWeeklyKills",50);kafra_walk(board,{1});check(nums[add_str("PNWeeklyStamps")]==1&&nums[add_str("PNWeeklyClaimed")]==1,"completed week receives one cosmetic stamp");
  kafra_walk(board,{});check(nums[add_str("PNWeeklyStamps")]==1,"repeat interaction cannot duplicate stamp");
  check(sd->status.zeny==1234567&&sd->weight==0,"practice reward adds no economy value or inventory");
 }
 script_free_code(reset);script_free_code(lab);script_free_code(board);script_free_code(build);
 std::cout<<"WEEKLY_PRACTICE_NATIVE_OK reset, opt-in, objectives, claim, duplicate guard and cosmetic-only reward\n";
}
