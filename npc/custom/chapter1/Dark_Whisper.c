1@ch1a,1,1,0	script	ch1_odw_control	-1,{
	end;
	
OnInstanceInit:
	set_instance_var("map$", instance_mapname("1@ch1a"));
	set_instance_var("control$", instance_npcname(strnpcinfo(0)));
	set_instance_var("event", 0);
	set_instance_var("control", 0);
	set_instance_var("summon", 0);
	instance_event("ch1_odw_control", "OnEvent", false);
end;

OnEvent:
	.@event = get_instance_var("event") + 1;
	set_instance_var("event", .@event);
	switch( .@event ) {
		case 1:
			instance_enable("Est#0", true);
			instance_enable("#lim01", true);
			break;
		case 2:
			inc_instance_var("control");
			instance_announce instance_id(),"???: My beloved children. Are you all peacefully resting?",bc_map,0xFFAA00;
			sleep 3000;
			npctalk "That voice must be the Immortal Monarch, right? I need to see that smug face myself.",instance_npcname("Est#0");
			sleep 3000;
			instance_announce instance_id(),"???: Soon, I'm going to introduce you to the pitiful humans of the surface. Be ready~",bc_map,0xFFAA00;
			sleep 3000;
			instance_announce instance_id(),"SYSTEM : Initiating infected incubation pod release process.",bc_map,0xEEFF;
			sleep 3000;
			npctalk "Great, we've walked in at the worst possible moment. Didn't think I'd have to use the disguise scroll this soon.",instance_npcname("Est#0");
			sleep 1000;
			specialeffect EF_QUAKEBODY2,AREA,instance_npcname("Est#0");
			sleep 700;
			setnpcdisplay(instance_npcname("Est#0"),"Est", 22484);
			specialeffect EF_LANDBODY,AREA,instance_npcname("Est#0");
			sleep 1500;
			npctalk "Disguising as an infected makes me feel sluggish…",instance_npcname("Est#0");
			npcspeed 300, instance_npcname("Est#0");
			unitwalk getnpcid(0, instance_npcname("Est#0")), 205, 64, get_instance_var("control$") + "::OnEvent";
			break;
		case 3:
			unitwalk getnpcid(0, instance_npcname("Est#0")), 205, 74, get_instance_var("control$") + "::OnEvent";
			break;
		case 4:
			npctalk "Wait a second! If we destroy everything before the incubation starts, then we won't even have to face the Infected.",instance_npcname("Est#0");
			sleep 3000;
			npctalk "We also won't need to sneak around in these annoying disguises either.",instance_npcname("Est#0");
			sleep 3000;
			npctalk "Why didn't I think of this sooner? Let's pump a few bullets into this facility and see what happens.",instance_npcname("Est#0");
			sleep 3000;
			setarray .@npc, 58, 4, 23, 40, 30, 49, 19, 60, 36, 27, 53, 22, 48, 62, 57;
			for ( .@i = 0; .@i < getarraysize(.@npc); ++.@i ) {
				instance_enable("Infected#" + .@npc[.@i],true);
				specialeffect EF_CHEMICAL2DASH4,AREA,instance_npcname("Est#0");
				specialeffect EF_DESPERADO,AREA,instance_npcname("Infected#" + .@npc[.@i]);
				specialeffect EF_TORCH_RED,AREA,instance_npcname("Infected#" + .@npc[.@i]);
				sleep 250;
			}
			sleep 1000;
			npctalk "Haha! Let's blow up these incubation tanks too!",instance_npcname("Est#0");
			sleep 2000;
			specialeffect EF_CHEMICAL2DASH4,AREA,instance_npcname("Est#0");
			unitwalk getnpcid(0, instance_npcname("Est#0")), 208, 84, get_instance_var("control$") + "::OnEvent";
			specialeffect EF_DESPERADO,AREA,instance_npcname("#ch1_fire1");
			sleep 700;
			specialeffect EF_RAIN_PARTICLE2,AREA,instance_npcname("#ch1_fire1");
			sleep 300;
			specialeffect EF_TORCH_GREEN,AREA,instance_npcname("#ch1_fire1");
			specialeffect EF_CHEMICAL2DASH4,AREA,instance_npcname("Est#0");
			specialeffect EF_DESPERADO,AREA,instance_npcname("#ch1_fire2");
			sleep 700;
			specialeffect EF_RAIN_PARTICLE2,AREA,instance_npcname("#ch1_fire2");
			sleep 300;
			specialeffect EF_TORCH_GREEN,AREA,instance_npcname("#ch1_fire2");
			specialeffect EF_CHEMICAL2DASH4,AREA,instance_npcname("Est#0");
			specialeffect EF_DESPERADO,AREA,instance_npcname("#ch1_fire3");
			sleep 700;
			specialeffect EF_RAIN_PARTICLE2,AREA,instance_npcname("#ch1_fire3");
			sleep 300;
			specialeffect EF_TORCH_GREEN,AREA,instance_npcname("#ch1_fire3");
			specialeffect EF_CHEMICAL2DASH4,AREA,instance_npcname("Est#0");
			specialeffect EF_DESPERADO,AREA,instance_npcname("#ch1_fire4");
			sleep 700;
			specialeffect EF_RAIN_PARTICLE2,AREA,instance_npcname("#ch1_fire4");
			sleep 300;
			specialeffect EF_TORCH_GREEN,AREA,instance_npcname("#ch1_fire4");
			break;
		case 5:
			npctalk "I think I just saw something move. Let's check over there!",instance_npcname("Est#0");
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 6:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 1; .@i <= 8; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			npctalk "They're pouring out! Don't move!",instance_npcname("Est#0");
			break;
		case 7:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 9; .@i <= 15; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			npctalk "Stay still as much as possible. Whatever you do, don't provoke them!",instance_npcname("Est#0");
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 8:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 16; .@i <= 23; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 9:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 24; .@i <= 32; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 10:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 33; .@i <= 39; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 11:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 40; .@i <= 47; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 12:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 48; .@i <= 54; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 13:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 55; .@i <= 64; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 14:
			unitwarp getnpcid(0, instance_npcname("Infected#0")), get_instance_var("map$"), 210, 96;
			instance_enable("Infected#0", false);
			instance_enable("Infected#0", true);
			setarray .@ID, 22459, 22476, 22471;
			for ( .@i = 65; .@i <= 72; ++.@i ) {
				instance_enable("Infected#" + .@i,true);
				setnpcdisplay(instance_npcname("Infected#" + .@i), .@ID[rand(3)]);
			}
			unitwalk getnpcid(0, instance_npcname("Infected#0")),212,87,get_instance_var("control$") + "::OnEvent";
			break;
		case 15:
			instance_enable("Infected#0", false);
			instance_announce instance_id(),"???: We are about to go do something very important. Follow my guidance.",bc_map,0xFFAA00;
			sleep 3000;
			npctalk "We'll get caught up in this too! Brace yourself.",instance_npcname("Est#0");
			sleep 3000;
			instance_announce instance_id(),"SYSTEM : Infected ejection tunnel transfer initiated.",bc_map,0xEEFF;
			for ( .@i = 1; .@i <= 72; ++.@i ) {
				specialeffect EF_BEGINSPELL, AREA, instance_npcname("Infected#" + .@i);
				setnpcdisplay(instance_npcname("Infected#" + .@i), CLEAR_NPC);
				instance_enable("Infected#" + .@i, false);
				sleep 75;
			}
			sleep 400;
			instance_announce instance_id(),"SYSTEM : Warning!",bc_map,0xAAFF00;
			sleep 400;
			instance_announce instance_id(),"SYSTEM : Warning!",bc_map,0xAAFF00;
			sleep 400;
			instance_announce instance_id(),"SYSTEM : Warning!",bc_map,0xAAFF00;
			sleep 1000;
			instance_announce instance_id(),"SYSTEM : Entity does not match Infected genetic sequence. Ejecting through disposal tunnel to prevent incubation tank contamination.",bc_map,0xEEFF;
			sleep 2000;
			instance_enable("Est#1", true);
			instance_enable("#lim02", true);
			instance_warpall get_instance_var("map$"), 270, 294, instance_id();
			sleep 4000;
			instance_enable("#lim01", false);
			instance_enable("Est#0", false);
			instance_announce instance_id(),"SYSTEM : Proceeding as planned, the infected will descend into the Sanctuary of Verus through the ejection tunnel.",bc_map,0xEEFF;
			sleep 3000;
			instance_announce instance_id(),"SYSTEM : Eliminate any obstructions blocking the tunnel during descent.",bc_map,0xEEFF;
			sleep 1000;
			instance_event("ch1_odw_control", "OnSummon", false);
			sleep 3000;
			specialeffect EF_MAXPOWER,AREA,instance_npcname("Est#1");
			specialeffect EF_BEGINSPELL5,AREA,instance_npcname("Est#1");
			specialeffect EF_RG_COIN7,AREA,instance_npcname("Est#1");
			sleep 1000;
			for ( .@i = 0; .@i < getarraysize('mob_gid); ++.@i ) {
				specialeffect EF_GUIDEDATTACK, AREA, 'mob_gid[.@i];
				sleep 25;
			}
			for ( .@i = 0; .@i < getarraysize('mob_gid); ++.@i ) {
				if ( unitexists('mob_gid[.@i]) )
					unitskilluseid 'mob_gid[.@i], "NPC_SELFDESTRUCTION", 1, 'mob_gid[.@i], -10000;
			}
			sleep 3000;
			instance_announce instance_id(),"???: So it's you. " + get_instance_var("name$") + ".",bc_map,0xFFAA00;
			sleep 3000;
			instance_announce instance_id(),"???: I've long regretted not finishing you off back then.",bc_map,0xFFAA00;
			sleep 3000;
			npctalk "What? How come she knows your name, " + get_instance_var("name$") + "? That means... that you've already met before.", instance_npcname("Est#1");
			sleep 3000;
			instance_announce instance_id(),"???: Come up here. We have unfinished business to settle, don't we?",bc_map,0xFFAA00;
			sleep 3000;
			npctalk "Go on ahead. I'll make sure no Infected get out of this place and follow you.",instance_npcname("Est#1");
			inc_instance_var("control");
			instance_event("ch1_odw_control", "OnSummon", false);
			break;
		case 16:
			instance_announce instance_id(),"SYSTEM : All Infected eliminated due to external contaminants. Switching system to emergency mode. Initiating forced ejection of contaminants.",bc_map,0xFFFF00;
			sleep 3000;
			instance_announce instance_id(),"???: Stop it. Those kind of tricks won't work on them. Open the door to my chamber.",bc_map,0xFFAA00;
			sleep 3000;
			instance_announce instance_id(),"SYSTEM : Executing the Monarch's command. Opening the passage to the VIP room.",bc_map,0xEEFF;
			sleep 2000;
			inc_instance_var("control");
			instance_warpall get_instance_var("map$"), 252, 349, instance_id();
			instance_enable("Est#1", false);
			instance_enable("#lim02", false);
			instance_enable("#ch1_odw_warp_1", true);
			instance_enable("#lim03", true);
			instance_enable("Est#2", true);
			instance_enable("Immortal Monarch#1", true);
			break;
		case 17:
			inc_instance_var("control");
			sleep 1000;
			emotion ET_SURPRISE,getnpcid(0,"Est#2");
			sleep 1000;
			npctalk "So it really was you, Himmelmez!",instance_npcname("Est#2");
			sleep 3000;
			npctalk "And you are? Your face is not familiar. But I suppose you're not anyone important. ",instance_npcname("Immortal Monarch#1");
			sleep 3000;
			npctalk "Back during your last invasion, a good number of my comrades vanished because of you.",instance_npcname("Est#2");
			sleep 3000;
			npctalk "This is perfect, I always wanted to see your dumb face once.",instance_npcname("Est#2");
			sleep 3000;
			npctalk "Oh? Is that the only thing that you want to say to me?",instance_npcname("Immortal Monarch#1");
			sleep 3000;
			npctalk "No words needed! Die here and now!",instance_npcname("Est#2");
			specialeffect EF_CHEMICAL2DASH4,AREA,instance_npcname("Est#2");
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 750;
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 750;
			npctalk "How ticklish. Go ahead and thrash around all you like.",instance_npcname("Immortal Monarch#1");
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			npcspeed 900, instance_npcname("Immortal Monarch#1");
			unitwalk getnpcid(0, instance_npcname("Immortal Monarch#1")), 86, 148;
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 900;
			specialeffect EF_CHEMICAL2DASH4,AREA,instance_npcname("Est#2");
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 900;
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 900;
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 900;
			specialeffect EF_CHEMICAL2DASH4,AREA,instance_npcname("Est#2");
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 900;
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 900;
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 900;
			specialeffect EF_GUIDEDATTACK,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_DESPERADO,AREA,instance_npcname("Immortal Monarch#1");
			specialeffect EF_GRAVITATION,AREA,instance_npcname("Immortal Monarch#1");
			sleep 2000;
			npctalk "Is this it. Is this all you've got to show me?",instance_npcname("Immortal Monarch#1");
			sleep 3000;
			npctalk "What the...",instance_npcname("Est#2");
			sleep 3000;
			npctalk "Don't you know who I am? Foolish creature. You're annoying, get lost.",instance_npcname("Immortal Monarch#1");
			sleep 3000;
			specialeffect EF_BEGINSPELL_YB,AREA,instance_npcname("Immortal Monarch#1");
			sleep 2000;
			specialeffect EF_SPINMOVE,AREA,instance_npcname("Est#2");
			sleep 800;
			specialeffect EF_LORD,AREA,instance_npcname("Est#2");
			sleep 2500;
			instance_enable("Est#2",false);
			sleep 500;
			npctalk "Now. With that annoying pest gone, shall we have our fun? " + get_instance_var("name$") + ".",instance_npcname("Immortal Monarch#1");
			instance_warpall get_instance_var("map$"), 86, 143, instance_id();
			sleep 2000;
			instance_enable("Immortal Monarch#1",false);
			instance_event("ch1_odw_control", "OnSummonBoss", false);
			break;
		case 18:
			instance_warpall get_instance_var("map$"), 86, 151, instance_id();
			specialeffect EF_READYPORTAL,AREA,instance_npcname("Est#2");
			sleep 2000;
			specialeffect EF_TELEPORTATION,AREA,instance_npcname("Est#2");
			instance_enable("Est#2",true);
			specialeffect EF_SONICBLOW,AREA,instance_npcname("Est#2");
			sleep 1000;
			inc_instance_var("control");
			emotion ET_PANIC,getnpcid(0,instance_npcname("Est#2"));
			npctalk "Tsk... Was I too late?",instance_npcname("Est#2");
			specialeffect 1090, AREA, instance_npcname("Est#2");
			instance_event("#ch1_boss_exit", "OnStart", false);
			break;
	}
end;

OnSummonBoss:
	monster get_instance_var("map$"), 86, 148, "Immortal Monarch", 22496, 1, get_instance_var("control$") + "::OnBossKill";
	set_instance_var("gid", $@mobid[0]);
	instance_event("#ch1_boss_crystal", "OnStart", false);
end;

OnBossKill:
	stopnpctimer instance_npcname("#ch1_boss_crystal");
	killmonster get_instance_var("map$"), instance_npcname("#ch1_boss_crystal") + "::OnCrystalKill";
	instance_event("ch1_odw_control", "OnEvent", false);
end;

OnSummon:
	.@summon = get_instance_var("summon") + 1;
	set_instance_var("summon", .@summon);
	.@event$ = get_instance_var("control$") + "::OnMobKill";
	switch ( .@summon ) {
		case 1:
			.@map$ = get_instance_var("map$");
			setarray .@mob[0],
				22476, 266, 297,  
				22471, 265, 294,
				22461, 264, 298,
				22459, 264, 290,
				22471, 263, 296,
				22459, 262, 294,
				22471, 260, 300,
				22461, 261, 292,
				22476, 260, 297,
				22461, 258, 302,
				22459, 258, 299,
				22459, 258, 295,
				22471, 256, 302,
				22459, 254, 297,
				22461, 256, 298,
				22461, 256, 294,
				22461, 254, 302,
				22459, 252, 301;

			for ( .@i = 0; .@i < getarraysize(.@mob); .@i += 3 ) {
				monster .@map$, .@mob[.@i+1], .@mob[.@i+2], "Infected", .@mob[.@i], 1;
				'mob_gid[.@i] = .@gid = $@mobid[0];
				setunitdata .@gid, UMOB_MODE, MD_AGGRESSIVE | MD_CANATTACK | MD_IGNOREMELEE | MD_IGNOREMAGIC | MD_IGNORERANGED;
				setunitdata .@gid, UMOB_MAXHP, 500;
				setunitdata .@gid, UMOB_HP, 500;
				setunitdata .@gid, UMOB_SPEED, 2000;
			}
			'clear = 0;
			end;
			
		case 2:
			.@map$ = get_instance_var("map$");
			setarray .@mob[0],
				22461, 253, 308, 
				22461, 269, 329, 
				22461, 292, 327, 
				22461, 309, 304, 
				22461, 306, 279, 
				22461, 288, 262, 
				22461, 262, 257, 
				22461, 239, 266, 
				22461, 226, 288,
				22461, 225, 315,  
				22461, 236, 338;

			for ( .@i = 0; .@i < getarraysize(.@mob); .@i += 3 ) {
				monster .@map$, .@mob[.@i+1], .@mob[.@i+2], "Giant Infected", .@mob[.@i], 1, .@event$, Size_Large;
			}
			break;
	}
end;

OnMobKill:
	.@map$ = get_instance_var("map$");
	.@event$ = get_instance_var("control$") + "::OnMobKill";
	if ( mobcount( .@map$, .@event$) > 0 ) {
		instance_announce instance_id(), "SYSTEM : Infected damaged due to external contaminants. " + mobcount( .@map$, .@event$) + " remaining infected, proceed through the ejection tunnel.", bc_map, 0xEEFF;
	} else {
		instance_event("ch1_odw_control", "OnEvent", false);
	}
end;
}

1@ch1a,210,63,0	script(DISABLED)	#lim01	HIDDEN_WARP_NPC,1,4,{
	end;
	
OnTouch:
	if ( get_instance_var("control") == 0 ) {
		warp get_instance_var("map$"), 217, 63;
		sleep 1000;
		npctalk "I'm going to brief the operation, stay where you are.",instance_npcname("Est#0");
	}
end;
}

1@ch1a,264,294,0	script(DISABLED)	#lim02	HIDDEN_WARP_NPC,1,5,{
	end;

OnTouch:
	if ( get_instance_var("control") < 2 ) {
		warp get_instance_var("map$"), 270, 294;
		sleep 1000;
		npctalk "Get back! You might get hit if you stand on my line of fire!",instance_npcname("Est#1");
	}
end;
}

1@ch1a,89,153,0	script(DISABLED)	#lim03	HIDDEN_WARP_NPC,2,2,{
	end;
	
OnTouch:
	if ( get_instance_var("control") == 3 ) {
		instance_enable("#lim03", false);
		instance_event("ch1_odw_control", "OnEvent", false);
	}
end;
}

1@ch1a,214,63,5	script(DISABLED)	Est#0	4_F_ESTLOVELOY,3,3,{
	if ( !is_party_leader() ) {
		end;
	}
	if ( get_instance_var("control") == 0 ) {
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "As you can see, we're a small team. We don't have the time or resources to take on the entire Immortal Legion.";
		npctalk "As you can see, we're a small team. We don't have the time or resources to take on the entire Immortal Legion.",instance_npcname("Est#0");
		next;
		mes "[Est]";
		mes "That's why we need to avoid physical confrontation as much as possible until we reach the Immortal Witch. Do you understand?";
		npctalk "That's why we need to avoid physical confrontation as much as possible until we reach the Immortal Witch. Do you understand?",instance_npcname("Est#0");
		next;
		cutin "ep162_est02.bmp",2;
		mes "[Est]";
		mes "Just like when you infiltrated the Rgans base, I've brought scrolls that let us disguise ourselves to look like the infected. Use these.";
		npctalk "Just like when you infiltrated the Rgans base, I've brought scrolls that let us disguise ourselves to look like the infected. Use these.",instance_npcname("Est#0");
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "Stick close to me. That way, you'll benefit from the scroll's transformation effect too.";
		npctalk "Stick close to me. That way, you'll benefit from the scroll's transformation effect too.",instance_npcname("Est#0");
		next;
		if ( select("Please wait a moment.", "Let's move out.") == 1 ) {
			mes "[Est]";
			mes "Make it fast. We don't have much time.";
			close3;
		}
		cutin "",255;
		set_instance_var("name$", strcharinfo(0));
		instance_event("ch1_odw_control", "OnEvent", false);
		end;
	}
	end;
	
OnTouch:
	if ( get_instance_var("control") > 0 ) {
		transform 22463,60000,SC_MONSTER_TRANSFORM;
		end;
	}
end;
}

1@ch1a,267,294,1	script	Est#1	4_F_ESTLOVELOY,3,3,{
	end;
}

1@ch1a,86,157,4	script(DISABLED)	Immortal Monarch#1	CH1_MD_HEMEL,3,3,{
	end;
}

1@ch1a,86,146,1	script(DISABLED)	Est#2	4_F_ESTLOVELOY,1,1,{
	if ( get_instance_var("control") < 5 ) {
		end;
	}
	if ( isbegin_quest(12664) == 1 ) {
		cutin "ep162_est02.bmp",2;
		mes "[Est]";
		mes "You handled already before I could even step in. I thought things had gone on for too long and would turn into the worst case scenario.";
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "But that figure we glimpsed when Himmelmez fell, what do you think was it? Did you see it too?";
		next;
		select("I caught a glimpse.", "It looked like a black hand.");
		cutin "ep162_est02.bmp",2;
		mes "[Est]";
		mes "Ah, we definitely need to talk more about that. But for now, should we start wrapping things around here?";
		erasequest 12664;
		setquest 12665;
		getitem "1001972",20;
		getexp 326523723, 12000000;
		open_quest_ui 12665;
		close3;
	}
	cutin "ep162_est02.bmp",2;
	mes "[Est]";
	mes "Yeah, we'd better get out of here before it gets any later. I'm already sick of the word ejection announcements.";
	close2;
	cutin "",255;
	warp "hem_dun01",208,246;
	end;
}

1@ch1a,210,96,5	script(DISABLED)	Infected#0	CH1_IMMORTAL_CORPS1,1,1,{ end; }
1@ch1a,215,87,3	script(DISABLED)	Infected#1	CLEAR_NPC,1,1,{ end; }
1@ch1a,213,87,5	script(DISABLED)	Infected#2	CLEAR_NPC,1,1,{ end; }
1@ch1a,211,87,7	script(DISABLED)	Infected#3	CLEAR_NPC,1,1,{ end; }
1@ch1a,209,87,5	script(DISABLED)	Infected#4	CLEAR_NPC,1,1,{ end; }
1@ch1a,207,87,3	script(DISABLED)	Infected#5	CLEAR_NPC,1,1,{ end; }
1@ch1a,205,87,2	script(DISABLED)	Infected#6	CLEAR_NPC,1,1,{ end; }
1@ch1a,203,87,3	script(DISABLED)	Infected#7	CLEAR_NPC,1,1,{ end; }
1@ch1a,201,87,7	script(DISABLED)	Infected#8	CLEAR_NPC,1,1,{ end; }
1@ch1a,199,87,8	script(DISABLED)	Infected#9	CLEAR_NPC,1,1,{ end; }
1@ch1a,197,87,6	script(DISABLED)	Infected#10	CLEAR_NPC,1,1,{ end; }
1@ch1a,195,87,4	script(DISABLED)	Infected#11	CLEAR_NPC,1,1,{ end; }
1@ch1a,213,85,1	script(DISABLED)	Infected#12	CLEAR_NPC,1,1,{ end; }
1@ch1a,211,85,1	script(DISABLED)	Infected#13	CLEAR_NPC,1,1,{ end; }
1@ch1a,209,85,8	script(DISABLED)	Infected#14	CLEAR_NPC,1,1,{ end; }
1@ch1a,207,85,1	script(DISABLED)	Infected#15	CLEAR_NPC,1,1,{ end; }
1@ch1a,205,85,5	script(DISABLED)	Infected#16	CLEAR_NPC,1,1,{ end; }
1@ch1a,203,85,5	script(DISABLED)	Infected#17	CLEAR_NPC,1,1,{ end; }
1@ch1a,201,85,8	script(DISABLED)	Infected#18	CLEAR_NPC,1,1,{ end; }
1@ch1a,199,85,4	script(DISABLED)	Infected#19	CLEAR_NPC,1,1,{ end; }
1@ch1a,197,85,7	script(DISABLED)	Infected#20	CLEAR_NPC,1,1,{ end; }
1@ch1a,195,85,7	script(DISABLED)	Infected#21	CLEAR_NPC,1,1,{ end; }
1@ch1a,209,83,7	script(DISABLED)	Infected#22	CLEAR_NPC,1,1,{ end; }
1@ch1a,207,83,6	script(DISABLED)	Infected#23	CLEAR_NPC,1,1,{ end; }
1@ch1a,205,83,4	script(DISABLED)	Infected#24	CLEAR_NPC,1,1,{ end; }
1@ch1a,203,83,5	script(DISABLED)	Infected#25	CLEAR_NPC,1,1,{ end; }
1@ch1a,201,83,4	script(DISABLED)	Infected#26	CLEAR_NPC,1,1,{ end; }
1@ch1a,199,83,3	script(DISABLED)	Infected#27	CLEAR_NPC,1,1,{ end; }
1@ch1a,197,83,2	script(DISABLED)	Infected#28	CLEAR_NPC,1,1,{ end; }
1@ch1a,195,83,3	script(DISABLED)	Infected#29	CLEAR_NPC,1,1,{ end; }
1@ch1a,193,83,4	script(DISABLED)	Infected#30	CLEAR_NPC,1,1,{ end; }
1@ch1a,209,81,8	script(DISABLED)	Infected#31	CLEAR_NPC,1,1,{ end; }
1@ch1a,207,81,6	script(DISABLED)	Infected#32	CLEAR_NPC,1,1,{ end; }
1@ch1a,205,81,7	script(DISABLED)	Infected#33	CLEAR_NPC,1,1,{ end; }
1@ch1a,203,81,8	script(DISABLED)	Infected#34	CLEAR_NPC,1,1,{ end; }
1@ch1a,201,81,8	script(DISABLED)	Infected#35	CLEAR_NPC,1,1,{ end; }
1@ch1a,199,81,7	script(DISABLED)	Infected#36	CLEAR_NPC,1,1,{ end; }
1@ch1a,197,81,4	script(DISABLED)	Infected#37	CLEAR_NPC,1,1,{ end; }
1@ch1a,195,81,5	script(DISABLED)	Infected#38	CLEAR_NPC,1,1,{ end; }
1@ch1a,193,81,6	script(DISABLED)	Infected#39	CLEAR_NPC,1,1,{ end; }
1@ch1a,209,79,3	script(DISABLED)	Infected#40	CLEAR_NPC,1,1,{ end; }
1@ch1a,207,79,3	script(DISABLED)	Infected#41	CLEAR_NPC,1,1,{ end; }
1@ch1a,205,79,3	script(DISABLED)	Infected#42	CLEAR_NPC,1,1,{ end; }
1@ch1a,203,79,3	script(DISABLED)	Infected#43	CLEAR_NPC,1,1,{ end; }
1@ch1a,201,79,1	script(DISABLED)	Infected#44	CLEAR_NPC,1,1,{ end; }
1@ch1a,199,79,4	script(DISABLED)	Infected#45	CLEAR_NPC,1,1,{ end; }
1@ch1a,197,79,8	script(DISABLED)	Infected#46	CLEAR_NPC,1,1,{ end; }
1@ch1a,195,79,2	script(DISABLED)	Infected#47	CLEAR_NPC,1,1,{ end; }
1@ch1a,193,79,6	script(DISABLED)	Infected#48	CLEAR_NPC,1,1,{ end; }
1@ch1a,209,77,3	script(DISABLED)	Infected#49	CLEAR_NPC,1,1,{ end; }
1@ch1a,207,77,6	script(DISABLED)	Infected#50	CLEAR_NPC,1,1,{ end; }
1@ch1a,205,77,5	script(DISABLED)	Infected#51	CLEAR_NPC,1,1,{ end; }
1@ch1a,203,77,3	script(DISABLED)	Infected#52	CLEAR_NPC,1,1,{ end; }
1@ch1a,201,77,8	script(DISABLED)	Infected#53	CLEAR_NPC,1,1,{ end; }
1@ch1a,199,77,6	script(DISABLED)	Infected#54	CLEAR_NPC,1,1,{ end; }
1@ch1a,197,77,4	script(DISABLED)	Infected#55	CLEAR_NPC,1,1,{ end; }
1@ch1a,195,77,5	script(DISABLED)	Infected#56	CLEAR_NPC,1,1,{ end; }
1@ch1a,193,77,8	script(DISABLED)	Infected#57	CLEAR_NPC,1,1,{ end; }
1@ch1a,209,75,8	script(DISABLED)	Infected#58	CLEAR_NPC,1,1,{ end; }
1@ch1a,207,75,2	script(DISABLED)	Infected#59	CLEAR_NPC,1,1,{ end; }
1@ch1a,205,75,7	script(DISABLED)	Infected#60	CLEAR_NPC,1,1,{ end; }
1@ch1a,203,75,8	script(DISABLED)	Infected#61	CLEAR_NPC,1,1,{ end; }
1@ch1a,201,75,7	script(DISABLED)	Infected#62	CLEAR_NPC,1,1,{ end; }
1@ch1a,199,75,3	script(DISABLED)	Infected#63	CLEAR_NPC,1,1,{ end; }
1@ch1a,197,75,6	script(DISABLED)	Infected#64	CLEAR_NPC,1,1,{ end; }
1@ch1a,195,75,7	script(DISABLED)	Infected#65	CLEAR_NPC,1,1,{ end; }
1@ch1a,209,73,3	script(DISABLED)	Infected#66	CLEAR_NPC,1,1,{ end; }
1@ch1a,207,73,6	script(DISABLED)	Infected#67	CLEAR_NPC,1,1,{ end; }
1@ch1a,205,73,8	script(DISABLED)	Infected#68	CLEAR_NPC,1,1,{ end; }
1@ch1a,203,73,3	script(DISABLED)	Infected#69	CLEAR_NPC,1,1,{ end; }
1@ch1a,201,73,3	script(DISABLED)	Infected#70	CLEAR_NPC,1,1,{ end; }
1@ch1a,199,73,2	script(DISABLED)	Infected#71	CLEAR_NPC,1,1,{ end; }
1@ch1a,197,73,3	script(DISABLED)	Infected#72	CLEAR_NPC,1,1,{ end; }
1@ch1a,213,81,1	script	#ch1_fire1	CLEAR_NPC,1,1,{ end; }
1@ch1a,216,81,1	script	#ch1_fire2	CLEAR_NPC,1,1,{ end; }
1@ch1a,213,76,1	script	#ch1_fire3	CLEAR_NPC,1,1,{ end; }
1@ch1a,216,76,1	script	#ch1_fire4	CLEAR_NPC,1,1,{ end; }

1@ch1a,259,352,0	script(DISABLED)	#ch1_odw_warp_1	WARPNPC,2,2,{
	end;
	
OnTouch:
	warp get_instance_var("map$"), 89, 153;
	if ( isbegin_quest(12666) == 1 ) {
		erasequest 12666;
		setquest 12664;
	}
end;
}

1@ch1a,1,1,0	script	#ch1_boss_crystal	HIDDEN_WARP_NPC,{
	end;
	
OnStart:
	.@gid = get_instance_var("gid");
	if ( unitexists(.@gid) ) {
		unitskilluseid .@gid, "NPC_RELIEVE_ON", 10, .@gid, -10000;
	}
	startnpctimer;
end;

OnTimer10000:
	.@gid = get_instance_var("gid");
	if ( !unitexists(.@gid) ) {
		stopnpctimer;
		end;
	}
	if ( unitexists(.@gid) ) {
		unitskilluseid .@gid, "NPC_RUN", 10;
	}
end;

OnTimer20000:
	.@gid = get_instance_var("gid");
	if ( !unitexists(.@gid) ) {
		stopnpctimer;
		end;
	}
	.@gid = get_instance_var("gid");
	.@map$ = get_instance_var("map$");
	if ( unitexists(.@gid) && rand(1,2) == 2 ) {
		unitskilluseid .@gid, "NPC_RUN", 10;
	}
	if ( !unitexists(.@gid) ) {
		end;
	}
	switch( rand(1,5) ) {
		case 1:
			unittalk .@gid, "Let's make this more interesting.";
			break;
		case 2:
			unittalk .@gid, "This is boring...";
			break;	
		case 3:
			unittalk .@gid, "You're doing useless things...";
			break;
		case 4:
			unittalk .@gid, "Useless.";
			break;
		case 5:
			unittalk .@gid, "I'll end you and your friends.";
			break;
	}
	setarray .@x, 70, 70, 101, 86, 101;
	setarray .@y, 154, 134, 154, 125, 134;
	.@idx = rand(5);
	monster .@map$, .@x[.@idx], .@y[.@idx], "Guardian Crystal", 1914, 1, instance_npcname(strnpcinfo(0)) + "::OnCrystalKill";
	set_instance_var("crystal", $@mobid[0]);
	if ( unitexists(.@gid) ) {
		setunitdata $@mobid[0], UMOB_MAXHP, 500000;
		setunitdata $@mobid[0], UMOB_HP, 500000;
		setunitdata $@mobid[0], UMOB_MODE, MD_KNOCKBACKIMMUNE | MD_STATUSIMMUNE;
	}
end;

OnCrystalKill:
	.@gid = get_instance_var("gid");
	if ( !unitexists(.@gid) ) end;
	if ( unitexists(.@gid) ) {
		//unitskilluseid .@gid, "NPC_GROGGY_ON", 5, .@gid, -10000;
		unitskilluseid .@gid, "NPC_RELIEVE_OFF", 1, .@gid, -10000;
		sleep 5000;
		if ( unitexists(.@gid) ) {
			unitskilluseid .@gid, "NPC_RELIEVE_ON", 10, .@gid, -10000;
		}
		initnpctimer;
	}
end;
}

1@ch1a,1,1,0	script	#ch1_boss_exit	HIDDEN_WARP_NPC,{
	end;
	
OnStart:
	initnpctimer;
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 1 minute.",bc_map,0xFFFF00;
end;
	
OnTimer10000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 50 seconds.",bc_map,0xFFFF00;
end;

OnTimer20000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 40 seconds.",bc_map,0xFFFF00;
end;

OnTimer30000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 30 seconds.",bc_map,0xFFFF00;
end;

OnTimer40000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 20 seconds.",bc_map,0xFFFF00;
end;

OnTimer50000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 10 seconds.",bc_map,0xFFFF00;
end;

OnTimer55000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected in 5 seconds.",bc_map,0xFFFF00;
end;

OnTimer56000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected in 4 seconds.",bc_map,0xFFFF00;
end;

OnTimer57000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected in 3 seconds.",bc_map,0xFFFF00;
end;

OnTimer58000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected in 2 seconds.",bc_map,0xFFFF00;
end;

OnTimer59000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected by force.",bc_map,0xFFFF00;
end;

OnTimer60000:
	instance_warpall "hem_dun01", 208, 246, instance_id();
end;

OnTimer61000:
	stopnpctimer;
	sleep 10;
	instance_destroy();
end;
}

1@ch1b,1,1,0	script	ch1_rdw_control	-1,{
	end;
	
OnInstanceInit:
	set_instance_var("map$", instance_mapname("1@ch1b"));
	set_instance_var("player_count", 0);
	set_instance_var("level", 0);
	set_instance_var("fail_tick", 10);
	set_instance_var("crystal_spawned", 0);
	set_instance_var("quit", 0);
	instance_enable("Wizard Professor#1", true);
	instance_enable("Immortal Monarch#2", true);
end;

OnFail:
	set_instance_var("fail_tick", 10);
	set_instance_var("crystal_spawned", 0 );
	stopnpctimer instance_npcname("ch1_rdw_pc_counter");
	stopnpctimer instance_npcname("ch1_rdw_heal");
	stopnpctimer instance_npcname("ch1_rdw_level_0");
	stopnpctimer instance_npcname("ch1_rdw_level_1");
	stopnpctimer instance_npcname("ch1_rdw_level_2");
	instance_warpall get_instance_var("map$"), 86, 143, instance_id();
	for ( .@i = 1; .@i <= 5; ++.@i ) {
		instance_enable("#rdw_gate_" + .@i, false);
	}
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_control") + "::OnBossKill";
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_level_0") + "::OnArmorKill";
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_level_1") + "::OnCrystalKill";
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_level_2") + "::OnSkillDead";
	instance_announce instance_id(), "Attack failed because of someone dying/abandon.", BC_MAP;
	instance_enable("Wizard Professor#2", true);
end;

OnStart:
	instance_enable("Wizard Professor#1", false);
	npcspeed 200, instance_npcname("Immortal Monarch#2");
	unitwalk getnpcid(0, instance_npcname("Immortal Monarch#2")), 86, 148, instance_npcname(strnpcinfo(0)) + "::OnBossSummon";
end;

OnBossSummon:
	instance_warpall get_instance_var("map$"), 86, 143, instance_id();
	instance_enable("Immortal Monarch#2", false);
	monster get_instance_var("map$"), 86, 148, "Immortal Monarch", 22497, 1, instance_npcname("ch1_rdw_control") + "::OnBossKill";
	getunitdata $@mobid[0], .@data;
	if ( unitexists($@mobid[0]) ) {
		switch( get_instance_var("level") ) {
			case 1:
				.@HP = 200000000;
				.@attack = 4000;
				.@stats = 50;
				.@res = 100;
				break;
			case 2:
				.@HP = 500000000;
				.@attack = 6000;
				.@stats = 75;
				.@res = 150;
				break;
			case 3:
				.@HP = 800000000;
				.@attack = 8000;
				.@stats = 100;
				.@res = 200;
				break;
			case 4:
				.@HP = 1100000000;
				.@attack = 10000;
				.@stats = 150;
				.@res = 300;
				break;
			case 5:
				.@HP = 1500000000;
				.@attack = 12000;
				.@stats = 200;
				.@res = 400;
				break;
		}
		set_instance_var("max_hp", .@HP);
		setunitdata $@mobid[0], UMOB_MAXHP, .@HP;
		setunitdata $@mobid[0], UMOB_ATKMIN, .@data[UMOB_ATKMIN] + .@attack;
		setunitdata $@mobid[0], UMOB_ATKMAX, .@data[UMOB_ATKMAX] + .@attack;
		setunitdata $@mobid[0], UMOB_STR, .@data[UMOB_STR] + .@stats;
		setunitdata $@mobid[0], UMOB_INT, .@data[UMOB_INT] + .@stats;
		setunitdata $@mobid[0], UMOB_RES, .@data[UMOB_RES] + .@res;
		setunitdata $@mobid[0], UMOB_MRES, .@data[UMOB_MRES] + .@res;
	}
	set_instance_var("gid", $@mobid[0]);
	unitskilluseid $@mobid[0], "NPC_RELIEVE_ON", 10, $@mobid[0], -10000;
	instance_event("ch1_rdw_pc_counter", "OnStart", false);
	instance_event("ch1_rdw_level_0", "OnStart", false);
	if ( get_instance_var("level") >= 2 ) {
		instance_event("ch1_rdw_level_2", "OnStart", false);
	}
end;

OnBossKill:
	stopnpctimer instance_npcname("ch1_rdw_pc_counter");
	stopnpctimer instance_npcname("ch1_rdw_heal");
	stopnpctimer instance_npcname("ch1_rdw_level_0");
	stopnpctimer instance_npcname("ch1_rdw_level_1");
	stopnpctimer instance_npcname("ch1_rdw_level_2");
	for ( .@i = 1; .@i <= 5; ++.@i ) {
		instance_enable("#rdw_gate_" + .@i, false);
	}
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_control") + "::OnBossKill";
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_level_0") + "::OnArmorKill";
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_level_1") + "::OnCrystalKill";
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_level_2") + "::OnSkillDead";
	instance_warpall get_instance_var("map$"), 86, 151, instance_id();
	sleep 2000;
	specialeffect EF_READYPORTAL,AREA,instance_npcname("Wizard Professor#3");
	sleep 2000;
	specialeffect EF_TELEPORTATION,AREA,instance_npcname("Wizard Professor#3");
	instance_enable("Wizard Professor#3", true);
	specialeffect EF_SONICBLOW,AREA,instance_npcname("Wizard Professor#3");
	sleep 1000;
	emotion ET_PANIC,getnpcid(0,instance_npcname("Wizard Professor#3"));
	npctalk "Looks like it's over, right?", instance_npcname("Wizard Professor#3");
	specialeffect 1090, AREA, instance_npcname("Wizard Professor#3");
end;
}

1@ch1b,86,157,4	script(DISABLED)	Immortal Monarch#2	CH1_MD_HEMEL,3,3,{
	end;
}

1@ch1b,86,144,8	script(DISABLED)	Wizard Professor#1	9,3,3,{
	if ( !is_party_leader() ) {
		end;
	}
	mes "[Wizard Professor]";
	mes "This is a virtual reacreation for research, but your senses are the same in reality. You might die here and feel the pain in reality~ So you better consider the simulation level.";
	next;
	.@s = select(
		"Cancel",
		"Simulation Level 1",
		(CH1_RDW >= 1 && BaseLevel >= 265 ? "^FF0000Simulation Level 2^000000" : ""),
		(CH1_RDW >= 2 && BaseLevel >= 265 ? "^FF0000Simulation Level 3^000000" : ""),
		(CH1_RDW >= 3 && BaseLevel >= 265 ? "^FF0000Simulation Level 4^000000" : ""),
		(CH1_RDW >= 4 && BaseLevel >= 265 ? "^FF0000Simulation Level 5^000000" : "")
	) - 1;
	if ( .@s == 0 ) {
		mes "[Wizard Professor]";
		mes "It seems you are not decided yet. You better think about it more.";
		close;
	}
	mes "[Wizard Professor]";
	mes "The virtual space reproduction pain level is ^0000ff" + .@s + "^000000? Shall we start with this~";
	next;
	if ( select("Cancel", "Proceed") == 1 ) {
		mes "[Wizard Professor]";
		mes "It seems you are not decided yet. You better think about it more.";
		close;
	}
	set_instance_var("level", .@s);
	.@party_id = getcharid(1);
	getpartymember .@party_id,0;
	getpartymember .@party_id,1;
	getpartymember .@party_id,2;
	set_instance_var("party_id",.@party_id);
	set_instance_var("party_leader", strcharinfo(0));
	.@count = $@partymembercount;
	if( .@count > 0 ){
		.@map$ = get_instance_var("map$");
		for( .@i = 0; .@i < $@partymembercount; .@i++ ){
			if( !isloggedin($@partymemberaid[.@i],$@partymembercid[.@i]) ) continue;
			if( strcharinfo(3,$@partymembercid[.@i]) != .@map$ ) continue;
			.@cid[.@x] = $@partymembercid[.@i];
			.@aid[.@x] = $@partymemberaid[.@i];
			.@name$[.@x] = $@partymembername$[.@i];
			.@x++;
		}
	}
	set_instance_var("player_count",.@x);
	for( .@i = 0; .@i < .@x; .@i++ ) {
		set_instance_var("cid_" + .@i,.@cid[.@i]);
		set_instance_var("aid_" + .@i,.@aid[.@i]);
		set_instance_var("name_" + .@i + "$",.@name$[.@i]);
	}
	instance_event("ch1_rdw_control", "OnStart", false);
	end;
	
OnTouch:
end;

OnInstanceInit:
	setunitdata getnpcid(0), UNPC_CLASS, 9;
	setunitdata getnpcid(0), UNPC_SEX, 0;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_CLOTHCOLOR, 3;
	setunitdata getnpcid(0), UNPC_HEADTOP, 1938;
	setunitdata getnpcid(0), UNPC_ROBE, 65;
end;
}

1@ch1b,86,144,8	script(DISABLED)	Wizard Professor#2	9,3,3,{
	if ( get_instance_var("quit") ) {
		mes "[Wizard Professor]";
		mes "It looks like this place is about to collapse. Shall we head outside?";
		next;
		if ( select("Cancel", "Go outside") == 1 ) {
			end;
		}
		warp "hem_dun01",208,246;
		end;
	}
	if ( is_party_leader() == true ) {
		mes "[Wizard Professor]";
		mes "You were unlucky. Your party members coordination is essential for this virtual attack, so try harder again next time.";
		next;
		if ( select("Complete Attack", "Abandon Attack") == 2 ) {
			set_instance_var("quit", true);
			instance_event("#ch1_boss_exit-2", "OnStart", false);
			end;
		}
		instance_enable("Wizard Professor#2", false);
		instance_event("ch1_rdw_control", "OnBossSummon", false);
		end;
	}
	end;
	
OnTouch:
end;
	
OnInstanceInit:
	setunitdata getnpcid(0), UNPC_CLASS, 9;
	setunitdata getnpcid(0), UNPC_SEX, 0;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_CLOTHCOLOR, 3;
	setunitdata getnpcid(0), UNPC_HEADTOP, 1938;
	setunitdata getnpcid(0), UNPC_ROBE, 65;
end;
}

1@ch1b,86,144,8	script(DISABLED)	Wizard Professor#3	9,3,3,{
	if ( checkquest(12662,HUNTING) == 2 ) {
		mes "[Wizard Professor]";
		mes "That was very impressive.";
		mes "Let's see... here's your reward for clearing ^0000cdlevel " + get_instance_var("level") + "^000000.";
		if ( CH1_RDW < 4 ) {
			next;
			CH1_RDW++;
			mes "[Wizard Professor]";
			mes "It seems like a good time to move on to the next level now. I have unlocked ^0000cdlevel " + (CH1_RDW + 1) + "^000000 for you.";
		}
		erasequest 12662;
		getitem "Ch1_Root_Coin", 10;
		switch ( get_instance_var("level") ) {
			case 2: getitem "Ch1_MD_Reward_1", 1; break;
			case 3: getitem "Ch1_MD_Reward_2", 1; break;
			case 4: getitem "Ch1_MD_Reward_3", 1; break;
			case 5: getitem "Ch1_MD_Reward_4", 1; break;
		}
		close;
	}
	mes "[Wizard Professor]";
	mes "It looks like this place is about to collapse. Shall we head out?";
	next;
	if ( select("Cancel", "Go outside") == 1 ) {
		end;
	}
	warp "hem_dun01",208,246;
	end;
	
OnTouch:
end;

OnInstanceInit:
	setunitdata getnpcid(0), UNPC_CLASS, 9;
	setunitdata getnpcid(0), UNPC_SEX, 0;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_CLOTHCOLOR, 3;
	setunitdata getnpcid(0), UNPC_HEADTOP, 1938;
	setunitdata getnpcid(0), UNPC_ROBE, 65;
end;
}

1@ch1b,1,1,0	script	ch1_rdw_pc_counter	-1,{
	end;
	
OnStart:
	initnpctimer;
end;

OnTimer1000:
	function boss_exist;
	stopnpctimer;
	.@gid = get_instance_var("gid");
	if ( get_instance_var("boss_dead") || !.@gid ) {
		end;
	}
	if ( get_instance_var("counter_started") ) {
		end;
	}
	.@map$ = get_instance_var("map$");
	.@count = get_instance_var("player_count");
	if ( getmapusers(.@map$) != .@count ) {
		instance_event(strnpcinfo(0), "OnCount", false);
		end;
	}
	if ( !boss_exist() ) {
		end;
	}
	for ( .@i = 0; .@i < .@count; .@i++ ) {
		while ( !attachrid(get_instance_var("aid_" + .@i)) ) {
			instance_announce instance_id(), "A player died/left the dungeon, the attack will fail in " + get_instance_var("fail_tick") + " turn.", BC_MAP;
			set_instance_var("fail_tick", get_instance_var("fail_tick") - 1);
			sleep2 1000;
			if ( !boss_exist() ) {
				end;
			}
			if ( attachrid(get_instance_var("aid_" + .@i)) ) {
				break;
			}
			if ( get_instance_var("fail_tick") == 0 ) {
				instance_event("ch1_rdw_control", "OnFail", false);
				end;
			}
			if ( !boss_exist() ) {
				end;
			}
		}
		if ( !boss_exist() ) {
			end;
		}
		while ( strcharinfo(3) != .@map$ ) {
			instance_announce instance_id(), "A player died/left the dungeon, the attack will fail in " + get_instance_var("fail_tick") + " turn.", BC_MAP;
			set_instance_var("fail_tick", get_instance_var("fail_tick") - 1);
			sleep2 1000;
			if ( !boss_exist() ) {
				end;
			}
			if ( strcharinfo(3) == .@map$ ) {
				break;
			}
			if ( get_instance_var("fail_tick") == 0 ) {
				instance_event("ch1_rdw_control", "OnFail", false);
				end;
			}
			if ( !boss_exist() ) {
				end;
			}
		}
		while ( Hp < 1 ) {
			instance_announce instance_id(), "A player died/left the dungeon, the attack will fail in " + get_instance_var("fail_tick") + " turn.", BC_MAP;
			set_instance_var("fail_tick", get_instance_var("fail_tick") - 1);
			sleep2 1000;
			if ( !boss_exist() ) {
				end;
			}
			if ( Hp > 0 ) {
				break;
			}
			if ( get_instance_var("fail_tick") == 0 ) {
				instance_event("ch1_rdw_control", "OnFail", false);
				end;
			}
			if ( !boss_exist() ) {
				end;
			}
		}	
	}
	if ( get_instance_var("fail_tick") < 10 ) {
		inc_instance_var("fail_tick");
	}
	if ( boss_exist() ) {
		initnpctimer;
	}
end;

function	boss_exist	{
	return unitexists( get_instance_var("gid") );
}

}

1@ch1b,1,1,0	script	ch1_rdw_level_0	HIDDEN_WARP_NPC,{
	end;
	
OnStart:
	startnpctimer;
end;

OnTimer10000:
	.@gid = get_instance_var("gid");
	if ( !unitexists(.@gid) ) {
		stopnpctimer;
		end;
	}
	if ( unitexists(.@gid) && rand(1,3) == 3 ) {
		unitskilluseid .@gid, "NPC_RUN", 10;
	}
end;

OnTimer20000:
	.@gid = get_instance_var("gid");
	stopnpctimer;
	if ( !unitexists(.@gid) ) {
		end;
	}
	.@map$ = get_instance_var("map$");
	if ( unitexists(.@gid) ) {
		unitskilluseid .@gid, "NPC_RUN", 10;
	}
	if ( !unitexists(.@gid) ) {
		end;
	}
	switch( rand(1,4) ) {
		case 1:
			unittalk .@gid, "Quite interesting.";
			break;
		case 2:
			unittalk .@gid, "Boring humans...";
			break;	
		case 3:
			unittalk .@gid, "Useless.";
			break;
		case 4:
			unittalk .@gid, "Useless beings.";
			break;
	}
	instance_event("ch1_rdw_level_1", "OnSpawnGuardian", false);
	killmonster .@map$, instance_npcname(strnpcinfo(0)) + "::OnArmorKill";
	if ( get_instance_var("level") >= 3 ) {
		monster .@map$, 250, 348, "Guardian Crystal", 1914, 1, instance_npcname(strnpcinfo(0)) + "::OnArmorKill";
		if ( unitexists($@mobid[0]) ) {
			setunitdata $@mobid[0], UMOB_MAXHP, 500000;
			setunitdata $@mobid[0], UMOB_HP, 500000;
			setunitdata $@mobid[0], UMOB_MODE, MD_KNOCKBACKIMMUNE | MD_STATUSIMMUNE;
		}
	}
	setarray .@x, 70, 70, 101, 86, 101;
	setarray .@y, 154, 134, 154, 125, 134;
	.@idx = rand(5);
	monster .@map$, .@x[.@idx], .@y[.@idx], "Guardian Crystal", 1914, 1, instance_npcname(strnpcinfo(0)) + "::OnArmorKill";
	if ( unitexists($@mobid[0]) ) {
		setunitdata $@mobid[0], UMOB_MAXHP, 500000;
		setunitdata $@mobid[0], UMOB_HP, 500000;
		setunitdata $@mobid[0], UMOB_MODE, MD_KNOCKBACKIMMUNE | MD_STATUSIMMUNE;
	}
	if ( get_instance_var("level") >= 3 ) {
		.@idx2 = rand(5);
		while ( .@idx == .@idx2 ) {
			.@idx2 = rand(5);
		}
		.@idx2 += 1;
		instance_enable("#rdw_gate_" + .@idx2, true);
		set_instance_var("last_gate", .@idx2);
	}
	initnpctimer;
end;

OnArmorKill:
	stopnpctimer;
	.@gid = get_instance_var("gid");
	if ( !unitexists(.@gid) ) end;
	.@map$ = get_instance_var("map$");
	.@crystal = get_instance_var("crystal");
	if ( get_instance_var("level") >= 3 && mobcount(.@map$, instance_npcname(strnpcinfo(0)) + "::OnArmorKill") ) {
		end;
	}
	if ( unitexists(.@gid) ) {
		unitskilluseid .@gid, "NPC_GROGGY_ON", 5, .@gid, -10000;
		unitskilluseid .@gid, "NPC_RELIEVE_OFF", 1, .@gid, -10000;
		if ( unitexists(.@crystal) ) {
			unitskilluseid .@crystal, "NPC_GROGGY_ON", 5, .@crystal, -10000;
			unitskilluseid .@crystal, "NPC_RELIEVE_OFF", 1, .@crystal, -10000;
		}
		switch ( get_instance_var("level") ) {
			case 1:
				.@shield = 6;
				break;
			case 2:
				.@shield = 7;
				break;
			case 3:
				.@shield = 8;
				break;
			case 4:
			case 5:
				.@shield = 9;
				
				break;
		}
		unitskilluseid .@gid, "NPC_RELIEVE_ON", .@shield, .@gid, -10000;
		if ( unitexists(.@crystal) ) {
			unitskilluseid .@crystal, "NPC_RELIEVE_ON", .@shield - 1, .@crystal, -10000;
		}
		sleep 5000;
		if ( unitexists(.@gid) ) {
			unitskilluseid .@gid, "NPC_RELIEVE_OFF", 1, .@gid, -10000;
			unitskilluseid .@gid, "NPC_RELIEVE_ON", 10, .@gid, -10000;
		}
		if ( unitexists(.@crystal) ) {
			unitskilluseid .@crystal, "NPC_RELIEVE_OFF", 1, .@crystal, -10000;
			unitskilluseid .@crystal, "NPC_RELIEVE_ON", 10, .@crystal, -10000;
		}
		if ( get_instance_var("level") >= 3 && get_instance_var("last_gate") > 0 ) {
			instance_enable("#rdw_gate_" + get_instance_var("last_gate"), false);
			set_instance_var("last_gate", 0);
		}
		initnpctimer;
	}
end;
}

1@ch1b,1,1,0	script	ch1_rdw_level_1	-1,{
	end;

OnSpawnGuardian:
	stopnpctimer;
	function boss_exist;
	if ( !boss_exist() ) {
		end;
	}
	function crystal_exist;
	if ( !crystal_exist() && get_instance_var("crystal_spawned") ) {
		end;
	}
	if ( get_instance_var("level") < 3 ) {
		.@pos = rand(1, 30);
	} else {
		.@pos = rand(1, 25);
	}
	switch ( .@pos ) {
		case 1: .@x = 83; .@y = 135; break;
		case 2: .@x = 83; .@y = 152; break;	
		case 3: .@x = 88; .@y = 135; break;
		case 4: .@x = 88; .@y = 152; break;
		case 5: .@x = 82; .@y = 132; break;
		case 6: .@x = 89; .@y = 132; break;
		case 7: .@x = 82; .@y = 154; break;
		case 8: .@x = 89; .@y = 154; break;
		case 9: .@x = 88; .@y = 141; break;
		case 10: .@x = 88; .@y = 146; break;
		case 11: .@x = 83; .@y = 141; break;
		case 12: .@x = 83; .@y = 146; break;
		case 13: .@x = 94; .@y = 153; break;
		case 14: .@x = 95; .@y = 151; break;
		case 15: .@x = 95; .@y = 135; break;
		case 16: .@x = 94; .@y = 133; break;
		case 17: .@x = 78; .@y = 134; break;
		case 18: .@x = 76; .@y = 136; break;
		case 19: .@x = 77; .@y = 153; break;
		case 20: .@x = 76; .@y = 151; break;
		case 21: .@x = 86; .@y = 135; break;
		case 22: .@x = 94; .@y = 143; break;
		case 23: .@x = 86; .@y = 152; break;
		case 24: .@x = 77; .@y = 143; break;
		case 25: .@x = 85; .@y = 157; break;
		case 26: .@x = 70; .@y = 134; break;
		case 27: .@x = 70; .@y = 154; break;
		case 28: .@x = 101; .@y = 134; break;
		case 29: .@x = 101; .@y = 153; break;
		case 30: .@x = 86; .@y = 125; break;
	}
	if ( !boss_exist() ) {
		end;
	}
	.@map$ = get_instance_var("map$");
	if ( !crystal_exist() && !get_instance_var("crystal_spawned") ) {
		instance_announce instance_id(), "A Vessel containing Himmelmez's life force has appeared. Himmelmez will continue to heal as long as the vessel is alive.", BC_MAP, 0xFFFF00;
		monster .@map$, .@x, .@y, "--en--", 22499, 1, instance_npcname(strnpcinfo(0)) + "::OnCrystalKill", Size_Large;
		set_instance_var("crystal", $@mobid[0]);
		set_instance_var("crystal_spawned", true);
		unitskilluseid $@mobid[0], "NPC_RELIEVE_ON", 10, $@mobid[0];
		initnpctimer instance_npcname("ch1_rdw_heal");
	} 
	else if ( crystal_exist() ) {
		unitwarp get_instance_var("crystal"), .@map$, .@x, .@y;
	}
end;

OnCrystalKill:
	stopnpctimer instance_npcname("ch1_rdw_heal");
end;

function	crystal_exist	{
	return unitexists( get_instance_var("crystal") );
}

function	boss_exist	{
	return unitexists( get_instance_var("gid") );
}

}

1@ch1b,1,1,0	script	ch1_rdw_heal	-1,{
	end;
	
OnTimer10000:
	stopnpctimer;
	function crystal_exist;
	function boss_exist;
	if ( !crystal_exist() || !boss_exist() ) {
		end;
	}
	getunitdata get_instance_var("gid"), .@data;
	switch( get_instance_var("level") ) {
		case 1:
			.@regen_percentage = 10;
			break;
		case 2:
			.@regen_percentage = 11;
			break;		
		case 3:
			.@regen_percentage = 12;
			break;
		case 4:
			.@regen_percentage = 13;
			break;
		case 5:
			.@regen_percentage = 15;
			break;
	}
	.@HEAL = .@data[UMOB_HP] + ((.@regen_percentage * get_instance_var("max_hp")) / 100);
	if ( boss_exist() ) {
		setunitdata get_instance_var("gid"), UMOB_HP, min( .@HEAL, get_instance_var("max_hp") );
		getunitdata get_instance_var("crystal"), .@data2;
		if ( crystal_exist() ) {
			if ( .@data2[UMOB_HP] < .@HEAL ) {
				setunitdata get_instance_var("crystal"), UMOB_HP, min ( .@HEAL, get_instance_var("max_hp") );
			}
		}
	}
	if ( !crystal_exist() || !boss_exist() ) {
		end;
	}
	initnpctimer;
end;

function	crystal_exist	{
	return unitexists( get_instance_var("crystal") );
}

function	boss_exist	{
	return unitexists( get_instance_var("gid") );
}

}

1@ch1b,1,1,0	script	ch1_rdw_level_2	-1,{
	end;
	
OnStart:
	sleep 3000;
	instance_event(strnpcinfo(0), "OnUseSkill", false);
end;

OnSkill1:
	.@x = 81;
	.@y = 138;
	monster get_instance_var("map$"), .@x, .@y, "", 20562, 1, instance_npcname(strnpcinfo(0)) + "::OnSkillDead";
	setunitdata $@mobid[0],UMOB_DMGIMMUNE,true;
	setunitdata $@mobid[0], UMOB_INT, 750;
	.@gid = $@mobid[0];
	.@boss_gid = get_instance_var("gid");
	if ( unitexists(.@boss_gid) ) {
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y, -10000, false, 0, true;
	}
	sleep 200;
	for ( .@i = 1; .@i <= 8; ++.@i ) {
		if ( !unitexists(.@boss_gid) ) {
			end;
		}
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x + (2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x + (-2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y + (2 * .@i), -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y + (-2 * .@i), -10000, false, 0, true;
		sleep 200;
	}
	if ( unitexists(.@boss_gid) ) {
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y, -10000, false, 0, true;
	}
	sleep 200;
	for ( .@i = 1; .@i <= 8; ++.@i ) {
		if ( !unitexists(.@boss_gid) ) {
			end;
		}
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x + (2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x + (-2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y + (2 * .@i), -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y + (-2 * .@i), -10000, false, 0, true;
		sleep 200;
	}
end;

OnSkill2:
	.@x = 77;
	.@y = 152;
	monster get_instance_var("map$"), .@x, .@y, "", 20562, 1, instance_npcname(strnpcinfo(0)) + "::OnSkillDead";
	setunitdata $@mobid[0],UMOB_DMGIMMUNE,true;
	setunitdata $@mobid[0], UMOB_INT, 750;
	.@gid = $@mobid[0];
	.@boss_gid = get_instance_var("gid");
	if ( unitexists(.@boss_gid) ) {
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y, -10000, false, 0, true;
	}
	sleep 200;
	for ( .@i = 1; .@i <= 8; ++.@i ) {
		if ( !unitexists(.@boss_gid) ) {
			end;
		}
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x + (2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x + (-2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y + (2 * .@i), -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y + (-2 * .@i), -10000, false, 0, true;
		sleep 200;
	}
	if ( unitexists(.@boss_gid) ) {
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y, -10000, false, 0, true;
	}
	sleep 200;
	for ( .@i = 1; .@i <= 8; ++.@i ) {
		if ( !unitexists(.@boss_gid) ) {
			end;
		}
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x + (2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x + (-2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y + (2 * .@i), -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y + (-2 * .@i), -10000, false, 0, true;
		sleep 200;
	}
end;

OnSkill3:
	.@x = 96;
	.@y = 153;
	monster get_instance_var("map$"), .@x, .@y, "", 20562, 1, instance_npcname(strnpcinfo(0)) + "::OnSkillDead";
	setunitdata $@mobid[0],UMOB_DMGIMMUNE,true;
	setunitdata $@mobid[0], UMOB_INT, 750;
	.@gid = $@mobid[0];
	.@boss_gid = get_instance_var("gid");
	if ( unitexists(.@boss_gid) ) {
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y, -10000, false, 0, true;
	}
	sleep 200;
	for ( .@i = 1; .@i <= 8; ++.@i ) {
		if ( !unitexists(.@boss_gid) ) {
			end;
		}
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x + (2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x + (-2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y + (2 * .@i), -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y + (-2 * .@i), -10000, false, 0, true;
		sleep 200;
	}
	if ( unitexists(.@boss_gid) ) {
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y, -10000, false, 0, true;
	}
	sleep 200;
	for ( .@i = 1; .@i <= 8; ++.@i ) {
		if ( !unitexists(.@boss_gid) ) {
			end;
		}
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x + (2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x + (-2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y + (2 * .@i), -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y + (-2 * .@i), -10000, false, 0, true;
		sleep 200;
	}
end;

OnSkill4:
	.@x = 96;
	.@y = 134;
	monster get_instance_var("map$"), .@x, .@y, "", 20562, 1, instance_npcname(strnpcinfo(0)) + "::OnSkillDead";
	setunitdata $@mobid[0],UMOB_DMGIMMUNE,true;
	setunitdata $@mobid[0], UMOB_INT, 750;
	.@gid = $@mobid[0];
	.@boss_gid = get_instance_var("gid");
	if ( unitexists(.@boss_gid) ) {
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y, -10000, false, 0, true;
	}
	sleep 200;
	for ( .@i = 1; .@i <= 8; ++.@i ) {
		if ( !unitexists(.@boss_gid) ) {
			end;
		}
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x + (2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x + (-2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y + (2 * .@i), -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_GROUNDDRIVE", 5, .@x, .@y + (-2 * .@i), -10000, false, 0, true;
		sleep 200;
	}
	if ( unitexists(.@boss_gid) ) {
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y, -10000, false, 0, true;
	}
	sleep 200;
	for ( .@i = 1; .@i <= 8; ++.@i ) {
		if ( !unitexists(.@boss_gid) ) {
			end;
		}
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x + (2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x + (-2 * .@i), .@y, -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y + (2 * .@i), -10000, false, 0, true;
		unitskillusepos .@gid, "NPC_RAINOFMETEOR", 1, .@x, .@y + (-2 * .@i), -10000, false, 0, true;
		sleep 200;
	}
end;

OnUseSkill:
	.@boss_gid = get_instance_var("gid");
	if ( !unitexists(.@boss_gid) ) {
		end;
	}
	killmonster get_instance_var("map$"), instance_npcname("ch1_rdw_level_2") + "::OnSkillDead";
	if ( !unitexists(.@boss_gid) ) {
		end;
	}
	instance_event("ch1_rdw_level_2", "OnSkill1", false);
	instance_event("ch1_rdw_level_2", "OnSkill2", false);
	instance_event("ch1_rdw_level_2", "OnSkill3", false);
	instance_event("ch1_rdw_level_2", "OnSkill4", false);
	initnpctimer;
end;

OnTimer6000:
	killmonster get_instance_var("map$"), instance_npcname(strnpcinfo(0)) + "::OnSkillDead";
end;

OnTimer14000:
	stopnpctimer;
	instance_event(strnpcinfo(0), "OnUseSkill", false);
end;

OnSkillDead:
end;
}

1@ch1b,1,1,0	script	ch1_rdw_level_4	-1,{
	end;
	
OnStart:
end;
}

1@ch1b,1,1,0	script	ch1_rdw_level_5	-1,{
	end;
	
OnStart:
end;
}

1@ch1b,70,134,1	script(DISABLED)	#rdw_gate_1	OBJ_A2,1,1,{
	end;
	
OnTouch:
	if ( get_instance_var("level") >= 3 ) {
		warp get_instance_var("map$"), 270, 294;
	}
end;
}

1@ch1b,70,154,1	duplicate(#rdw_gate_1)	#rdw_gate_2	OBJ_A2,1,1
1@ch1b,101,154,1	duplicate(#rdw_gate_1)	#rdw_gate_3	OBJ_A2,1,1
1@ch1b,101,134,1	duplicate(#rdw_gate_1)	#rdw_gate_4	OBJ_A2,1,1
1@ch1b,86,125,1	duplicate(#rdw_gate_1)	#rdw_gate_5	OBJ_A2,1,1

1@ch1b,1,1,0	script	#ch1_boss_exit-2	HIDDEN_WARP_NPC,{
	end;
	
OnStart:
	initnpctimer;
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 1 minute.", BC_MAP, 0xFFFF00;
end;
	
OnTimer10000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 50 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer20000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 40 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer30000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 30 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer40000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 20 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer50000:
	instance_announce instance_id(),"SYSTEM : The simulated room became unstable. All entities will be ejected in 10 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer55000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected in 5 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer56000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected in 4 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer57000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected in 3 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer58000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected in 2 seconds.", BC_MAP, 0xFFFF00;
end;

OnTimer59000:
	instance_announce instance_id(),"SYSTEM : All entities will be ejected by force.", BC_MAP, 0xFFFF00;
end;

OnTimer60000:
	instance_warpall "hem_dun01", 208, 246, instance_id();
end;

OnTimer61000:
	stopnpctimer;
	sleep 10;
	instance_destroy();
end;
}

1@ch1b,259,352,0	script	#ch1_odw_warp_2	WARPNPC,2,2,{
	end;
	
OnTouch:
	warp get_instance_var("map$"), 89, 153;
end;
}