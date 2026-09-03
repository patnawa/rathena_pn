//===== rathena script ===========================================|
//= Chapter 1 - Call of the world tree - Prologue                =|
//===== By: ======================================================|
//= crazyarashi                                                  =|
//===== Current Version: =========================================|
//= 1.0 Initial Version                                          =|
//================================================================|

prt_fild05,353,252,3	script	Small Ash Tree#c1p1_01	4_CH1_BRANCH,{
	if ( BaseLevel < 200 ) {
		mes "There's a sparkling young tree.";
		mes " ";
		mes "^4d4dffThis quest is available for players who are level 200 and above.^000000";
		close;
	}
	if ( BaseLevel >= 200 && isbegin_quest(18368) == 0 ) {
		cutin "ch1_delegacy04.png",2;
		mes "[Tiny Fairy]";
		mes "Finally, a person who resonated with the world tree!";
		mes "Hi? How are you? You look strong and healthy.";
		mes "How about we go to a magnificent place.";
		next;
		mes "[Tiny Fairy]";
		mes "You are the one, if you can see me! Adventurer, you have been chosen!";
		mes "You've met me because of the will of the world tree, you should go with me!";
		next;
		select("Where did you suddenly come from?");
		mes "[Lapine Lara]";
		mes "I'm Lara from Eclage!";
		mes "Let me repeat it again, I feel such great energy from you.";
		mes "Go with me!";
		next;
		select("I'm not interested in buying. I'm also not going.");
		mes "[Lapine Lara]";
		mes "The people from Midgard are really the same.";
		mes "I'm not selling anything here, nor I am doing solicitations. I'm here to prevent the possible end of the world.";
		next;
		select("An apocalypse believer? Not buying the excuse.");
		mes "[Lapine Lara]";
		mes "I keep telling this to everyone I meet, but no one believes me...";
		mes "How about this? Are you not curious to see the village in the world tree?";
		mes "Only the top adventurers of Midgard gets invited~!";
		next;
		select("Oh, that's interesting!", "Tell me more.");
		mes "[Lapine Lara]";
		mes "I'm a messenger of the world tree. On it's branches, we established a village for adventurers called Ashridge!";
		mes "When do you think you'll get another chance to go to the world tree in person?";
		mes "It's a huge tree, a very huge tree! The world tree itself! Don't you want to go and visit it yourself?";
		next;
		select("I'm bought. Let's go!", "I can't decline now, right?");
		mes "[Lapine Lara]";
		mes "There's no running now! A good choice. Surely, you'll make a lot of friends there!";
		mes "I gathered and sent many adventurers from all over the Midgard continent.";
		next;
		mes "[Lapine Lara]";
		mes "I've connected a passage through Hvergelmir to where the world tree's branches extends.";
		mes "You'll be able to go the village through this Ash Tree. Now, let's go to Ashridge!";
		setquest 18368;
		next;
		if ( select("Let's go now.","Give me a moment.") == 2 ) {
			mes "[Lapine Lara]";
			mes "Okay. Since we're preparing for something big, I'll let you prepare first.";
			mes "Once you're ready, just put your hands on the branch of this ash tree!";
			close3;
		}
		mes "[Lapine Lara]";
		mes "Do you see this Small Ash Tree? It's a miniature world tree.";
		mes "When you place your hands on the branch, you'll be transported to the actual world tree.";
		mes "Come, place your hands on it!";
		close2;
		cutin "",255;
		warp "ygg_edge",255,240;
		end;
	}
	mes "You can travel to Ashridge through the branch.";
	next;
	if ( select("Place your hands on it.","Stop.") == 2 ) {
		mes "You decided to use it later.";
		close;
	}
	if ( strnpcinfo(2) == "c1p1_01" )
		warp "ygg_edge", 212, 95;
	else
		warp "ygg_edge", 194, 97;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "BaseLevel >= 200 && isbegin_quest(18368) == 0";
end;
}

function	script	CH1_Complete	{
	return isbegin_quest(18377) == 2;
}


pay_fild01,308,341,3	duplicate(Small Ash Tree#c1p1_01)	Small Ash Tree#c1p1_02	4_CH1_BRANCH

ygg_edge,253,246,4	script	Lapine Messenger#c1p1_04	4_CH1_DELEGACY02,{
	if ( isbegin_quest(18368) == 1 ) {
		cutin "ch1_delegacy02.png",2;
		mes "[Lapine Messenger]";
		mes "Welcome to Ashridge, the end of the world tree's branch!";
		mes "When you need to return to Midgard, you can go through to Hvergelmir's Spring.";
		mes "Or you can follow the young branches through here to return to Prontera or Payon.";
		next;
		mes "[Lapine Messenger]";
		mes "Do you mind following this branch?";
		mes "Just keep going straight~ Yes. All the way to the fruit~!";
		mes "Don't worry, surely one of your companions is here~!";
		completequest 18368;
		setquest 18369;
		navigateto("ygg_fruit", 80, 122);
		close3;
	}
	npctalk "This branch leads to Hvergelmir's Spring.", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18368) == 1";
end;
}

ygg_fruit,73,117,0	script	#ch1p1evt01	HIDDEN_WARP_NPC,1,1,{
	end;
	
OnTouch:
	if ( isbegin_quest(18368) == 1 ) {
		completequest 18368;
		setquest 18369;
	}
	if ( isbegin_quest(18369) == 1 ) {
		cloaknpc("Maysel#c1p1may01", true, getcharid(0));
		cloaknpc("Est#c1p1est01", false, getcharid(0));
		cloaknpc("Debris#c1p1demi01", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(18370) == 1 ) {
		//= Custom: official keeps the NPC invisible
		cloaknpc("Debris#c1p1demi01", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(18378) == 1 ) {
		cloaknpc("Debris#c1p1demi01", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(12665) == 1 ) {
		cloaknpc("Est#c1p1est01", false, getcharid(0));
		cloaknpc("Debris#c1p1demi01", false, getcharid(0));
		end;
	}
end;
}

ygg_fruit,80,122,3	script	Lapine Shasha#c1p1ssha01	4_CH1_DELEGACY01,{
	if ( isbegin_quest(18369) == 1 ) {
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Another adventurer has arrived! Welcome.";
		mes "I am Shasha from Eclage, My job here is pass on the will of the world tree.";
		next;
		mes "[Shasha]";
		mes "Firstly, you should greet the others who arrived before you.";
		mes "You will be working together from now on, do you understand?";
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "If they're gathering the talented people in the continent, of course you were also called.";
		next;
		select("Why are you here Est?");
		mes "[Est]";
		mes "These little guys suddenly appeared asking for help, so I came here to verify it myself.";
		mes "I only realized the urgency the situation after seeing it for myself. That's why I'm staying here to support them.";
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "Shasha, Is it fine if I leave first? Our information is not lacking, I need to investigate more.";
		next;
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Feel free to do so. The others have left already, saying that they've found something interesting.";
		next;
		cutin "3rd_gn_dbris01.bmp",2;
		mes "[Debris]";
		mes "Hm? Who are you?";
		mes "Did you also meet a little fairy and ended up coming here out of curiosity?";
		mes "Your name....? Hm? I'll just call you Popo.";
		next;
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "Nice to meet you!";
		mes "Sometimes it's fun to step outside the lab and experience thing yourself.";
		mes "When else would I get a chance to go study a place like this?";
		next;
		mes "[Debris]";
		mes "Anyways, I'm the one in charge of analyzing the things that people bring here.";
		mes "Do you know that I'm a botanist and quite an expert when it comes to plants?";
		mes "I'm excited working with you Popo!";
		next;
		mes "[Debris]";
		mes "Studying this tree and the fairies is also a great experience, don't you agree?";
		mes "Thinking about it, a lot of people came in here but they've already gone off somewhere to investigate.";
		mes "Popo, Maysel is here. Go ahead and introduce yourself.";
		changequest 18369, 18370;
		close2;
		cutin "",255;
		cloaknpc("Est#c1p1est01", true, getcharid(0));
		cloaknpc("Maysel#c1p1may01", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(18370) == 1 ) {
		npctalk "Maysel, perfect timing!", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(18371) == 1 ) {
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "One of the world tree's branches is connected to the Land of Darkness.";
		mes "Ask Naviel to guide you if you don't know the way.";
		close3;
	}
	if ( isbegin_quest(18378) == 1 ) {
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "How was the Land of Darkness?";
		mes "We've received some information about the parallel gates that's appearing there and we are currently analyzing it.";
		next;
		mes "[Shasha]";
		mes "I'll share the decision that we've made so far.";
		mes "First, we've decided to call the parallel dimension the Second World.";
		mes "We needed a way to distinguish it to avoid confusion from the world we live in.";
		mes "Eager to help, a lot of people already left.";
		next;
		select("The second world?");
		cutin "gc_mayssel01.bmp",0;
		mes "[Maysel]";
		mes "Didn't you experience the invasion from beyond our dimension?";
		mes "If I recall it correctly, she was called Himmelmez....";
		mes "The collapsed of the dimensional boundary is what Yggdrasil feared.";
		next;
		mes "[Maysel]";
		mes "Enemies from that world is more threatening than ours.";
		mes "The Twisted Land of Darkness is the results of another world colliding with ours...";
		next;
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "Geffen was mentioned, right?";
		mes "So it comes to the point that we're already connected to a parallel world.";
		mes "Isn't the adventurer here already familiar with it?";
		next;
		select("Isn't that the place we go every year...?");
		mes "[Debris]";
		mes "It's probably the same place.";
		mes "There are signs that proves it is.";
		mes "The important thing now is to continue investigating.";
		next;
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Yes. If Yggdrasil itself sense it as a threat, then this is incredibly serious situation.";
		mes "This is truly something that will determine the fate of our world..!";
		next;
		mes "[Shasha]";
		mes "A lot of people are already conducting investigations in various locations...";
		mes "The Gray Area will be handled by the Assassin Guild.";
		next;
		cutin "gc_mayssel01.bmp",0;
		mes "[Maysel]";
		mes "They specialize in gathering information discreetly.";
		mes "Other than that...";
		mes "The most problematic one now is the region that's too hot to approach.";
		mes "It's currently being studied by the Alchemist and Wizard Guild.";
		next;
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "That's right.";
		mes "We're deciding whether to create a heat resistance potion or maybe even a scroll.";
		next;
		mes "[Debris]";
		mes "For that, we'll need Popo's help.";
		mes "Popo, do you know what's a Dragon Scale ^i[1036]? It's an excellent material for resisting both fire and ice.";
		next;
		mes "[Debris]";
		mes "You understand what you have to do, yes? You're an adventurer, so have you figure it out?";
		mes "Bring me 5 Dragon Scales.";
		mes "And I promise you that I'll make you something incredibly effective!";
		next;
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Everyone else already moved to their respective locations to begin investigating.";
		mes "It would be great if you could assist is by getting the Dragon Scales.";
		next;
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "Popo, just bring the Dragon Scales to my lab!";
		mes "I'll be waiting there!";
		changequest 18378, 18373;
		close2;
		cutin "",255;
		cloaknpc("Debris#c1p1demi01", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(18373) == 1 ) {
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Debris' lab is in the Long Fruit.";
		mes "Ask Naviel if you need help finding it.";
		close3;
	}
	if ( isbegin_quest(8969) == 1 ) {
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Ah! Welcome back!";
		mes "Did you find anything useful in Brimir?";
		mes ".. I guess the results weren't that great based on your expression.";
		next;
		cutin "gc_mayssel01.bmp",0;
		mes "[Maysel]";
		mes "According to the reports we've received, Brimir is already a decayed dimension.";
		mes "A cure couldn't possibly exist there.";
		mes "It's up to us to develop a proper treatment through our own research.";
		next;
		mes "[Shasha]";
		mes "Hmhm.";
		mes "Thanks to people who stayed in Ashridge and continued their research, we've progressed quite a bit.";
		cutin "ch1_delegacy01.png",1;
		next;
		mes "[Shasha]";
		mes "The parasitic spores can be countered through divine power, magic, and medicine.";
		next;
		select("Will the cure be ready soon?");
		mes "[Shasha]";
		mes "It will take longer than expected.";
		mes "While the spores have some reactions on different kind of power, they don't completely vanish unless all three are applied simultaneously..";
		mes "We only see results when it's on a certain harmony.";
		next;
		cutin "gc_mayssel01.bmp",0;
		mes "[Maysel]";
		mes "We have no way to remove the roots yet, but we can halt it the progression of the disease.";
		mes "There's currently a hypothesis that the true cure when we found the perfect balance and harmony between the three powers.";
		next;
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "We need to find a way to perfectly combine divine power, magic, and herbal compounds in harmony.";
		mes "A perfect catalyst that will unify those three things";
		mes "It feels like we're missing one piece of information.";
		next;
		cutin "gc_mayssel01.bmp",0;
		mes "[Maysel]";
		mes "We definitely need to research it more.";
		mes "We can also go to Muspelheim now.";
		mes "The fire resistance potion is already finished and the advanced party is already on area.";
		next;
		cloaknpc("Newt#c1p1nuit01", false, getcharid(0));
		mes "[Lapine Newt]";
		mes "Shasha! We're back!";
		mes "Huh? Adventurer~! You're here on time~!";
		mes "Shasha! Guess what we found? It might be the key to the cure to the plague!";
		cutin "ch1_delegacy02.png",1;
		next;
		select("What do you mean?");
		mes "[Newt]";
		mes "As you know we explored the hot regions~!";
		mes "The Harbans in Volund Valley are skilled artisans.";
		mes "They mentioned something about an special thing and it sounds powerful!";
		next;
		mes "[Newt]";
		mes "I'm not exactly sure what it is, they said nothing is impossible with that thing.";
		mes "Isn't that exactly what we need right now?";
		next;
		mes "[Newt]";
		mes "Anyway, I'm off to organize what I found!";
		mes "What Shasha's looking for might be in Volund!";
		mes "Okay, I got to go. I'm super busy!";
		erasequest 8969;
		setquest 18374;
		close2;
		cutin "",255;
		cloaknpc("Newt#c1p1nuit01", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(18374) == 1 ) {
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "What could that be?";
		mes "If what Newt said is true, that might be the key to the cure.";
		mes "Adventurer, go to Debris in the long fruit.";
		mes "Please meet him and head to Muspelheim.";
		next;
		mes "[Shasha]";
		mes "Muspelheim is in the same world. The residents who lives there are friendly.";
		mes "Please find whatever that special thing that he mentioned.";
		erasequest 18374;
		setquest 18375;
		close3;
	}
	if ( isbegin_quest(18375) == 1 ) {
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Debris in the long fruit conducting his research.";
		mes "You'll get the fire resistance potion there.";
		mes "Once you received the protection, please go to the scout party in Muspelheim.";
		close3;
	}
	if ( isbegin_quest(12665) == 1 ) {
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Welcome back. Huh? What's with your expression.";
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "Something's off. It's strange no matter how much I think about it.";
		mes "Adventurer, you remembered what happened at the end, right?";
		mes "How did it look to you?";
		next;
		cutin "3rd_gn_dbris01.bmp",0;
		mes "[Debris]";
		mes "What about it? What's going on?";
		mes "Didn't you defeat the witch, doesn't that mean that we don't to worry about the disease?";
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "We did defeat the witch. But we never confirmed that we ended her.";
		next;
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "What do you mean?";
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "Exactly what I said. Adventurer, think carefully.";
		mes "Right after Himmelmez fell, a dimensional rift opened behind her.";
		mes "Then...";
		next;
		select("It looked like something snatched Himmelmez away.");
		mes "[Est]";
		mes "Yes.";
		mes "The threat the world tree is sensing.";
		mes "Maybe we're facing a massive enemy that we don't even know the true form of yet.";
		mes "We need to find out what it is.";
		next;
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "So you're saying there's something else that caused the world to end up like this, something beyond the dimensional rift Morroc created that broke the balance of the world?";
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "That's what we need to figure out right now, no?";
		mes "Sigh, but how do we even figure that out?";
		mes "Everything happened so fast. Suddenly, there's another being that can interfere between dimensions...";
		next;
		cutin "3rd_gn_dbris01.bmp",0;
		mes "[Debris]";
		mes "Then we've got no choice but to keep investigating until we can figure it out, right?";
		mes "We've got plenty of mages who've studied the Illusion Codex, right~?";
		next;
		cutin "ch1_delegacy01.png",1;
		mes "[Shasha]";
		mes "Sounds like a plan.";
		mes "Let's keep investigating the place Est want to keep an eye on until we uncover something.";
		next;
		cutin "ep162_est01.bmp",2;
		mes "[Est]";
		mes "Tch. I guess I've got no choice.";
		mes "Alright. Let's leave that to those Illusion Codex types or whatever.";
		next;
		cutin "gc_mayssel01.bmp",1;
		mes "[Maysel]";
		mes "Then we'll reinvestigate the areas where rifts occurred, focusing on any common areas or familiar signs.";
		next;
		cutin "3rd_gn_dbris04.bmp",0;
		mes "[Debris]";
		mes "I'll also research anything that might help.";
		mes "Popo, you better work hard too!";
		next;
		cutin "ch1_delegacy01.png",0;
		mes "[Shasha]";
		mes "Yggdrasil is currently silent.";
		mes "It normally doesn't express its will so strongly.";
		mes "The fact that it's gone this far is what's strange.";
		next;
		mes "[Shasha]";
		mes "For now, we'll just have to search for more clues with what we have.";
		mes "Adventurer, take some time to rest before your next journey.";
		next;
		mes "[Shasha]";
		mes "Oh! I'll make sure the Illusion Codex team is informed for you.";
		mes "Feel free to visit her anytime.";
		erasequest 12665;
		setquest 18377;
		completequest 18377;
		setquest 12660;
		getitem "Ch1_Root_Coin",10;
		getexp 195914233, 4000000;
		close3;
	}
	if ( CH1_Complete() ) {
		cutin "ch1_delegacy01.png",0;
		mes "[Shasha]";
		mes "Yggdrasil is currently silent.";
		mes "It normally doesn't express its will so strongly.";
		mes "The fact that it's gone this far is what's strange.";
		next;
		mes "[Shasha]";
		mes "Oh! I'll make sure the Illusion Codex team is informed for you.";
		mes "Feel free to visit her anytime.";
		close3;
	}
	end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18369) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18378) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8969) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18374) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(12665) == 1";
end;
}

ygg_fruit,82,120,3	script	Maysel#c1p1may01	4_F_MAYSEL,{
	if ( isbegin_quest(18370) == 1 ) {
		cutin "ch1_delegacy01.png",0;
		mes "[Shasha]";
		mes "Maysel, You're just in time.";
		mes "This is the new adventurer. The adventurer will be helping us investigate the parallel gates!";
		next;
		cutin "gc_mayssel01.bmp",1;
		mes "[Maysel]";
		mes "Oh, I see. Have we met at the Criaturia Academy before?";
		mes "By the way, Shasha, the others have already arrived at the Land of Darkness.";
		next;
		mes "[Maysel]";
		mes "The gate connected to Ashridge is working perfectly.";
		mes "It's truly impressive.";
		mes "However, one of the distorted space is causing some issues...";
		next;
		cutin "ch1_delegacy01.png",0;
		mes "[Shasha]";
		mes "Aha! If you're short on hands, this amazing adventurer here will help you right away!";
		next;
		select("What's this all about?");
		cutin "gc_mayssel01.bmp",1;
		mes "[Maysel]";
		mes "Hehe. You'll understand once you see it.";
		mes "This world is all about experience.";
		next;
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "Oh, yes. Popo here is strong and will surely be a huge help!";
		mes "Fieldwork isn't really my thing.";
		next;
		cutin "ch1_delegacy01.png",0;
		mes "[Shasha]";
		mes "We can tell by just looking. Anyway, the world is in great danger!";
		next;
		select("It feels like it's always in danger...");
		mes "[Shasha]";
		mes "Do you remember when another world started merging into Midgard?";
		mes "Can you imagine that same scenario happening again?";
		next;
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "Two world merging, what kind of a horrifying idea is that?";
		mes "Why do you think parallel dimension exists in the first place?";
		mes "It's called parallel because they're not supposed to meet ever.";
		next;
		cutin "ch1_delegacy01.png",0;
		mes "[Shasha]";
		mes "But it did happen, correct?";
		mes "Normally, my home Alfheimr and the lower world Jotunheimr shouldn't be connected, not without the Bifrost or the world tree.";
		mes "But Morroc's dimensional rift completely threw the balance into chaos";
		next;
		mes "[Shasha]";
		mes "If the worlds that should have been separated starts interfering and affect each other, it could bring everything to ruin.";
		mes "That's why the world tree has started to act.";
		next;
		mes "[Shasha]";
		mes "The will of the world tree was originally meant to be conveyed directly to the people of Midgard, but there was a problem with the world's link.";
		mes "That's why we Lapines have went into Midgard to deliver its will.";
		next;
		cutin "3rd_gn_dbris01.bmp",2;
		mes "[Debris]";
		mes "I'll ask a question on behalf of the adventurer!";
		mes "How is this new phenomenon, is the parallel gates different from the known dimensional rifts?";
		next;
		cutin "ch1_delegacy01.png",0;
		mes "[Shasha]";
		mes "Let's call the place where we are now as World 1, okay?";
		mes "The previous dimensional rifts were disruptions within World 1's own dimension.";
		mes "Like a basket of apples where the big apples, small apples, red apples and green apples got mixed together.";
		next;
		mes "[Shasha]";
		mes "But the phenomenon that we call decaying or parallel gates.";
		mes "It's the phenomenon where our world starts colliding with world 2 and world 3.";
		mes "Imagine if an apple basket, peach basket, and strawberry basket got dumped in the same basket together...";
		next;
		cutin "gc_mayssel01.bmp",1;
		mes "[Maysel]";
		mes "So, parallel dimensions are no longer parallel?";
		mes "That means the dimension that the world tree governed have collapsed....";
		next;
		cutin "ch1_delegacy01.png",0;
		mes "[Shasha]";
		mes "Exactly. The great sage did everything possible to prevent it, but in the end, the dimensional rifts still appeared.";
		mes "As a result, our world started eroding and merging with the other worlds.";
		next;
		select("Was it Himmelmez's invasion? Didn't we took care of that?");
		mes "[Shasha]";
		mes "Do you think there's only one parallel world?";
		mes "You're wrong. There are hundreds, even thousands of world lines.";
		mes "In places you've never seen before, the other world lines are also colliding as well.";
		next;
		mes "[Shasha]";
		mes "Alright! Enough explanation.";
		mes "Adventurer, your first task is to head north to the forbidden area, the Twisted Land of Darkness.";
		mes "Seeing is believing! Go there and witness the parallel gates yourself.";
		next;
		mes "[Shasha]";
		mes "There are already others investigating, so feel free to meet up with them.";
		next;
		select("How do I go to the Land of Darkness?");
		mes "[Shasha]";
		mes "One of the world tree's branches is connected to the Land of Darkness.";
		mes "If you don't know the way, ask Naviel to guide you.";
		changequest 18370, 18371;
		getexp 65304744, 4000000;
		getitem "Ch1_Root_Coin",10;
		close3;
	}
	npctalk "Handling information is what I do best.", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18370) == 1";
end;
}

ygg_fruit,76,116,1	script(CLOAKED)	Newt#c1p1nuit01	4_CH1_DELEGACY02,{
	end;
}

ygg_fruit,77,117,1	duplicate(dummynpc)	Est#c1p1est01	4_F_ESTLOVELOY

ygg_fruit,77,124,4	script(CLOAKED)	Debris#c1p1demi01	4_LGTSCIENCE,{
	if ( isbegin_quest(18370) == 0 ) end;
	npctalk "Sometimes, things like this can improve my results~", "", BC_SELF;
	end;
}

ygg_fruit,71,78,3	script	Debris#c1p1demi02	4_LGTSCIENCE,{
	if ( isbegin_quest(18373) == 1 ) {
		if ( countitem("Dragon_Scale") < 5 ) {
			cutin "3rd_gn_dbris01.bmp",2;
			mes "[Debris]";
			mes "Popo, I need atleast five Dragon Scales to research fire resistance potion!";
			mes "Now, hurry up and bring them to me!";
			close3;
		}
		mes "[Debris]";
		mes "Oh~ Popo you're back?";
		mes "Let's see, did you bring the Dragon Scales?";
		mes "Great. With these, I'll be able to craft an amazing potion!";
		next;
		mes "[Debris]";
		mes "By the way, they asked me to tell you to head to Second World's Geffen.";
		mes "A person called Ascetic Jeon is waiting for you there.";
		next;
		select("When can we go to the hot area?");
		mes "[Debris]";
		mes "... Popo, don't be so impatient.";
		mes "I'll work hard so you don't end up as a crispy jerky....";
		mes "Just focus on what you need to do now...";
		next;
		mes "[Debris]";
		mes "Now, go explore the world in the Distorted Space!";
		delitem "Dragon_Scale", 5;
		changequest 18373, 18372;
		getitem "Ch1_Root_Coin", 10;
		getexp 65304744, 4000000;
		close3;
	}
	if ( isbegin_quest(18372) == 1 ) {
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "Do what you need to do.";
		mes "Head to the second world's Geffen and meet the Ascetic Jeon.";
		close3;
	}
	if ( isbegin_quest(18375) == 1 ) {
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "Popo!!! Welcome!";
		mes "I've been waiting for you!";
		next;
		specialeffect2 EF_BEGINSPELL_YB;
		sleep2 500;
		specialeffect2 EF_BLUELINE;
		mes "[Debris]";
		mes "Popo! How are you feeling?";
		mes "The advanced party already got solid results, so there's nothing to worry about!";
		mes "Now you can wander anywhere in Volund Valley freely!";
		next;
		mes "[Debris]";
		mes "Are you wondering where Volund Valley is?";
		mes "It's in Muspelheim... Aha? You haven't been there before!";
		mes "Don't worry. You're heading there now.";
		next;
		mes "[Debris]";
		mes "Now the flames of Muspelheim won't even come near you anymore.";
		mes "Don't worry!";
		mes "Go and find the advanced party!";
		erasequest 18375;
		setquest 18376;
		getitem "Ch1_Root_Coin",10;
		getexp 65304744, 4000000;
		close3;
	}
	if ( isbegin_quest(18376) == 1 ) {
		cutin "3rd_gn_dbris04.bmp",2;
		mes "[Debris]";
		mes "Now the flames of Muspelheim won't even come near you anymore.";
		mes "Don't worry!";
		mes "Go and find the advanced party!";
		close3;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18373) == 1 && countitem(\"Dragon_Scale\") >= 5";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18375) == 1";
end;
}

ygg_roots,334,138,5	script	Investigation Point#ch01_1	4_POINT_BLUE,2,2,{
	end;
	
OnTouch:
	if ( isbegin_quest(18371) == 1 ) {
		select("Look around.");
		mes "- The ground is eroded with giant roots everywhere. -";
		mes "- It looks dangerous, so it's best to move carefully. -";
		emotion ET_STARE_ABOUT, getcharid(3);
		next;
		mes "- Carefully navigate the area and investigate the area. -";
		changequest 18371, 24069;
		close2;
		navigateto("ygg_roots", 299, 59);
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18371) == 1";
end;
}

ygg_roots,299,59,3	script	Investigation Point#ch01_2	4_POINT_BLUE,2,2,{
	end;

OnTouch:
	if ( isbegin_quest(24069) == 1 ) {
		select("Look around.");
		mes "- The grounds seems to be barely intact. -";
		mes "- The cracks looks so deep that it looks like a bottomless abyss. -";
		mes "- I should be careful not to fall while investigating this area. -";
		emotion ET_STARE_ABOUT, getcharid(3);
		changequest 24069, 24070;
		close2;
		navigateto("ygg_roots", 186, 117);
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24069) == 1";
end;
}

ygg_roots,186,117,3	script	Investigation Point#ch01_3	4_POINT_BLUE,2,2,{
	end;
	
OnTouch:
	if ( isbegin_quest(24070) == 1 ) {
		select("Look around.");
		mes "- The tree roots are intertwined. -";
		mes "- Is it me or it moved a little -";
		emotion ET_STARE_ABOUT, getcharid(3);
		next;
		mes "<FONT SIZE = 16><B>- *Crash* -</B></FONT>";
		next;
		mes "[???]";
		mes "<FONT SIZE = 16><B>Waaaaaah!!!</B></FONT>";
		next;
		mes "- As the ground shooked violently, a loud scream echoes nearby. -";
		mes "- Head towards the source of the sound and check if someone needs your help. -";
		changequest 24070, 24071;
		close2;
		navigateto("ygg_roots", 167, 135);
	}
end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24070) == 1";
end;
}

ygg_roots,167,135,4	script	Source of Sound#ch01	4_POINT_BLUE,{
	if ( isbegin_quest(24071) == 1 ) {
		//= Custom: Official repeats the first dialogue for no reason
		cutin "ch1_chez03.png",2;
		mes "[???]";
		mes "<FONT SIZE = 16><B>Kyaah!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Yggdrasil!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Let me go!!!</B></FONT>";
		cloaknpc("Chez#ch01", false, getcharid(0));
		cloaknpc("Yvain#ch01", false, getcharid(0));
		npctalk "Chez : Kyaah!!!", "Chez#ch01", BC_SELF;
		next;
		mes "[???]";
		mes "<FONT SIZE = 16><B>Yvain is stuck on the ground!</B></FONT>";
		next;
		mes "[???]";
		mes "<FONT SIZE = 16><B>Yvain, are you alright?</B></FONT>";
		mes "<FONT SIZE = 16><B>I'll get you out right away!!!</B></FONT>";
		next;
		mes "[???]";
		mes "<FONT SIZE = 16><B>Graaaaaaaah!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>These roots are too strong, it won't budget at all!!!</B></FONT>";
		emotion ET_PROFUSELY_SWAT, getnpcid(0,"Chez#ch01");
		npctalk "Chez : Graaaaaaaah!!!", "Chez#ch01", BC_SELF;
		next;
		cutin "ch1_yvain04.png",2;
		mes "[Yvain]";
		mes "Chez... Don't pull the roots like that will only tighten up.";
		mes "Argh... I can't breath...";
		next;
		mes "[Yvain]";
		mes "Calm down. Look around and see if someone is nearby to help us.";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Oh!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Adventurer!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Adventurer!!!!!!!!!!!!!!!!!!!!!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Please help us!!!!</B></FONT>";
		emotion ET_HELP, getnpcid(0,"Chez#ch01");
		npctalk "Chez : Help!!!!", "Chez#ch01", BC_SELF;
		next;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Grab the root over there and lift it up!</B></FONT>";
		mes "<FONT SIZE = 16><B>I'll pull Yvain out after you create an opening!</B></FONT>";
		changequest 24071, 24072;
		close2;
		cutin "",255;
		navigateto("ygg_roots", 166, 135);
		end;
	}
	if ( isbegin_quest(24072) == 1 ) {
		cloaknpc("Chez#ch01", false, getcharid(0));
		cloaknpc("Yvain#ch01", false, getcharid(0));
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Oh!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Adventurer!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Adventurer!!!!!!!!!!!!!!!!!!!!!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Please help us!!!!</B></FONT>";
		emotion ET_HELP, getnpcid(0,"Chez#ch01");
		npctalk "Chez : Help!!!!", "Chez#ch01", BC_SELF;
		next;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Hold the root over there and lift it up!</B></FONT>";
		mes "<FONT SIZE = 16><B>I'll pull Yvain out after you lift it up!</B></FONT>";
		close2;
		cutin "",255;
		navigateto("ygg_roots", 166, 135);
		end;
	}
	if ( isbegin_quest(24073) == 1 ) {
		mes "- Let's look around until we get more information. -";
		mes "- Let's start by checking the place up <B>ahead</B> where the grounds shakes a lot. -";
		close2;
		navigateto("ygg_roots", 153, 203);
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24071) == 1";
end;
}

ygg_roots,165,134,7	script(CLOAKED)	Chez#ch01	4_F_FAIRYKID4,{
	if ( isbegin_quest(24072) == 1 ) {
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Oh!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Adventurer!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Adventurer!!!!!!!!!!!!!!!!!!!!!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Please help us!!!!</B></FONT>";
		emotion ET_HELP, getnpcid(0,"Chez#ch01");
		npctalk "Chez : Heeelp!!!!", "Chez#ch01", BC_SELF;
		next;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Hold the root over there and lift it up!</B></FONT>";
		mes "<FONT SIZE = 16><B>I'll pull Yvain out after you lift it up!</B></FONT>";
		close2;
		cutin "",255;
		navigateto("ygg_roots", 166, 135);
		end;
	}
	end;
}

ygg_roots,166,135,5	script(CLOAKED)	Yvain#ch01	4_F_FAIRYKID2,2,2,{
	if ( isbegin_quest(24072) == 1 ) {
		select("Lift the roots.");
		mes "["+strcharinfo(0)+"]";
		mes "Hee-yah!!!";
		next;
		cutin "ch1_chez04.png",2;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>We did it!!!!</B></FONT>";
		emotion ET_SURPRISE, getnpcid(0,"Chez#ch01");
		npctalk "Chez : We did it!!!!", "Chez#ch01", BC_SELF;
		next;
		cutin "ch1_chez01.png",2;
		mes "[Chez]";
		mes "Ivain, are you okay?";
		next;
		cutin "ch1_yvain02.png",2;
		mes "[Yvain]";
		mes "Adventurer, thank you for your help.";
		mes "Thanks to you, I was able to escape that predicament.";
		next;
		mes "[Chez]";
		mes "Thank you very much!!";
		emotion ET_THANKS, getnpcid(0,"Chez#ch01");
		emotion ET_THANKS, getnpcid(0,"Yvain#ch01");
		cutin "ch1_chez02.png",2;
		npctalk "Yvain : Thank you so much.", "Chez#ch01", BC_SELF;
		npctalk "Chez : Thank you.", "Yvain#ch01", BC_SELF;
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Ahhh Yggdrasil!!";
		mes "Even though Yvain is an earth fairy, mistaking him for a ground is too much!";
		mes "That was really dangerous!";
		next;
		select("Ask about the situation.");
		mes "[Chez]";
		mes "As you just saw, Yggdrasil's roots are entwining the decayed land to stabilize it, it just happens to be that Yvain got caught up in it.";
		next;
		mes "[Chez]";
		mes "<B>Yggdrasil is trying to hold the decaying ground with it's roots</B>, Yvain is an earth fairy, if he was completely bounded up he might not be released until the land has completly recovered.";
		next;
		cutin "ch1_yvain01.png",2;
		mes "[Yvain]";
		mes "It's understandable that Yggdrasil would get confused since earth faries are born from the land.";
		mes "Thinking about it, having an earth fairy bounded to the land would help the land recover faster.";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Don't say such scary things!";
		mes "If you get stuck there, it means that I will have to wander this vast place all alone!";
		next;
		mes "[Chez]";
		mes "And who knows how long it'll take for the land to recover!";
		mes "And what if you're still stuck even after everything recovered!";
		mes "Your spirit will return to the Mana Spring, and you'll leave me to deal with all of your remaining work alone!";
		emotion ET_KEK, getnpcid(0,"Chez#ch01");
		next;
		cutin "ch1_yvain03.png",2;
		mes "[Yvain]";
		mes "Sounds like a good way to gain some experience.";
		next;
		cutin "ch1_yvain02.png",2;
		mes "[Yvain]";
		mes "Anyways, it seems that I'm still at risk of getting caught up in the roots again, I should return to Ashridge and help with other tasks.";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>No!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Why do I always end up alone!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>I hate this!!!!</B></FONT>";
		npctalk "Chez : Wah!!!!!!", "Chez#ch01", BC_SELF;
		next;
		mes "[Chez]";
		mes "And if it's not bad enough Yve left just the two of us here because he said he was too busy!!!";
		mes "I don't want to investigate such dangerous place all by myself!";
		next;
		mes "[Chez]";
		mes "- Glances -";
		emotion ET_STARE_ABOUT, getnpcid(0,"Chez#ch01");
		next;
		mes "[Chez]";
		mes "...";
		next;
		cutin "ch1_chez02.png",2;
		mes "[Chez]";
		mes "Then, I'll just stick with you adventurer.";
		mes "Come on! Let's go!";
		mes "Yvain you should return to the village and we should go straight to investigating!";
		emotion ET_HNG, getnpcid(0,"Chez#ch01");
		next;
		cutin "ch1_yvain03.png",2;
		mes "[Yvain]";
		mes "Please be careful on investigating.";
		mes "Normally, just touching the earth would tell me everything I need to know, but the energy is to weak in this region for me to sense anything.";
		next;
		cutin "ch1_yvain04.png",2;
		mes "[Yvain]";
		mes "For now, we have no choice but to explore and find clues ourselves.";
		next;
		mes "[Yvain]";
		mes "The <B>area up ahead</B> looks particularly unstable, it would be best to investigate it first.";
		changequest 24072, 24073;
		close2;
		cutin "",255;
		navigateto("ygg_roots", 153, 203);
		cloaknpc("Chez#ch01", true, getcharid(0));
		cloaknpc("Yvain#ch01", true, getcharid(0));
		end;
	}
	end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24072) == 1";
end;
}

ygg_roots,153,203,3	script	Magi#ch01_1	4_F_FAIRYKID4,{
	if ( isbegin_quest(24073) == 1 ) {
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Ah!";
		mes "Chez! Adventurer!";
		cloaknpc("Chez#ch01_m1", false, getcharid(0));
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Magi!!";
		mes "Why are you alone!";
		mes "Are you okay?";
		mes "Are you hurt anywhere?";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "I'm a little... startled, but I'm fine.";
		mes "More importantly, I got separated from my friends because of the earthquake...";
		mes "It was really scary...";
		mes "I'm so glad I ran into you two.";
		emotion ET_CRY, getnpcid(0,"Magi#ch01_1");
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Where did you got separated from your friends?";
		mes "Is it over there?";
		mes "Is it dangerous?";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "It's not too far from here...";
		mes "But near that area, we discovered a <B>distorted space</B>.";
		mes "If we head back to where the distorted space was, we should be able to find them.";
		next;
		cutin "ch1_chez01.png",2;
		mes "[Chez]";
		mes "I hope they're safe.";
		mes "Alright, let's head that way together.";
		mes "We need to check that distorted space too.";
		changequest 24073, 24074;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_m1", true, getcharid(0));
		navigateto("ygg_roots", 161, 237);
		end;
	}
	if ( isbegin_quest(24074) == 1 ) {
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "It's not too far from here...";
		mes "But near that area, we discovered a <B>distorted space</B>.";
		mes "If we head back to where the distorted space was, we should be able to find them.";
		close2;
		cutin "",255;
		navigateto("ygg_roots", 161, 237);
		end;
	}
	cutin "4job_Magi_02.png",2;
	mes "[Magi]";
	mes "I'm a little... startled, but I'm fine.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24073) == 1";
end;
}

ygg_roots,150,204,5	script(CLOAKED)	Chez#ch01_m1	4_F_FAIRYKID4,{
	end;
}

ygg_roots,161,237,5	script	Magi#ch01_2	4_F_FAIRYKID4,{
	if ( isbegin_quest(24074) == 1 ) {
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "This is the place...";
		mes "It's a little different from before, but I think we are on the right place.";
		next;
		mes "[Magi]";
		mes "The earthquake shook the ground so much.";
		mes "The area might have shifted a bit, we should look on a different area.";
		mes "I don't see my friends anywhere... Should we head <B>inside</B> to check?";
		changequest 24074, 24075;
		close2;
		cutin "",255;
		navigateto("hem_fild", 208, 22);
		end;
	}
	if ( isbegin_quest(24075) == 1 ) {
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "This is the place...";
		mes "It's a little different from before, but I think we are on the right place.";
		next;
		mes "[Magi]";
		mes "The earthquake shook the ground so much.";
		mes "The area might have shifted a bit, we should look on a different area.";
		mes "I don't see my friends anywhere... Should we head <B>inside</B> to check?";
		close2;
		cutin "",255;
		navigateto("hem_fild", 208, 22);
		end;
	}
	if ( isbegin_quest(24076) == 1 ) {
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "I wonder where everyone is?";
		cloaknpc("Chez#ch01_m2", false, getcharid(0));
		cloaknpc("Mark#ch01_m2", false, getcharid(0));
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "Those guys... I'm not too worried about them, but should we look around a bit before returning to the village?";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "At least show some concern for them.";
		next;
		cutin "ep18_mark_02.png",2;
		mes "[Mark]";
		mes "Alright, alright.";
		next;
		cutin "ch1_chez01.png",2;
		mes "[Chez]";
		mes "Shall we get going?";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Waaaaaaaaah!!!</B></FONT>";
		npctalk "Chez : Waaaaaaaaah!!!", "Chez#ch01_m2", BC_SELF;
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "<FONT SIZE = 16><B>Kyah!!!</B></FONT>";
		npctalk "Magi : Kyah!!!", "Magi#ch01_2", BC_SELF;
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "What was that just now!!";
		mes "Everyone, watch your feet!";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Ah!";
		mes "It stopped.";
		mes "That was terrifying!";
		mes "Is everyone fine?";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "This place wasn't like this before... something really is wrong...";
		mes "Yggdrasil is trying to restore the land, so we need to...";
		next;
		cutin "",255;
		mes "[???]";
		mes "Agh!!!";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Someone screamed from over there!";
		mes "Let's go check on them.";
		changequest 24076, 24077;
		close2;
		cutin "",255;
		navigateto("ygg_roots", 112, 246);
		cloaknpc("Chez#ch01_m2", true, getcharid(0));
		cloaknpc("Mark#ch01_m2", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(24077) == 1 ) {
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Someone screamed from over there!";
		mes "Let's go check on them.";
		close2;
		cutin "",255;
		navigateto("ygg_roots", 112, 246);
		end;
	}
	cutin "4job_Magi_02.png",2;
	mes "[Magi]";
	mes "I'm afraid because the ground keeps shaking.";
	mes "I really hope no one get hurts.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24074) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24076) == 1";
end;
}

ygg_roots,162,235,7	script(CLOAKED)	Chez#ch01_m2	4_F_FAIRYKID4,{
	end;
}

ygg_roots,163,237,3	script(CLOAKED)	Mark#ch01_m2	4_EP18_MARK,{
	end;
}

hem_fild,208,22,3	script	Investigator#ch01_1	4_M_ALCHE_C,{
	if ( isbegin_quest(24075) == 1 ) {
		mes "[Investigator]";
		mes "Hm?";
		mes "A mage.";
		mes "Are you a part of <B>Mark's</B> party by any chance?";
		emotion ET_QUESTION, getnpcid(0,"Investigator#ch01_1");
		cloaknpc("Chez#ch01_h", false, getcharid(0));
		cloaknpc("Magi#ch01_h", false, getcharid(0));
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Oh!";
		mes "Do you know Mark?";
		emotion ET_SURPRISE, getnpcid(0,"Magi#ch01_h");
		next;
		cutin "",255;
		mes "[Investigator]";
		mes "Phew!";
		mes "What a relief!";
		mes "Mark has been looking all over for his friends!";
		emotion ET_SMILE, getnpcid(0,"Investigator#ch01_1");
		next;
		mes "[Investigator]";
		mes "Wait here for a bit.";
		mes "He should be back anytime soon.";
		next;
		mes "[Investigator]";
		mes "He said he would return after healing the people who got injured in the earthquake.";
		next;
		mes "[Investigator]";
		mes "I was also trapped under some rocks and injured my leg, but saved my life and healed my injury.";
		emotion ET_THROB, getnpcid(0,"Investigator#ch01_1");
		next;
		mes "[Mark]";
		mes "Oh!";
		mes "Magi!!!";
		mes "You're safe!";
		mes "Thank goodness.";
		emotion ET_DELIGHT, getnpcid(0,"Magi#ch01_h");
		emotion ET_DELIGHT, getnpcid(0,"Mark#ch01_h");
		cutin "ep18_mark_02.png",2;
		cloaknpc("Mark#ch01_h", false, getcharid(0));
		next;
		cutin "4job_Magi_01.png",2;
		mes "[Magi]";
		mes "Mark!";
		mes "What about the others?";
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "They must be around somewhere.";
		mes "I hope they're not causing any trouble...";
		emotion ET_HNG, getnpcid(0,"Mark#ch01_h");
		next;
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Mark!";
		mes "There you are!";
		cloaknpc("Maxim#ch01_h", false, getcharid(0));
		next;
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "And these people are... Oh! Are they the ones you were looking for?";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Yes.";
		mes "But not all of them.";
		next;
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "I see!";
		mes "Since you've found one of your party member, I'll be heading off now.";
		mes "Something seems to be happening in Geffen!";
		mes "Thanks for the healing.";
		next;
		cutin "ep18_mark_02.png",2;
		mes "[Mark]";
		mes "Okay.";
		mes "Take care.";
		cloaknpc("Maxim#ch01_h", true, getcharid(0));
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "...";
		emotion ET_THINK, getcharid(3);
		emotion ET_THINK, getnpcid(0,"Magi#ch01_h");
		emotion ET_THINK, getnpcid(0,"Mark#ch01_h");
		emotion ET_THINK, getnpcid(0,"Chez#ch01_h");
		emotion ET_THINK, getnpcid(0,"Investigator#ch01_1");
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "...";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Geffen...";
		next;
		cutin "",255;
		mes "[Investigator]";
		mes "Is he talking about the Geffen we know?";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "I'm not sure.";
		mes "We'll need to investigate more, but it's too risky with out current members. I think we should return to Ashridge first and look for other members.";
		next;
		cutin "",255;
		mes "[Investigator]";
		mes "Since Mark still need to find his friends, I'll stay here and keep an eye on the situation. When you return to the village, make sure you report what happened here";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "Okay.";
		mes "If anything dangerous happens, don't hesitate to run.";
		next;
		cutin "",255;
		mes "[Investigator]";
		mes "Don't worry.";
		mes "Running away is my specialty.";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "Alright, let's <B>head out of the distorted space</B> to check our current location and return to Ashridge from there.";
		changequest 24075, 24076;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_h", true, getcharid(0));
		cloaknpc("Magi#ch01_h", true, getcharid(0));
		cloaknpc("Mark#ch01_h", true, getcharid(0));
		navigateto("ygg_roots", 161, 237);
		end;
	}
	if ( isbegin_quest(24076) == 1 ) {
		cloaknpc("Chez#ch01_h", false, getcharid(0));
		cloaknpc("Magi#ch01_h", false, getcharid(0));
		cloaknpc("Mark#ch01_h", false, getcharid(0));
		cutin "",255;
		mes "[Investigator]";
		mes "Since Mark still need to find his friends, I'll stay here and keep an eye on the situation. When you return to the village, make sure you report what happened here";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "Okay.";
		mes "If anything dangerous happens, don't hesitate to run.";
		next;
		cutin "",255;
		mes "[Investigator]";
		mes "Don't worry.";
		mes "Running away is my specialty.";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "Alright, let's <B>head out of the distorted space</B> to check our current location and return to Ashridge from there.";
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_h", true, getcharid(0));
		cloaknpc("Magi#ch01_h", true, getcharid(0));
		cloaknpc("Mark#ch01_h", true, getcharid(0));
		navigateto("ygg_roots", 161, 237);
		end;
	}
	mes "[Investigator]";
	mes "Don't worry.";
	mes "Running away is my specialty.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24075) == 1";
end;
}

hem_fild,207,20,7	script(CLOAKED)	Chez#ch01_h	4_F_FAIRYKID4,{
	end;
}

hem_fild,205,21,5	script(CLOAKED)	Magi#ch01_h	4_F_FAIRYKID4,{
	end;
}

hem_fild,206,25,5	script(CLOAKED)	Mark#ch01_h	4_EP18_MARK,{
	end;
}

hem_fild,204,26,5	script(CLOAKED)	Maxim#ch01_h	4_M_RUSBALD,{
	end;
}

ygg_roots,112,246,5	script	Investigator#ch01_2	4_F_SURA,{
	if ( isbegin_quest(24077) == 1 ) {
		mes "[Investigator]";
		mes "<FONT SIZE = 16><B>Ahhhh my toes!!!</B></FONT>";
		npctalk "Investigator : Ahhhh!!!!!!!!!!!", "Investigator#ch01_2", BC_SELF;
		emotion ET_KEK, getnpcid(0,"Investigator#ch01_2");
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Are you okay?";
		mes "Hold still.";
		mes "I'll heal you right now...";
		cloaknpc("Chez#ch01_su", false, getcharid(0));
		cloaknpc("Magi#ch01_su", false, getcharid(0));
		cloaknpc("Mark#ch01_su", false, getcharid(0));
		npctalk "Magi : Eek... That must really hurt.", "Magi#ch01_su", BC_SELF;
		npctalk "Chez : Yes, he should've worn proper shoes.", "Chez#ch01_su", BC_SELF;
		next;
		mes "[Investigator]";
		mes "<FONT SIZE = 16><B>Wow!</B></FONT>";
		mes "Thank you!";
		emotion ET_THANKS, getnpcid(0,"Investigator#ch01_2");
		cutin "",255;
		next;
		mes "[Investigator]";
		mes "Are you all here as investigator too?";
		next;
		select("Yes.");
		mes "[Investigator]";
		mes "The ground keeps on shaking, investigating is really tough.";
		mes "Even if you avoid getting seriously injured, it's exhausting...";
		npctalk "Investigator : I feel dizzy...", "Investigator#ch01_2", BC_SELF;
		next;
		select("Ask about Mark's friends.");
		mes "[Investigator]";
		mes "Hm...";
		mes "I'm not completely sure, but I think I saw someone who looked similar a <B>little west</B> from here.";
		mes "They were using <B>some kind of cloak</B>, so I couldn't get a good look at their face.";
		emotion ET_SCRATCH, getnpcid(0,"Investigator#ch01_2");
		next;
		cutin "ep18_mark_02.png",2;
		mes "[Mark]";
		mes "That information is enough.";
		mes "Thank you for informing us.";
		next;
		cutin "",255;
		mes "[Investigator]";
		mes "Oh, You're too humble~";
		mes "You're the one who healed me, I should be the one thanking you.";
		mes "Now hurry and go find your friend.";
		mes "Now that I'm feeling better, I should get back to investigating.";
		next;
		cutin "ep18_mark_02.png",2;
		mes "[Mark]";
		mes "Alright.";
		mes "Stay safe, I'll see you back in the village.";
		next;
		cutin "",255;
		mes "[Investigator]";
		mes "See you later~";
		changequest 24077, 24078;
		close2;
		cloaknpc("Chez#ch01_su", true, getcharid(0));
		cloaknpc("Magi#ch01_su", true, getcharid(0));
		cloaknpc("Mark#ch01_su", true, getcharid(0));
		navigateto("ygg_roots", 39, 268);
		end;
	}
	if ( isbegin_quest(24078) == 1 ) {
		mes "[Investigator]";
		mes "Hm...";
		mes "I'm not completely sure, but I think I saw someone who looked similar a little <B>west</B> from this area.";
		mes "They were using <B>some kind of cloak</B>, so I didn't see their faces.";
		emotion ET_SCRATCH, getnpcid(0,"Investigator#ch01_2");
		next;
		mes "[Investigator]";
		mes "Now hurry and go find your friends.";
		mes "I'm already feeling better, I should get back to investigating.";
		close2;
		navigateto("ygg_roots", 39, 268);
		end;
	}
	mes "[Investigator]";
	mes "The ground keeps on shaking, investigating the area is really tough because of it.";
	mes "Even though you avoided getting seriously injured, it's really exhausting...";
	npctalk "Investigator : I feel nauseous...", "Investigator#ch01_2", BC_SELF;
	close;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24077) == 1";
end;
}

ygg_roots,110,245,7	script(CLOAKED)	Chez#ch01_su	4_F_FAIRYKID4,{
	end;
}

ygg_roots,111,244,5	script(CLOAKED)	Magi#ch01_su	4_F_FAIRYKID4,{
	end;
}

ygg_roots,114,246,3	script(CLOAKED)	Mark#ch01_su	4_EP18_MARK,{
	end;
}

ygg_roots,39,268,3	script	Mark#ch01_1	4_EP18_MARK,{
	if ( isbegin_quest(24078) == 1 ) {
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Hmm...";
		mes "These footprints...";
		mes "The earthquake erased some of it, but they look like Tamarin's footprints.";
		next;
		mes "[Mark]";
		mes "Let's move around a bit more.";
		mes "I think he's nearby.";
		changequest 24078, 24079;
		close2;
		cutin "",255;
		navigateto("ygg_roots", 91, 149);
		end;
	}
	if ( isbegin_quest(24079) == 1 ) {
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Hmm...";
		mes "These footprints...";
		mes "The earthquake some of it, but they look like Tamarin's footprints.";
		next;
		mes "[Mark]";
		mes "Let's move around a bit more.";
		mes "I think he's nearby.";
		close2;
		cutin "",255;
		navigateto("ygg_roots", 91, 149);
		end;
	}
	cutin "ep18_mark_01.png",2;
	mes "[Mark]";
	mes "Hmm...";
	mes "These tracks...";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24078) == 1";
end;
}

ygg_roots,91,149,5	script	Tamarin#ch01_1	4_EP18_TAMARIN,{
	if ( isbegin_quest(24079) == 1 ) {
		cutin "ep18_tamarin_02.png",2;
		mes "[Tamarin]";
		mes "Hey~";
		mes "Magi! Mark!";
		mes "You're safe!";
		cloaknpc("Chez#ch01_t", false, getcharid(0));
		cloaknpc("Magi#ch01_t", false, getcharid(0));
		cloaknpc("Mark#ch01_t", false, getcharid(0));
		next;
		cutin "4job_Magi_04.png",2;
		mes "[Magi]";
		mes "We finally found you.";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Ah!";
		mes "Didn't you see the traces that I left?";
		emotion ET_SMILE, getnpcid(0,"Tamarin#ch01_1");
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Some of them were erased, but we recognized that it's yours.";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "I discovered a new <B>distorted space</B>, that's why I wasn't able to return to my original location.";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "A new distorted space?";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Yes!";
		mes "It seems like a new one, unless the one we were just at a while ago moved because of the earthquake.";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "When we went inside earlier, it was Geffen.";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "Was it really Geffen...";
		next;
		cutin "ep18_tamarin_04.png",2;
		mes "[Tamarin]";
		mes "Geffen?";
		next;
		mes "[Tamarin]";
		mes "Why is Geffen in a place like this???";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "The person we met inside said that it's definitely Geffen.";
		next;
		cutin "ep18_tamarin_04.png",2;
		mes "[Tamarin]";
		mes "I... see.";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "The details can always be reported later and we'll send investigators later, so don't worry about this for now.";
		npctalk "Chez : It's not what's important at the moment.", "Chez#ch01_t", BC_SELF;
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "That's true.";
		mes "Then, let's head to the new Distorted Space for now.";
		mes "<B>Alf</B> is also there.";
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "Now, we just need to find Dew...";
		next;
		cutin "ep18_tamarin_03.png",2;
		mes "[Tamarin]";
		mes "Only the most troublesome one left.";
		emotion ET_HNG, getnpcid(0,"Magi#ch01_t");
		emotion ET_HNG, getnpcid(0,"Mark#ch01_t");
		emotion ET_HNG, getnpcid(0,"Tamarin#ch01_1");
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "Yes...";
		changequest 24079, 24080;
		close2;
		cutin "",255;
		navigateto("ygg_roots", 40, 129);
		cloaknpc("Chez#ch01_t", true, getcharid(0));
		cloaknpc("Magi#ch01_t", true, getcharid(0));
		cloaknpc("Mark#ch01_t", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(24080) == 1 ) {
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Then, let's head to the new Distorted Space for now.";
		mes "<B>Alf</B> is also there.";
		close2;
		cutin "",255;
		navigateto("ygg_roots", 40, 129);
		end;
	}
	cutin "ep18_tamarin_03.png",2;
	mes "[Tamarin]";
	mes "Only the most troublesome one left.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24079) == 1";
end;
}

ygg_roots,91,152,3	script(CLOAKED)	Chez#ch01_t	4_F_FAIRYKID4,{
	end;
}

ygg_roots,92,152,3	script(CLOAKED)	Magi#ch01_t	4_F_FAIRYKID4,{
	end;
}

ygg_roots,93,150,3	script(CLOAKED)	Mark#ch01_t	4_EP18_MARK,{
	end;
}

ygg_roots,40,129,5	script	Tamarin#ch01_2	4_EP18_TAMARIN,{
	if ( isbegin_quest(24080) == 1 ) {
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Come.";
		mes "It's over here.";
		cloaknpc("Chez#ch01_t2", false, getcharid(0));
		cloaknpc("Mark#ch01_t2", false, getcharid(0));
		cloaknpc("Magi#ch01_t2", false, getcharid(0));
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "On the surface, it looks just like the <B>Distorted Space connected to Geffen</B>.";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Where is Alf...?";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "He probably went in first since he's not out here.";
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "Seriously...";
		emotion ET_FRET, getnpcid(0,"Mark#ch01_t2");
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Let's go inside, Alf might be in danger alone.";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Well, I think he's probably cloaked somewhere...";
		changequest 24080, 24081;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_t2", true, getcharid(0));
		cloaknpc("Mark#ch01_t2", true, getcharid(0));
		cloaknpc("Magi#ch01_t2", true, getcharid(0));
		navigateto("uknw_ruin", 244, 239);
		end;
	}
	if ( isbegin_quest(24081) == 1 ) {
		mes "[Tamarin]";
		mes "Come.";
		mes "It's over here.";
		cutin "ep18_tamarin_01.png",2;
		close2;
		cutin "",255;
		navigateto("uknw_ruin", 244, 239);
		end;
	}
	if ( isbegin_quest(24082) == 1 ) {
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Hm, all the parallel gates are strange.";
		mes "It would be more odd if gaps formed by the parallel gates are actually normal...";
		cloaknpc("Chez#ch01_t2", false, getcharid(0));
		cloaknpc("Mark#ch01_t2", false, getcharid(0));
		cloaknpc("Magi#ch01_t2", false, getcharid(0));
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "That's the reason we are investigating, to get answers.";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Since we've gathered quite the information, should we head back to Ashridge?";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "What about Dew?!";
		emotion ET_HUK, getnpcid(0,"Magi#ch01_t2");
		emotion ET_HUK, getnpcid(0,"Tamarin#ch01_2");
		npctalk "Magi : Wait!!!!", "Magi#ch01_t2", BC_SELF;
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Hm...";
		mes "He's probably fine on his own...";
		emotion ET_HNG, getnpcid(0,"Mark#ch01_t2");
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "But still...";
		next;
		cutin "ep18_tamarin_02.png",2;
		mes "[Tamarin]";
		mes "Then, let's look for him <B>on the way back to the village</B>.";
		mes "He might have even returned before us.";
		emotion ET_PROFUSELY_SWAT, getnpcid(0,"Tamarin#ch01_2");
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "That's true...";
		changequest 24082, 24083;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_t2", true, getcharid(0));
		cloaknpc("Mark#ch01_t2", true, getcharid(0));
		cloaknpc("Magi#ch01_t2", true, getcharid(0));
		navigateto("ygg_roots", 204, 178);
		end;
	}
	cutin "ep18_tamarin_01.png",2;
	mes "[Tamarin]";
	mes "Watch your step.";
	mes "You might get hurt.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24080) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24082) == 1";
end;
}

ygg_roots,42,131,3	script(CLOAKED)	Chez#ch01_t2	4_F_FAIRYKID4,{
	end;
}

ygg_roots,43,128,3	script(CLOAKED)	Mark#ch01_t2	4_EP18_MARK,{
	end;
}

ygg_roots,43,131,3	script(CLOAKED)	Magi#ch01_t2	4_F_FAIRYKID4,{
	end;
}

uknw_ruin,244,239,5	script	Alf#ch01	4_EP18_ALF,{
	if ( isbegin_quest(24081) == 1 ) {
		cutin "ep18_alf_04.png",2;
		mes "[Alf]";
		mes "You're here.";
		mes "I did some investigating before you arrived, but I didn't get any particularly useful information.";
		cloaknpc("Chez#ch01_b", false, getcharid(0));
		cloaknpc("Mark#ch01_b", false, getcharid(0));
		cloaknpc("Magi#ch01_b", false, getcharid(0));
		cloaknpc("Tamarin#ch01_b", false, getcharid(0));
		next;
		cutin "ep18_alf_01.png",2;
		mes "[Alf]";
		mes "As you can probably tell, this place is extremely distorted.";
		mes "Even the creatures wandering in here are creepy...";
		mes "Nothing about this place can be called normal.";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "The Geffen we saw earlier was a bit decayed, but it wasn't this chaotic...";
		mes "But it was also creepy in it's own way.";
		next;
		cutin "ep18_alf_04.png",2;
		mes "[Alf]";
		mes "Hm...";
		mes "So that place isn't good either?";
		next;
		mes "[Alf]";
		mes "Anyway, <B>there's nothing else to figure out here</B>.";
		mes "I'll keep investigating for now, but if all parallel gates are in this condition, we should report it to <B>Ashridge</B>.";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Let's head back outside for now.";
		npctalk "Tamarin : I really don't want to stay here.", "Tamarin#ch01_b", BC_SELF;
		npctalk "Magi : It's scary here.", "Magi#ch01_b", BC_SELF;
		changequest 24081, 24082;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_b", true, getcharid(0));
		cloaknpc("Mark#ch01_b", true, getcharid(0));
		cloaknpc("Magi#ch01_b", true, getcharid(0));
		cloaknpc("Tamarin#ch01_b", true, getcharid(0));
		navigateto("ygg_roots", 40, 129);
		end;
	}
	if ( isbegin_quest(24082) == 1 ) {
		cutin "ep18_alf_04.png",2;
		mes "[Alf]";
		mes "<B>there's nothing else to figure out here</B>.";
		mes "But, I'll keep on investigating for now...";
		close2;
		cutin "",255;
		navigateto("ygg_roots", 40, 129);
		end;
	}
	cutin "ep18_alf_04.png",2;
	mes "[Alf]";
	mes "<B>I haven't figure out anything yet.</B>.";
	mes "But, I'll keep on investigating for now...";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24081) == 1";
end;
}

uknw_ruin,248,239,3	script(CLOAKED)	Chez#ch01_b	4_F_FAIRYKID4,{
	end;
}

uknw_ruin,246,237,3	script(CLOAKED)	Mark#ch01_b	4_EP18_MARK,{
	end;
}

uknw_ruin,247,239,3	script(CLOAKED)	Magi#ch01_b	4_F_FAIRYKID4,{
	end;
}

uknw_ruin,244,236,3	script(CLOAKED)	Tamarin#ch01_b	4_EP18_TAMARIN,{
	end;
}

ygg_roots,204,178,3	script	Investigator#ch01_3	4_DOU_SIBA,{
	if ( isbegin_quest(24083) == 1 ) {
		select("Ask about Dew.");
		mes "[Investigator]";
		mes "Ah!";
		mes "A little while ago, I saw him heading East.";
		cloaknpc("Chez#ch01_d1", false, getcharid(0));
		cloaknpc("Mark#ch01_d1", false, getcharid(0));
		cloaknpc("Magi#ch01_d1", false, getcharid(0));
		cloaknpc("Tamarin#ch01_d1", false, getcharid(0));
		next;
		mes "[Investigator]";
		mes "If you hurry up, you should be able to catch him.";
		next;
		cutin "ep18_mark_02.png",2;
		mes "[Mark]";
		mes "Thank you.";
		next;
		mes "[Tamarin]";
		mes "East, does that mean he is heading to the village?";
		mes "Let's go check.";
		cutin "ep18_tamarin_01.png",2;
		changequest 24083, 24084;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_d1", true, getcharid(0));
		cloaknpc("Mark#ch01_d1", true, getcharid(0));
		cloaknpc("Magi#ch01_d1", true, getcharid(0));
		cloaknpc("Tamarin#ch01_d1", true, getcharid(0));
		navigateto("ygg_roots", 297, 200);
		end;
	}
	if ( isbegin_quest(24084) == 1 ) {
		mes "[Investigator]";
		mes "Ah!";
		mes "A little while ago, I saw him heading East.";
		next;
		mes "[Investigator]";
		mes "If you hurry up, you should be able to catch him.";
		close2;
		navigateto("ygg_roots", 297, 200);
		end;
	}
	mes "[Investigator]";
	mes "Sniff sniff!";
	mes "The smell of the land is too strong, I can't pick up any other scent.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24083) == 1";
end;
}

ygg_roots,200,179,5	script(CLOAKED)	Chez#ch01_d1	4_F_FAIRYKID4,{
	end;
}

ygg_roots,202,177,5	script(CLOAKED)	Mark#ch01_d1	4_EP18_MARK,{
	end;
}

ygg_roots,201,179,5	script(CLOAKED)	Magi#ch01_d1	4_F_FAIRYKID4,{
	end;
}

ygg_roots,205,177,3	script(CLOAKED)	Tamarin#ch01_d1	4_EP18_TAMARIN,{
	end;
}

ygg_roots,297,200,3	script	Investigator#ch01_4	4_EP17_BOY_B,{
	if ( isbegin_quest(24084) == 1 ) {
		select("Ask about Dew.");
		cutin "ep172_barmund_b01.bmp",2;
		mes "[Investigator]";
		mes "Ah~";
		mes "I just met him.";
		cloaknpc("Chez#ch01_d2", false, getcharid(0));
		cloaknpc("Mark#ch01_d2", false, getcharid(0));
		cloaknpc("Magi#ch01_d2", false, getcharid(0));
		cloaknpc("Tamarin#ch01_d2", false, getcharid(0));
		next;
		mes "[Investigator]";
		mes "He asked for directions to Ashridge, I told him the directions but when I turned around, he already <B>passed by the village entrance and kept going further</B>.";
		npctalk "Mark : Of course he did...", "Mark#ch01_d2", BC_SELF;
		emotion ET_THINK, getnpcid(0,"Mark#ch01_d2");
		emotion ET_THINK, getnpcid(0,"Tamarin#ch01_d2");
		next;
		cutin "ep172_barmund_b05.bmp",2;
		mes "[Investigator]";
		mes "I tried to stop him, but he was too fast...";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Sigh...";
		mes "Thanks for letting us know.";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "By the way, have we met somewhere before?";
		emotion ET_SCRATCH, getnpcid(0,"Tamarin#ch01_d2");
		next;
		cutin "ep172_barmund_b04.bmp",2;
		mes "[Investigator]";
		mes "Who knows?";
		mes "More importantly, you should find <B>Dew</B>.";
		next;
		cutin "ep172_barmund_b01.bmp",2;
		mes "[Investigator]";
		mes "There aren't any dangerous monsters around here, there shouldn't be any problems as you watch our for the earthquakes.";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "I'm getting a very bad feeling about this...";
		mes "Let's hurry up.";
		changequest 24084, 24085;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_d2", true, getcharid(0));
		cloaknpc("Mark#ch01_d2", true, getcharid(0));
		cloaknpc("Magi#ch01_d2", true, getcharid(0));
		cloaknpc("Tamarin#ch01_d2", true, getcharid(0));
		navigateto("ygg_roots", 338, 243);
		end;
	}
	if ( isbegin_quest(24085) == 1 ) {
		cutin "ep172_barmund_b01.bmp",2;
		mes "[Investigator]";
		mes "He asked for directions to Ashridge, I told him the directions but when I turned around, he already <B>passed by the village entrance and kept going further</B>.";
		next;
		cutin "ep172_barmund_b05.bmp",2;
		mes "[Investigator]";
		mes "I tried to stop him, but he was too fast...";
		next;
		cutin "ep172_barmund_b04.bmp",2;
		mes "[Investigator]";
		mes "More importantly, you should find <B>Dew</B>.";
		next;
		cutin "ep172_barmund_b01.bmp",2;
		mes "[Investigator]";
		mes "There aren't any dangerous monsters around here, there shouldn't be any problems as you watch our for the earthquakes.";
		close2;
		cutin "",255;
		navigateto("ygg_roots", 338, 243);
		end;
	}
	cutin "ep172_barmund_b01.bmp",2;
	mes "[Investigator]";
	mes "There's so many interesting here.";
	mes "I wonder what will happen to the story this time?";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24084) == 1";
end;
}

ygg_roots,296,203,5	script(CLOAKED)	Chez#ch01_d2	4_F_FAIRYKID4,{
	end;
}

ygg_roots,294,200,5	script(CLOAKED)	Mark#ch01_d2	4_EP18_MARK,{
	end;
}

ygg_roots,295,203,5	script(CLOAKED)	Magi#ch01_d2	4_F_FAIRYKID4,{
	end;
}

ygg_roots,296,198,5	script(CLOAKED)	Tamarin#ch01_d2	4_EP18_TAMARIN,{
	end;
}

ygg_roots,338,243,5	script	Mark#ch01_2	4_EP18_MARK,{
	if ( isbegin_quest(24085) == 1 ) {
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "This is...";
		mes "Another new Distorted Space.";
		mes "The trail ended here, did he went in alone?";
		emotion ET_SCRATCH, getnpcid(0,"Mark#ch01_2");
		cloaknpc("Chez#ch01_d3", false, getcharid(0));
		cloaknpc("Magi#ch01_d3", false, getcharid(0));
		cloaknpc("Tamarin#ch01_d3", false, getcharid(0));
		next;
		cutin "ch1_chez01.png",2;
		mes "[Chez]";
		mes "Finding three parallel gates, isn't that quite the achievement?";
		mes "Let's go inside to find your friend and let's return to the village.";
		npctalk "Chez : I can't wait to brag to Yvain!", "Chez#ch01_d3", BC_SELF;
		next;
		cutin "ch1_chez02.png",2;
		mes "[Chez]";
		mes "I must report this great accomplishment right away!";
		mes "I can do this even without Yvain!";
		mes "Mwuhaha!";
		emotion ET_BLABLA,getnpcid(0, "Chez#ch01_d3");
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Now, let's... Oh...";
		mes "The entrance feels very hot...";
		emotion ET_PROFUSELY_SWAT, getnpcid(0,"Chez#ch01_d3");
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Come closer everyone.";
		mes "It's warm here.";
		mes "Is it just this spot?";
		next;
		cutin "ep18_dew_02.png",2;
		mes "[Dew]";
		mes "<FONT SIZE = 16><B>Graaaaaaaaaaaaah!!!!</B></FONT>";
		npctalk "Dew : Graaaaaaaaaaaaah!!!! I'm dying!!!!!!!!", "Dew#ch01_d3", BC_SELF;
		cloaknpc("Dew#ch01_d3", false, getcharid(0));
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Kyaaaaaaaaaaah!!!!!!!</B></FONT>";
		npctalk "Chez : Kyaaaaaaaaaaah!!!!!!!", "Chez#ch01_d3", BC_SELF;
		next;
		cutin "ep18_dew_02.png",2;
		mes "[Dew]";
		mes "<FONT SIZE = 16><B>Hot!!</B></FONT>";
		mes "<FONT SIZE = 14><B>Hot!!!</B></FONT>";
		mes "<FONT SIZE = 18><B>Gyaaaah!!!</B></FONT>";
		npctalk "Dew : Hot!! Hot!!! I'm burning!!!!!", "Dew#ch01_d3", BC_SELF;
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "<FONT SIZE = 16><B>Dew!!!</B></FONT>";
		next;
		cutin "4job_Magi_03.png",2;
		mes "[Magi]";
		mes "<FONT SIZE = 12><B>Kyaah!!!</B></FONT>";
		next;
		cutin "ep18_dew_02.png",2;
		mes "[Dew]";
		mes "<FONT SIZE = 14><B>Huff, huff!!</B></FONT>";
		mes "I'm alive!";
		mes "I thought I was going to melt...";
		next;
		cutin "4job_Magi_02.png",2;
		mes "[Magi]";
		mes "Are you okay?";
		next;
		cutin "ep18_dew_04.png",2;
		mes "[Dew]";
		mes "Oh!";
		mes "Isn't everyone here already?";
		mes "Where have you been?";
		mes "I was looking for you.";
		next;
		cutin "ep18_dew_02.png",2;
		mes "[Dew]";
		mes "Oh, what's more important is that we have a huge problem!";
		mes "Did a fire broke out in the village or something!";
		mes "It's scorching hot!";
		mes "Did it all go up into flames since everything's made of wood?";
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "That isn't the village.";
		mes "It's an entrance to a Distorted Space.";
		next;
		cutin "ep18_dew_03.png",2;
		mes "[Dew]";
		mes "Really?";
		mes "What!";
		mes "But... someone told me that the village is over here, was it not?";
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "You probably misunderstood him.";
		emotion ET_HNG, getnpcid(0,"Mark#ch01_2");
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "So, what was it like inside?";
		mes "I'm guessing it's not safe since you said it was hot.";
		next;
		cutin "ep18_dew_05.png",2;
		mes "[Dew]";
		mes "<B>It's insanely hot.</B>";
		mes "If you go in unprotected, you'll probably melt in no time, I think?";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Hmm, then we'll need a <B>Cold Magic Barrier</B>.";
		mes "We should report this back to Ashridge before anyone gets hurt.";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Then, I'll stay here to guard the entrance so no one walks in unknowingly.";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Yes, that's a good idea.";
		mes "Thanks.";
		mes "We'll be back soon.";
		changequest 24085, 18378;
		getexp 130609489, 4000000;
		getitem "Ch1_Root_Coin",10;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_d3", true, getcharid(0));
		cloaknpc("Magi#ch01_d3", true, getcharid(0));
		cloaknpc("Tamarin#ch01_d3", true, getcharid(0));
		cloaknpc("Dew#ch01_d3", true, getcharid(0));
		navigateto("ygg_fruit", 80, 122);
		end;
	}
	if ( isbegin_quest(18378) == 1 ) {
		cloaknpc("Chez#ch01_d3", false, getcharid(0));
		cloaknpc("Magi#ch01_d3", false, getcharid(0));
		cloaknpc("Tamarin#ch01_d3", false, getcharid(0));
		cloaknpc("Dew#ch01_d3", false, getcharid(0));
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "That isn't the village.";
		mes "It's an entrance to a Distorted Space.";
		next;
		cutin "ep18_dew_03.png",2;
		mes "[Dew]";
		mes "Really?";
		mes "What!";
		mes "But... someone told me that the village is over here, was it not?";
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "You probably misunderstood him.";
		emotion ET_HNG, getnpcid(0,"Mark#ch01_2");
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "So, what was it like inside?";
		mes "I'm guessing it's not safe since you said it was hot.";
		next;
		cutin "ep18_dew_05.png",2;
		mes "[Dew]";
		mes "<B>It's insanely hot.</B>";
		mes "If you go in unprotected, you'll probably melt in no time, I think?";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Hmm, then we'll need a <B>Cold Magic Barrier</B>.";
		mes "We should report this back to Ashridge before anyone gets hurt.";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Then, I'll stay here to guard the entrance so no one walks in unknowingly.";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Yes, that's a good idea.";
		mes "Thanks.";
		mes "We'll be back soon.";
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_d3", true, getcharid(0));
		cloaknpc("Magi#ch01_d3", true, getcharid(0));
		cloaknpc("Tamarin#ch01_d3", true, getcharid(0));
		cloaknpc("Dew#ch01_d3", true, getcharid(0));
		navigateto("ygg_fruit", 80, 122);
		end;
	}
	cutin "ep18_mark_01.png",2;
	mes "[Mark]";
	mes "It's hot inside, it's better to be careful.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24085) == 1";
end;
}

ygg_roots,339,246,5	script(CLOAKED)	Chez#ch01_d3	4_F_FAIRYKID4,{
	end;
}

ygg_roots,336,241,5	script(CLOAKED)	Magi#ch01_d3	4_F_FAIRYKID4,{
	end;
}

ygg_roots,340,241,3	script(CLOAKED)	Tamarin#ch01_d3	4_EP18_TAMARIN,{
	end;
}

ygg_roots,341,247,3	script(CLOAKED)	Dew#ch01_d3	4_EP18_DEW,{
	end;
}

ygg_roots,164,231,0	script	#c01ms00	HIDDEN_WARP_NPC,3,3,{
	end;

OnTouch:
	if ( isbegin_quest(18372) == 1 ) {
		cloaknpc("Ascetic Jeon#c01ms00", false, getcharid(0));
	}
end;
}

ygg_roots,164,231,3	script(CLOAKED)	Ascetic Jeon#c01ms00	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(18372) == 1 ) {
		cutin "ex_sa_masterj01.png",2;
		mes "[???]";
		mes "You, are you heading into Geffen? You're going, aren't you?";
		next;
		select("Yes, that's right... who are you...");
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "Just remember me as the ascetic from Jeon family, one who practices talismans and skilled in ascetic arts.";
		mes "You're just in time. I have sensed an ominous energy emanating from that direction, so I have called for you to accompany me to investigate what's happening.";
		next;
		select("Ominous energy?");
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Yes. Something is off. Plageus, signs of war... and other forces of evil. It seems that Asura's realm is about to be unleashed.";
		mes "We must stop it if we can. And if that's not possible, we should atleast minimize the damage..";
		next;
		select("Asura's realm?...");
		mes "[Ascetic Jeon]";
		mes "Are you afraid? It's normal to fear the unknown. Once you see the truth for yourself, that fear will fade away.";
		mes "We got a long journey ahead of us. Let's get moving, the others are waiting for us.";
		changequest 18372, 17891;
		close2;
		cutin "",255;
		cloaknpc("Ascetic Jeon#c01ms00", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18372) == 1";
end;
}

hem_fild,102,161,0	script	#c01ms01	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17891) == 1 ) {
		warp "ch1_sf03", 179, 251;
		end;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17891) == 1";
end;
}

ch1_sf03,179,251,0	script	#c01ms01_	HIDDEN_WARP_NPC,3,3,{
	end;

OnTouch:
	if ( isbegin_quest(17891) == 1 ) {
		cloaknpc("Reno#c01ms00", false, getcharid(0));
		cloaknpc("Lumin#c01ms00", false, getcharid(0));
		cloaknpc("Elena#c01ms00", false, getcharid(0));
		cloaknpc("Einhar#c01ms00", false, getcharid(0));
		cloaknpc("Gregor#c01ms00", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms01", false, getcharid(0));
		cloaknpc("Robin#c01ms00", false, getcharid(0));
		cloaknpc("Charles#c01ms00", false, getcharid(0));
		end;
	}
end;
}

ch1_sf03,190,257,5	script	Reno#c01ms00	4_M_KNG_RENO,{
	if ( isbegin_quest(17891) == 1 ) {
		cutin "reno_kn_01.bmp",1;
		mes "[Reno]";
		mes "Here! Over here!!";
		mes "Now that the adventurer has arrived, we are now complete! Now!";
		npctalk "You're late...", "Lumin#c01ms00", BC_SELF;
		next;
		cutin "162elena_01.png",2;
		mes "[Elena]";
		mes "I heard that the Geffen in the second world took a hit during the invasion? Monsters still how up there from time to time.";
		next;
		cutin "lumin_ac_01.bmp",0;
		mes "[Lumin]";
		mes "I heard that they could use some help.";
		next;
		cutin "162elena_01.png",2;
		mes "[Elena]";
		mes "Do we just need to deal with the monsters?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "The monsters are a problem, but I sense something more dangerous. An ominous energy is coming out from Geffen.";
		next;
		cutin "4job_einhar_01.png",1;
		mes "[Einhar]";
		mes "Ominous energy, What do you mean?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "I don't know yet. That's why I volunteered to come. It's something familiar...";
		next;
		cutin "reno_kn_01.bmp",1;
		mes "[Reno]";
		mes "We need to get moving instead of standing around here!";
		next;
		cutin "lumin_ac_01.bmp",0;
		mes "[Lumin]";
		mes "There's two paths ahead. Did anyone investigated the other side?";
		next;
		cutin "4job_robin_01.png",0;
		mes "[Robin]";
		mes "According to the reports, it's something similar to the realm of the valkyries.";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "Then, where should we first? Should we focus only on Geffen?";
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "There's no need for everyone to go together, we've got the strength and numbers, don't we? Let's move more efficiently.";
		npctalk "How do we move efficiently?", "Einhar#c01ms00", BC_SELF;
		npctalk "We don't know what's out there, wouldn't it be safer to stick together?", "Reno#c01ms00", BC_SELF;
		next;
		cutin "162elena_01.png",2;
		mes "[Elena]";
		mes "What's with all the overthinking? Split into two groups!";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "Then, I'll head Geffen. We can share info with the Geffen Tower.";
		next;
		cutin "4job_robin_01.png",0;
		mes "[Robin]";
		mes "Then, I'll take the other path. Since it's unexplored, we need to scout it first. You look like a strong bunch. You two come with me.";
		npctalk "Okay!!", "Reno#c01ms00", BC_SELF;
		npctalk "Sure, whatever...", "Lumin#c01ms00", BC_SELF;
		next;
		cutin "162elena_01.png",2;
		mes "[Elena]";
		mes "I'll go too. I can be more useful than just doing civilian support.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "I will investigate the ominous energy in Geffen.";
		next;
		cutin "4job_einhar_01.png",1;
		mes "[Einhar]";
		mes "If some needs help in Geffen, then I'll go there.";
		next;
		cutin "reno_kn_01.bmp",1;
		mes "[Reno]";
		mes "So we've pretty much decided, huh? Adventurer, where are you going?";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "We're short one member over here, so you should come with us to Geffen. Is there any complaints? Good.";
		next;
		select("Then, I'll go to Geffen.");
		cutin "lumin_ac_01.bmp",0;
		mes "[Lumin]";
		mes "Once everyone is finished, let's meet back in Ashridge.";
		mes "Dismissed!";
		changequest 17891, 17892;
		getitem "Ch1_Root_Coin",3;
		getexp 32652372, 2500000;
		close2;
		cutin "",255;
		cloaknpc("Reno#c01ms00", true, getcharid(0));
		cloaknpc("Lumin#c01ms00", true, getcharid(0));
		cloaknpc("Elena#c01ms00", true, getcharid(0));
		cloaknpc("Robin#c01ms00", true, getcharid(0));
		cloaknpc("Einhar#c01ms00", true, getcharid(0));
		cloaknpc("Charles#c01ms00", true, getcharid(0));
		cloaknpc("Gregor#c01ms00", true, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms01", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17891) == 1";
end;
}

ch1_sf03,187,257,5	script(CLOAKED)	Lumin#c01ms00	4_M_AC_RUMIN,{
	end;
}

ch1_sf03,192,255,3	script(CLOAKED)	Elena#c01ms00	4_F_ELENA,{
	end;
}

ch1_sf03,192,257,3	script(CLOAKED)	Einhar#c01ms00	4_4JOB_EINHAR,{
	end;
}

ch1_sf03,194,254,3	script(CLOAKED)	Charles#c01ms00	4_M_JOB_WIZARD,{
	end;
}

ch1_sf03,192,252,1	script(CLOAKED)	Gregor#c01ms00	4259,{
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_sf03,188,254,7	script(CLOAKED)	Ascetic Jeon#c01ms01	4_EXJOB_MASTER_J,{
	end;
}

ch1_sf03,186,255,7	script(CLOAKED)	Robin#c01ms00	4_4JOB_ROBIN,{
	end;
}

ch1_geffen,190,123,5	script	Geffen Resident#c01ms10	4_M_02,5,5,{
	if ( isbegin_quest(17892) == 1 ) {
		cloaknpc("Einhar#c01ms10", false, getcharid(0));
		cloaknpc("Gregor#c01ms10", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms10", false, getcharid(0));
		cloaknpc("Charles#c01ms10", false, getcharid(0));
		mes "[Geffen Resident]";
		mes "I heard Sunbi's second child caught it this time... Poor thing...";
		npctalk "They must be local residents.", "Gregor#c01ms10", BC_SELF;
		npctalk "Let's listen to what they're saying.", "Charles#c01ms10", BC_SELF;
		next;
		mes "[Geffen Resident]";
		mes "Oh~ Don't even get me started. Her mother was verge on tears, she kept asking what she can do and I felt my heart rip in two.";
		npctalk "She must be completely torn up inside.", "Geffen Resident#c01ms11", BC_SELF;
		next;
		mes "[Geffen Resident]";
		mes "What in the world is happening... What kind of disease is that, there's no sign of then improving at all.";
		npctalk "Exactly my thoughts!", "Geffen Resident#c01ms10", BC_SELF;
		next;
		cloaknpc("Geffen Resident#c01ms12", false, getcharid(0));
		mes "[Geffen Resident]";
		mes "Ah... Ahh... Hah...";
		npctalk "Shh-", "Geffen Resident#c01ms10", BC_SELF;
		npctalk "What's that!!", "Geffen Resident#c01ms11", BC_SELF;
		next;
		mes "[Geffen Resident]";
		mes "Ah... Ah....";
		next;
		cloaknpc("Geffen Resident#c01ms12", true, getcharid(0));
		mes "[Geffen Resident]";
		mes "Sigh- That house too, I heard the whole family got infected, right?";
		next;
		mes "[Geffen Resident]";
		mes "I think even the kid caught it... Should we simply just leave them be...";
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "A disease...?";
		npctalk "A disease... This doesn't look good.", "Ascetic Jeon#c01ms10", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "From everyone's reaction it seems that it's pretty widespread. Is it possible that it's some kind of plague?";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "I think we need to get on the bottom of this.";
		next;
		select("We should offer help too.");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Perhaps the ominous energy I sensed is related to this disease. The fact that I felt that strange energy here in Geffen and a disease is spreading at the same time, it seems too much to be a coincidence.";
		mes "I'll go follow the source of energy. If I learn anything, I'll report back immediately.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "Then, I'll head to the Geffen Tower as planned. I should be able to get some information from the mages there. I'm a little worried though, I can't see Geffen Tower from here...";
		mes "Is there even a Geffen Tower here? A Geffen without the Geffen Tower? Hahaha! I hope that's not the case.";
		npctalk "This person is just talking and laughing on his own. It's kinda contagious.", "Gregor#c01ms10", BC_SELF;
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "We'll talk with the residents and try to find out what's going on. We'll lend a hand if they need it. I heard that there's a monster invasion here and it's oddly quiet.";
		next;
		select("I feel the same.");
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "I'm curious about the disease. If it's something we already know, we might be able to create a cure. We need to meet the people who've caught it.";
		npctalk "엄청 믿음직하네.", "Charles#c01ms10", BC_SELF;
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "If anyone finds any important information, we should regroup here and share our findings.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "Then, take care out there!";
		next;
		cloaknpc("Ascetic Jeon#c01ms10", true, getcharid(0));
		cloaknpc("Charles#c01ms10", true, getcharid(0));
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Let's ask the people over there. It seems that they know something.";
		changequest 17892, 17893;
		close3;
	}
	if ( isbegin_quest(17893) == 1 ) {
		cloaknpc("Einhar#c01ms10", false, getcharid(0));
		cloaknpc("Gregor#c01ms10", false, getcharid(0));
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Greetings. I would like to ask something.";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "Who... an outsider? At a time like this?";
		npctalk "What's an outsider doing here?", "Geffen Resident#c01ms11", BC_SELF;
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "We came to meet someone, but we overheard some unsettling conversation while passing by.";
		npctalk "We are not trying to eavesdrop on purpose.", "Gregor#c01ms10", BC_SELF;
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "We heard you discussing some kind of disease, may we know about it too?";
		next;
		cutin "",255;
		select("So we can be cautious of it as well.");
		mes "[Geffen Resident]";
		mes "I understand but... it's hard to talk about what's happening here with strangers... especially since it's not a pleasant topic.";
		next;
		cutin "4job_gregor_01.png",0;
		mes "[Gregor]";
		mes "Ah! We're researchers! We came here because we lost contact with one of the scholars for some time now.";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "A~ I see. A scholar? Who's this scholar that you're talking about?";
		npctalk "Do you mean Orian?", "Geffen Resident#c01ms10", BC_SELF;
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "May we ask what kind of disease are you talking about? We'd like to know if it's alright.";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "We don't really know the specifics either. It suddenly started spreading from place to place and turned to what it is now. It seems like its a very infectious disease.";
		npctalk "If it's not, a lot of people won't be infected with it.", "Geffen Resident#c01ms10", BC_SELF;
		next;
		mes "[Geffen Resident]";
		mes "Like I'm telling you, it's not an infectious one. If it were, everyone in the same household of a person would've gotten it. Some do and some don't, am I wrong?";
		npctalk "Am I wrong?", "Geffen Resident#c01ms11", BC_SELF;
		next;
		mes "[Geffen Resident]";
		mes "It's true that there are households like that. But it's also a fact that some people get sick while caring for the ill.";
		next;
		cutin "4job_gregor_01.png",0;
		mes "[Gregor]";
		mes "So it's not confirmed if it's infectious or not.";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "It's infectious!";
		next;
		mes "[Geffen Resident]";
		mes "It's not!";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Please calm down. We now understand the situation. We'll look into it ourselves. Thank you for your time.";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "Okay. Just be careful. Monsters appear here from time to time.";
		npctalk "They don't look like regular folks. We shouldn't worry about them.", "Geffen Resident#c01ms11", BC_SELF;
		next;
		cutin "4job_gregor_01.png",0;
		mes "[Gregor]";
		mes "Hm... There's definitely a disease spreading, no? But we need to investigate more. Isn't our information not enough? I guess we need find something more definitive...";
		next;
		select("Let's ask other people.");
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Alright, that's the plan.";
		changequest 17893, 17894;
		close2;
		cutin "",255;
		cloaknpc("Gregor#c01ms10", true, getcharid(0));
		cloaknpc("Einhar#c01ms10", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(17894) == 1 ) {
		npctalk "Do you still have something to ask?", "Geffen Resident#c01ms10", BC_SELF;
		end;
	}
	npctalk "I'm getting out of here. Do you think there's a place where I can go?", "", BC_SELF;
	end;

OnTouch:
	if ( isbegin_quest(17892) == 1 ) {
		cloaknpc("Einhar#c01ms10", false, getcharid(0));
		cloaknpc("Gregor#c01ms10", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms10", false, getcharid(0));
		cloaknpc("Charles#c01ms10", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17893) == 1 ) {
		cloaknpc("Einhar#c01ms10", false, getcharid(0));
		cloaknpc("Gregor#c01ms10", false, getcharid(0));
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17892) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17893) == 1";
end;
}

ch1_geffen,192,123,3	script	Geffen Resident#c01ms11	4_F_GODEMOM,{
	npctalk "Who will it be next time...", "", BC_SELF;
	end;
}

ch1_geffen,188,121,1	script(CLOAKED)	Geffen Resident#c01ms12	4_F_MUT1,{
	end;
}

ch1_geffen,194,122,3	script(CLOAKED)	Einhar#c01ms10	4_4JOB_EINHAR,{
	npctalk "Indeed... It feels similar, but quite different.", "", BC_SELF;
	end;
}

ch1_geffen,193,120,1	script(CLOAKED)	Gregor#c01ms10	4259,{
	npctalk "The atmosphere here isn't good.", "", BC_SELF;
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_geffen,192,119,1	script(CLOAKED)	Charles#c01ms10	4_M_JOB_WIZARD,{
	npctalk "I don't see the Geffen Tower, is it just my imagination?", "", BC_SELF;
	end;
}

ch1_geffen,190,119,7	script(CLOAKED)	Ascetic Jeon#c01ms10	4_EXJOB_MASTER_J,{
	npctalk "There's definitely an ominous energy here.", "", BC_SELF;
	end;
}

ch1_geffen,149,55,3	script	Geffen Resident#c01ms13	4_F_TWGRANDMOM,3,3,{
	if ( isbegin_quest(17894) == 1 ) {
		cloaknpc("Gregor#c01ms11", false, getcharid(0));
		cloaknpc("Einhar#c01ms11", false, getcharid(0));
		cloaknpc("Geffen Resident#c01ms15", false, getcharid(0));
		mes "[Geffen Resident]";
		mes "Ah... Ugh... Phew...";
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "That person... isn't that the one from earlier? A zombie?";
		npctalk "That's a bit harsh.", "Einhar#c01ms11", BC_SELF;
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "A Zombie! Where! Eh, why are you calling a perfectly fine person a Zombie?";
		next;
		mes "[Geffen Resident]";
		mes "Wait? You're not from here, are you? Who are you people? You seem suspicious?";
		next;
		select("We're not suspicious people.");
		mes "[Geffen Resident]";
		mes "That's what exactly a suspicious people would say! And you know the people who said that? They all turned out to be someone suspicious!";
		next;
		mes "[Geffen Resident]";
		mes "Exactly! In times like these, didn't you think that a person showing up out of nowhere like this is not suspicious?";
		next;
		cutin "4job_einhar_01.png",0;
		mes "[Einhar]";
		mes "Oh, we came here after hearing about the disease to offer our aid. And the person here can...";
		npctalk "Take a look!", "Einhar#c01ms11", BC_SELF;
		specialeffect EF_HIT1, AREA, "Einhar#c01ms11";
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "Us? Is it our turn? Well, we're researchers. I major in Botany and other stuffs... how many should I say?";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "R-Really...? During times like this, that would be helpful...!";
		npctalk "Are you serious...?", "Geffen Resident#c01ms13", BC_SELF;
		next;
		mes "[Geffen Resident]";
		mes "The so called researchers from this city only worsen things up! It's the outsiders who's actually helping us right now!";
		npctalk "A savior came, a real savior.", "Geffen Resident#c01ms14", BC_SELF;
		next;
		cutin "4job_einhar_01.png",0;
		mes "[Einhar]";
		mes "What do you mean by real?";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "All the local researchers were in Geffen Tower. They studied all sort of things. But do you know when did the plague started?";
		npctalk "You mean the mages, right?", "Gregor#c01ms11", BC_SELF;
		next;
		mes "[Geffen Resident]";
		mes "It was right after the invasion and the Geffen Tower collapsed! Do you understand what it means? They must've been doing forbidden research there.";
		mes "It probably leaked out when the tower collapsed, don't you think so?";
		npctalk "From Geffen Tower...?", "Einhar#c01ms11", BC_SELF;
		next;
		mes "[Geffen Resident]";
		mes "Now that I think about it, just before the invasion happened. Weren't there some people who would disappear after going to the tower?";
		next;
		mes "[Geffen Resident]";
		mes "Do you think they were taken for some secret experiments?";
		npctalk "You know, that one guy? Derma's father. He went in and then...!", "Geffen Resident#c01ms14", BC_SELF;
		next;
		cutin "4job_gregor_02.png",2;
		mes "[Gregor]";
		mes "What's that supposed to mean?";
		next;
		cutin "4job_einhar_01.png",0;
		mes "[Einhar]";
		mes "The Geffen Tower collapsed during the invasion, correct? Right after that the disease started to spread... That is strange.";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "Ugh... ugh...";
		next;
		mes "[Geffen Resident]";
		mes "H-hey! Get ahold of yourself. Can you walk?";
		next;
		mes "[Geffen Resident]";
		mes "Ugh...";
		next;
		mes "[Geffen Resident]";
		mes "Geez. There's no helping it. I'll carry him for now, he doesn't even have a family to take care of him. Sigh. What can he do all alone? I'll walk you to the clinic.";
		npctalk "Can you carry him?", "Geffen Resident#c01ms13", BC_SELF;
		npctalk "We'll have to carry him, what else can we do?", "Geffen Resident#c01ms14", BC_SELF;
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "I'll help. It'd be good to observe someone who's sick up close.";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "Oh! That would be a big help. I forgot to mention, there are a lot of people who lived alone and haven't been seen lately... I wish they're okay. I should really check on them sometime.";
		next;
		cutin "4job_einhar_01.png",0;
		mes "[Einhar]";
		mes "I'll take a look around the village. Where should I take people who need help?";
		next;
		cutin "",255;
		mes "[Geffen Resident]";
		mes "The clinic. It's on the other side of the village, you'll find it easily.";
		next;
		select("I'll go to the Geffen Tower.");
		cutin "4job_einhar_01.png",0;
		mes "[Einhar]";
		mes "Sounds good. There might really be a connection Geffen Tower and the disease. Charles also went into the tower, maybe he already discovered something.";
		mes "Take care.";
		npctalk "See you later. Good luck!", "Gregor#c01ms11", BC_SELF;
		changequest 17894, 17895;
		close2;
		cutin "",255;
		cloaknpc("Gregor#c01ms11", true, getcharid(0));
		cloaknpc("Einhar#c01ms11", true, getcharid(0));
		cloaknpc("Geffen Resident#c01ms15", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(17895) == 1 ) {
		npctalk "Eh, didn't you say that you were going to the Geffen Tower?", "", BC_SELF;
		end;
	}
	npctalk "What if they didn't come back forever...", "", BC_SELF;
	end;
	
OnTouch:
	if ( isbegin_quest(17894) == 1 ) {
		cloaknpc("Gregor#c01ms11", false, getcharid(0));
		cloaknpc("Einhar#c01ms11", false, getcharid(0));
		cloaknpc("Geffen Resident#c01ms15", false, getcharid(0));
		end;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17894) == 1";
end;
}

ch1_geffen,150,57,3	script	Geffen Resident#c01ms14	4_M_SEAMAN,{
	npctalk "Sigh- I'm really worried, seriously worried.", "", BC_SELF;
	end;
}

ch1_geffen,147,53,7	script(CLOAKED)	Geffen Resident#c01ms15	4_F_MUT1,{
	end;
}

ch1_geffen,151,55,3	script(CLOAKED)	Gregor#c01ms11	4259,{
	npctalk "Isn't the atmosphere weird?", "", BC_SELF;
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_geffen,147,56,5	script(CLOAKED)	Einhar#c01ms11	4_4JOB_EINHAR,{
	npctalk "That person looks like they need help.", "", BC_SELF;
	end;
}

ch1_geffen,120,115,0	script	#c01ms10	HIDDEN_WARP_NPC,5,5,{
	end;
	
OnTouch:
	if ( isbegin_quest(17895) == 1 ) {
		cloaknpc("Charles#c01ms12", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms12", false, getcharid(0));
		end;
	}
	if ( 
		isbegin_quest(17896) == 1 || 
		isbegin_quest(17897) == 1 || 
		isbegin_quest(17898) == 1 ||
		isbegin_quest(17899) == 1 ||
		isbegin_quest(17918) == 1
	) {
		cloaknpc("Charles#c01ms12", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms12", false, getcharid(0));
		cloaknpc("Clover#c01ms10", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17920) == 2 ) {
		cloaknpc("Clover#c01ms10", false, getcharid(0));
	}
	if ( isbegin_quest(17925) > 0 ) {
		cloaknpc("Ascetic Jeon#c01ms12", false, getcharid(0));
		end;
	}
end;
}

ch1_geffen,120,115,1	script(CLOAKED)	Charles#c01ms12	4_M_JOB_WIZARD,{
	if ( isbegin_quest(17895) == 1 ) {
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "So you're saying even if remove this, nothing's going to come pouring right away?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "It depends on what's inside.";
		npctalk "No, I mean...!", "Charles#c01ms12", BC_SELF;
		next;
		select("What are you doing together?");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Oh, why are you here?";
		next;
		select("The others went to help the villagers...");
		cutin "nov_magicsoul02.png",2;
		mes "[Charles]";
		mes "You're just in time. Look at this. See that one? The Geffen Tower collapsed! The Geffen Tower! The symbol of knowledge and wisdom! The mighty Geffen Tower that no on could bring down, collapsed!";
		mes "What's happening to this world!!! Where's all the mages?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "We should assume that they all perished. Just look at the state of the Geffen Tower.";
		npctalk "I don't believe it! Why are you so negative about this?", "Charles#c01ms12", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Look at this wicked energy. It's no ordinary one. It's powerful and corrupt... it can deceive people and lead them to ruin...";
		next;
		select("Ascetic Jeon, so this is where you also ended up?");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Yes. I followed the trail of the ominous energy and led me here. An ominous energy and mages... it's no coincidence.";
		npctalk "Could you stop speaking like that in front of a mage?", "Charles#c01ms12", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "I am sure this energy is connected to the collapse of the tower.";
		mes "Maybe it's the reason why the tower is destroyed. Or they disturbed something that they shouldn't have.";
		next;
		cutin "nov_magicsoul02.png",2;
		mes "[Charles]";
		mes "Oh, stop it! That's not what happened! Look at this barrier. You think something like this would stop something cursed! If it some kind of disaster, they won't set it up like this. Alright?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "What is a barrier? A structure meant to block the entrance and exit from a specific place.";
		mes "Can you explain why is this barrier blocking the way to the underground? Doesn't it make you wonder anything especially with the collapsed tower on top of it";
		npctalk "You're clearly biased against mages.", "Charles#c01ms12", BC_SELF;
		next;
		select("There's a barrier?");
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "The Geffen Tower collapsed and the mages who should be here are gone. Only a faint trace of mana remained, so I cleared the rubble and found this barrier.";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "Take a closer look on this barrier. Only a mage can create something like this, right? That means the mage who casted it is still around.";
		next;
		cutin "nov_magicsoul02.png",2;
		mes "[Charles]";
		mes "We just need to find them and ask! Stop blaming the mages everytime!";
		mes "Maybe they went underground to save the world and set this up to keep people out.!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Calm down. Nothing works out when you're angry.";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "You're the one making me angry! Fine!! Let's just break the seal! Let's find out who is right!";
		next;
		select("I also think Geffen tower is the cause...");
		cutin "nov_magicsoul02.png",2;
		mes "[Charles]";
		mes "What...? Are you serious?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Do you think that the cause of the disease spreading here is also the Geffen Tower? Hm...";
		npctalk "Are you even sure with that???", "Charles#c01ms12", BC_SELF;
		next;
		cutin "nov_magicsoul02.png",2;
		mes "[Charles]";
		mes "No way! Even the most reckless mage wouldn't dare...";
		mes "Actually, wait. Maybe this? This isn't the same as our world... If that's the case... hm... might be possible...? Hm... or maybe... hm...";
		npctalk "He seems to be convincing himself now.", "Ascetic Jeon#c01ms12", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Regardless of what it is, we need to break this barrier and go inside. Especially that there's a possibility that it's the source. Now we just have to worry what lies below.";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "We won't know unless we break the barrier.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Then we have no choice.";
		next;
		cloaknpc("Clover#c01ms10", false, getcharid(0));
		mes "[???]";
		mes "W-wait a moment! Please stop! Step away from there!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "And who are you? Are you a Geffen Tower mage?";
		npctalk "That's a Geffen Tower mage!", "Charles#c01ms12", BC_SELF;
		next;
		cutin "ch1_clover03.png",2;
		mes "[???]";
		mes "Eeek! How did you know...! I even disguised myself so no one would recognize me...!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "That robe, that hat. Anyone could tell that you're a mage.";
		npctalk "Seriously, how did you even think that would work?", "Charles#c01ms12", BC_SELF;
		next;
		select("It's obvious that you're a mage.");
		cutin "ch1_clover03.png",2;
		mes "[???]";
		mes "...Are you going to hurt me? Throw stones at me? Or maybe, capture me and do horrible experiments...?";
		mes "Plesae don't, I'm just a simple mage...!!";
		npctalk "As expected of a mage.", "Ascetic Jeon#c01ms12", BC_SELF;
		specialeffect EF_HIT1, AREA, "Clover#c01ms10";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Ah, she's running away...!";
		npctalk "An attack?", "Ascetic Jeon#c01ms12", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "She's running? In that case...";
		next;
		specialeffect EF_PORTAL4, AREA, "Ascetic Jeon#c01ms12";
		specialeffect EF_NPC_STOP, AREA, "Clover#c01ms10";
		cutin "ch1_clover03.png",2;
		mes "[???]";
		mes "Ahh!!! What is this?? Let me go!";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "A mage from Geffen Tower. I've got so many questions for you. See this? I'm also a mage. You better answer carefully.";
		npctalk "Judging by that sloppy reaction, maybe she's not cut out to be a mage. Time for a career change.", "Ascetic Jeon#c01ms12", BC_SELF;
		changequest 17895, 17896;
		close3;
	}
	if ( isbegin_quest(17896) == 1 || isbegin_quest(17897) == 1 ) {
		npctalk "What's is she up to?", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17898) == 1 ) {
		npctalk "Let me know if you find anything out at the clinic. I'll be working on a way to release the barrier here.", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17899) == 1 ) {
		npctalk "I finally found a way to solve the problem! To release this barrier!", "", BC_SELF;
		end;
	}
	if( isbegin_quest(17918) == 1 ) {
		npctalk "That was a fascinating experience. I can't believe that there's still a chance for me to have another experience like that.", "", BC_SELF;
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17895) == 1";
end;
}

function	script	CH1_Daily_Jeon_Check	{
	for ( .@i = 17926; .@i <= 17930; ++.@i ) {
		if ( isbegin_quest(.@i) > 0 )
			return;
	}
	pctalk "Let's go back to report.";
	setquest 17931;
	return;
}

ch1_geffen,117,117,5	script(CLOAKED)	Ascetic Jeon#c01ms12	4_EXJOB_MASTER_J,{
	switch ( checkquest(17932, PLAYTIME) ) {
		case -1:
			break;
		case 0:
		case 1:
			cutin "ex_sa_masterj01.png",0;
			mes "[Ascetic Jeon]";
			mes "Why have you come so soon? You might feel fine physically, but your soul is not in the same state.";
			mes "Take enough rest and cleanse your body and mind.";
			close3;
		case 2:
			erasequest 17932;
			break;
	}
	if ( isbegin_quest(17931) == 1 ) {
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "So you've handed out all the purification talismans?";
		mes "Then rest for today and focus on purifying your body and mind. Let's meet again after dawn. Good work.";
		erasequest 17931;
		setquest 17932;
		getitem "Ch1_Root_Coin",5;
		close3;
	}
	if ( 
		isbegin_quest(17925) == 2 &&
		isbegin_quest(17926) > 0 ||
		isbegin_quest(17927) > 0 ||
		isbegin_quest(17928) > 0 ||
		isbegin_quest(17929) > 0 ||
		isbegin_quest(17930) > 0
	) {
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "It seems you haven't handed out all the purification talismans yet?";
		mes "You should hurry. It's best to distribute them before the day ends.";
		close3;
	}
	if ( 
		isbegin_quest(17925) == 2 &&
		isbegin_quest(17926) == 0 &&
		isbegin_quest(17927) == 0 &&
		isbegin_quest(17928) == 0 &&
		isbegin_quest(17929) == 0 &&
		isbegin_quest(17930) == 0
	) {
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Of, you're here? How are you feeling today? Can you go around the decayed Verus and hand out the purification talismans?";
		next;
		if ( select("I'll do it!", "Oh, not today...") == 2 ) {
			mes "[Ascetic Jeon]";
			mes "I understand. The work is important, but taking care of yourself comes first.";
			close3;
		}
		mes "[Ascetic Jeon]";
		mes "Okay. I'm glad you're able to do it. Then take these and go around the decayed Verus, hand them to those consumed by madness.";
		setquest 17926;
		setquest 17927;
		setquest 17928;
		setquest 17929;
		setquest 17930;
		getitem "Ch1_Purifi_Amulet",5;
		getexp 32652372, 2000000;
		close3;
	}
	if ( isbegin_quest(17895) == 1 ) {
		npctalk "I think we should listen to him first.", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17896) == 1 ) {
		npctalk "I think we should listen to her first.", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17897) == 1 ) {
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "So... if this really is a barrier to block monsters and diseases from underground.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "It is a barrier to block the monsters. I don't know anything about the disease though...";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "If we break this seal recklessly, monsters could flood out from the entrance and the disease could spread more. And no need to say but, we're the ones going to die first.";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "How can you say that so casually! Don't even think about touching the barrier!";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Even if we don't touch it, it looks to me that's its about to break any moment. Is it just me?";
		mes "So what's better, break it down now or let it break on it's own? Or you let me the genius mage stabilize it?";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Hmm... if you do that, the barrier will stabilize and stay up longer. Am I right?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "That won't do. We need to go down there ourselves. We have to confirm what's inside there. Only then we can device a plan.";
		mes "We will block the entry again after entering.";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "But still...";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "I get it now! You don't trust us! Even though I, the great mage is offering you to fix it!";
		npctalk "This kind of treatment, isn't it refreshing?", "Charles#c01ms12", BC_SELF;
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "But what if something goes wrong! It explode! Then crashed! Then people screaming around, what then!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Don't worry. I'll work with him. The fact that we made it here shows that we're not ordinary.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "What's that supposed to mean?";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "So, it's decided? I'll do it, okay? I'm starting now? Yes?";
		npctalk "Why are you ignoring me?", "Clover#c01ms10", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "We'll figure out how to safely deactivate the barrier and go in, Adventurer, go tell the others what we're going to do.";
		next;
		select("So Geffen Tower is not the cause of the disease?");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Yes. Since the timing of the disease and invasion match, we need to investigate further to get answers. We're going to work on how to release the barrier.";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "You need to make it clean that this didn't origin from the Geffen Tower. Otherwise, people might come up with completely wrong treatments.";
		mes "I don't think this disease is your typical disease. To understand that, we have to go underground.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "I agree. It might even be something that doesn't spread from person to person.";
		next;
		mes "[Ascetic Jeon]";
		mes "Deliver the message and also investigate the disease that's spreading here more. Symptoms, incubation time, patients connections... find out what you can.";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Ah! Tehn you should go to the clinic. Most of the patients are being treated there. It's the largest clinic in city. You'll definitely find out more there.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "I'll leave it to you.";
		changequest 17897, 17898;
		getitem "Ch1_Root_Coin",7;
		getexp 97957116, 5500000;
		close3;
	}
	if ( isbegin_quest(17898) == 1 ) {
		npctalk "Come straight back here if you learn anything at the clinic.", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17899) == 1 ) {
		npctalk "Wait a moment, I need to concentrate.", "", BC_SELF;
		end;
	}
	if( isbegin_quest(17918) == 1 ) {
		npctalk "Looks like there's a lot we need to sort out. We'll have to be prepared for what's ahead.", "", BC_SELF;
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17897) == 1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(17925) == 2 && isbegin_quest(17926) == 0 && isbegin_quest(17927) == 0 && isbegin_quest(17928) == 0 && isbegin_quest(17929) == 0 && isbegin_quest(17930) == 0 && isbegin_quest(17931) == 0 && checkquest(17932, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(17925) == 2 && isbegin_quest(17926) == 0 && isbegin_quest(17927) == 0 && isbegin_quest(17928) == 0 && isbegin_quest(17929) == 0 && isbegin_quest(17930) == 0 && isbegin_quest(17931) == 0 && checkquest(17932, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17931) == 1";
end;
}

ch1_geffen,122,118,3	script(CLOAKED)	Clover#c01ms10	1_M_WIZARD,{
	switch ( checkquest(17934, PLAYTIME) ) {
		case -1:
			break;
		case 0:
		case 1:
			cutin "ch1_clover01.png",2;
			mes "[Clover]";
			mes "Adventurer!";
			mes "Aren't you suffering from serious muscle pains? Why are you back so early?";
			next;
			mes "[Clover]";
			mes "Although it's only 300 monster, dealing with that many must be exhausting.";
			mes "Don't push yourself, get plenty of rest and come back after dawn.";
			close3;
		case 2:
			erasequest 17934;
			break;
	}
	if ( isbegin_quest(17925) == 2 && isbegin_quest(17933) == 0 ) {
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Adventurer!";
		mes "Have you stretched out properly? You're going to have to take down a huge number of robots and undead down there, that's no easy task.";
		next;
		if ( select("Of course.", "Not yet... I need more time.") == 2 ) {
			cutin "ch1_clover01.png",2;
			mes "[Clover]";
			mes "Oh no, you haven't stretched enough. That won't do. Take your time and go slow! Got it?";
			close3;
		}
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Let's see... Please defeat ^0000cd300 monsters^000000 in the decayed Verus.";
		mes "From what I saw, that's the minimum to keep the people underground safe. I'm counting on you!";
		setquest 17933;
		close3;
	}
	if ( isbegin_quest(17933) == 1 ) {
		if ( checkquest(17933, HUNTING) < 2 ) {
			cutin "ch1_clover01.png",2;
			mes "[Clover]";
			mes "Adventurer!";
			mes "Hey, how is everything going, I mean in the decayed Verus?";
			next;
			mes "[Clover]";
			mes "Please take care of ^0000CD300 monsters^000000!";
			close3;
		}
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Adventurer!!! You've worked so hard.";
		mes "It must have been tough.";
		next;
		mes "[Clover]";
		mes "You should get plenty of rest for today and come back after dawn. Got it?";
		erasequest 17933;
		setquest 17934;
		getitem "Ch1_Root_Coin",10;
		getexp 208473000, 145403733;
		close3;
	}
	if ( isbegin_quest(17896) == 1 ) {
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "A mage? Are you really one? Are you trying to trick me...?";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Does it look like I'm deceiving you? I thought you were smarter than that when you approached us.";
		npctalk "Considering how she was confronted by the residents, it's understandble that she's afraid.", "Ascetic Jeon#c01ms12", BC_SELF;
		npctalk "She tried to run. You're the one who caught her!", "Charles#c01ms12", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Like I said before, I'm a mage too. I have no reason to harm you.";
		mes "Actually, why would we even try to hard someone we've never met before?";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Are you really a mage...? Where did you come from? What region?";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Does that matter right now? Why don't you tell us what you're doing here, and what this barrier here is for? Maybe we can help. Or should we just break the barrier right now?";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "No, you can't! If you remove that, something terrible will happen. Hordes of monsters will come out.";
		npctalk "Monsters...", "Ascetic Jeon#c01ms12", BC_SELF;
		next;
		cutin "nov_magicsoul03.png",1;
		mes "[Charles]";
		mes "It it's a sealing barrier. Alright, Tell us everything you know about this barrier. Also about the Geffen Tower and the mages.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "I haven't introduced myself, please call me Clover. I'm a Geffen Tower mage. Well, not officially... but I'll be one soon.";
		npctalk "What do you mean by soon?", "Charles#c01ms12", BC_SELF;
		next;
		mes "[Clover]";
		mes "And this... Although it looks like this now, but this is Geffen Tower. Or, atleast it used to be...";
		next;
		cutin "",255;
		select("Why did it ended up like this?");
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Let's go with that first. We had nothing to do with the disease.";
		mes "We were just ordinary mages, studying our own interests and teaching it to others here at Geffen Tower.";
		next;
		mes "[Clover]";
		mes "But then, I don't know if you've heard it already... Geffen Tower was destroyed by a mysterious attack. Did you see the monsters? They suddenly started to attacking people.";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "If it was strong enough to destroy Geffen Tower, that must've been an enormous power. Where is the one who did it now?";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "I think they're underground? They probably went to destroy the lower levels too, I think?";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "So it destroyed the tower and went underground? What's even down there? Weren't there only graves down there?";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Actually. The thing is... One day... there was a mysterious ancient ruins that suddenly appeared deep underground beneath the Geffen Tower";
		next;
		cutin "nov_magicsoul02.png",1;
		mes "[Charles]";
		mes "Hold on, the more I hear. The less I understand. Am I the only one? So you're saying some ancient ruins appeared under Geffen Tower, then an unknown attacker destroyed the tower and went into those ruins?";
		mes "Do you think it's goal is the ruin down there?";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "I don't know what's their goal. And how they got here... But after the ruins appeared, mages kept going in and out to research it.";
		npctalk "Would you explain more?", "Charles#c01ms12", BC_SELF;
		next;
		select("Ancient ruins of a civilization? Sounds familiar...");
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Anyway, the monster started pouring through the underground entrance after the attack. The mages from Geffen Tower went in to stop it.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Then, why are you here alone?";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Oh, because I'm still an apprentice mage of Geffen Tower. To become an official mage, I need to finish my thesis and get it approved by my master.";
		npctalk "I was so close to becoming one...!", "Clover#c01ms10", BC_SELF;
		npctalk "That's unfortunate.", "Charles#c01ms12", BC_SELF;
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Anyways, since I'm not yet a full mage, they told me to stay out here and guard the barrier.";
		mes "They said it was meant to stop the monsters from getting out. My seniors and master put it up for protection...";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "For something like that... Isn't this barrier a bit too unstable?";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Is that so?? It think it was done in a rush, even I can tell that something feels off about it, but I really can't fix it myself...";
		mes "And I couldn't show myself near the residents either because I'm a mage. I had a hard time.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Now that you mentioned it, there were lots of rumors going around that the disease came from Geffen Tower. Is that why you've been hiding?";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "That's not true! Who even started that rumor! It's true that the disease started after the tower fell, but that's just coincidence!";
		npctalk "Yeah, right. Mages also research diseases. ... am I wrong...?", "Charles#c01ms12", BC_SELF;
		npctalk "Why are you saying that, you're a mage too!!", "Clover#c01ms10", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "From what I've heard, perhaps the disease wasn't caused by the mages, but the invasion itself.";
		mes "The shock from the attack could've awakened some unknown disease lying dormant underground.";
		npctalk "That's what I like to hear!", "Clover#c01ms10", BC_SELF;
		npctalk "That makes a lot more sense.", "Charles#c01ms12", BC_SELF;
		changequest 17896, 17897;
		close3;
	}
	if ( isbegin_quest(17897) == 1 ) {
		npctalk "Finally, someone on the same page!", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17899) == 1 ) {
		specialeffect EF_BIG_PORTAL, AREA, "to_hem_dun01";
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "What kind of spell do you plan to use in that situation?";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Well... Ah, they arrived. Let's continue this next time.";
		next;
		select("The barrier changed?");
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Did you notice? It took some effort. Now, no one except the Geffen Tower mages and us can get in or out.";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "They were amazing. They had an intense discussion, and they reached a conclusion like zip and zap ??! Bang! During my time as a mage, it's my first time seeing something like that!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "There is no door a ascetic cannot open with ascetic arts.";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "That's so cool... Uhm, would you teach me ascetic magic too? I'm so curious what would happen if magic and ascetic arts were combined. I bet it'd be super strong!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "If you are reborned, come find me and I might consider. Now, we have a long way to go.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "We're finally going underground...! Master! Seniors! Just wait a little longer! Clover is on the way!!";
		completequest 17899;
		setquest 17900;
		getitem "Ch1_Root_Coin",3;
		getexp 32652372, 1000000;
		close2;
		cutin "",255;
		cloaknpc("Charles#c01ms12", true, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms12", true, getcharid(0));
		cloaknpc("Clover#c01ms10", true, getcharid(0));
		navigateto("hem_dun01", 25, 116);
		end;
	}
	if( isbegin_quest(17918) == 1 ) {
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Phew! We made it out safely!";
		npctalk "Nothing beats fresh air!", "Clover#c01ms10", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Hm, are we sure this is the outside? What if we're still inside and we don't know about it?";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Eek, don't say things like that. You're scaring me.";
		npctalk "Wait... are you serious?", "Clover#c01ms10", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "It's a mages duty, duty to question everything.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "That's enough jokes. We need to take this to others and have them analyze it. Just being near it gives me headaches, we better deal with it quickly.";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Let's head to the city hall and take a closer look. Let's show everyone and hear their thoughts!";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Uhm... I want to go but, I'll stay here! I feel bad enough that you've brought me all the way down there, I'm sorry!";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Why? Are you worried about the barrier? There's no need to guard it anymore, remember the work that we did on it?";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "It's just... the cause of the plague is still unknown, people still look at the mages suspiciously... and I'm worried about my seniors and master. I'll think about how I can help in my own way.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "If that puts your mind at ease, then it's a correct choice. If anything urgent comes up, come to the city hall.";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Okay, I'll do that! Thank you so much!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Let's be on our way. We need to examine this as soon as possible.";
		erasequest 17918;
		setquest 17919;
		close2;
		cutin "",255;
		cloaknpc("Charles#c01ms12", true, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms12", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(17919) == 1 ) {
		npctalk "Adventurer, aren't you going to the city hall?", "Clover#c01ms10", BC_SELF;
		end;
	}
	if ( isbegin_quest(17920) == 2 && isbegin_quest(17922) == 0 ) {
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Adventurer? You went to investigate the plague? Did you find out anything?";
		mes "How did it go?";
		next;
		if ( select("The plague is...", "I was just passing by.") == 2 ) {
			mes "[Clover]";
			mes "You must be really busy! Please drop by and tell me sometime. I'm so curious!";
			close3;
		}
		mes "[Clover]";
		mes "Ah...! Is that what it was? Gr, I won't let them get away with this!!!";
		next;
		select("Calm down.");
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Huff, huff- Calm down, calm down. Okay, I'm calm.";
		mes "But still, those guys! It's because of them!! My master and seniors are suffering!";
		next;
		mes "[Clover]";
		mes "I'm really worried if they're properly eating down there, those folks only knows magic they won't manage to live properly.";
		npctalk "I'm also worried about their research papers...", "Clover#c01ms10", BC_SELF;
		next;
		mes "[Clover]";
		mes "It seems like they've managed so far... is there anything I can do to help?";
		mes "I wish there was something I could do, but no matter how much I think, nothing comes to mind and I feel so helpless. I guess I'll have to go down...!";
		next;
		select("Just ask them what they need.");
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Should I go down there? No matter how much I worry up here, it's not going to actually help.";
		next;
		mes "[Clover]";
		mes "In that case, I'll just ask them directly what they need!";
		mes "Let's go!";
		setquest 17922;
		close3;
	}
	if ( isbegin_quest(17925) == 1 ) {
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Will talismans really work?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Yes. We won't see the effect in time, if we won't try, no?";
		npctalk "You're doubting too much...", "Ascetic Jeon#c01ms12", BC_SELF;
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "No, it's not that I don't trust you... It's just hard that something small could do something like that... But, it's not that I don't believe you! I really believe you!";
		next;
		select("Talismans?");
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Ascetic Jeon said the purification talismans she made can reduce ones madness.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "It will dispel the energy of the Dark Whisper to be precise. By carrying one, it will purify their energy and prevent further contamination.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "So carrying just one is enough?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Of course. But it's only a temporary countermeasure.";
		mes "The fundamental solution is to eliminate the Dark Whisper completely and after that we will need to do a large-scale purification to remove all lingering energy.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "But will the mad people would be willing to carry it?";
		npctalk "They seem very hard-headed.", "Clover#c01ms10", BC_SELF;
		next;
		select("Can't we just say it's a relic for rebirth?");
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Oh... that's a great idea! That everyone would definitely carry one.";
		mes "How long does the talisman effects last? A week? A month?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "A day.";
		next;
		emotion ET_HUK, getnpcid(0,"Clover#c01ms10");
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Eh! A day! Isn't that too short?";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Like I said. Unless the main problem is removed, the energy will continue to pour like a bottomless jar.";
		mes "Think of it like dropping a dose of antidote into a poisoned lake.";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Ah...! That makes sense. I guess even an effect that last for a day is already awesome?";
		npctalk "I really want to learn ascetic magic one day...", "Clover#c01ms10", BC_SELF;
		next;
		mes "[Clover]";
		mes "Thank you so much for your help.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "We came to help in the first place. You being grateful is enough.";
		completequest 17925;
		getitem "Ch1_Root_Coin",10;
		getexp 65304744, 4000000;
		close;
	}
	npctalk "I have a good feeling about this. It feels like everything's going well. Just a feeling, you know? A hunch.", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17896) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17899) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17918) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17920) == 2 && isbegin_quest(17922) == 0";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17925) == 1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(17925) == 2 && isbegin_quest(17933) == 0 && checkquest(17934, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(17925) == 2 && isbegin_quest(17933) == 0 && checkquest(17934, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(17933, HUNTING) == 2";
end;
}

ch1_gef_in,70,57,0	script	#rch1_hw01	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17898) == 1 ) {
		cloaknpc("Doctor Neumann#rch1_01", false, getcharid(0));
		cloaknpc("Einhar#rch1_01", false, getcharid(0));
		cloaknpc("Gregor#rch1_01", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(16917) == 1 ) {
		cloaknpc("Einhar#rch1_03", false, getcharid(0));
		cloaknpc("Gregor#rch1_03", false, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,70,64,5	script(CLOAKED)	Doctor Neumann#rch1_01	4_CH1_NEUMANN,{
	if ( isbegin_quest(17898) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Greetings. It's a pleasure to meet you. Are you the one in charge here?";
		mes "We're investigating the disease that's spreading. My name is Einhar Enad.";
		next;
		cutin "4job_gregor_01.png",1;
		emotion ET_THINK, getnpcid(0,"Gregor#rch1_01");
		mes "[Gregor]";
		mes "This place... Yes. This scent is familiar to me.";
		mes "No offense meant. I'm Gregor, Gregor Diamant.";
		next;
		cutin "",255;
		mes "["+strcharinfo(0)+"]";
		mes "I'm "+strcharinfo(0)+". Nice to meet you.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "We'd like to ask about the disease that you're treating here at the clinic....";
		mes "Would it be possible to get some information?";
		next;
		cutin "ch1_neumann01.png",2;
		mes "[Doctor Neumann]";
		mes "...I'm Neumann. I'm the one in charge here, but.";
		mes "How do I know your true identity.... You don't seem to be from here.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "We were sent by Clover, the apprentice mage from the Geffen Tower.";
		mes "We're investigating in order to resolve the problem here. We'd appreciate your cooperation.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "With the disease spreading everywhere, it's not like we could use that information for anything bad anyway.";
		mes "Not that we'd do anything bad either.";
		next;
		cutin "ch1_neumann02.png",2;
		mes "[Doctor Neumann]";
		mes "Fair point. ...Okay. Right now, we really are short-staffed.";
		mes "If you're willing to help out, I'll grant you access inside.";
		next;
		cutin "ch1_neumann01.png",2;
		mes "[Doctor Neumann]";
		mes "If you want to investigate, you may speak to the patient directly.";
		mes "Just be sure not to force them and interfere with their recovery.";
		next;
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "That's more than enough. Thank you.";
		mes "Wht should we help on first?";
		next;
		cutin "ch1_neumann02.png",2;
		mes "[Doctor Neumann]";
		mes "I'm too busy to give specific instructions. Check on the patient and see what they need.";
		mes "If you hear anything from them, try to help however you can.";
		next;
		cutin "ch1_neumann01.png",2;
		mes "[Doctor Neumann]";
		mes "I'll be working nearby, come and find me if you need anything.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Let's get started. It's been a long time since I've treated living patients....";
		next;
		cutin "4job_einhar_01.png",2;
		emotion ET_THINK, getnpcid(0,"Einhar#rch1_01");
		mes "[Einhar]";
		mes ".......";
		next;
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "...Just kidding. Don't look at me like that, I'm fully capable of treating them.";
		mes strcharinfo(0)+", go take a look around the rooms on the back of the clinic.";
		completequest 17898;
		setquest 16912;
		setquest 16913;
		setquest 16914;
		setquest 16915;
		setquest 16916;
		close2;
		cutin "",255;
		cloaknpc("Doctor Neumann#rch1_01", true, getcharid(0));
		cloaknpc("Einhar#rch1_01", true, getcharid(0));
		cloaknpc("Gregor#rch1_01", true, getcharid(0));
		cloaknpc("Doctor Neumann#rch1_02", false, getcharid(0));
		cloaknpc("Einhar#rch1_02", false, getcharid(0));
		cloaknpc("Gregor#rch1_02", false, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17898) == 1";
end;
}

ch1_gef_in,74,55,1	script(CLOAKED)	Gregor#rch1_03	4259,{
	if ( isbegin_quest(16917) == 1 ) {
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "We got everything we need for today.";
		mes "......so, we're going to regroup for now.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "I'll leave the details to Einhar. Try talking to him.";
		mes "Once we've regroup and shared information, we will still help out again.";
		close3;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}


ch1_gef_in,75,58,3	script(CLOAKED)	Einhar#rch1_03	4_4JOB_EINHAR,{
	if ( isbegin_quest(16917) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Ah, you're here. We were just catching our breath ourselves.";
		mes "Have you heard anything from the patients?";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "We've heard quite a few interesting stories.";
		next;
		cutin "4job_einhar_01.png",2;
		emotion ET_SWEAT, getnpcid(0,"Einhar#rch1_03");
		mes "[Einhar]";
		mes "Interesting.... I see that's what interest you.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Then, let me start.";
		mes "The disease called Soren Syndrome begins like a cold, then it develops into a variety of thing like joint pain and sharp nerve pains that feels like stabbing.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "And once the high fever starts... survival rate becomes very low.";
		next;
		mes "[Einhar]";
		mes "Some healthy individual managed to recover, it's rare though, it's much more difficult for children, elderly and patients who are already ill.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Well... yes. I've heard a lot about the doctor too.";
		mes "Even here in the clinic, there are a few people who distrust him.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "They say if the patients comes in with a different illness, the doctor sometimes refuse treatment, maybe because there's risk of catching the disease?";
		mes "One patient was in an urgent condition but was declined, though that patient recovered, the patient were left with the side effects.";
		next;
		mes "[Gregor]";
		mes "Most patients are thank that the clinic is still operating despite the risk of catching the disease....";
		mes "Well, those are the perspectives that we've heard.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "That's about what me and Gregor heard.";
		mes "Now, "+strcharinfo(0)+", do you mind sharing the stories that you've heard.";
		next;
		while ( .@s < 3 ) {
			cutin "",255;
			switch( select(
				(!.@m[0] ? "Cause of Soren Syndrome" : ""),
				(!.@m[1] ? "Unusual Symptoms" : ""),
				(!.@m[2] ? "The Doctor's Origin" : "")
			)) {
				case 1:
					cutin "4job_gregor_01.png",1;
					mes "[Gregor]";
					mes "So, it started with people who had gone far outside the city....";
					mes "From there, it started spreading into the city.";
					.@s++;
					.@m[0] = true;
					break;
				case 2:
					cutin "4job_einhar_01.png",2;
					mes "[Einhar]";
					mes "A symptom involving severe weight loss and extreme hunger.";
					mes "And the doctor check on them personally?";
					.@s++;
					.@m[1] = true;
					break;
				case 3:
					cutin "4job_gregor_01.png",1;
					mes "[Gregor]";
					mes "To think that he stayed to treat the people, even though he's not from here.";
					mes "That's quite a sense of responsiblity, unlike what we've heard.";
					.@s++;
					.@m[2] = true;
					break;
			}
			next;
		}
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "I see. ...Oh, I also heard something else.";
		mes "Besides this clinic that is run by Doctor Neumann, there are also facilities run by mages and acolytes.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Should we also check those plces?";
		mes "I'm curious why some people would go there instead of the clinic.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Should an acolyte like you say that?";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "I'm sure everyone has their own reasons.";
		mes "Anyways, since we're done here, let's go check the <NAVI>[Acolyte's Clinic]<INFO>ch1_gef_in.gat,29,174,0,101,0</INFO></NAVI> and the <NAVI>[Mage's Clinic]<INFO>ch1_gef_in.gat,70,170,0,101,0</INFO></NAVI>. I'll go ahead of you.";
		completequest 16917;
		setquest 16925;
		setquest 16926;
		close2;
		cutin "",255;
		cloaknpc("Einhar#rch1_03", true, getcharid(0));
		cloaknpc("Gregor#rch1_03", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16917) == 1";
end;
}

function	script	CH1_Clinic_Quest_Check	{
	for ( .@i = 16912; .@i <= 16924; ++.@i ) {
		if ( isbegin_quest(.@i) != 0 )
			return;
	}
	setquest 16917;
	pctalk "I think I got enough information. Let's meet with the others.";
	navigateto("ch1_gef_in", 75, 58);
	cloaknpc("Einhar#rch1_02", true, getcharid(0));
	cloaknpc("Gregor#rch1_02", true, getcharid(0));
	cloaknpc("Einhar#rch1_03", false, getcharid(0));
	cloaknpc("Gregor#rch1_03", false, getcharid(0));
	return;
}

ch1_gef_in,66,62,5	script(CLOAKED)	Einhar#rch1_01	4_4JOB_EINHAR,{
	if ( isbegin_quest(17898) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "This person seems to be the doctor in charge here.";
		mes "Let's talk to him.";
		close3;
	}
	end;
}

ch1_gef_in,67,60,3	script(CLOAKED)	Gregor#rch1_01	4259,{
	if ( isbegin_quest(17898) == 1 ) {
		cutin "4job_gregor_04.png",1;
		mes "[Gregor]";
		mes "You and Einhar sure moves fast.";
		mes "I can't match your pace... slow down a bit.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Well, I know we're in a hurry but still....";
		close3;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_gef_in,75,71,0	script	#rch1_hw02	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17898) == 2 && isbegin_quest(16917) == 0 ) {
		cloaknpc("Doctor Neumann#rch1_02", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(16928) == 1 ) {
		cloaknpc("Doctor Neumann#rch1_02", false, getcharid(0));
		cloaknpc("Patient in Care#rch1_01", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(16931) == 1 ) {
		cloaknpc("Doctor Neumann#rch1_02", true, getcharid(0));
		cloaknpc("Patient in Care#rch1_01", true, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,110,69,0	script	#rch1_hw03	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17898) == 2 && isbegin_quest(16917) == 0 ) {
		cloaknpc("Einhar#rch1_02", false, getcharid(0));	
		end;
	}
end;
}

ch1_gef_in,111,109,0	script	#rch1_hw04	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17898) == 2 && isbegin_quest(16917) == 0 ) {
		cloaknpc("Gregor#rch1_02", false, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,81,74,3	script(CLOAKED)	Doctor Neumann#rch1_02	4_CH1_NEUMANN,{
	if ( isbegin_quest(16928) == 1 ) {
		cutin "ch1_neumann01.png",2;
		mes "[Doctor Neumann]";
		mes "I thought you already went somewhere....";
		mes "And now you're back. What's the matter?";
		next;
		cutin "",255;
		select("We need a blood sample from a patient");
		cutin "ch1_neumann02.png",2;
		mes "[Doctor Neumann]";
		mes "A blood sample? What is it for....";
		mes "That's my responsiblity.";
		next;
		cutin "",255;
		select("We have an expert among us.");
		cutin "ch1_neumann02.png",2;
		mes "[Doctor Neumann]";
		mes "......Alright. If you insist so.";
		mes "I'll allow you to get a blood sample.";
		next;
		cutin "",255;
		select("I'd like to get it myself.");
		cutin "ch1_neumann04.png",2;
		emotion ET_ANGER, getnpcid(0,"Doctor Neumann#rch1_02");
		mes "[Doctor Neumann]";
		mes "...Lady Barbara, would it be alright if this person draws a little bit of your blood?";
		mes "It'll just sting for a moment.";
		next;
		cutin "ch1_sick03.png",0;
		mes "[Patient in Care]";
		mes "Oh, my blood? How much?";
		mes "Will it hurt?";
		next;
		cutin "",255;
		select("Show the kit.", "Only a little.");
		cutin "ch1_sick03.png",0;
		mes "[Patient in Care]";
		mes "A little... if it's only that much. I'll be happy to help with your research.";
		mes "Please be gentle....";
		next;
		cutin "ch1_neumann04.png",2;
		mes "[Doctor Neumann]";
		mes "Your kit looks safe enough....... I'll stay and keep an eye on things just in case something happens.";
		mes "She's my <NAVI>[patient]<INFO>ch1_gef_in.gat,79,73,0,101,0</INFO></NAVI> after all.";
		erasequest 16928;
		setquest 16935;
		close3;
	}
	if ( isbegin_quest(17898) == 2 && isbegin_quest(16922) == 0 ) {
		cutin "ch1_neumann04.png",2;
		mes "[Doctor Neumann]";
		mes "I'm very busy as it is....";
		mes "Try speaking directly with the patients. You'll get more information that way.";
		close3;
	}
	if ( isbegin_quest(16922) == 1 ) {
		cutin "ch1_neumann01.png",2;
		mes "[Doctor Neumann]";
		mes "What is it Have you been checking on the patients?";
		mes "Even if you ask me something, I'm too busy right now.";
		next;
		select("There's a patient who lost weight but eats compulsively.");
		cutin "ch1_neumann02.png",2;
		mes "[Doctor Neumann]";
		mes "Ah, I see. Is that so.";
		mes "...Thank you for letting me know. I need to go check on them myself.";
		next;
		mes "[Doctor Neumann]";
		mes "......What could be the cause of such symptoms....";
		next;
		cutin "",255;
		select("What?");
		cutin "ch1_neumann01.png",2;
		mes "[Doctor Neumann]";
		mes "Nothing. Just talking to myself.";
		mes "I'll finish what I'm doing and go see the patient....";
		next;
		mes "[Doctor Neumann]";
		mes "You should go finish your tasks.";
		mes "There's so much to do.";
		erasequest 16922;
		cloaknpc("Doctor Neumann#rch1_02", true, getcharid(0));
		close2;
		cutin "",255;
		CH1_Clinic_Quest_Check();
		end;
	}
	cutin "ch1_neumann04.png",2;
	mes "[Doctor Neumann]";
	mes "I'm busy. I'd take event an ant if I could.";
	mes "It's not that I'm avoiding conversation, but I ask for your understanding.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16922) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16928) == 1";
end;
}

ch1_gef_in,79,73,7	script(CLOAKED)	Patient in Care#rch1_01	4_F_PATIENT01,{
	if ( isbegin_quest(16935) == 1 ) {
		cutin "ch1_sick03.png",0;
		mes "[Patient in Care]";
		mes "Uhm, I'm a little scared.";
		mes "Please don't make it hurt too much....";
		next;
		emotion ET_HUK, getnpcid(0,"Patient in Care#rch1_01");
		mes "[Patient in Care]";
		mes ".......";
		mes "......Ouch!";
		next;
		mes "[Patient in Care]";
		mes ".......";
		mes "......Ugh....";
		next;
		cutin "",255;
		select("All done.");
		cutin "ch1_sick03.png",0;
		mes "[Patient in Care]";
		mes "Phew, that stung a bit.";
		mes "But I don't mind, if it helps with Soren Syndrome's research....";
		next;
		emotion ET_CRY, getnpcid(0,"Patient in Care#rch1_01");
		mes "[Patient in Care]";
		mes "Please do your best.";
		mes "Ow, it still stings....";
		next;
		cutin "ch1_neumann04.png",2;
		mes "[Doctor Neumann]";
		mes "Are you done now?";
		mes "I think I'll head to my house for a bit and try to get some sleep.";
		next;
		mes "[Doctor Neumann]";
		mes "While I'm gone, don't do stuffs to patients like this without my permission.";
		mes "If it's something urgent, you should come to my house instead.";
		pctalk "I should take back the sample to the city hall.";
		erasequest 16935;
		setquest 16931;
		close3;
	}
	cutin "ch1_sick03.png",0;
	mes "[Patient in Care]";
	mes "Doctor Neumann you're the best. I slept well last night.";
	mes "It seems that the medicine is working. My fever also went down.";
	next;
	cutin "ch1_neumann01.png",2;
	mes "[Doctor Neumann]";
	mes "Is that so? That's good to hear. I'm glad that you're getting better.";
	mes "Let's try reducing the dosage a little today.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16935) == 1";
end;
}

ch1_gef_in,114,71,7	script(CLOAKED)	Einhar#rch1_02	4_4JOB_EINHAR,{
	cutin "4job_einhar_02.png",2;
	mes "[Einhar]";
	mes "Alright, mister.... Please calm down and slowly inhale this medicine.";
	mes "It's a cough medicine, so please bear with it.";
	next;
	cutin "ch1_saram02.png",1;
	emotion ET_PROFUSELY_SWAT, getnpcid(0,"Coughing Patient#rch1_01");
	mes "[Coughing Patient]";
	mes "Cough, cough!";
	mes "W-why! Why do I have to inhale spicy stuffs like this when I'm already coughing? Kuh....";
	next;
	cutin "4job_einhar_01.png",2;
	mes "[Einhar]";
	mes "You have to try. Now, please inhale it slowly.";
	next;
	cutin "ch1_saram02.png",1;
	mes "[Coughing Patient]";
	mes "*inhales*... Cough, cough, Kehkeh!";
	close3;
}

ch1_gef_in,115,73,5	script	Coughing Patient#rch1_01	4_M_04,{
	cutin "ch1_saram02.png",1;
	mes "[Coughing Patient]";
	mes "Kehk, cough, cough!";
	mes "Ugh, sorry. I can't help it....";
	next;
	mes "[Coughing Patient]";
	mes "You must feel uneasy if someone keeps coughing like that.";
	mes "I do try to cover my mouth when I cough and was my hands....";
	close3;
}

ch1_gef_in,113,106,3	script(CLOAKED)	Gregor#rch1_02	4259,{
	cutin "4job_gregor_01.png",1;
	mes "[Gregor]";
	mes "What kind of meals do you usually provide here?";
	next;
	cutin "vr_f02.png",1;
	mes "[Clinic's Meal Handler]";
	mes "We mostly provide things that are soft and easy to eat.";
	mes "Like Porridge, or Soft Bread....";
	next;
	cutin "4job_gregor_01.png",1;
	mes "[Gregor]";
	mes "Soft bread sounds costly.";
	next;
	cutin "vr_f02.png",1;
	mes "[Clinic's Meal Handler]";
	mes "That's why we usually go with grain-based porridge.";
	mes "We boil it well with some meat and vegetables to keep it nutritionally balanced.";
	next;
	cutin "4job_gregor_02.png",1;
	mes "[Gregor]";
	mes "Eating the same thing everyday must be tough....";
	close3;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_gef_in,113,102,3	script	Clinic Volunteer#rch1	4_F_02,{
	cutin "vr_f02.png",1;
	mes "[Clinic's Meal Handler]";
	mes "Now that everyone has eaten... I can finally take a breather.";
	mes "Oh, can I help you with something?";
	next;
	cutin "",255;
	switch( select("Ask if you can get a meal.", "Ask if they have sweets.", "Ask if there's anything you can help with.") ) {
		case 1:
			if ( isbegin_quest(16920) == 1 ) {
				cutin "vr_f02.png",1;
				mes "[Clinic's Meal Handler]";
				mes "Someone is asking for more food? Which patient is it?";
				next;
				cutin "",255;
				select("Explain the patient.");
				cutin "vr_f02.png",1;
				mes "[Clinic's Meal Handler]";
				mes "If it's that person, he already asked for more food earlier....";
				mes "Did that person lost a lot of weight?";
				next;
				cutin "",255;
				select("From what I've heard, yes.");
				cutin "vr_f02.png",1;
				mes "[Clinic's Meal Handler]";
				mes "I see.... We do get patients like that.";
				mes "People who lose a lot of weight and is compulsively eating.";
				next;
				mes "[Clinic's Meal Handler]";
				mes "Sadly, those patients... often don't hold out for long.";
				mes "We need to report it separately to Doctor Neumann too.";
				next;
				mes "[Clinic's Meal Handler]";
				mes "Here, I'll pack some bread. Please bring it to a patient.";
				mes "Could you also report the patient's condition to Doctor Neumann?";
				next;
				mes "[Clinic's Meal Handler]";
				mes "It's bad news... I feel hesitant to go myself.";
				mes "That patient used to be my neighbor....";
				next;
				cutin "",255;
				select("I'll deliver the message.");
				cutin "vr_f02.png",1;
				mes "[Clinic's Meal Handler]";
				mes "Thank you. Then, you have two things to do.";
				mes "Please give the bread to the <NAVI>[Patient]<INFO>ch1_gef_in.gat,65,112,0,101,0</INFO></NAVI>, and report to <NAVI>[Doctor Neumann]<INFO>ch1_gef_in.gat,81,74,0,101,0</INFO></NAVI> afterwards.";
				erasequest 16920;
				setquest 16921;
				setquest 16922;
				close3;
			}
			cutin "vr_f02.png",1;
			mes "[Clinic's Meal Handler]";
			mes "Oh... I apologize.";
			mes "Volunteers are free to eat outside and return afterwards.";
			next;
			mes "[Clinic's Meal Handler]";
			mes "We'd love to provide you a meal, but we are tight on budget at the moment....";
			close3;
		case 2:
			if ( isbegin_quest(16923) == 1 ) {
				cutin "vr_f02.png",1;
				mes "[Clinic's Meal Handler]";
				mes "Oh, is it the for kid who was admitted with her grandmother?";
				mes "The food here taste too healthy for a child.";
				next;
				mes "[Clinic's Meal Handler]";
				mes "As you already know sweets are hard to come by... Let's see.";
				mes "I'll take a quick look and see what we have.";
				next;
				emotion ET_DELIGHT, getnpcid(0,"Clinic's Meal Handler#rch1");
				mes "[Clinic's Meal Handler]";
				mes "We happen to have some leftover sweet potatoes. Let's add sugar like this~ A little more~";
				mes "Let it melt well and evenly coat....";
				next;
				mes "[Clinic's Meal Handler]";
				mes "Here, I've prepared a sweet food. Please take it to the <NAVI>[Young Patient]<INFO>ch1_gef_in.gat,106,106,0,101,0</INFO></NAVI>.";
				mes "Thank you so much for your thoughtfulness.";
				erasequest 16923;
				setquest 16924;
				close3;
			}
			cutin "vr_f02.png",1;
			mes "[Clinic's Meal Handler]";
			mes "A snack? Is it for yourself...?";
			mes "We haven't been able to prepare food for everyone. I'm afraid we don't have any....";
			close3;
		case 3:
			cutin "vr_f02.png",1;
			mes "[Clinic's Meal Handler]";
			mes "Oh, Thank you for your concern. The things have settled down a bit.";
			mes "Everything urgent is finished.";
			next;
			mes "[Clinic's Meal Handler]";
			mes "Try checking with someone else and see if there's work to do.";
			mes "I really appreciate the offer though.";
			close3;		
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16920) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16923) == 1";
end;
}

ch1_gef_in,109,39,5	script	Coughing Patient#rch1_0	4_M_PATIENT01,{
	if ( isbegin_quest(17898) < 2 ) {
		cutin "ch1_sick01.png",2;
		mes "[Patient with Severe Cough]";
		mes "Cough, cough!";
		mes "Wearing a mask all day is so suffocating.";
		mes "But I guess it's better than spreading it to others... Cough!";
		close3;
	}
	if ( isbegin_quest(16912) == 1 ) {
		cutin "ch1_sick01.png",2;
		emotion ET_KEK, getnpcid(0,"Coughing Patient#rch1_0");
		mes "[Patient with Severe Cough]";
		mes "Cough, cough! Ugh, my chest. cough!";
		next;
		cutin "",255;
		if ( select("Are you alright?", "Give him a pat on the back.") == 2 ) {
			cutin "ch1_sick01.png",2;
			mes "[Patient with Severe Cough]";
			mes "Hak, cough! Hak, kah.";
			mes "Huff, huff. I can finally breath a little.";
		} else {
			cutin "ch1_sick01.png",2;
			mes "[Patient with Severe Cough]";
			mes "Ugh, cough! Hak!";
			mes "Uhm, could you pass me some water....";
			next;
			cutin "",255;
			select("Pass the water.");
			cutin "ch1_sick01.png",2;
			mes "[Patient with Severe Cough]";
			mes "Gulp gulp gulp....";
			next;
			mes "[Patient with Severe Cough]";
			mes "Phew...! Ah, I feel a little bit better.";
		}
		next;
		cutin "ch1_sick01.png",2;
		mes "[Patient with Severe Cough]";
		mes "Ah, thank you. Who are you? You don't look familiar.";
		next;
		cutin "",255;
		select("I'm here to help out the clinic.");
		cutin "ch1_sick01.png",2;
		emotion ET_BEST, getnpcid(0,"Coughing Patient#rch1_0");
		mes "[Patient with Severe Cough]";
		mes "Thank you. You are doing a good thing.";
		mes "Everyone else is too afraid to come near us, fearing that they'll catch it too. They said the disease spreads from coughing.";
		next;
		mes "[Patient with Severe Cough]";
		mes "Not that I blame them, of course. I was also the same before I got sick.";
		mes "I really appreciate you coming to help.";
		next;
		mes "[Patient with Severe Cough]";
		mes "At least for now it's just coughing, I don't have a fever yet. No join pain or any kind of severe pain.";
		mes "Once the fever starts, the disease becomes more dangerous. When the high fever comes... sadly, there's almost no hope of recovering.";
		next;
		mes "[Patient with Severe Cough]";
		mes "I think I'll get better... I'm still hoping that I would.";
		mes "I'm still young and don't have any other health issues. If I'm lucky that is....";
		erasequest 16912;
		close2;
		cutin "",255;
		CH1_Clinic_Quest_Check();
		end;
	}
	cutin "ch1_sick01.png",2;
	mes "[Patient with Severe Cough]";
	mes "Cough, cough...! Ugh, my whole body is starting to ache.";
	mes "Even so, thanks to your help, it's more bearable than before.....";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16912) == 1";
end;
}

ch1_gef_in,104,33,5	script	Bath Craved Patient#rch1_01	4_F_PATIENT01,{
	if ( isbegin_quest(17898) < 2 ) {
		cutin "ch1_sick04.png",2;
		mes "[Bath Craved Patient]";
		mes "Bath... I want to bath. But I don't have the strength to do so....";
		mes "I need to wash my hands and brush my teeth to recover quickly.";
		close3;
	}
	if ( isbegin_quest(16913) == 1 ) {
		cutin "ch1_sick04.png",2;
		emotion ET_CRY, getnpcid(0,"Bath Craved Patient#rch1_01");
		mes "[Bath Craved Patient]";
		mes "It's suffocating... I want to go home. My body hurts so much....";
		mes "If I could just soak in warm water and bath a bit, I'll feel refreshed.";
		next;
		mes "[Bath Craved Patient]";
		mes "Ah, you're a new face. You don't look like a patient... Who are you?";
		next;
		cutin "",255;
		select("I'm a volunteer.");
		cutin "ch1_sick04.png",2;
		mes "[Bath Craved Patient]";
		mes "Ah...! Then, can you help me out a bit?";
		mes "I feel so stuffy right now....";
		next;
		mes "[Bath Craved Patient]";
		mes "There's a bathroom here, but I don't have the strength to move.";
		mes "I wish I could atleast get a hot towel instead.";
		next;
		mes "[Bath Craved Patient]";
		mes "Could you ask the <NAVI>[Clinic Volunteer]<INFO>ch1_gef_in.gat,115,40,0,101,0</INFO></NAVI> standing by the water container over there to get me one?";
		mes "I want to wipe myself off a bit with the wet towel.";
		next;
		cutin "",255;
		if ( select("I'll go get it for you.", "I'm a bit busy right now.") == 2 ) {
			cutin "ch1_sick04.png",2;
			mes "[Bath Craved Patient]";
			mes "Ah, I guess it can't be helped if you're busy. I don't want to be a bother.";
			mes "Ugh... it's suffocating.";
			close3;
		}
		cutin "ch1_sick04.png",2;
		mes "[Bath Craved Patient]";
		mes "Thank you. I'll be counting on you.";
		mes "It would be perfect if it's warm one.";
		next;
		mes "[Bath Craved Patient]";
		mes "I used to be a herbalist. My job was to go find herbs near Geffen.";
		mes "Sigh... I can't believe I got sick like this.";
		next;
		mes "[Bath Craved Patient]";
		mes "But even Velveta the hunter also caught it.";
		mes "I think the people who went outside the village were the first one who caught it.";
		erasequest 16913;
		setquest 16918;
		close3;
	}
	if ( isbegin_quest(16918) == 1 ) {
		cutin "ch1_sick04.png",2;
		mes "[Bath Craved Patient]";
		mes "Could you ask the <NAVI>[Clinic Volunteer]<INFO>ch1_gef_in.gat,115,40,0,101,0</INFO></NAVI> standing by the water container over there to get me one?";
		close3;
	}
	if ( isbegin_quest(16919) == 1 ) {
		mes "[Bath Craved Patient]";
		mes "Oh, a warm towel. Thank you. Let me sit down a bit....";
		mes "I'll wipe myself. It'll feel awkward asking for help with something like that.";
		next;
		mes "[Bath Craved Patient]";
		mes "They said disease can be prevented by washing your hands and staying clean.";
		mes "I should've been more careful....";
		erasequest 16919;
		close2;
		cutin "",255;
		CH1_Clinic_Quest_Check();
		end;
	}
	cutin "ch1_sick04.png",2;
	mes "[Bath Craved Patient]";
	mes "I wish I could soak my body in hot water....";
	mes "It's better than nothing. I feel much better. Thank you.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16913) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16919) == 1";
end;
}

ch1_gef_in,115,40,3	script	Clinic Volunteer#rch1_	1_F_01,{
	if ( isbegin_quest(16918) == 1 ) {
		mes "[Clinic Volunteer]";
		mes "Are you a new volunteer? Is there anything I can help you with?";
		next;
		cutin "",255;
		if ( select("A patient is asking for a warm towel...", "I'm alright for now.") == 2 ) {
			cutin "ch1_saram04.png",2;
			mes "[Clinic Volunteer]";
			mes "Really? Well, if you ever need help, feel free to let me know.";
			mes "I'll be around here for a while.";
			close3;
		}
		cutin "ch1_saram04.png",2;
		mes "[Clinic Volunteer]";
		mes "Ah... I got it. I'll prepare one right away.";
		mes "We always have warm water ready.";
		next;
		mes "[Clinic Volunteer]";
		mes "Since the doctor is always busy, we have to be more diligent.";
		mes "Did you know? Doctor Neumann isn't from here.";
		next;
		mes "[Clinic Volunteer]";
		mes "I heard that he came to Geffen for a conference, but he couldn't overlook the situation and decided to stay here.";
		mes "A truly righteous person.";
		next;
		emotion ET_DELIGHT, getnpcid(0,"Clinic Volunteer#rch1_");
		mes "[Clinic Volunteer]";
		mes "Alright, it's ready. Here's the warm wet towel.";
		mes "Hurry and bring it to the <NAVI>[Bath Craved Patient]<INFO>ch1_gef_in.gat,102,33,0,101,0</INFO></NAVI> before it cools down.";
		erasequest 16918;
		setquest 16919;
		close3;
	}
	cutin "ch1_saram04.png",2;
	mes "[Clinic Volunteer]";
	mes "I'm sorry, things are a bit hectic right now, I can't even catch a breath....";
	mes "If it's not urgent, could you come back later?";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16918) == 1";
end;
}

ch1_gef_in,65,112,7	script	Skinny Patient#rch1_	4_M_PATIENT02,{
	if ( isbegin_quest(17898) < 2 ) {
		cutin "ch1_sick02.png",0;
		mes "[Skinny Patient]";
		mes "Ugh... I'm so hungry. It hasn't been long since I've last eaten....";
		mes "It's a shame, I want to eat more....";
		close3;
	}
	if ( isbegin_quest(16914) == 1 ) {
		cutin "ch1_sick02.png",0;
		mes "[Skinny Patient]";
		mes "...Hey.";
		next;
		emotion ET_HUK, getnpcid(0,"Skinny Patient#rch1_");
		mes "[Skinny Patient]";
		mes "......Hey...!";
		next;
		cutin "",255;
		if ( select("Are you talking to me?", "Pretend not to hear.") == 2 ) {
			cutin "ch1_sick02.png",0;
			mes "[Skinny Patient]";
			mes "Over here, don't leave......!";
			close3;
		}
		cutin "ch1_sick02.png",0;
		mes "[Skinny Patient]";
		mes "Yes, I'm talking to you. Are you a new volunteer? I have a favor to ask.";
		mes "Do you have anything to eat?";
		next;
		mes "[Skinny Patient]";
		mes "No, I'm not possessed by a glutton spirit.";
		mes "...Or maybe I am?";
		next;
		mes "[Skinny Patient]";
		mes "I can't stand it anymore, I get hungry all day long.";
		mes "I get hungry as soon as I finish eating....";
		next;
		emotion ET_OTL,getnpcid(0, "Skinny Patient#rch1_");
		mes "[Skinny Patient]";
		mes "If you don't have any food... Could you pleask the <NAVI>[Clinic's Meal Handler]<INFO>ch1_gef_in.gat,113,102,0,101,0</INFO></NAVI> for one?";
		mes "Tell them I'm starving to death....";
		next;
		navigateto("ch1_gef_in.gat", 113, 102);
		cutin "",255;
		if ( select("I'll let her know.", "Hold on a bit.") == 2 ) {
			cutin "ch1_sick02.png",0;
			mes "[Skinny Patient]";
			mes "Ah...!";
			mes "Come on, If I can... I wouldn't be asking like this!";
			close3;
		}
		cutin "ch1_sick02.png",0;
		mes "[Skinny Patient]";
		mes "Thank you. It would be better if you could bring it right away.";
		mes "I lost so much weight even though I'm eating like this... Strange....";
		erasequest 16914;
		setquest 16920;
		close3;
	}
	if ( isbegin_quest(16920) == 1 ) {
		cutin "ch1_sick02.png",0;
		mes "[Skinny Patient]";
		mes "I've lost so much much weight even though I'm eating like this.";
		mes "So strange....";
		close3;
	}
	if ( isbegin_quest(16921) == 1 ) {
		cutin "ch1_sick02.png",0;
		mes "[Skinny Patient]";
		mes "Ah, you're back? I've been waiting for a while.";
		mes "Did you bring it... something to eat?";
		next;
		cutin "",255;
		select("Hand over the bread.");
		cutin "ch1_sick02.png",0;
		emotion ET_BEST, getnpcid(0,"Skinny Patient#rch1_");
		mes "[Skinny Patient]";
		mes "Ah! ...Bread!";
		mes "Bread's great... it's a tough grain-based bread, but it tastes good when you chew it well....";
		next;
		mes "[Skinny Patient]";
		mes "Munch, munch... Gulp....";
		mes "It's a shame that it would run out after a while....";
		erasequest 16921;
		close2;
		cutin "",255;
		CH1_Clinic_Quest_Check();
		end;
	}
	mes "[Skinny Patient]";
	mes "Munch, munch... gulp.";
	mes "Munch, munch, munch....";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16914) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16921) == 1";
end;
}

ch1_gef_in,108,106,5	script	Young Patient#rch1_01	4_PATIEN_KID,{
	if ( isbegin_quest(17898) < 2 ) {
		cutin "ch1_kid01.png",2;
		mes "[Young Patient]";
		mes (Sex ? "Big brother" : "Big sister") + ", I want to eat something sweet....";
		mes "I'll be honest, the food here isn't yummy at all.";
		close3;
	}
	if ( isbegin_quest(16915) == 1 ) {
		cutin "ch1_kid01.png",2;
		emotion ET_SMILE, getnpcid(0,"Young Patient#rch1_01");
		mes "[Young Patient]";
		mes "Hewwo. " + (Sex ? "Big brother." : "Big sister.");
		mes "Are you a nurse?";
		next;
		cutin "ch1_saram08.png",2;
		mes "[Elderly Patient]";
		mes "Oh dear, my grandchild thinks all the volunteers are nurses.";
		mes "The nurse who used to be here isn't around anymore....";
		next;
		mes "[Elderly Patient]";
		mes "She caught the disease while caring for patients... she's gone now.";
		mes "Oh, I shouldn't be saying stuffs like this.";
		next;
		mes "[Elderly Patient]";
		mes "Anyway, please understand if little Yuri here calls you the wrong thing.";
		mes "Thank you for your understanding.";
		next;
		cutin "ch1_kid01.png",2;
		mes "[Young Patient]";
		mes "Cough, cough. There's nothing here, "  + (Sex ? "big brother." : "big sister.");
		mes "...Yuri want to eat something yummy!";
		next;
		mes "[Young Patient]";
		mes "The food here is not yummy.";
		mes "I want to eat something sweet.";
		next;
		cutin "ch1_saram08.png",2;
		mes "[Elderly Patient]";
		mes "Now, what are you saying suddenly....";
		mes "We should be just thankful for the food that they give.";
		next;
		cutin "ch1_kid01.png",2;
		emotion ET_THANKS, getnpcid(0,"Young Patient#rch1_01");
		mes "[Young Patient]";
		mes "Yuri is thankful for the food.";
		mes "But I would be more thankful, if it's yummy food!";
		next;
		cutin "",255;
		if ( select("I'll ask around.", "Stop complaining.") == 2 ) {
			cutin "ch1_kid01.png",2;
			mes "[Young Patient]";
			mes "...? But I'm not full yet. I'm hungry.";
			mes "Cough, my hungry tummy wants yummy food!";
			close3;
		}
		cutin "ch1_kid01.png",2;
		mes "[Young Patient]";
		mes "Cough, Waah, Thank you!!";
		mes "Yuri likes sweets. I can eat it anytime!";
		next;
		cutin "ch1_saram08.png",2;
		mes "[Elderly Patient]";
		mes "Oh... thank you for putting up with a child's whim.";
		mes "I appreciate it.... If you could ask the <NAVI>[Clinic's Meal Handler]<INFO>ch1_gef_in.gat,113,102,0,101,0</INFO></NAVI>, it would mean a lot for us.";
		erasequest 16915;
		setquest 16923;
		close3;
	}
	if ( isbegin_quest(16923) == 1 ) {
		cutin "ch1_kid01.png",2;
		mes "[Young Patient]";
		mes "Cough, cough....";
		mes "I'm coughing because my tummy's growling for something yummy.";
		close3;
	}
	if ( isbegin_quest(16924) == 1 ) {
		cutin "ch1_kid01.png",2;
		mes "[Young Patient]";
		mes (Sex ? "Big brother" : "Big sister") + "! I've been waiting.";
		mes "Did you bring something yummy?";
		next;
		cutin "",255;
		select("Give the Sweet Potato Sweets.");
		cutin "ch1_kid01.png",2;
		mes "[Young Patient]";
		mes "Wow, it looks yummy!";
		mes "Is it made with Sweet Potato?";
		next;
		mes "[Young Patient]";
		mes "Crunch, crunch, munch, munch... Gulp.";
		mes "It's sweet! It's yummy!";
		next;
		emotion ET_BIGTHROB, getnpcid(0,"Young Patient#rch1_01");
		mes "[Young Patient]";
		mes "I'm going to save it, and slowly eat it!";
		mes "Thank you, Yuri loves " + (Sex ? "big brother" : "big sister") + " so much!";
		next;
		cutin "ch1_saram08.png",2;
		mes "[Elderly Patient]";
		mes "Oh, you really brought something sweet....";
		mes "Thank you so much. I don't know how we'll ever repay your kindness.";
		erasequest 16924;
		close2;
		cutin "",255;
		CH1_Clinic_Quest_Check();
		end;
	}
	cutin "ch1_kid01.png",2;
	mes "[Young Patient]";
	mes "Oh? You're the " + (Sex ? "big brother" : "big sister") + " who gave me the sweets.";
	mes "Thank you. It was very delicious.";
	next;
	mes "[Young Patient]";
	mes "Yuri planned to save it....";
	mes "But it was so sweet and crispy and yummy that I ended up eating it all before I knew it.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16915) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16924) == 1";
end;
}

ch1_gef_in,105,104,7	script	Elderly Patient#rch1	1_F_ORIENT_04,{
	if ( isbegin_quest(17898) < 2 ) {
		cutin "ch1_saram08.png",2;
		mes "[Elderly Patient]";
		mes "Owh... my back. It hurts from lying down for too long.";
		mes "I'm just trying to sit up for a bit.";
		close3;
	}
	if ( isbegin_quest(16916) == 1 ) {
		cutin "ch1_saram08.png",2;
		mes "[Elderly Patient]";
		mes "Owh, my back. Are you a new volunteer?";
		mes "I've been lying down for so long my back started to hurt... so I sat up for a moment.";
		next;
		mes "[Elderly Patient]";
		mes "My grandchild keeps getting restless too, playing on the bad and I've been struggling to stop her.";
		mes "She keeps pouting, saying that she's bored, and saying that the food tastes bad.";
		next;
		mes "[Elderly Patient]";
		mes "My daughter and son-in-law are too busy with work, so I was the only on raising my grandchild...";
		mes "And somehow, we both ended up catching the Soren Syndrome.";
		next;
		mes "[Elderly Patient]";
		mes "I've live a long life, so I have no regrets even if I die now.";
		mes "But little Yuri is still so young....";
		next;
		mes "[Elderly Patient]";
		mes "I don't mind what happens to me. I just hope Yuri can be saved....";
		mes "Can you do that for me?";
		next;
		cutin "",255;
		if ( select("Both you and your grandchild will be healthy.", "I can't make a promise like that.") == 2 ) {
			cutin "ch1_saram08.png",2;
			mes "[Elderly Patient]";
			mes "That's true. It's just an old woman speaking nonsense.";
			mes "If life and death could be decided by will, our kind doctor would have saved everyone already....";
			next;
			mes "[Elderly Patient]";
			mes "Even at my age, I'm still afraid of dying.";
			mes "But I'd rather see Yuri live longer than I do....";
		} else {
			cutin "ch1_saram08.png",2;
			mes "[Elderly Patient]";
			mes "Thank you for saying that....";
			mes "But if only one of us can be saved... please save Yuri.";
			next;
			mes "[Elderly Patient]";
			mes "Of course, nothing would be better than both of us getting well....";
			mes "But things don't always go the way you hope....";
		}
		next;
		mes "[Elderly Patient]";
		mes "Do you know what happen to those die with Soren Syndrome?";
		mes "First is they cremate them because it's contagious.";
		next;
		mes "[Elderly Patient]";
		mes "But there was a case in the gypsy city, where they cremated someone and the smoke spread the disease terribly.";
		mes "Eventually, everyone in the city got infected and the whole place was lost.";
		next;
		mes "[Elderly Patient]";
		mes "Someone who barely escaped came here and told us what happened, so we had to stop doing cremations.";
		mes "Now we bury the bodies very far away from city, only the doctors and few others do the burials.";
		next;
		mes "[Elderly Patient]";
		mes "With cremation, I atleast thought that the wind or water would carry us to our loved ones, my children and grandchildrens.";
		mes "But now that's out of the option.";
		next;
		mes "[Elderly Patient]";
		mes "Thinking about it, even after death, I'll be buried cold and alone somewhere faraway....";
		mes "Sorry for taking your time even though you're busy.";
		next;
		emotion ET_THINK, getnpcid(0,"Elderly Patient#rch1");
		mes "[Elderly Patient]";
		mes ".......";
		next;
		mes "[Elderly Patient]";
		mes "Oh, what am I even saying. I've made you listen to an old woman's rambling sorrows.";
		mes "I already took too much of your time, you must be busy.";
		next;
		mes "[Elderly Patient]";
		mes "Thank you for keeping me accompany.";
		mes "I won't forget your kindness....";
		erasequest 16916;
		close2;
		cutin "",255;
		CH1_Clinic_Quest_Check();
		end;
	}
	cutin "ch1_saram08.png",2;
	mes "[Elderly Patient]";
	mes "Yuri still has so many days ahead... I just hope she recovers soon.";
	mes "Lately, I've been so worried I can hardly sleep at night....";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16916) == 1";
end;
}


ch1_gef_in,28,162,0	script	#rch1_hw05	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16925) == 1 ) {
		cloaknpc("Einhar#rch1_04", false, getcharid(0));
		cloaknpc("Gregor#rch1_04", false, getcharid(0));
	}
end;
}

ch1_gef_in,31,176,7	script(CLOAKED)	Einhar#rch1_04	4_4JOB_EINHAR,{
	if ( isbegin_quest(16925) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "This place.... It might be better to let Gregor do the talking instead of me.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "I'm also an acolyte after all. I think there might be things that people are more willing to open up about when someone isn't one.";
		close3;
	}
	end;
}

ch1_gef_in,29,174,7	script(CLOAKED)	Gregor#rch1_04	4259,{
	if ( isbegin_quest(16925) == 1 ) {
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Why am I the one to talk to the patients?";
		mes "Einhar, you're the acolyte here.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Precisely, it's because I'm also an acolyte.";
		mes "I want to hear things from a different perspective.";
		next;
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "Hm... that kind of attitude is also important in researches.";
		mes "Fine, excuse me. Are you a patient here?";
		next;
		cutin "ch1_sick02.png",0;
		mes "[Bedridden Patient]";
		mes "Yes, Hello?";
		mes "I overheard while you were talking, sounds like one of you is an acolyte and you are not.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "That's right. He wields the staff of the gods, while I wield the staff of biochemistry.";
		mes "May I ask why are you receiving treatment here instead of the clinic?";
		next;
		cutin "ch1_sick02.png",0;
		mes "[Bedridden Patient]";
		mes "The acolytes here cares for us gently and treat us warmly.";
		mes "At the clinic... the smell of medicine gave me terrible headaches.";
		next;
		mes "[Bedridden Patient]";
		mes "Whether I get treated here or there....";
		mes "I believe in the end that my life would be in the hands of the gods.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Is that so. Well, since there's no cure yet, I suppose that's a fair point.";
		next;
		cutin "ch1_sick02.png",0;
		mes "[Bedridden Patient]";
		mes "If I'm going to be treated I'd rather be be treated in a peaceful place.";
		mes "There are daily prayer sessions here. When the acolytes bless us with their divine powers, my coughing eases quite a bit.";
		next;
		erasequest 16925;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		if ( isbegin_quest(16926) == 0 ) {
			mes "Now that we've visited all the clinics, I think it's time to finally regroup.";
			mes "Shall we return to the <NAVI>[city hall]<INFO>ch1_gef_in.gat,179,193,0,101,0</INFO></NAVI>?";
			setquest 16927;
		} else {
			mes "I understand how they feel.";
			mes "Shall we go visit the <NAVI>[mage's clinic]<INFO>ch1_gef_in.gat,70,170,0,101,0</INFO></NAVI> as well?";
		}
		close2;
		cutin "",255;
		cloaknpc("Einhar#rch1_04", true, getcharid(0));
		cloaknpc("Gregor#rch1_04", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16925) == 1";
end;
}

ch1_gef_in,69,164,0	script	#rch1_hw06	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16926) == 1 ) {
		cloaknpc("Einhar#rch1_05", false, getcharid(0));
		cloaknpc("Gregor#rch1_05", false, getcharid(0));
	}
end;
}

ch1_gef_in,67,169,7	script(CLOAKED)	Gregor#rch1_05	4259,{
	if ( isbegin_quest(16926) == 1 ) {
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "I'm much better at doing experiments than investigations.";
		mes "Einhar should do the talking.";
		close3;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_gef_in,70,170,1	script(CLOAKED)	Einhar#rch1_05	4_4JOB_EINHAR,{
	if ( isbegin_quest(16926) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "I'm an acolyte so I don't know much about magic....";
		mes "But I can atleast try listening to their stories.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Well. I think... they'd be hesistant to share if we all at once talk to them.";
		mes "We'll let you handle this.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Excuse me.";
		mes "Are you feeling any better?";
		next;
		cutin "ch1_sick01.png",2;
		emotion ET_ANGER, getnpcid(0,"Displeased Patient#rch1_01");
		mes "[Displeased Patient]";
		mes "I'm not getting any better. I've been coughing all day and I feel like I'm dying.";
		mes "Who are you? You might catch the disease if you come to a place like this.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Well, we are scholars... we are researching a cure for the Soren Syndrome.";
		mes "Why are you here in the mage's clinic instead of the doctor's clinic?";
		next;
		cutin "ch1_sick01.png",2;
		mes "[Displeased Patient]";
		mes "I wouldn't act like this if I wanted to be here! I wanted to go to the doctor's clinic too.";
		mes "But they said there were too many patients and not available beds....";
		next;
		mes "[Displeased Patient]";
		mes "if I stayed at home, I might pass it on to my family.";
		mes "That's why I'm here temporarily. I'll go to the doctor's clinic once it opens up.";
		next;
		emotion ET_SWEAT, getnpcid(0,"Displeased Patient#rch1_01");
		mes "[Displeased Patient]";
		mes "Oh, just to be clear, I'm not saying that the mages here are incapable.";
		mes "They are working really hard on treating and researching it with magic.......";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Were there any improvements?";
		next;
		cutin "ch1_sick01.png",2;
		mes "[Displeased Patient]";
		mes "I'm not sure. But after receiving the treatment, my body does feel a little bit better.";
		mes "...Honestly, I'm thankful to them. I guess I should stop complaining about it.";
		next;
		erasequest 16926;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		if ( isbegin_quest(16925) == 0 ) {
			mes "Now that we've visited all the clinics, I think it's time to finally regroup.";
			mes "Shall we return to the <NAVI>[city hall]<INFO>ch1_gef_in.gat,179,193,0,101,0</INFO></NAVI>?";
			setquest 16927;
		} else {
			mes "I understand how you feel.";
			mes "Then, shall we visit the <NAVI>[Acolyte's Clinic]<INFO>ch1_gef_in.gat,29,174,0,101,0</INFO></NAVI> as well?";
		}
		close2;
		cutin "",255;
		cloaknpc("Einhar#rch1_05", true, getcharid(0));
		cloaknpc("Gregor#rch1_05", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16926) == 1";
end;
}

ch1_gef_in,182,182,0	script	#rch1_hw07	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( 
	    isbegin_quest(16927) == 1 || 
		isbegin_quest(16934) == 1 ||
		isbegin_quest(16928) == 1 || 
		isbegin_quest(16929) == 1 || 
		isbegin_quest(16930) == 1 ||
		isbegin_quest(16931) == 1 ||
		isbegin_quest(16932) == 1 ||
		isbegin_quest(16933) == 1
	) {
		cloaknpc("Einhar#rch1_06", false, getcharid(0));
		cloaknpc("Gregor#rch1_06", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17919) == 1 || isbegin_quest(17920) == 1 ) {
		cloaknpc("Gregor#c01ms40", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms40", false, getcharid(0));
		cloaknpc("Einhar#c01ms40", false, getcharid(0));
		cloaknpc("Charles#c01ms40", false, getcharid(0));
		end;
	}
	if ( 
		isbegin_quest(23271) == 1 || 
		isbegin_quest(23275) == 1 || 
		isbegin_quest(23290) == 1 || 
		isbegin_quest(23298) == 1 
	) {
		cloaknpc("Maxim#ch1_t_maxim1", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(23293) == 1 ) {
		cloaknpc("Maxim's Journals#ch1", false, getcharid(0));
		end;	
	}
end;
}

ch1_gef_in,175,187,3	script(CLOAKED)	Maxim's Journals#ch1	4_POINT_WHITE,{
	if ( isbegin_quest(23293) == 1 ) {
		mes "Several journals with Maxim's handwriting are laid out.";
		mes "I need to find the one that with the records of Margot's condition.";
		while ( true ) {
			next;
			switch( select("Black Journal", "Purple Journal", "Small Journal") ) {
				case 1:
					mes "You picked up the Black journal.";
					mes "It contains notes on symptoms like fever and coughing. Is this the right journal?";
					next;
					mes "Several names are listed, but Margot's name is not in it.";
					mes "This doesn’t seem to be the right journal.";
					next;
					mes "There must be a journal somewhere where Margot's records is written...";
					break;
				case 2:
					mes "You picked up the Purple journal.";
					mes "It contains information about running a night arena.";
					next;
					mes "There must be a journal somewhere where Margot's records is written...";
					break;
				case 3:
					mes "You picked up the small journal.";
					mes "Margot's condition is written down at regular intervals.";
					next;
					mes "You've found the notebook Maxim asked for. Let's bring it back to <NAVI>[Maxim]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI>.";
					erasequest 23293;
					setquest 23294;
					close;
			}
		}
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23293) == 1";
end;
}

ch1_gef_in,183,194,5	script(CLOAKED)	Einhar#rch1_06	4_4JOB_EINHAR,{
	if ( isbegin_quest(16927) == 1 ) {
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Gregor seems to be gleaming with excitement for a while now.";
		mes "You can see it even from here... honestly, it's a bit overwhelming.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Anyways, you should try talking to Gregor.";
		mes "It seems like he has a favor to ask....";
		close3;
	}
	if ( isbegin_quest(16934) == 1 ) {
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "I was just running some small errands while waiting...";
		mes "Please ask Gregor for the results.";
		close3;
	}
	if ( isbegin_quest(16931) == 0 && isbegin_quest(16932) == 0 && isbegin_quest(16933) == 0 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Have you collected all the blood samples?";
		close3;
	}
	if ( 
		isbegin_quest(16931) == 2 && isbegin_quest(16932) == 2 && isbegin_quest(16933) == 1 ||
		isbegin_quest(16931) == 1 && isbegin_quest(16932) == 2 && isbegin_quest(16933) == 2 ||
		isbegin_quest(16931) == 2 && isbegin_quest(16932) == 1 && isbegin_quest(16933) == 2
	) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "We've secured all the necessary samples. Gregor will be pleased.";
		mes "I'll let him know. You can take a short break and we'll check the results afterwards.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "He's moving his hands at incredible speed right now....";
		mes "The results will be just a matter of time.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "After a while, please ask <NAVI>[Gregor]<INFO>ch1_gef_in.gat,179,193,0,101,0</INFO></NAVI> about the results.";
		mes "I'll just listen to him as well.";
		erasequest 16931;
		erasequest 16932;
		erasequest 16933;
		setquest 16934;
		ch1_blood_lq = 0;
		close3;
	}
	if ( isbegin_quest(16931) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Do all the infected people have something in common?";
		mes "Let's compare it from the blood samples that we brought. Thank you for bringing this.";
		completequest 16931;
		ch1_blood_lq = 16931;
		close3;
	}
	if ( isbegin_quest(16932) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "There's definitely something present in the uninfected persons that wasn't found on the patients....";
		mes "I wonder how it'll compare with the recovered patient? Thank you for collecting this.";
		completequest 16932;
		ch1_blood_lq = 16932;
		close3;
	}
	if ( isbegin_quest(16933) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "I'm glad there are patients who've fully recovered. Both their recovery and the progress in research are good signs.";
		mes "Gregor is completely absorbed in his work right now. I'll thank you on his behalf. You've done well.";
		completequest 16933;
		ch1_blood_lq = 16933;
		close3;
	}
	switch( ch1_blood_lq ) {
		case 16931:
			cutin "4job_einhar_02.png",2;
			mes "[Einhar]";
			mes "Do all the infected people have something in common?";
			mes "Let's compare it from the blood samples that we brought. Thank you for bringing yours.";
			close3;
		case 16932:
			cutin "4job_einhar_02.png",2;
			mes "[Einhar]";
			mes "There's definitely something present in the uninfected persons that wasn't found on the patients....";
			mes "I wonder how it'll compare with the recovered patient? Thank you for collecting this.";
			close3;
		case 16933:
			cutin "4job_einhar_02.png",2;
			mes "[Einhar]";
			mes "I'm glad there are patients who've fully recovered. Both their recovery and the progress in research are good signs.";
			mes "Gregor is completely absorbed in his work right now. I'll thank you on his behalf. You've done well.";
			close3;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16931) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16932) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16933) == 1";
end;
}

ch1_gef_in,179,193,7	script(CLOAKED)	Gregor#rch1_06	4259,{
	if ( isbegin_quest(16927) == 1 ) {
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Alright, I started to feel like we are detectives considering the amount of people we talked to.";
		mes "I'd say that we've done enough investigation. Now, it's time for biochemistry to take over.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Doctor Neumann's Clinic, acolyte's clinic, and mage's clinic....";
		mes "On the surface, They didn't seem to have any major differences in the patients' recovery.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "They said receiving blessings with divine power or treatment with magic made them feel a little bit better.";
		mes "Does it really have an effect?";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Doctor Neumann's clinic also have recovering patients.";
		mes "It's possible that the divine blessings and magic treatments are just placebos or it can be also that it truly has an effect.";
		next;
		cutin "4job_gregor_04.png",1;
		mes "[Gregor]";
		mes "The problem is if they do have an effect, that doesn't mean it will lead to the cure.";
		mes "We need to prove it with data and samples.";
		next;
		cutin "",255;
		select("Ask what his plan is.");
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "We need to collect blood samples from the patients";
		mes "A sample each from Doctor Neumann's clinic, acolyte's clinic, and the mage's clinic.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "For control, we'll also need sample from someone who's never been infected... and another sample from someone who was infected but recovered completely.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Hm? Wouldn't someone who recovered completely be the same as someone who haven't been infected?";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "No, not really. If you recover from a disease, your blood will retain signs that you overcame it.";
		mes "It's the system of the body where they remember the disease and prevent it from returning.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "That's the main reason we also need a sample from some who recovered .";
		mes "Okay... Since I've already talked to someone at the acolyte's clinic, I'll go there.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Then, I'll go ask the person I spoke to the mage's clinic if I can collect a sample.";
		mes "What about the rest?";
		next;
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "Einhar, I'll need you to assist me with my research after.";
		mes "Can you handle the rest, "+strcharinfo(0)+"?";
		next;
		cutin "",255;
		if ( select("Why am I stuck with all of the errands!", "I'll go.") == 1 ) {
			cutin "4job_gregor_03.png",1;
			mes "[Gregor]";
			mes "Oh, so you think collecting blood samples is just an errand? Do you really think that?";
			next;
			cutin "4job_einhar_01.png",2;
			emotion ET_PROFUSELY_SWAT, getnpcid(0,"Einhar#rch1_06");
			mes "[Einhar]";
			mes "It's important.......";
			next;
			cutin "",255;
			select("I'll go right away.");
		}
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "A quick decision. I like that mindset.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Earlier I heard at Doctor Neumann's clinic, that there was a person called <NAVI>[Dapan]<INFO>ch1_gef_in.gat,75,143,0,101,0</INFO></NAVI> who caught the disease and recovered completely.";
		mes "You should be able to visit him at his home.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Also could you stop by <NAVI>[Doctor Neumann's clinic]<INFO>ch1_gef_in.gat,81,74,0,101,0</INFO></NAVI> and ask there for a suitable sample for someone who's currently infected.";
		mes "As for someone who have never been infected... I think you could look around the <NAVI>[streets]<INFO>ch1_geffen.gat,180,147,0,101,0</INFO></NAVI>?";
		next;
		cutin "4job_gregor_04.png",1;
		mes "[Gregor]";
		mes "Then off we go. Here's a kit to collect and store blood samples.";
		mes "Try to be quick. If blood isn't stored properly, it'll spoil fast.";
		erasequest 16927;
		setquest 16928;
		setquest 16929;
		setquest 16930;
		getexp 130609489, 4000000;
		getitem "Ch1_Root_Coin",5;
		close3;
	}
	if ( isbegin_quest(16934) == 1 ) {
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Hm, It's my first time doing this kind of experiment, I didn't think this would be fun.";
		mes "We've got quite an interesting results too....";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "This isn't something to be done for fun....";
		mes "Please let us know the results.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Yes, yes, our Inquisitor friend is always so strict.";
		mes "Anyway, I looked at difference of the blood samples....";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "There is a substance present in the blood of infected patients that doesn't appear in the blood of either uninfected or recovered individuals.";
		mes "And as the disease progress, this substance multiplies.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Let's call it the infection agent. The infection agent shows a certain pattern in it's growth.";
		mes "The growth is halted by a bit when exposed to divine or magic power.";
		next;
		mes "[Gregor]";
		mes "You can see that from the samples that me and Einhar collected.";
		mes "But even when Einhar poured divine power into the blood sample... the infection agent wasn't completely destroyed.";
		next;
		cutin "4job_einhar_01.png",2;
		emotion ET_SWEAT, getnpcid(0,"Einhar#rch1_06");
		mes "[Einhar]";
		mes "You have no idea how much energy I poured in it to try and destroy it.";
		mes "Clearly, the adventurer is the lucky one for going out to collect the rest of the samples....";
		next;
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "That's exactly why I brought you with me, for stuffs like that.";
		mes "Anyways, remember when I said that a recovered person would probably have traces of it, it didn't have any traces of it.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "I thought the body would create some antibodies for it, but the infection agent just disappeared entirely.";
		mes "Well, I suppose that can happen.";
		next;
		cutin "",255;
		select("What's the conclusion?");
		cutin "4job_gregor_01.png",1;
		emotion ET_SCRATCH, getnpcid(0,"Gregor#rch1_06");
		mes "[Gregor]";
		mes "So impatient. Are you the type to skip to the end of a mystery novel just to see who did it?";
		mes "Anyway, We only tested it with Einhar's divine power....";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "But we still don't know what will happen if we combine it with magic or traditional treatment methods.";
		mes "If all the clinic work together and pool their manpower to research, we might just find a cure.";
		next;
		cutin "4job_einhar_02.png",2;
		emotion ET_SURPRISE, getnpcid(0,"Einhar#rch1_06");
		mes "[Einhar]";
		mes "I see...! You mean to combine all their knowledge and expertise.";
		mes "That's something none of the clinics might have thought of.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Yes. That's why we need to share it with everyone.";
		mes "How about we start with Doctor Neumann's clinic?";
		next;
		cutin "",255;
		select("He said that he'll stay in his house.");
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "......Oh? I guess we'll have to reach out to him tomorrow.";
		next;
		cutin "",255;
		select("He said to come there if it's urgent.");
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "This matter is pretty important....";
		mes "We might be a bit of a bother, but we need to give him a visit.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Yes, it's urgent. People are still dying even now.";
		mes "Then, let's go to <NAVI>[Doctor Neumann's house]<INFO>ch1_geffen.gat,176,66,0,101,0</INFO></NAVI>.";
		erasequest 16934;
		setquest 16936;
		close2;
		cutin "",255;
		cloaknpc("Einhar#rch1_06", true, getcharid(0));
		cloaknpc("Gregor#rch1_06", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(16931) == 0 && isbegin_quest(16932) == 0 && isbegin_quest(16933) == 0 ) {
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Have you taken the blood samples?";
		close3;
	}
	if ( 
		isbegin_quest(16931) == 2 && isbegin_quest(16932) == 2 && isbegin_quest(16933) == 1 ||
		isbegin_quest(16931) == 1 && isbegin_quest(16932) == 2 && isbegin_quest(16933) == 2 ||
		isbegin_quest(16931) == 2 && isbegin_quest(16932) == 1 && isbegin_quest(16933) == 2
	) {
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Alright, all that running around is about to pay off.";
		mes "I still need to analyze the last blood sample you've brought, so could you wait just a moment?";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "I'll check it and let you know. Take a quick rest.";
		mes "<NAVI>[I]<INFO>ch1_gef_in.gat,179,193,0,101,0</INFO></NAVI> will wrap this up quickly and let you know right away.";
		setquest 16934;
		close3;
	}
	if ( isbegin_quest(16931) == 1 ) {
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Ah, just put it over there. Einhar and I also brought the samples that we got from the patient.";
		mes "We need to check if there's something common among it. Please go get the rest of the samples.";
		completequest 16931;
		ch1_blood_lq = 16931;
		close3;
	}
	if ( isbegin_quest(16932) == 1 ) {
		cutin "4job_gregor_04.png",1;
		mes "[Gregor]";
		mes "I see... There is something on the patients' blood samples that isn't in the uninfected person.";
		mes "I'll have to investigate this more.";
		completequest 16932;
		ch1_blood_lq = 16932;
		close3;
	}
	if ( isbegin_quest(16933) == 1 ) {
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "It's time to prove our hypothesis. Let's check if there's a difference from the blood of someone who recovered.";
		mes "You did well collecting the sample. We'll take it from here.";
		completequest 16933;
		ch1_blood_lq = 16933;
		close3;
	}
	switch ( ch1_blood_lq ) {
		case 16931:
			cutin "4job_gregor_01.png",1;
			mes "[Gregor]";
			mes "Ah, just put it over there. Einhar and I also brought the samples that we got from the patient.";
			mes "We need to check if there's something common among it. Please go get the rest of the samples.";
			close3;
		case 16932:
			cutin "4job_gregor_04.png",1;
			mes "[Gregor]";
			mes "I see... There is something on the patients' blood samples that isn't in the uninfected person.";
			mes "I'll have to investigate this more.";
			close3;
		case 16933:
			cutin "4job_gregor_01.png",1;
			mes "[Gregor]";
			mes "It's time to prove our hypothesis. Let's check if there's a difference from the blood of someone who recovered.";
			mes "You did well collecting the sample. We'll take it from here.";
			close3;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16927) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16931) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16932) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16933) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16934) == 1";
end;
}

ch1_gef_in,173,186,7	script(CLOAKED)	Einhar#c01ms40	4_4JOB_EINHAR,{
	if ( isbegin_quest(17919) == 1 ) {
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Ah! Perfect timing.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "They're back, they're back. Where have you been and why it took you so long?";
		mes "We've been investigating what we can here, taking the infected to the clinic, and learning what we can about the plague.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "The disease is truly vicious. No conventional treatments have shown any progress. All we can do is slow it down.";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "That sounds terrible. This might help you.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "We found the source under the Geffen Tower. That's where they're coming from. The same one who attacked our dimension before.";
		npctalk "So they came here too? Did you into any of them?", "Einhar#c01ms40", BC_SELF;
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "This is a living cell... no, it's spores? It looks like a spore, isn't it? Was there a lab down there?";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "It's a tactical base, biochemical weapon factory, research lab, and also has living quarters... basically, it's an all-in-one facility.";
		next;
		select("It came from the laboratory.");
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "I sense an evil aura from it. This thing is a sinful existence...";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Yes. That aura was what drew us there in the first place. It's an evil thing that transcends human decency.";
		next;
		cutin "4job_gregor_04.png",1;
		mes "[Gregor]";
		mes "I'll take a look into this spores. Leave them to me.";
		next;
		specialeffect EF_HIT1, AREA, "Gregor#c01ms40";
		cloaknpc("Gregor#c01ms40", true, getcharid(0));
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "How is the situation here? I'm curious about what you've found out and the outcome of the disease.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "The situation is not good. It starts with a respiratory disease, then the skin starts to crack, and it gradually spread throughout the body. Once it reaches that stage, they seem to isolate the infected somewhere outside the city.";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Do you know what happens to those who are quarantined?";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Not at all. We haven't been able to confirm that.";
		npctalk "You won't guess what we saw down there-", "Charles#c01ms40", BC_SELF;
		next;
		specialeffect EF_HIT1, AREA, "Gregor#c01ms40";
		cloaknpc("Gregor#c01ms40", false, getcharid(0));
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "This!!! It's identical! The samples you've brought match exactly with what's in the patients' blood! Even the living cell! Whom did you take this cell again?";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "The spores in the blood roam the host's body and gradually mutate, this one looks like the final stage of the mutation, am I right? But it's quite different from the patients' cells, no?";
		next;
		cutin "ex_sa_masterj02.png",2;
		mes "[Ascetic Jeon]";
		mes "I was about to bring that up.";
		erasequest 17919;
		setquest 17920;
		close3;
	}
	npctalk "You've done far more than expected. Thank you for your hard work.", "Einhar#c01ms40", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17919) == 1";
end;
}

ch1_gef_in,175,188,3	script(CLOAKED)	Ascetic Jeon#c01ms40	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(17919) == 1 ) {
		npctalk "It would be better to explain the sitation to them.", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17920) == 1 ) {
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "There was a place that looked like Verus underneath the Geffen Tower.";
		mes "The people who entered the place and the mages who also entered to deal with the monster invasion became obsessed with the ancient civilization there.";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "The people who worships the civilization down there told us an interesting story. They said those who are blessed will stop breathing and will be reborn as a new human.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Blessing?? Is that what I think it is...?";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "Yes. People who got infected with the disease will die and will be reborn as an Immortal Corps.";
		next;
		emotion ET_SURPRISE, getnpcid(0,"Gregor#c01ms40");
		emotion ET_SURPRISE, getnpcid(0,"Einhar#c01ms40");
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "That explains a lot of things.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Yes. It explains why there are those Immortal Corps like things showing up in the city. We were wondering where it's coming from since Geffen is sealed off.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "The rumors in city about the Geffen Tower. People in city believes that the tower is the source of the plague.";
		mes "But it was just a misunderstanding, the invaders released the spores at the same time they attacked the tower.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "To sum it up, the invaders released the spores, and those who get infected eventually becomes an Immortal Corps. Was that a fair conclusion?";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "A logical conclusion. We need to device a plan accordingly.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "If everyone who dies from the disease turns into an Immortal Corps, then won't their numbers only go up from here?";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "The plague is not divine will, nor it's not nature's cruelty. It's an artifical weapon made by Himmelmez and her army.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "Before anything else, we should name that thing. We should call it something reasonable.";
		mes "Hm... ^0000cdParasitic Spore^000000. Since it parasites on humans and forcefully mutate them.";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "It fits the description. Now we need to find a way to remove this Parasitic Spore from the human body. Let's share the methods we've tried so far.";
		next;
		cutin "ex_sa_masterj02.png",2;
		mes "[Ascetic Jeon]";
		mes "Fortunately, We got an acolyte, a biochemist, mages, and an ascetic here. With all these different fields, we can try various approaches.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Before that, we need share the existence of the Parasitic Spore with the doctors here. As a doctor, they might know something and come up with a countermeasures.";
		next;
		select("I'll deliver the news.");
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Okay, we'll count on you.";
		completequest 17920;
		setquest 17921;
		getitem "Ch1_Root_Coin",5;
		getexp 97957116, 2000000;
		close3;
	}
	if ( isbegin_quest(17921) == 1 ) {
		npctalk "Explain everything to the doctors in detail. If they still don't understand, it's best to bring them here.", "", BC_SELF;
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17920) == 1";
end;
}

ch1_gef_in,172,188,7	script(CLOAKED)	Charles#c01ms40	4_M_JOB_WIZARD,{
	npctalk "We should explain it to the others.", "", BC_SELF;
	end;
}

ch1_gef_in,175,190,3	script(CLOAKED)	Gregor#c01ms40	4259,{
	npctalk "Hey! You did well!", "", BC_SELF;
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_geffen,180,147,7	script	Aire#rch1_01	4_F_HUWOMAN,{
	if ( isbegin_quest(16929) == 1 ) {
		cutin "tuto_michaela.png",2;
		emotion ET_QUESTION, getnpcid(0,"Aire#rch1_01");
		mes "[Aire]";
		mes "......Are you alright?";
		next;
		cutin "tuto_michaela02.png",2;
		mes "[Aire]";
		mes "Ah, you just looked so troubled that I spoke to you without thinking.";
		mes "Can I help you with anything?";
		next;
		cutin "",255;
		if ( select("B-blood... I need blood.", "Do you mind giving a blood sample for research?") == 1 ) {
			cutin "tuto_michaela02.png",2;
			emotion ET_HUK, getnpcid(0,"Aire#rch1_01");
			mes "[Aire]";
			mes "......Kyaa!";
			mes "What are you saying? Asking for blood like that?";
		} else {
			cutin "tuto_michaela.png",2;
			emotion ET_QUESTION, getnpcid(0,"Aire#rch1_01");
			mes "[Aire]";
			mes "Oh, but I'm not infected or anything. What do you mean for research?";
		}
		next;
		cutin "",255;
		select("Explain the situation.......");
		cutin "tuto_michaela.png",2;
		mes "[Aire]";
		mes "Ah, so you need a blood of a non-infected person.";
		next;
		cutin "tuto_michaela.png",2;
		mes "[Aire]";
		mes "Okay. I don't mind doing a little sting if it will help cure Soren Syndrome....";
		next;
		cutin "tuto_michaela02.png",2;
		mes "[Aire]";
		mes "I'll try to endure it even if it's a big needle.";
		mes "......though I would prefer a smaller one.";
		next;
		cutin "",255;
		select("Draw blood using the kit.");
		cutin "tuto_michaela02.png",2;
		emotion ET_CRY, getnpcid(0,"Aire#rch1_01");
		mes "[Aire]";
		mes "Somehow... the kit looks scary to look at....";
		next;
		cutin "tuto_michaela.png",2;
		mes "[Aire]";
		mes "Phew... is it done?";
		mes "I hope that helps!";
		pctalk "I should take back the sample to the city hall.";
		erasequest 16929;
		setquest 16932;
		close3;
	}
	cutin "tuto_michaela02.png",2;
	mes "[Aire]";
	mes "I really hope a cure for Soren Syndrome will have a cure soon.";
	mes "Losing friends and relatives are just too heartbreaking.......";
	next;
	mes "[Aire]";
	mes "The thought of me suddenly dying too scares me.";
	mes "Just thinking about it drains all my energy....";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16929) == 1";
end;
}

ch1_gef_in,75,143,3	script	Dapan#rch1_01	4_M_HUMAN_01,{
	if ( isbegin_quest(16930) == 1 ) {
		cutin "ch1_saram06a.png",0;
		mes "[Dapan]";
		mes "Hm, who are you? What brings you here?";
		mes "I was about to rest down for a bit.";
		next;
		cutin "",255;
		select("Explain about the blood samples.");
		cutin "ch1_saram06a.png",0;
		mes "[Dapan]";
		mes "Ah, you want to study my blood since I had Soren Syndrome and recovered.";
		mes "I don't mind giving some if it's for that, I'll roll up my sleeve.";
		next;
		mes "[Dapan]";
		mes "Here, take as much as you like.";
		mes "Ever since I recovered, I felt like having a second life... like I was given an extra life.";
		next;
		mes "[Dapan]";
		mes "That's why I plan to live a good life and do a lot more good.";
		next;
		cutin "",255;
		select("Draw his blood.");
		cutin "ch1_saram06a.png",0;
		emotion ET_KEK, getnpcid(0,"Dapan#rch1_01");
		mes "[Dapan]";
		mes "......Ugh, What a strange feeling. I feel kind off chilly.";
		mes "I hope this helps the research for the cure.";
		next;
		mes "[Dapan]";
		mes "Being alive is such a wonderful, a truly wonderful thing....";
		mes "I hope the others who are sick can also recover and leave their clinic beds safely.";
		pctalk "I should take back the sample to the city hall.";
		erasequest 16930;
		setquest 16933;
		close3;
	}
	cutin "ch1_saram06a.png",0;
	mes "[Dapan]";
	mes "If there's anything I can do to help with Soren Syndrome. I'll gladly do it.";
	mes "I sometimes help out at the clinic too. I can't go often since I'm still weak after recovering....";
	next;
	mes "[Dapan]";
	mes "But I try to do as much as I can.";
	mes "I really hope others can recover too.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16930) == 1";
end;
}

ch1_geffen,176,66,0	script	#rch1_hw08	HIDDEN_WARP_NPC,4,4,{
	end;
	
OnTouch:
	if ( isbegin_quest(16936) == 1 ) {
		cloaknpc("Einhar#rch1_07", false, getcharid(0));
		cloaknpc("Gregor#rch1_07", false, getcharid(0));
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "......Wait a moment.";
		mes "I hear something from inside the house.";
		next;
		cutin "4job_gregor_01.png",1;
		emotion ET_QUESTION, getnpcid(0,"Gregor#rch1_07");
		mes "[Gregor]";
		mes "What do you mean?";
		mes "I don't hear anything.";
		next;
		cutin "",255;
		mes "[????]";
		mes "...This...! How do I long do I have to keep this up...!";
		next;
		select("I can hear someone shouting.");
		cutin "4job_gregor_02.png",1;
		emotion ET_SWEAT, getnpcid(0,"Gregor#rch1_07");
		mes "[Gregor]";
		mes "Yeah... I can hear it now.";
		mes "Though it's hard to make out exactly what they're saying.";
		next;
		cutin "",255;
		mes "[????]";
		mes "How long do we have to keep doing this damn thing...! Is there even really a cure...?";
		next;
		mes "[????]";
		mes "Hey, calm down..., someone might hear us from outside...!";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Something is definitely not right.";
		next;
		cutin "",255;
		mes "[????]";
		mes "What does it matter even if we're to be found out! I can't go on living like we're innocent!";
		mes "Maybe you can, but I can't!";
		next;
		mes "[????]";
		mes "I'm going to tell everyone! I'm going to tell all of them the horrible things that we've done... I'll do it right now!";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Someone's coming to the door. They'll come out any second now.......";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Let's hide. It seems like a bad idea to be seen now.";
		cloaknpc("Einhar#rch1_07", true, getcharid(0));
		cloaknpc("Gregor#rch1_07", true, getcharid(0));
		specialeffect2 EF_CLOAKING3;
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "I agree. Let's hide for a moment.";
		cloaknpc("Henry Bauer#rch1_01", false, getcharid(0));
		next;
		cutin "",255;
		emotion ET_ANGER, getnpcid(0,"Henry Bauer#rch1_01");
		mes "[Henry Bauer]";
		mes "For something like this, there's no way that divine punishment won't come for us!";
		mes "I believed you when you said that you'd find a cure for Becky.";
		next;
		cutin "ch1_neumann04.png",2;
		cloaknpc("Doctor Neumann#rch1_03", false, getcharid(0));
		mes "[Doctor Neumann]";
		mes "Huhh, huff... I told you to wait a moment...! I'm researching how to cure her!";
		next;
		cutin "",255;
		mes "[Henry Bauer]";
		mes "How long am I suppose to wait? Until my child dies?";
		mes "I've dirtied my hands and nothing has changed, Becky is still dying!";
		next;
		cutin "ch1_neumann02.png",2;
		mes "[Doctor Neumann]";
		mes "Calm down, calm down... Henry. I understand how you feel.";
		mes "I'm doing everything I can. So please....";
		next;
		cutin "",255;
		mes "[Henry Bauer]";
		mes "There's nothing you can do to make me believe that!";
		mes "I'm not asking for anything else.... I just want my child to be saved!";
		next;
		emotion ET_FRET, getnpcid(0,"Henry Bauer#rch1_01");
		mes "[Henry Bauer]";
		mes "I'm tired of those slick excuses!";
		mes "I'm going to tell it... I'll tell the truth!";
		next;
		cutin "ch1_neumann03.png",2;
		emotion ET_HUK, getnpcid(0,"Doctor Neumann#rch1_03");
		cloaknpc("Henry Bauer#rch1_01", true, getcharid(0));
		mes "[Doctor Neumann]";
		mes "Please, wait... hey, what are you thinking!";
		mes "Wait! I said wait!";
		next;
		cutin "4job_gregor_02.png",1;
		cloaknpc("Doctor Neumann#rch1_03", true, getcharid(0));
		mes "[Gregor]";
		mes "......and they're gone.";
		mes "They were talking about something serious.";
		next;
		cutin "4job_einhar_01.png",2;
		cloaknpc("Einhar#rch1_07", false, getcharid(0));
		cloaknpc("Gregor#rch1_07", false, getcharid(0));
		mes "[Einhar]";
		mes "Divine punishment, dirtied his hands... what does he mean by that?";
		mes "As I've said, something is definitely not right.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Yes. Both of them are gone now.";
		mes "Einhar, do you sense anyone else inside?";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "......I'm not sure, but I don't think there's anyone inside.";
		next;
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "Then, let's go in.";
		mes "Their conversation was something suspicious, right? Let's go in and investigate.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Things like that...! Let's not do it!";
		mes "It would be trespassing!";
		next;
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "We might not get another chance.";
		mes "This may be our only opportunity to find what they're hiding.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Einhar, this might not be morally right, but it's the right thing to do.";
		mes "I'll take responsiblity if something goes wrong.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "......You don't have to go that far.";
		mes "Let's head in. I don't plan on letting you take the blame alone.";
		next;
		mes "[Einhar]";
		mes "Is that all right for you, "+strcharinfo(0)+"?";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "In times like these, even if it's not you should say it is.";
		mes "Let's go <NAVI>[inside]<INFO>ch1_gef_in.gat,111,169,0,101,0</INFO></NAVI>.";
		erasequest 16936;
		setquest 16937;
		close2;
		cutin "",255;
		cloaknpc("Einhar#rch1_07", true, getcharid(0));
		cloaknpc("Gregor#rch1_07", true, getcharid(0));
		end;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16936) == 1";
end;
}

ch1_geffen,180,66,3	script(CLOAKED)	Gregor#rch1_07	4259,{
	cutin "4job_gregor_01.png",1;
	mes "[Gregor]";
	mes "I think we should stay here for now....";
	close3;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_geffen,178,65,5	script(CLOAKED)	Einhar#rch1_07	4_4JOB_EINHAR,{
	cutin "4job_einhar_01.png",2;
	mes "[Einhar]";
	mes "Shh! I can hear something.";
	close3;
}

ch1_geffen,177,58,7	script(CLOAKED)	Doctor Neumann#rch1_03	4_CH1_NEUMANN,{
	cutin "ch1_neumann03.png",2;
	mes "[Doctor Neumann]";
	mes "Hm, not now. I don't want to talk about it!";
	close3;
}

ch1_geffen,178,61,1	script(CLOAKED)	Henry Bauer#rch1_01	4_M_MASKMAN,{
	mes "[Henry Bauer]";
	mes "Don't come near me!";
	close;
}

ch1_gef_in,106,175,0	script	#rch1_hw09	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16937) == 1 ) {
		cloaknpc("Einhar#rch1_08", false, getcharid(0));
		cloaknpc("Gregor#rch1_08", false, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,101,174,5	script(CLOAKED)	Einhar#rch1_08	4_4JOB_EINHAR,{
	if ( isbegin_quest(16937) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Hmm? Gregor's face... he looks like a cat staring at a toy.";
		mes "I think he found something.";
		close3;
	}
}

ch1_gef_in,111,169,7	script(CLOAKED)	Gregor#rch1_08	4259,{
	if ( isbegin_quest(16937) == 1 ) {
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Hm? There's a journal here.";
		mes "Let's see... It's a research about the compulsive eating symptoms.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Compulsive eating... that's the symptom some patients experienced.";
		mes "The people who keeps looking for food, yet keep losing weight.";
		next;
		cutin "",255;
		select("That's what I've heard.");
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "......Hm? 'Research on methods to induce the compulsive eating state on a patient'...?";
		mes "The compulsive eating state, this isn't something good.";
		next;
		cutin "",255;
		select("Those patients don't last long.");
		cutin "4job_einhar_01.png",2;
		emotion ET_THINK, getnpcid(0,"Einhar#rch1_08");
		mes "[Einhar]";
		mes ".......";
		mes "Dirtied his hands... is this what he meant.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "There's nothing to gain from doing that.";
		mes "Doctor Neumann did seem like he was genuinely searching for a cure.";
		next;
		cutin "4job_gregor_04.png",1;
		mes "[Gregor]";
		mes "We don't have enough clues.";
		mes "Let's keep looking. Shall we go <NAVI>[further inside]<INFO>ch1_gef_in.gat,144,171,0,101,0</INFO></NAVI>?";
		erasequest 16937;
		setquest 16938;
		close2;
		cutin "",255;
		cloaknpc("Einhar#rch1_09", false, getcharid(0));
		cloaknpc("Gregor#rch1_09", false, getcharid(0));
		cloaknpc("Einhar#rch1_08", true, getcharid(0));
		cloaknpc("Gregor#rch1_08", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16937) == 1";
end;
}

ch1_gef_in,140,170,0	script	#rch1_hw10	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16938) == 1 ) {
		cloaknpc("Einhar#rch1_09", false, getcharid(0));
		cloaknpc("Gregor#rch1_09", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(16939) == 1 ) {
		cloaknpc("Einhar#rch1_09", true, getcharid(0));
		cloaknpc("Gregor#rch1_09", true, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,145,170,1	script(CLOAKED)	Gregor#rch1_09	4259,{
	if ( isbegin_quest(16938) == 1 ) {
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "It's a small room, but let's still search it carefully.";
		mes "Places like this are actually better for hiding things.";
		close3;
	}
	if ( isbegin_quest(16939) == 1 ) {
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "We don't have time to pick the doors right now.";
		close3;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_gef_in,144,171,1	script(CLOAKED)	Einhar#rch1_09	4_4JOB_EINHAR,{
	if ( isbegin_quest(16938) == 1 ) {
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "The door is locked.";
		mes "It'll be hard to open without leaving a trace before they return.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Wait... I think there's someone in the room. The presence is faint, so I'm not sure....";
		mes "For now, we should stay quiet.";
		next;
		cutin "4job_gregor_01.png",1;
		emotion ET_DELIGHT, getnpcid(0,"Gregor#rch1_09");
		mes "[Gregor]";
		mes "Hm, you got used to this pretty quickly.";
		next;
		cutin "4job_einhar_01.png",2;
		emotion ET_PROFUSELY_SWAT, getnpcid(0,"Einhar#rch1_09");
		mes "[Einhar]";
		mes "No, That's not what I meant!";
		mes "......I wasn't trying to talk about picking the lock. Look at this.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "It's a book with a lock on it. It was well-hidden.";
		mes "...it might be something important.";
		next;
		mes "[Einhar]";
		mes "It also looks hard to open....";
		mes "But we can take this whole thing with us.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "There's dust built up on it. It's not something that opened often.";
		mes "We might be fine taking it.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "We'll have to think about how to put it back later....";
		mes "Depending on the contents, there might be no need to return it.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Right. You've become flexible.";
		mes "The problem is how to open it.......";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "There is a magician who came with is. Yes, someone called Professor Telcos.";
		mes "From the looks of it, it's something sealed with magic. He might know how to open it.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "...Now that I think about it. Maybe he can create a copy of this book for us.";
		mes "If we quickly copy it and return it, we might be able to put it back before Doctor Neumann return.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "I'll go to Ashridge and show him this book.";
		mes "Gregor and "+strcharinfo(0)+", how about heading to the clinic for now?";
		next;
		mes "[Einhar]";
		mes "Please stall Doctor Neumann if you see him there.";
		mes "I'll copy the book quickly and return it.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "You're adapting to this quickly. Even I'm a bit surprised. Jokes aside, it's a good idea.";
		mes "Alright, "+strcharinfo(0)+", let's go to the <NAVI>[clinic]<INFO>ch1_geffen.gat,167,173,0,101,0</INFO></NAVI>.";
		erasequest 16938;
		setquest 16939;
		getitem "Ch1_Root_Coin",5;
		getexp 130609489, 4000000;
		close3;
	}
	if ( isbegin_quest(16939) == 1 ) {
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "I'm a little concerned about that locked door....";
		close3;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16938) == 1";
end;
}

ch1_geffen,169,171,0	script	#c01ms11	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16939) == 1 ) {
		cloaknpc("Ascetic Jeon#c01ms13", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17923) == 1 ) {
		cloaknpc("Ascetic Jeon#c01ms13", false, getcharid(0));
		cloaknpc("Clover#c01ms11", false, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,172,187,3	script(CLOAKED)	Maxim#ch1_t_maxim1	4_M_RUSBALD,{
	if ( isbegin_quest(23271) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Oh, adventurer. You're back! Don't worry, things are more or less sorted out on my end, but we still have a long way to go. More importantly, have you found out anything about the Soren Syndrome?";
		next;
		select("Can you tell me about the doctor?");
		mes "[Maxim]";
		mes "After what you've experienced, it's natural to be curious about who the doctor really was. To think that someone we thought was a doctor was actually committing terrible things.";
		next;
		mes "[Maxim]";
		mes "I wish I could tell you more, but honestly, I don't know much about him beyond the fact that he was a doctor. I'm sorry.";
		next;
		mes "[Maxim]";
		mes "I was busy with other matters and spent most of my time outside Geffen, so I'm not very familiar with what was going on there. I've only run into the doctor a few times.";
		next;
		mes "[Maxim]";
		mes "Even I was shocked to learn he was a fraud and I barely know him, can you imagine how the patients he treated would feel?";
		next;
		mes "[Maxim]";
		mes "It's no exaggeration to say almost every resident in Geffen had some connection to him, people are going to be completely thrown off when this information goes public.";
		next;
		mes "[Maxim]";
		mes "Anyway, I'm sure anyone living in Geffen could tell you more about him than I can.";
		next;
		mes "[Maxim]";
		mes "Oh, I see. I get it now! That actually gave me a good idea. Sometimes talking to people gives you new ideas, just like now.";
		next;
		mes "[Maxim]";
		mes "So if you're wondering about Neumann, it would be better to ask the residents of Geffen rather than me. Most of the time clues are often hidden in everyday life.";
		next;
		mes "[Maxim]";
		mes "People might even avoid the topic once word gets out that Neumann was a fake doctor, so now's the best time to investigate.";
		next;
		mes "[Maxim]";
		mes "We can't exactly talk to Neumann himself anymore, so we don't really have any other options.";
		next;
		mes "[Maxim]";
		mes "As I recall, he had been staying in Geffen for quite a while. I bet even a non-resident would have at least one thing to say about him..";
		next;
		mes "[Maxim]";
		mes "Since you've been curious about the doctor, I'm getting curious myself. Let me know if you hear anything from the citizens.";
		erasequest 23271;
		setquest 23272;
		setquest 23273;
		setquest 23274;
		close3;
	}
	if ( isbegin_quest(23275) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Have you heard much about Neumann from the citizens? I've been wondering what sort of things you'd learn after you left.";
		next;
		mes "[Maxim]";
		mes "Wait, the citizens don't know much about Neumann either? That only makes me more curious...";
		next;
		mes "[Maxim]";
		mes "Actually, I've been thinking about doing my own investigation into Neumann ever since you brought it up. If you still want to learn more about him, I'll gladly help!";
		next;
		mes "[Maxim]";
		mes "It really is strange though. Neumann worked here as a doctor for who knows how long, and yet no one seems to know anything concrete about him.";
		next;
		mes "[Maxim]";
		mes "I worry that people may have gotten so worn down they treated his kindness like it was just something useful and forgot to give the right gratitute for it...";
		next;
		mes "[Maxim]";
		mes "Instead of speculating, wouldn't it be better to ask someone who actually knew him? Like his wife, Margot, who worked as a nurse.";
		next;
		mes "[Maxim]";
		mes "They were both so busy working as a doctor and nurse, she must know something about him since they got married.";
		next;
		mes "[Maxim]";
		mes "Now that the doctor is gone, there's no one to take care of Margot. If it weren't for you, her condition might have been neglected.";
		next;
		mes "[Maxim]";
		mes "She must be feeling lonely staying in that room alone, so why don't you listen to their love story? I've never met anyone who didn't like talking about that kind of thing!";
		next;
		mes "[Maxim]";
		mes "<NAVI>[Margot]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI> is recovering in the doctor's house. She's not in good health and only stays in her room, so she's probably lonely. You should go see her..";
		erasequest 23275;
		setquest 23276;
		close3;
	}
	if ( isbegin_quest(23276) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Have you visited Margot yet? If you're curious about what kind of person Neumann was, asking his wife Margot would be the best way to find out.";
		next;
		mes "[Maxim]";
		mes "<NAVI>[Margot]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI> is recovering in the doctor's house. She's not in good health and only stays in her room, so she's probably lonely. You should go see her..";
		close3;
	}
	if ( isbegin_quest(23290) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "If it isn't the adventurer? What brings you back to Geffen this time? Haha, you're always welcome here.";
		next;
		mes "[Maxim]";
		mes "You say you've brought a developed cure? So Margot's blood really was helpful. Truly a mysterious thing.";
		next;
		mes "[Maxim]";
		mes "I'm grateful to everyone in Ashridge who worked so hard, and I'm itching to get this cure out to the people of Geffen right away.";
		next;
		mes "[Maxim]";
		mes "But isn't this exactly the kind of moment where we need to be cautious? Most of the patients here in Geffen are already deeply disheartened by the fact that Neumann turned out to be a fake.";
		next;
		mes "[Maxim]";
		mes "Of course, I'm sure the scholars of Ashridge developed this cure using the best of their skills, but it still hasn't been tested on any patients yet, has it?";
		next;
		mes "[Maxim]";
		mes "If this doesn't work by any chance, the people will fall into an even more despair.";
		next;
		mes "[Maxim]";
		mes "So let's confirm the effects first. Actually, since you left Geffen, Margot's condition has worsened.";
		next;
		mes "[Maxim]";
		mes "Now is the perfect time, we're at a total loss with Neumann gone. The one who needs the cure most right now is the nurses.";
		next;
		mes "[Maxim]";
		mes "We shouldn't waste any time, let's try administering the cure to <NAVI>[Margot]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI> immediately. We don't have anything to lose, no?";
		erasequest 23290;
		setquest 23291;
		close3;
	}
	if ( isbegin_quest(23291) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "We shouldn't waste any time, let's try administering the cure to <NAVI>[Margot]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI> immediately. We don't have anything to lose, no?";
		close3;
	}
	if ( isbegin_quest(23298) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Have you distributed everything? I hope that it brings hope to the people of Geffen.";
		next;
		mes "[Maxim]";
		mes "Now all that's left is to monitor the results. You still have plenty of work ahead of you. So, leave that to me.";
		next;
		mes "[Maxim]";
		mes "That aside, you've helped my world once again. I don't even know how to begin thanking you.";
		next;
		mes "[Maxim]";
		mes "I almost feel embarrassed always receiving your help. If there's ever anything I can do for you, please don't hesitate to say so.";
		next;
		mes "[Maxim]";
		mes "There's something I need to tell you but I'd like to let you rest first, given how much you've done. Come speak to me once you've rest up.";
		erasequest 23298;
		setquest 23299;
		close3;
	}
	if ( isbegin_quest(23299) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "I know you've travel around a lot and worked hard, but can I ask one last favor?";
		next;
		mes "[Maxim]";
		mes "Even if we've created a cure, it will not mean anything if the source of the disease remains.";
		next;
		mes "[Maxim]";
		mes "As long the dark whispers exist, the disease will continue to spread. I heard from your friends while you were away that there's plan to completely destroy them.";
		next;
		mes "[Maxim]";
		mes "Unfortunately, I will only be a burden if I come with you, that's why I'll stay here in Geffen. Well, it's time for you to go.";
		next;
		mes "[Maxim]";
		mes "Head to the <NAVI>[decayed Verus]<INFO>hem_dun01,209,248,0,101,0</INFO></NAVI> and regroup with the other. They called the operation Witch Hunt Operation. Quite the fitting name, don't you think?";
		erasequest 23299;
		setquest 12663;
		getitem "Ch1_Root_Coin",10;
		getexp 195914233, 8000000;
		close3;
	}
	if ( isbegin_quest(12663) == 1 ) {
		mes "[Maxim]";
		mes "Head to the <NAVI>[decayed Verus]<INFO>hem_dun01,209,248,0,101,0</INFO></NAVI> and regroup with the other. They called the operation Witch Hunt Operation. Quite the fitting name, don't you think?";
		close3;
	}
	cutin "ch1_maxim.png",2;
	mes "[Maxim]";
	mes "If you want to know more about the doctor, asking the residents of Geffen would be more helpful than asking me.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23271) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23275) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23290) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23298) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23299) == 1";
end;
}

ch1_geffen,167,173,5	script(CLOAKED)	Ascetic Jeon#c01ms13	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(16939) == 1 ) {
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Here you are. How did things go at the clinic?";
		next;
		select("Well...");
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "A substance that seems to be the source of infection? We might get an answer if we find something similar.";
		mes "It's growth can be halted with divine and magic power but can't be cured with it... that's atleast something. Good work.";
		next;
		select("What brings you here?");
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "We've found a way to open the barrier blocking the underground.";
		mes "We spent quite some time figuring out how to open it without the disease spreading further and still completely block the monsters from the inside.";
		next;
		cutin "ex_sa_masterj02.png",2;
		mes "[Ascetic Jeon]";
		mes "Now that we've found a way, all that left is to open it. We're short on power to investigate the underground, so I came to get you. We need you to come with us.";
		erasequest 16939;
		setquest 17899;
		close2;
		cutin "",255;
		cloaknpc("Ascetic Jeon#c01ms13", true, getcharid(0));
		navigateto("ch1_geffen", 122, 118);
		end;
	}
	if ( isbegin_quest(17923) == 1 ) {
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Why are you two here together? Did something happen to the barrier?";
		next;
		select("That's not it.");
		cutin "ch1_clover01.png",0;
		mes "[Clover]";
		mes "I can't stop worrying about my seniors who's currently underground. When I went to ask if they needed anything, they said what worries them most are the people that gets consumed by madness.";
		mes "Why do even normal people turn into a fanatic?";
		next;
		cutin "ch1_clover03.png",0;
		mes "[Clover]";
		mes "They're worried something might happen if the madness grows stronger.";
		mes "That would be really bad. My seniors and master are still down there.";
		npctalk "Also my thesis...", "Clover#c01ms10", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "I see. I'm actually worried about that too.";
		next;
		cutin "ch1_clover01.png",0;
		mes "[Clover]";
		mes "The robots and undeads are attacking the people are a problem too, but the adventurer here will help out.";
		mes "Can we do anything about those fanatics?";
		next;
		select("We came here to ask for advice.");
		cutin "ch1_clover01.png",0;
		mes "[Clover]";
		mes "We'd really appreciate any kind of help, do you know anyone suitable?";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Everyone's busy right now.";
		next;
		cutin "ch1_clover01.png",0;
		mes "[Clover]";
		mes "I see. When do you think the others will be available?";
		next;
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "If you want, I can take a look at it myself? Thought, I have some doubts if I could be of help.";
		npctalk "Oh! You'll go yourself master Jeon? That's great!", "Clover#c01ms10", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Hm... Okay, I'll go ahead and scout the area.";
		next;
		cloaknpc("Ascetic Jeon#c01ms13", true, getcharid(0));
		cutin "ch1_clover02.png",0;
		mes "[Clover]";
		mes "Eh? All of the sudden? Won't you wait for us?";
		mes "Why is she so fast? Let's hurry and follow her!";
		erasequest 17923;
		setquest 17924;
		close2;
		cutin "",255;
		cloaknpc("Clover#c01ms11", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16939) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17923) == 1";
end;
}

ch1_geffen,165,171,5	script(CLOAKED)	Clover#c01ms11	1_M_WIZARD,{
	npctalk "I don't know if it's okay to be walking around like this. What if someone recognizes me...", "", BC_SELF;
	end;
}

hem_dun01,25,112,0	script	#c01ms20	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17900) == 1 ) {
		cloaknpc("Charles#c01ms20", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms20", false, getcharid(0));
		cloaknpc("Clover#c01ms20", false, getcharid(0));
	}
end;
}

hem_dun01,20,109,0	duplicate(#c01ms20)	#c01msmv21	HIDDEN_WARP_NPC,4,0
hem_dun01,28,110,0	duplicate(#c01ms20)	#c01msmv22	HIDDEN_WARP_NPC,4,0
hem_dun01,28,117,0	duplicate(#c01ms20)	#c01msmv23	HIDDEN_WARP_NPC,0,4
hem_dun01,16,118,0	duplicate(#c01ms20)	#c01msmv24	HIDDEN_WARP_NPC,2,2

hem_dun01,25,116,3	script(CLOAKED)	Charles#c01ms20	4_M_JOB_WIZARD,{
	if ( isbegin_quest(17900) == 1 ) {
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "This place... Isn't this Verus?";
		mes "Why is it connected to the underground of Geffen?";
		npctalk "A familiar place indeed.", "Ascetic Jeon#c01ms20", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Though it looks the same, we must be more cautious and assume that it's not the same. Things are different here.";
		next;
		cutin "ch1_clover01.png",1;
		mes "[Clover]";
		mes "Familar? Have you been here before? Or... Is there a similar place somewhere else? What's that one like? How for along have you researched it?";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "We shouldn't focus on that one right now~ Investigating this place comes first. On glance, there's only monsters roaming around and not much else.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "The sun is artifical but it as bright as the real one, there shouldn't be issue with food or water judging by the environment. The problem is I don't see any survivors.";
		next;
		select("Are they all dead?");
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "It's the source of the disease and this place is filled with monsters, I wouldn't be surprised if that's the case.";
		next;
		cutin "ch1_clover01.png",1;
		mes "[Clover]";
		mes "I believe that there are survivors. Even if we can't see anyone right now, there should be traces. I need to look for my master and seniors.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "We better go as a unit. We need to investigate this place anyway and we are familiar with this place, it'll be much smoother than going alone.";
		next;
		cutin "ch1_clover02.png",1;
		mes "[Clover]";
		mes "Oh, thank you very much. I'll make sure they repay your kindness if we find my master and seniors.";
		npctalk "Don't people usually repay it themselves?", "Charles#c01ms20", BC_SELF;
		next;
		select("Where should we start investigating?");
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "From the center, obviously! Isn't that the most open place? We'll start there.";
		erasequest 17900;
		setquest 17901;
		close2;
		cutin "",255;
		cloaknpc("Charles#c01ms20", true, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms20", true, getcharid(0));
		cloaknpc("Clover#c01ms20", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17900) == 1";
end;
}

hem_dun01,20,115,7	script(CLOAKED)	Ascetic Jeon#c01ms20	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(17900) == 1 ) {
		npctalk "This place is familiar. Interesting.", "", BC_SELF;
		end;
	}
	end;
}

hem_dun01,23,114,1	script(CLOAKED)	Clover#c01ms20	1_M_WIZARD,{
	if ( isbegin_quest(17900) == 1 ) {
		npctalk "Wait, how... is something like this under Geffen??", "", BC_SELF;
		end;
	}
	end;
}


hem_dun01,85,139,0	script	#c01ms21	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( 
		isbegin_quest(17901) == 1 || 
		isbegin_quest(17902) == 1 || 
		isbegin_quest(17903) == 1 || 
		isbegin_quest(17922) == 1 ||
		isbegin_quest(17924) == 1
	) {
		warp "ch1_sf03", 269, 185;
		end;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17901) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17902) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17903) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17922) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17924) == 1";
end;
}

ch1_sf03,269,185,0	script	#c01ms22	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17900) == 1 ) {
		cloaknpc("Ascetic Jeon#c01ms21", false, getcharid(0));
		cloaknpc("Charles#c01ms21", false, getcharid(0));
		cloaknpc("Clover#c01ms21", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17901) == 1 || isbegin_quest(17902) == 1 || isbegin_quest(17903) == 1 ) {
		cloaknpc("Ascetic Jeon#c01ms21", false, getcharid(0));
		cloaknpc("Charles#c01ms21", false, getcharid(0));
		cloaknpc("Clover#c01ms21", false, getcharid(0));
		cloaknpc("Sua#c01ms20", false, getcharid(0));
		cloaknpc("Parsifal#c01ms20", false, getcharid(0));
		cloaknpc("Believer#c01ms20", false, getcharid(0));
		cloaknpc("Believer#c01ms21", false, getcharid(0));
		cloaknpc("Believer#c01ms22", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17922) == 1 ) {
		cloaknpc("Sua#c01ms20", false, getcharid(0));
		cloaknpc("Parsifal#c01ms20", false, getcharid(0));
		cloaknpc("Clover#c01ms21", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17924) == 1 ) {
		cloaknpc("Sua#c01ms20", false, getcharid(0));
		cloaknpc("Parsifal#c01ms20", false, getcharid(0));
		cloaknpc("Clover#c01ms21", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms21", false, getcharid(0));
		end;
	}
end;
}

ch1_sf03,264,171,1	script(CLOAKED)	Clover#c01ms21	1_M_WIZARD,{
	if ( isbegin_quest(17901) == 1 ) {
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "What do you think this place was used for? It's my first time coming down here, everything feels so new and fascinating!";
		mes "What kind of people built this place? Where are they now? Why did they come here?";
		npctalk "You sure have a lot of question. I don't mean it in a bad way though.", "Charles#c01ms21", BC_SELF;
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "I wish I could've come here sooner! I wish my seniors didn't stopped me!";
		next;
		cutin "ex_sa_masterj01.png",1;
		mes "[Ascetic Jeon]";
		mes "Knowing one's limit is important for learning too, your seniors weren't wrong.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "But, aren't the monsters here too few from what I've expected?";
		next;
		cutin "ex_sa_masterj01.png",1;
		mes "[Ascetic Jeon]";
		mes "That's what is worries me. From what we've heard, monsters pouring out of here and had to be stopped, there should be a lot more of them...";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Maybe my seniors and master already dealt with them? We need just to find them now.";
		next;
		select("That would be ideal...");
		cutin "ex_sa_masterj01.png",1;
		mes "[Ascetic Jeon]";
		mes "Wait. I hear a group approaching.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "...Where? It's sounds like they're heading towards this way. Be ready, it could be monsters.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "How can you hear that? Just how sharp is your ears?";
		next;
		cloaknpc("Sua#c01ms20", false, getcharid(0));
		cloaknpc("Parsifal#c01ms20", false, getcharid(0));
		cloaknpc("Believer#c01ms20", false, getcharid(0));
		cloaknpc("Believer#c01ms21", false, getcharid(0));
		cloaknpc("Believer#c01ms22", false, getcharid(0));
		npctalk "Come on, let's hurry.", "Believer#c01ms21", BC_SELF;
		npctalk "We're going to be late at this rate.", "Parsifal#c01ms20", BC_SELF;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "They're here!!! Huh... but they're people? They're not monsters! Thank goodness.";
		next;
		cutin "ex_sa_masterj01.png",1;
		mes "[Ascetic Jeon]";
		mes "Don't rush to conclusion. We'll be in trouble if they're a group of fanatics.";
		next;
		cutin "",255;
		mes "[???]";
		mes "Clover? Is that you Clover?";
		next;
		mes "[???]";
		mes "What? Clover? Clover's here?";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Senior Sua! Master!";
		npctalk "Those are your seniors and master??", "Charles#c01ms21", BC_SELF;
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Clover! You're supposed to be guarding the barrier from outside, what are you doing here?";
		mes "Did the barrier break? Don't tell me, did you come here out of curiousity? Oh, you're in big trouble!";
		next;
		mes "[Parsifal]";
		mes "She came here because she wanted to. Let her be. There's nothing we can do now.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "So, what brings you here? And who are these people?";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Well. Master and senior haven't returned after a long time... the people outside are blaming the mages for the current plague.";
		mes "These people are the one who helped me.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Thank you for helping my reckless junior. Your support means a lot.";
		mes "But, why are the mages being blamed for the plague? Why so sudden?";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Well, there were rumors going around that the disease is one of Geffen Tower's research. They say the outbreak began when the tower collapsed.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "And there are people who actually believes that?";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "There's a lot! Then, these people suddenly showed up.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "Indeed, we suddenly showed up! We came to investigate because we highly suspect this place to be the source of the plague.";
		mes "Geffen Tower might not be the direct cause, but the timing of the invasion and this place being attacked. Everything just lines up too well. It's also the main reason of our suspicion.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Just exactly... who are you people?";
		next;
		mes "[Parsifal]";
		mes "Does it matter who they are? These people know about the blessing! They came here to receive it! This place is a land of blessing! I too shall be blessed!";
		npctalk "Master?", "Clover#c01ms21", BC_SELF;
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "This place is an ideal world for all people! What everyone seek is in here! Yes, you've come to the right place! Everyone should come! Everyone should come to this sacred grounds!";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Uhm, senior. I'm not sure if I should say this. But I'll say it cautiously. Does master seems a bit strange to you?";
		mes "If you ask what's strange, I can point exactly what's strange, but I can't bring myself to say something disrespectful towards master. But, something's really weird.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "You always say everything without care anyway. So you think he's strange too? Well, you're right. The truth is...";
		erasequest 17901;
		setquest 17902;
		close;
	}
	if ( isbegin_quest(17902) == 1 ) {
		npctalk "It's really senior Sua! Master too!!", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17903) == 1 ) {
		npctalk "What is master even talking about? I don't understand any of it.", "", BC_SELF;
		end;
	}
	npctalk "Is it really okay for me to be here...?", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17901) == 1";
end;
}

ch1_sf03,265,173,3	script(CLOAKED)	Sua#c01ms20	2_F_MAGICMASTER,{
	if ( isbegin_quest(17901) == 1 ) {
		npctalk "Why did Clover come here...?", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17902) == 1 ) {
		mes "[Sua]";
		mes "Actually, it's not just master who ended up like this. Do you remember the residents who came in before we mages started managing the place?";
		mes "It seems that they all became a fanatic of the civilization here. It's a completely different world from the one we live in.";
		npctalk "This place does look pretty unique. If I could study it, I'd love to take it apart.", "Clover#c01ms21", BC_SELF;
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Those people said, this place is not something created by humans. It's created by the gods. They want to show us another world. One day, we will be all part of it.";
		mes "They seriously believed that and even started a religion. They started bringing in others, and more kept coming...";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Are you saying that master believes in that? Eh- I don't buy it.";
		mes "Master, who is the epitome of being cold and rational thinking, falling for a cult?";
		npctalk "Hm... I'd like to hear more details.", "Ascetic Jeon#c01ms21", BC_SELF;
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Remember how we blocked the underground after discovering that people are going in here? Then, the invasion happened and monsters started coming out.";
		mes "So we came down ourselves and put up a barrier to keep anyone from going in and out.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Yes. That's right. I stayed and guarded that place after all. We even reinforced the barrier. I'm curious where is the other seniors?";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Well... we've dealt with the monsters... we managed to actually reduce their numbers significantly.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "Another thing, I'm curious to how your survived after all this time?";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Ah, that? As you can see, there's sunlight here and no shortage of water, surviving wasn't really an issue. But the threat is not that";
		npctalk "What threat?", "Ascetic Jeon#c01ms21", BC_SELF;
		next;
		cutin "",255;
		mes "[Sua]";
		mes "The other surviving mages slowly started to believe in the new religion. Just like what happen to the residents. Even those who were highly skeptical like master.";
		next;
		cutin "ex_sa_masterj01.png",1;
		mes "[Ascetic Jeon]";
		mes "Hm... so they gradually started losing their minds...";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "But, you seem fine senior?";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Fortunately, I've held on this far. If I hadn't, master might've gotten hurt while mingling with those people. Ugh, just witnessing the things they do...";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "Get hurt, how could you say that! It's a blessing, a blessing!";
		erasequest 17902;
		setquest 17903;
		close;
	}
	if ( isbegin_quest(17903) == 1 ) {
		npctalk "Just ignore what the master says.", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17922) == 1 ) {
		mes "[Sua]";
		mes "Clover? Why are you back here? Do you have some business here?";
		mes "This isn't exactly a pleasant place, try to not come here often.";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "It's okay. I'm just checking on you. Besides, we're working on a cure for the disease, so we should good news anytime soon. Hang in there a bit longer. Everything's going to be okay.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "I didn't know you were such a positive person.";
		mes "...hearing it does lift my spirits a bit.";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Senionr, is there anything wrong? Is it hard to live underground? Do you want to come with us after all?";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Like I said before. I'm fine. Really.";
		mes "More importantly, what about the cure? Any progress on it?";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "A lot of amazing people came. You remember the one who came here with me, I can't go into details but they said they've found something.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "But, senior. I know you've decided to stay here, I just can't feel at ease about it. There's no way being here like this is comfortable.";
		mes "Is there anything I can help you with? Or anything you need?";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Wow- I never thought I'd hear that kind of concern from you. Clover's all grown up.";
		mes "But really, I'm fine. We've got most of the supplies we need here, I think? I mean it.";
		npctalk "Still...", "Clover#c01ms21", BC_SELF;
		next;
		mes "[Sua]";
		mes "Hm... If I had to pick something, maybe do something about the fanatics? I really don't get how normal people can fall into such madness. Just look at our master.";
		npctalk "Are you talking about me?", "Parsifal#c01ms20", BC_SELF;
		next;
		mes "[Sua]";
		mes "It's scary to think what might happen later. Right now, they're only expressing their beliefs in words... but if history taught me anything, fanatics don't have a good track record...";
		next;
		mes "[Sua]";
		mes "The robots and the new humans are another problem. Sane people try to avoid them, but the fanatics are desperate to make contact no matter what. It causes a lot of incidents.";
		next;
		mes "[Sua]";
		mes "If I only had more time, I'd deal with it myself... but I'm afraid if I use a large-scale magic, the people here might get hurt.";
		next;
		select("I can handle the robots and new humans.");
		mes "[Sua]";
		mes "Oh! Thank you! I wonder it's really okay for me to rely on you like this...";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "What about the madness situation? Master? Can you be my research subject?";
		mes "I, Clover! Will cure you in the name of my honor! My previous thesis is basically decayed anyway, so I'll make this my new one!";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "Sua, what is this girl talking about?";
		next;
		mes "[Sua]";
		mes "Master, look over there. Doesn't it seem like that person is about to be reborn soon?";
		npctalk "What? Where? Who?", "Parsifal#c01ms20", BC_SELF;
		next;
		select("I think my friends can figure it out too...");
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Exactly! They'll be able figure something out too! They even strengthened the barrier outside, surely it will work, right? People from different fields have come.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "That's great! Thank you so much for caring.";
		mes "When everyone gets better and we make it out of here, I'll make sure to look after you.";
		completequest 17922;
		setquest 17923;
		close;
	}
	if ( isbegin_quest(17923) == 1 ) {
		npctalk "Aren't you going back to Geffen?", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17924) == 1 ) {
		mes "[Sua]";
		mes "Ah, is this what I think it is about? In that case, we'll all be fine.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "I like how you quickly understand.";
		mes "Ah, perfect timing. I've thought of a way to dispel their madness.";
		npctalk "Already? But we just got here?", "Clover#c01ms21", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Isn't there an artificial sun here? Thanks to that sun, animals, plants, and even humans can live in these sealed spaces without a ray of natural light.";
		next;
		cutin "ex_sa_masterj02.png",0;
		mes "[Ascetic Jeon]";
		mes "Normally, that would've been fine for sustaining life. But the appearance of Dark Whisper is the problem.";
		next;
		cutin "ex_sa_masterj03.png",0;
		mes "[Ascetic Jeon]";
		mes "No matter how well-made it is, the artificial sun is still artificial. When it came into contact with the ominous energy emitted by Dark Whisper, it began to affect people's cognitive functions.";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "How does that even happen?";
		next;
		cutin "ex_sa_masterj03.png",0;
		mes "[Ascetic Jeon]";
		mes "Do I have to explain in full details?";
		next;
		select("No! Please keep it short!");
		npctalk "Ehhh... I want to hear it.", "Clover#c01ms21", BC_SELF;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "The first people who came here saw this very advanced civilization, got filled with curiosity and admiration for this other world. That energy became it's seed.";
		next;
		cutin "ex_sa_masterj02.png",0;
		mes "[Ascetic Jeon]";
		mes "But as the Dark Whisper's energy started to stagnate in this sealed place, the seed started to sprout. The two energies are not compatible.";
		mes "That's why Dark Whisper took root here.";
		next;
		select("How do we get rid of it?");
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "For now, it's best to leave this place. Staying here for too long isn't good for anyone.";
		erasequest 17924;
		setquest 17925;
		close2;
		cutin "",255;
		cloaknpc("Ascetic Jeon#c01ms21", true, getcharid(0));
		cloaknpc("Clover#c01ms21", true, getcharid(0));
		end;
	}
	npctalk "I have lot of things in mind.", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17902) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17922) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17924) == 1";
end;
}

ch1_sf03,261,175,5	script(CLOAKED)	Parsifal#c01ms20	ACADEMY_MASTER,{
	if ( isbegin_quest(17922) == 1 ) {
		npctalk "Do you know when the blessing of rebirth will descend upon us?", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17903) == 1 ) {
		mes "[Parsifal]";
		mes "This place is the stage for humanity's evolution! Just look at everything around you!";
		mes "Does this look like something that could be achieved with human knowledge! No, this is a will of a higher being meant to bring enlightenment to mankind!";
		npctalk "Master has completely gone crazy... I mean, is that what you really think?", "Clover#c01ms21", BC_SELF;
		next;
		mes "[Parsifal]";
		mes "We must all be reborn into forms worthy of the divine will! Only then can we truly follow that will!";
		npctalk "The way he talks is grandiose, maybe because he's a mage.", "Ascetic Jeon#c01ms21", BC_SELF;
		npctalk "What can you do? That's just how we mages are.", "Charles#c01ms21", BC_SELF;
		next;
		select("God and rebirth, eh?");
		mes "[Parsifal]";
		mes "Ah, sounds like you're interested? Perfect timing, we're actually on our witness the rebirth of one of our own.";
		mes "Why not join us? Seeing the moment of rebirth with your own eyes will lead you to the truth.";
		npctalk "Rebirth?", "Clover#c01ms21", BC_SELF;
		npctalk "Rebirth?", "Charles#c01ms21", BC_SELF;
		npctalk "Master!!", "Sua#c01ms20", BC_SELF;
		next;
		cutin "ex_sa_masterj03.png",1;
		mes "[Ascetic Jeon]";
		mes "I don't know what rebirth is he talking about, but it's probably not unrelated to the mess we're in. Fine. Seeing it for ourselves might not be a bad idea.";
		next;
		select("Perhaps, it's the cause of their madness.");
		cutin "",255;
		mes "[Sua]";
		mes "Are you serious? It's not something worth seeing. You should leave now.";
		mes "Clover, take them and get out of here.";
		npctalk "Sua, don't stop them.", "Parsifal#c01ms20", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "Don't stop us. We came here to investigate this place. Witnessing it might not be a bad idea.";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "Yes, yes. A true mage, aren't you? Today's your lucky day. You'll witness a whole new world.";
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "Great! Let's go!";
		npctalk "Ah... I'm going crazy...", "Sua#c01ms20", BC_SELF;
		next;
		cloaknpc("Sua#c01ms20", true, getcharid(0));
		cloaknpc("Parsifal#c01ms20", true, getcharid(0));
		cloaknpc("Believer#c01ms20", true, getcharid(0));
		cloaknpc("Believer#c01ms21", true, getcharid(0));
		cloaknpc("Believer#c01ms22", true, getcharid(0));
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Let's go.";
		erasequest 17903;
		setquest 17904;
		getitem "Ch1_Root_Coin",5;
		getexp 97957116, 1950000;
		close2;
		cutin "",255;
		cloaknpc("Ascetic Jeon#c01ms21", true, getcharid(0));
		cloaknpc("Charles#c01ms21", true, getcharid(0));
		cloaknpc("Clover#c01ms21", true, getcharid(0));
		end;
	}
	npctalk "No, only listen to what I'm saying!", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17903) == 1";
end;
}

ch1_sf03,263,175,5	script(CLOAKED)	Ascetic Jeon#c01ms21	4_EXJOB_MASTER_J,{
	npctalk "This place has an unusual energy.", "", BC_SELF;
	end;
}

ch1_sf03,260,173,7	script(CLOAKED)	Charles#c01ms21	4_M_JOB_WIZARD,{
	npctalk "Those people are mages. I should talk to them later.", "", BC_SELF;
	end;
}

ch1_sf03,259,176,5	script(CLOAKED)	Believer#c01ms20	4_F_BELIEVER01,{
	npctalk "Strangers, welcome to this sacred place.", "", BC_SELF;
	end;
}

ch1_sf03,263,177,5	script(CLOAKED)	Believer#c01ms21	4_GEFFEN_04,{
	npctalk "It's fate that brought you here!!", "", BC_SELF;
	end;
}

ch1_sf03,261,171,1	script(CLOAKED)	Believer#c01ms22	4_M_HUMAN_01,{
	npctalk "Well? Can't you feel the blessings?", "", BC_SELF;
	end;
}

hem_dun01,182,199,0	script	#c01ms23	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( 
		isbegin_quest(17904) == 1 || 
		isbegin_quest(17905) == 1 || 
		isbegin_quest(17906) == 1 || 
		isbegin_quest(17907) == 1 || 
		isbegin_quest(17917) == 1 
	) {
		warp "ch1_sf03", 106, 172;
		end;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17904) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17905) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17906) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17907) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17917) == 1";
end;
}

ch1_sf03,106,172,0	script	#c01ms24	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17904) == 1 ) {
		cloaknpc("Ascetic Jeon#c01ms22", false, getcharid(0));
		cloaknpc("Charles#c01ms22", false, getcharid(0));
		cloaknpc("Clover#c01ms22", false, getcharid(0));
		cloaknpc("Parsifal#c01ms21", false, getcharid(0));
		cloaknpc("Sua#c01ms21", false, getcharid(0));
		cloaknpc("Believer#c01ms23", false, getcharid(0));
		cloaknpc("Believer#c01ms25", false, getcharid(0));
		cloaknpc("Believer#c01ms26", false, getcharid(0));
		cloaknpc("Believer#c01ms27", false, getcharid(0));
		cloaknpc("Believer#c01ms28", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17905) == 1 || isbegin_quest(17906) == 1 || isbegin_quest(17907) == 1 ) {
		cloaknpc("Ascetic Jeon#c01ms23", false, getcharid(0));
		cloaknpc("Charles#c01ms23", false, getcharid(0));
		cloaknpc("Clover#c01ms23", false, getcharid(0));
		cloaknpc("Parsifal#c01ms22", false, getcharid(0));
		cloaknpc("Sua#c01ms22", false, getcharid(0));
		cloaknpc("Believer#c01ms24", false, getcharid(0));
		cloaknpc("Believer#c01ms25", false, getcharid(0));
		cloaknpc("Believer#c01ms26", false, getcharid(0));
		cloaknpc("Believer#c01ms27", false, getcharid(0));
		cloaknpc("Believer#c01ms28", false, getcharid(0));
		cloaknpc("Collapsed Person#c01ms20", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17917) == 1 ) {
		cloaknpc("Ascetic Jeon#c01ms22", false, getcharid(0));
		cloaknpc("Charles#c01ms22", false, getcharid(0));
		cloaknpc("Clover#c01ms22", false, getcharid(0));
		cloaknpc("Parsifal#c01ms23", false, getcharid(0));
		cloaknpc("Sua#c01ms23", false, getcharid(0));
		cloaknpc("Believer#c01ms30", false, getcharid(0));
		cloaknpc("Believer#c01ms31", false, getcharid(0));
		cloaknpc("Believer#c01ms32", false, getcharid(0));
		cloaknpc("Believer#c01ms33", false, getcharid(0));
		end;
	}
end;
}

ch1_sf03,107,173,7	script(CLOAKED)	Charles#c01ms22	4_M_JOB_WIZARD,{
	if ( isbegin_quest(17904) == 1 ) {
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "There are quite a few people here. I guess more survived than we thought. Maybe even more than we see here?";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "Are we late? We ran into my disciple on the way here. I brought her along with us.";
		npctalk "Welcome! We are happy to have you here!", "Believer#c01ms23", BC_SELF;
		npctalk "Why does master looks so normal when he talks to those people?", "Clover#c01ms22", BC_SELF;
		next;
		mes "[Believer]";
		mes "You're not late. Actually. You've come just in time. By the way, you have so many disciples. Professor, you must be blessed with great connections. It's a great sign that you have so many loyal followers.";
		next;
		mes "[Believer]";
		mes "Is your disciple the only one that came today? What about the others?";
		next;
		mes "[Believer]";
		mes "No need to look for others. We only need those who wants to come here willingly. We don't need a random stragglers.";
		next;
		mes "[Believer]";
		mes "But, the worlds needs to know about this paradise.";
		next;
		mes "[Believer]";
		mes "That's why we must hurry and be reborn. Then, we'll go out to the world and spread the gospel.";
		next;
		mes "[Believer]";
		mes "I wonder when will it begin...";
		next;
		cutin "ex_sa_masterj03.png",0;
		mes "[Ascetic Jeon]";
		mes "There isn't any people who are infected with the disease, isn't it different from what we expected?";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "That's not quite true. A lot of people already died, these people are just survivors.";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "Wait, if these are the survivors, then how many mages was deployed in here? They're too few, seriously few.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "A lot of mages came in here to deal with the monsters. Every mage in the Geffen Tower was deployed.";
		next;
		mes "[Parsifal]";
		mes "It's not the time for chat. Newcomers, come over here where you can see. You're in luck. You'll witness first-hand someone being reborned.";
		next;
		mes "[Believer]";
		mes "Yes, come more closer. Once you see it with your own eyes, you'll never look at the world the same way again. You'll be someone enlightened.";
		npctalk "You're lucky people.", "Believer#c01ms26", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",0;
		mes "[Charles]";
		mes "What's that...?";
		erasequest 17904;
		setquest 17905;
		close2;
		cutin "",255;
		cloaknpc("Ascetic Jeon#c01ms22", true, getcharid(0));
		cloaknpc("Charles#c01ms22", true, getcharid(0));
		cloaknpc("Clover#c01ms22", true, getcharid(0));
		cloaknpc("Parsifal#c01ms21", true, getcharid(0));
		cloaknpc("Sua#c01ms21", true, getcharid(0));
		cloaknpc("Believer#c01ms23", true, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms23", false, getcharid(0));
		cloaknpc("Charles#c01ms23", false, getcharid(0));
		cloaknpc("Clover#c01ms23", false, getcharid(0));
		cloaknpc("Parsifal#c01ms22", false, getcharid(0));
		cloaknpc("Sua#c01ms22", false, getcharid(0));
		cloaknpc("Believer#c01ms24", false, getcharid(0));
		cloaknpc("Collapsed Person#c01ms20", false, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17904) == 1";
end;
}

ch1_sf03,105,174,7	script(CLOAKED)	Ascetic Jeon#c01ms22	4_EXJOB_MASTER_J,{
	npctalk "People are gathering.", "", BC_SELF;
	end;
}

ch1_sf03,108,171,7	script(CLOAKED)	Clover#c01ms22	1_M_WIZARD,{
	if ( isbegin_quest(17917) == 1 ) {
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Who are all of these people?";
		next;
		cutin "",255;
		mes "[Believer]";
		mes "Seize them! They've trespassed in the sacred temple!";
		npctalk "Capture and sacrifice them!!", "Believer#c01ms30", BC_SELF;
		next;
		mes "[Believer]";
		mes "Ah, move aside! So, how was the temple? Who did you meet? Was it full of new humans?";
		npctalk "Sacrifice, are you out of your mind? We don't do barbaric things like that!", "Believer#c01ms31", BC_SELF;
		next;
		mes "[Believer]";
		mes "What did it look like? Was it like a paradise?";
		npctalk "Come on, tell us everything!", "Believer#c01ms32", BC_SELF;
		next;
		mes "[Believer]";
		mes "These people must be punished! Are you planning to go in too? I won't allow it! I'll stop you no matter what!";
		npctalk "They invaded the sacred place!", "Believer#c01ms33", BC_SELF;
		next;
		mes "[Parsifal]";
		mes "We have the right to know! A duty to find out! These people came out safely!";
		mes "At the very least, the entrance is not blocked, right? Doesn't that mean access is not prohibited?";
		next;
		mes "[Believer]";
		mes "Blasphemous...!";
		npctalk "I misjudged you.", "Believer#c01ms23", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "It's a total chaos, pure madness. How are we supposed to get out of this mess?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Be careful not to damage our samples. I'll find a way to resolve this.";
		next;
		select("I'll help too.");
		cutin "",255;
		mes "[Sua]";
		mes "Clover! You're back?";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Senior! What's going on here? Why is master involved here again?";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "After you left, people split into two groups. Some wanted to follow and investigate after seeing your group go in, and the others started claiming as blasphemy and that you needed to be punished.";
		mes "Did you find anything?";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "We did find something, but. We don't know what exactly it is yet...";
		npctalk "We'll need to bring outside and analyze it.", "Clover#c01ms22", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "How are we getting out of here? As much as I appreciate all this passionate discussion. I wish they'd do it after we leave this place.";
		npctalk "We'll have to use the confusion to slip out.", "Ascetic Jeon#c01ms22", BC_SELF;
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Senior, come with us. Let's bring master too. Staying in a place like this can't be better than being outside, right?";
		npctalk "It'll be easier on you too.", "Clover#c01ms22", BC_SELF;
		next;
		cutin "",255;
		mes "[Sua]";
		mes "I can't... I don't know if we should go outside. Isn't the source of the disease in this place?";
		npctalk "But it's also outside too.", "Clover#c01ms22", BC_SELF;
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Clover, take a look at this.";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "That mark on your arm...! Senior, are you infected too? Since when...!";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "What? Sua, did you also receive the blessing of rebirth? That's great, truly great!";
		next;
		mes "[Sua]";
		mes "Me being infected isn't the only issue. You saw it right? If we let master out of this place, won't he be a threat to society?";
		npctalk "Indeed... innocent people could fall under that strange influence.", "Ascetic Jeon#c01ms22", BC_SELF;
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "You're right, the outside world think mages are the cause of the disease, it would be better to go out after that's all cleared up.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "We'll do what we can here. We've made it this far, we can hold out a little bit longer. You should go outside and keep the barrier safe.";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "I'll definitely uncover the truth about the plague and come back for you. There must be a cure. Please stay healthy until then. Don't become like the others!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Now seems like the best time to leave. Let's go quietly.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Please stay safe.";
		npctalk "Clover! Where are you going!", "Parsifal#c01ms21", BC_SELF;
		npctalk "Argh, not this again. Clover is running an errand for me. Got it?", "Sua#c01ms21", BC_SELF;
		erasequest 17917;
		setquest 17918;
		close2;
		cloaknpc("Ascetic Jeon#c01ms22", true, getcharid(0));
		cloaknpc("Charles#c01ms22", true, getcharid(0));
		cloaknpc("Clover#c01ms22", true, getcharid(0));
		end;
	}
	npctalk "Wait a minute, I'm a bit confused right now.", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17917) == 1";
end;
}

ch1_sf03,107,174,7	script(CLOAKED)	Parsifal#c01ms21	ACADEMY_MASTER,{
	npctalk "You've come at the right time, the perfect time.", "", BC_SELF;
	end;
}

ch1_sf03,110,173,7	script(CLOAKED)	Sua#c01ms21	2_F_MAGICMASTER,{
	npctalk "Ugh, I'm not sure you coming here was the right choice...", "", BC_SELF;
	end;
}

ch1_sf03,110,174,3	script(CLOAKED)	Believer#c01ms23	4_F_BELIEVER01,{
	npctalk "Welcome. You're about to witness a new world.", "", BC_SELF;
	end;
}

ch1_sf03,112,182,7	script(CLOAKED)	Believer#c01ms24	4_F_BELIEVER01,{
	npctalk "Isn't it a sacred sight to witness?", "", BC_SELF;
	end;
}

ch1_sf03,113,181,7	script(CLOAKED)	Believer#c01ms25	4_EP18_GW_MAN01,{
	npctalk "May the blessing of rebirth come to me aswell....", "", BC_SELF;
	end;
}

ch1_sf03,114,179,7	script(CLOAKED)	Believer#c01ms26	4_M_TWBOY,{
	npctalk "It's beautiful...", "", BC_SELF;
	end;
}

ch1_sf03,116,179,7	script(CLOAKED)	Believer#c01ms27	4_F_LGTGIRL,{
	npctalk "Will I become like that too soon? I will, I'll definitely will.", "", BC_SELF;
	end;
}

ch1_sf03,112,179,7	script(CLOAKED)	Believer#c01ms28	4_M_MASKMAN,{
	npctalk "This is the future of humanity!", "", BC_SELF;
	end;
}

ch1_sf03,111,176,3	script(CLOAKED)	Believer#c01ms30	4_M_TWBOY,{
	npctalk "Newcomers, feels free to ask me any question.", "", BC_SELF;
	end;
}

ch1_sf03,106,176,5	script(CLOAKED)	Believer#c01ms31	4_EP18_GW_MAN01,{
	npctalk "Are you a new believer?", "", BC_SELF;
	end;
}

ch1_sf03,112,173,3	script(CLOAKED)	Believer#c01ms32	4_M_MASKMAN,{
	npctalk "There will be a new life...!", "", BC_SELF;
	end;
}

ch1_sf03,108,177,5	script(CLOAKED)	Believer#c01ms33	4_F_BELIEVER01,{
	npctalk "Why not me? Why?", "", BC_SELF;
	end;
}

ch1_sf03,116,181,1	script(CLOAKED)	Clover#c01ms23	1_M_WIZARD,{
	if ( isbegin_quest(17905) == 1 ) {
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Someone has collapsed!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "That person is dead. Judging by the condition, he must have died from the disease. What is this rebirth, and why are they gathering around a dead person?";
		next;
		cutin "",255;
		mes "[Believer]";
		mes "What disease! That person received the blessing of rebirth! Watch your words!";
		npctalk "They're newcomers, they don't know a thing. Be patient with them.", "Believer#c01ms24", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Judging by the condition of the skin, I'm certain that it's caused by the disease. What are they saying?";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "Oh, so you have seen others that have been blessed before, right?";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "It's not a blessing, it's clearly a disease, no?";
		next;
		cutin "",255;
		mes "[Believer]";
		mes "No, these people!! It's a blessing!!! This blessing is what allowed them to create this paradise, to become just like the one who've returned!";
		npctalk "I can't take this anymore!", "Believer#c01ms27", BC_SELF;
		next;
		cutin "ex_sa_masterj03.png",0;
		mes "[Ascetic Jeon]";
		mes "Do you mean this is what you call rebirth?";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "Yes, that's right. The blessing begins with losing your breath. After that your skin will change, a high fever will burn all your impurities in your body, and expel the rest.";
		npctalk "Breath is both the beginning and the end.", "Parsifal#c01ms22", BC_SELF;
		next;
		mes "[Parsifal]";
		mes "The one before you is the state where all impurities have been expelled and have already stopped breathing, entering into a pure state to shed their wordly body. That's how one isolate themself to purify both body and soul.";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "But up there, people are dying from the same condition!";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Clover, Shh... there's no use explaining that to these people.";
		npctalk "But still....", "Clover#c01ms23", BC_SELF;
		next;
		mes "[Parsifal]";
		mes "Witness it with your own eyes. Then you will understand everything.";
		npctalk "Master is only talking normally when it comes to this.", "Clover#c01ms23", BC_SELF;
		next;
		mes "[Believer]";
		mes "Why would they need to witness it, when they're full of doubts? We only brought them here for this important moment is because they're your disciples...";
		next;
		mes "[Parsifal]";
		mes "Enlightenment is something everyone needs. Since they've come this far on their own, perhaps it's fate that brought them here.";
		npctalk "Wise words.", "Believer#c01ms24", BC_SELF;
		next;
		mes "[Believer]";
		mes "Hm... very well. But the sacred moment of rebirth is about to come, no one must make a sound.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "And why is that?";
		next;
		cutin "",255;
		mes "[Believer]";
		mes "For the reborned person can return to the sacred place undisturbed.";
		npctalk "There's still so much for you to learn.", "Believer#c01ms28", BC_SELF;
		next;
		mes "[Parsifal]";
		mes "If you're unlucky and draw it's attention by making a sound, you'll die at the hands of the reborn not having received the blessing and disturbing the sacred moment.";
		npctalk "What are they even talking about..", "Charles#c01ms23", BC_SELF;
		next;
		select("I also don't understand.");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Watching may help. This could be a chance to learn about the disease, let's stay silent for now and observe.";
		next;
		specialeffect EF_M05, AREA, "Collapsed Person#c01ms20";
		specialeffect EF_BEGINSPELL_N2, AREA, "Collapsed Person#c01ms20";
		mes "[Parsifal]";
		mes "Oh, here it comes!";
		erasequest 17905;
		setquest 17906;
		close3;
	}
	npctalk "Hm, is it really fine to let it be??", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17905) == 1";
end;
}

ch1_sf03,111,181,7	script(CLOAKED)	Parsifal#c01ms22	ACADEMY_MASTER,{
	if ( isbegin_quest(17906) == 1 ) {
		mes "[Parsifal]";
		mes "Oh, Ohhh! It's starting!";
		npctalk "Shh- Be quiet.", "Believer#c01ms25", BC_SELF;
		next;
		specialeffect EF_SOULBREAKER3, AREA, "Collapsed Person#c01ms20";
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "The b-body, it's splitting apart...!";
		next;
		specialeffect EF_SPR_MASH, AREA, "Collapsed Person#c01ms20";
		cutin "",255;
		mes "[Parsifal]";
		mes "He's shedding his old body. Watch carefully.";
		npctalk "Ahhhh, it's beautiful.", "Believer#c01ms27", BC_SELF;
		next;
		specialeffect 1033, AREA, "Collapsed Person#c01ms20";
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "That person was dead, how did his body split like that? Is there some parasite involved?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "How bizzare. It attaches to a person, drains everything, and once the host dies, it takes over it's body...";
		npctalk "Should I capture it and experiment on it?", "Charles#c01ms23", BC_SELF;
		next;
		select("This is horrific.");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Wait, let's see what will come out of it.";
		next;
		specialeffect EF_ITEM_CURSE, AREA, "New Human#c01ms20";
		specialeffect EF_FLOWERCAST, AREA, "New Human#c01ms20";
		cloaknpc("Collapsed Person#c01ms20", true, getcharid(0));
		cloaknpc("New Human#c01ms20", false, getcharid(0));
		cutin "",255;
		mes "[Believer]";
		mes "Oooh- oh!!! it's a new human! A new human has been born...!";
		npctalk "Keep your voice down. Don't interrupt it.", "Believer#c01ms26", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "That thing... doesn't it look familiar??";
		next;
		select("It's an Immortal Corps.");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Those infected with the disease become an Immortal Corps... We need to find the root and cause of this.";
		npctalk "Then everything that was roaming around outside...?", "Clover#c01ms23", BC_SELF;
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Immortal Corps? Do you know anything about them?";
		next;
		mes "[Parsifal]";
		mes "Lower your voice! The new human is surveying the area. Be careful not to draw it's attention.";
		next;
		specialeffect EF_HIT1, AREA, "New Human#c01ms20";
		cloaknpc("New Human#c01ms20", true, getcharid(0));
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "It disappeared...!";
		cutin "",255;
		erasequest 17906;
		setquest 17907;
		close;
	}
	if ( isbegin_quest(17907) == 1 ) {
		npctalk "So? How was it? What do you think?", "", BC_SELF;
		end;
	}
	if ( isbegin_quest(17908) == 1 ) {
		npctalk "Where are you going??", "", BC_SELF;
		end;
	}
	npctalk "Watch closely!", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17906) == 1";
end;
}

ch1_sf03,116,180,3	script(CLOAKED)	Sua#c01ms22	2_F_MAGICMASTER,{
	npctalk "I'm not really sure if I should let you see this...", "", BC_SELF;
	end;
}

ch1_sf03,109,175,3	script(CLOAKED)	Parsifal#c01ms23	ACADEMY_MASTER,{
	npctalk "There's so much to see and learn!", "", BC_SELF;
	end;
}

ch1_sf03,108,176,5	script(CLOAKED)	Sua#c01ms23	2_F_MAGICMASTER,{
	npctalk "We should send them away now.", "", BC_SELF;
	end;
}

ch1_sf03,114,183,7	script(CLOAKED)	Ascetic Jeon#c01ms23	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(17907) == 1 ) {
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "What do you all think? Is that person special? Or all the people who are infected with the disease will go through the same process?";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "They called it the birth of a new human, I bet everyone will undergo the same transformation.";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "Exactly. Everyone who received the blessing will be reborn and become a new human.";
		npctalk "I can't believe it...", "Clover#c01ms23", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Are you saying everyone who receives this blessing shows every symptoms that you mentioned earlier? Do all of them turn into that?";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "That's right. It's astounding. That beings of such a civilization are offering even us the chance to be part of it...!";
		npctalk "Please, I want to receive the blessing too...", "Believer#c01ms25", BC_SELF;
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Then the monsters outside and the ones that we saw in city... They were all people...!";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Didn't you sealed off the city?";
		npctalk "What? Sealed? How are new believers suppose to go in?", "Parsifal#c01ms22", BC_SELF;
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "It was sealed. Even so, monster still kept appearing somehow, so we thought there's a place that we overlooked... But it turns out all of them are actually...";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "People who turned into an Immortal Corps, this isn't something we should take lightly. We need to find the reason why people who died from the disease become part of the Immortal Corps.";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Just now, where did that Immortal Corps went? We need to capture one to figure things out.";
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Those who transformed like that always head toward one place. Over there, they gather at that structure.";
		npctalk "Structure? Huh, that's intriguing.", "Charles#c01ms23", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "We need to find the connection between the disease and why it's creating an Immortal Corps. We should go investigate that structure.";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "At the very least, we need to find where they're going. You little apprentice, come with us.";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Huh? Me too?";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "What, did you forget that you're a mage too after pretending not to be one? Do you want to miss a chance like this? Do you plan on staying as an apprentice forever?";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Ah?!! Yes! I'll go! I'm going! I'm absolutely going!!";
		next;
		cutin "",255;
		mes "[Parsifal]";
		mes "Are you going to follow the new human? You shouldn't do it! It will bring a great misfortune!";
		npctalk "It will bring bad luck! Don't do it!", "Believer#c01ms25", BC_SELF;
		npctalk "Hm, what's wrong with these people?", "Charles#c01ms23", BC_SELF;
		next;
		cutin "",255;
		mes "[Sua]";
		mes "Hold on. We don't know anything about new humans yet, right? We don't know what they want either.";
		next;
		mes "[Believer]";
		mes "Well... the thing is... if you approach a new human, they'll try to hurt you. They don't want to be in contact with people like us.";
		next;
		mes "[Sua]";
		mes "Maybe you got too close to it. If we learn more, we'll be able to inform others better, am I wrong?";
		next;
		mes "[Parsifal]";
		mes "Right! You're right!! We need to know the truth if we want to share it with others! That's what being a mage is all about! Then. Maybe I should be the one to go...!";
		npctalk "Master! You're the one who said you want to examine the remains of what the new human left behind!", "Sua#c01ms22", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "We need to make it quick! Let's go!";
		erasequest 17907;
		setquest 17908;
		close2;
		cutin "",255;
		cloaknpc("Ascetic Jeon#c01ms23", true, getcharid(0));
		cloaknpc("Charles#c01ms23", true, getcharid(0));
		cloaknpc("Clover#c01ms23", true, getcharid(0));
		end;
	}
	npctalk "For now, we should just observe.", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17907) == 1";
end;
}

ch1_sf03,115,182,7	script(CLOAKED)	Charles#c01ms23	4_M_JOB_WIZARD,{
	npctalk "Isn't this interesting? Pay attention to it.", "", BC_SELF;
	end;
}

ch1_sf03,121,186,3	script(CLOAKED)	Collapsed Person#c01ms20	4_M_DSTMANDEAD,{
	end;
}

ch1_sf03,121,186,3	script(CLOAKED)	New Human#c01ms20	CH1_IMMORTAL_CORPS1,{
	end;
}

hem_dun01,220,210,0	script	#c01ms25	HIDDEN_WARP_NPC,5,5,{
	end;
	
OnTouch:
	if ( isbegin_quest(17908) == 1 ) {
		cloaknpc("Charles#c01ms24", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms24", false, getcharid(0));
		cloaknpc("Clover#c01ms24", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17909) == 1 || isbegin_quest(17910) == 1 || isbegin_quest(17911) == 1 || isbegin_quest(17912) == 1 || isbegin_quest(17916) ) {
		warp "ch1_dw", 264, 294;
		end;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17909) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17910) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17911) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17912) == 1";
end;
}

hem_dun01,218,206,0	script	#c01msmv25	HIDDEN_WARP_NPC,6,0,{
	end;
	
OnTouchNPC:
	unitskilluseid getattachedrid(), "AL_TELEPORT", 1;
end;
}

hem_dun01,226,206,0	duplicate(#c01msmv25)	#c01msmv26	HIDDEN_WARP_NPC,0,6
hem_dun01,226,214,0	duplicate(#c01msmv25)	#c01msmv27	HIDDEN_WARP_NPC,0,6
hem_dun01,218,214,0	duplicate(#c01msmv25)	#c01msmv28	HIDDEN_WARP_NPC,6,0

hem_dun01,221,209,7	script(CLOAKED)	Charles#c01ms24	4_M_JOB_WIZARD,{
	if ( isbegin_quest(17908) == 1 ) {
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "This, thing?? I've seen it somewhere before, I think?";
		npctalk "Is it that thing?", "Clover#c01ms24", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Hm... just as I suspected. After I saw the Immortal Corps, I immediately thought that this was Himmelmez's doing. She came here herself.";
		mes "The ominous energy that I'm sensing is coming from this very area. This is our destination.";
		next;
		select("Dark Whisper! Do you think it's inside?");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "We don't know until we go in. There's no other way but to check it ourselves.";
		next;
		cutin "ch1_clover03.png",1;
		mes "[Clover]";
		mes "Hm, what is that? What are you talking about? Can you tell me?";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "It's the thing that invaded this place. It's the one releasing the monsters, and is the root cause of all this. It also showed up from where we came from.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Yes, they also attempted to invade our dimension too. We managed to drove them back with effort, but we didn't think that they moved to another dimension.";
		next;
		cutin "ch1_clover03.png",1;
		mes "[Clover]";
		mes "A parallel dimension? It's been invading different dimensions? Something like that is coming for us?";
		mes "So, she's the cause of all this disastes, correct? Unforgivable!";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "Are you getting cold feet by just looking at it? Why are you suddenly rambling? Is it too much for you?";
		next;
		cutin "ch1_clover01.png",1;
		mes "[Clover]";
		mes "No! I may not be a full-fledged mage yet, but I'm smart enough to get into Geffen Tower as an apprentice!";
		mes "Just a bit more and I'll be a full-fledged mage! Please believe me! Not anyone can maintain a barrier like I did!";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "Yes, yes. I got it already. It's good to be fired up. But don't forget that you're still unexperienced. There's no need to push yourself and leave it to us veterans.";
		next;
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "I'm still curious about a few things. Like how this thing got buried underground. How do you think it ended up here?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "It's a strange power. Let's head inside to find out.";
		erasequest 17908;
		setquest 17909;
		getitem "Ch1_Root_Coin",5;
		getexp 97957116, 1950000;
		close2;
		cutin "",255;
		cloaknpc("Charles#c01ms24", true, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms24", true, getcharid(0));
		cloaknpc("Clover#c01ms24", true, getcharid(0));
		warp "ch1_dw", 264, 294;
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17908) == 1";
end;
}

hem_dun01,219,209,7	script(CLOAKED)	Ascetic Jeon#c01ms24	4_EXJOB_MASTER_J,{
	npctalk "It's really similar to ours.", "", BC_SELF;
	end;
}

hem_dun01,219,211,7	script(CLOAKED)	Clover#c01ms24	1_M_WIZARD,{
	npctalk "What, what is that??", "", BC_SELF;
	end;
}

ch1_dw,264,294,0	script	#c01ms30	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17909) == 1 ) {
		cloaknpc("Charles#c01ms30", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms30", false, getcharid(0));
		cloaknpc("Clover#c01ms30", false, getcharid(0));
		cloaknpc("Immortal Corps#c01ms30", false, getcharid(0));
		end;
	}
end;
}

ch1_dw,260,295,1	script(CLOAKED)	Ascetic Jeon#c01ms30	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(17909) == 1 ) {
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "I see a monster there. Let's keep following it.";
		next;
		cutin "ch1_clover01.png",1;
		mes "[Clover]";
		mes "Could it be the same one from earlier? It doesn't look familiar with this place. It looks like it's here for the first time.";
		next;
		select("So this is what it looks inside.");
		cutin "nov_magicsoul01.png",2;
		mes "[Charles]";
		mes "What kind of facility is this? An invasion facility? A production facility? A residential facility? All of the above? Where's the core structure? The command center? How much power do they have?";
		next;
		cutin "ch1_clover01.png",1;
		mes "[Clover]";
		mes "Ah! It's moving! Let's follow it!";
		next;
		cloaknpc("Immortal Corps#c01ms30", true, getcharid(0));
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "We need to move carefully, so we don't draw its attention.";
		erasequest 17909;
		setquest 17910;
		close2;
		cutin "",255;
		cloaknpc("Charles#c01ms30", true, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms30", true, getcharid(0));
		cloaknpc("Clover#c01ms30", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17909) == 1";
end;
}

ch1_dw,262,296,1	script(CLOAKED)	Charles#c01ms30	4_M_JOB_WIZARD,{
	npctalk "Ho... how is a freshly created entity be able to locate this place?", "", BC_SELF;
	end;
}

ch1_dw,262,294,1	script(CLOAKED)	Clover#c01ms30	1_M_WIZARD,{
	npctalk "These stairs seem endless. It seems it will take long to reach the top.", "", BC_SELF;
	end;
}

ch1_dw,256,301,1	script(CLOAKED)	Immortal Corps#c01ms30	CH1_IMMORTAL_CORPS1,{
	end;
}

ch1_dw,267,77,1	script(CLOAKED)	Immortal Corps#c01ms31	CH1_IMMORTAL_OFFICER1,{
	end;
}

ch1_dw,264,77,1	script(CLOAKED)	Immortal Corps#c01ms32	CH1_IMMORTAL_OFFICER2,{
	end;
}

ch1_dw,265,79,1	script(CLOAKED)	Immortal Corps#c01ms33	CH1_IMMORTAL_CORPS3,{
	end;
}

ch1_dw,266,79,1	script(CLOAKED)	Immortal Corps#c01ms34	CH1_IMMORTAL_CORPS1,{
	end;
}

ch1_dw,265,65,0	script	#c01ms31	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(17910) == 1 ) {
		cloaknpc("Charles#c01ms31", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms31", false, getcharid(0));
		cloaknpc("Clover#c01ms31", false, getcharid(0));
		cloaknpc("Immortal Corps#c01ms31", false, getcharid(0));
		cloaknpc("Immortal Corps#c01ms32", false, getcharid(0));
		cloaknpc("Immortal Corps#c01ms33", false, getcharid(0));
		cloaknpc("Immortal Corps#c01ms34", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(17916) == 1 ) {
		cloaknpc("Charles#c01ms31", false, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms31", false, getcharid(0));
		cloaknpc("Clover#c01ms31", false, getcharid(0));
		end;
	}
end;
}

ch1_dw,269,67,1	script(CLOAKED)	Clover#c01ms31	1_M_WIZARD,{
	if ( isbegin_quest(17910) == 1 ) {
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "It stopped over there.";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "That's a group of Immortal Corps. It's joining them. Let's hide our presence.";
		close2;
		pcblock(true);
		cutin "",255;
		npctalk "......", "Immortal Corps#c01ms31", BC_SELF;
		npctalk "Are they talking among themselves?", "Clover#c01ms31", BC_SELF;
		npctalk "They can communicate?", "Charles#c01ms31", BC_SELF;
		sleep2 400;
		npctalk "......", "Immortal Corps#c01ms32", BC_SELF;
		npctalk "......", "Immortal Corps#c01ms33", BC_SELF;
		npctalk "......", "Immortal Corps#c01ms34", BC_SELF;
		sleep2 400;
		specialeffect EF_HIT1, AREA, "Immortal Corps#c01ms31";
		specialeffect EF_HIT1, AREA, "Immortal Corps#c01ms32";
		specialeffect EF_HIT1, AREA, "Immortal Corps#c01ms33";
		specialeffect EF_HIT1, AREA, "Immortal Corps#c01ms34";
		cloaknpc("Immortal Corps#c01ms31", true, getcharid(0));
		cloaknpc("Immortal Corps#c01ms32", true, getcharid(0));
		cloaknpc("Immortal Corps#c01ms33", true, getcharid(0));
		cloaknpc("Immortal Corps#c01ms34", true, getcharid(0));
		sleep2 400;
		pcblock(false);
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "They went inside.";
		npctalk "Isn't their security a bit looser than expected?", "Charles#c01ms31", BC_SELF;
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "Strangely, there's aren't any other Immortal Corps here. I thought they'd swarm us the moment we entered.";
		mes "Are they preparing some kind of welcome party for newcomers?";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "I don't know the reason, but this is a good opportunity. We should investigate and quickly get out.";
		npctalk "It feels like we're robbing an empty house, it's kind of thrilling. Ah, not that I want to be a criminal or something!", "Clover#c01ms31", BC_SELF;
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Wait, hold on. I think I'm starting to piece it all together. A person with the disease dies and becomes an Immortal Corps.";
		mes "Then, the legions enters this place and joins the group.";
		next;
		select("Is this part of Himmelmez's scheme?");
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Most likely. If the rebirth is related to the plague, there must be a medium for it. It might not be a disease at all, it could be something else. A spell, magic... or maybe a curse...";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "I'm not sure about that, but it's safe to disease originated here. Let's find proof!!";
		next;
		cutin "ch1_clover01.png",2;
		mes "[Clover]";
		mes "We will also need monster samples too. To compare them to the patients. Right?";
		next;
		select("You mean chop the monsters up?");
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Ah, no! Something like hair... or other things.";
		npctalk "Wait, it has hair?", "Charles#c01ms31", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Just hitting one and getting something from it should be enough.";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "That should work. Alright, let's split up and search! Clover, you're with me. I'll show you how a real mage use magic.";
		erasequest 17910;
		setquest 17911;
		setquest 17912;
		close2;
		cutin "",255;
		cloaknpc("Charles#c01ms31", true, getcharid(0));
		cloaknpc("Ascetic Jeon#c01ms31", true, getcharid(0));
		cloaknpc("Clover#c01ms31", true, getcharid(0));
		end;
	}
	npctalk "Did you find anything good?", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17910) == 1";
end;
}

ch1_dw,264,66,7	script(CLOAKED)	Ascetic Jeon#c01ms31	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(17916) == 1 ) {
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Perfect timing.";
		next;
		cutin "ch1_clover03.png",2;
		mes "[Clover]";
		mes "Ugh- I can't hold it in- I can't anymore! How can things like this exist in this world? Why are the bad guys is always insanely evil?";
		npctalk "Calm down, Clover. We got excellent people to deal with it.", "Charles#c01ms31", BC_SELF;
		npctalk "Argh, just wait and see you evil beings!!!", "Clover#c01ms31", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Did you manage to find anything?";
		next;
		select("I found something...");
		cutin "ex_sa_masterj02.png",0;
		mes "[Ascetic Jeon]";
		mes "Hm... as I thought, this place isn't just any facility. If my hunch is right, some truly vile things are going on here.";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "They're generating power here, storing dangerous materials, and pushing forward with a plan to invade. We'll have to deal with this place eventually. If we leave them be, things will just get worse.";
		npctalk "I'll help too!!!", "Clover#c01ms31", BC_SELF;
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "Give that to me. Once we're outside, I'll have the others inspect it. We'll need to discuss how to deal with this place also.";
		next;
		cutin "nov_magicsoul01.png",1;
		mes "[Charles]";
		mes "Shouldn't we burn this place down? Looks like incineration is the answer. It's the best and quickest way to destroy everything without issues.";
		next;
		cutin "ch1_clover02.png",2;
		mes "[Clover]";
		mes "Oh, right now? Burn everything? I'm in! Fire magic is my specialty! I'll give it my all!";
		next;
		cutin "ex_sa_masterj01.png",0;
		mes "[Ascetic Jeon]";
		mes "We'll discuss that later. We need to get out of here first and do something about the plague. Let's move.";
		if ( countitem("Ch1_Sample") ) 
			delitem "Ch1_Sample", countitem("Ch1_Sample");
		erasequest 17916;
		setquest 17917;
		getitem "Ch1_Root_Coin",12;
		getexp 130609489, 3000000;
		close2;
		cutin "",255;
		warp "hem_dun01", 226, 212;
		end;
	}
	npctalk "There should be clues here. Search carefully.", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17916) == 1";
end;
}

ch1_dw,267,66,1	script(CLOAKED)	Charles#c01ms31	4_M_JOB_WIZARD,{
	npctalk "I'd like to investigate everything. It's not the right time though.", "", BC_SELF;
	end;
}

function	script	CH1_Legion_Check	{
	return isbegin_quest(17913) == 2 && isbegin_quest(17914) == 2 && isbegin_quest(17915) == 2;
}

ch1_dw,340,85,3	script	Immortal Corps#c01ms35	CH1_IMMORTAL_CORPS2,{
	if ( isbegin_quest(17912) == 1 ) {
		.@id = atoi( replacestr(strnpcinfo(2), "c01ms3", "") );
		switch ( .@id ) {
			case 5:
				if ( isbegin_quest(17913) == 0) {
					pctalk "Hm... It looks like it's carrying something.";
					progressbar_npc "3131FF", 1;
					cloaknpc("Immortal Corps#c01ms35", true, getcharid(0));
					setquest 17913;
					completequest 17913;
					monster "ch1_dw", 338, 85, "--en--", 22461, 1;
				}
				break;
			case 6:
				if ( isbegin_quest(17914) == 0) {
					pctalk "This one looks a bit strange. Let's search it.";
					progressbar_npc "3131FF", 1;
					cloaknpc("Immortal Corps#c01ms35", true, getcharid(0));
					setquest 17914;
					completequest 17914;
					monster "ch1_dw", 338, 82, "--en--", 22461, 1;
				}
				break;
			case 7:
				if ( isbegin_quest(17915) == 0) {
					pctalk "This one looks suspicious.";
					progressbar_npc "3131FF", 1;
					cloaknpc("Immortal Corps#c01ms35", true, getcharid(0));
					setquest 17915;
					completequest 17915;
					monster "ch1_dw", 338, 82, "--en--", 22461, 1;
				}
				break;
		}
		if ( CH1_Legion_Check() ) {
			erasequest 17912;
			erasequest 17913;
			erasequest 17914;
			erasequest 17915;
			getitem "Ch1_Sample", 1;
			pctalk "This one...! It's a bit strange? I'd better take this one.";
			if ( isbegin_quest(17911) == 0 ) {
				setquest 17916;
			}
		} else {
			pctalk "Doesn't look like there's anything worth taking. I should look for something else.";
		}
	}
	end;
	
OnInit:
	.@id = atoi( replacestr(strnpcinfo(2), "c01ms3", "") );
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17912) == 1 && isbegin_quest(" + (17908 + .@id) + ") == 0";
end;
}

ch1_dw,340,82,3	duplicate(Immortal Corps#c01ms35)	Immortal Corps#c01ms36	CH1_IMMORTAL_CORPS1
ch1_dw,340,78,3	duplicate(Immortal Corps#c01ms35)	Immortal Corps#c01ms37	CH1_IMMORTAL_CORPS3

ch1_dw,320,94,7	script	Immortal Corps#c01ms38	CH1_IMMORTAL_CORPS2,{
	pctalk "Oh... it's best not to touch it.";
	end;
}

ch1_dw,318,95,3	script	Immortal Corps#c01ms39	CH1_IMMORTAL_CORPS3,{
	pctalk "Looking at it, it seems unfinished.";
	end;
}

ch1_dw,217,96,3	script	Advanced Device#c01ms30	4_POINT_WHITE,{
	if ( isbegin_quest(17911) == 1 ) {
		.@id = atoi( replacestr(strnpcinfo(2), "c01ms3", "") );
		switch ( .@id ) {
			case 0:
			case 1:
			case 3:
				pctalk "I've got a feeling. There's something here.";
				progressbar_npc "3131FF", 1;
				pctalk "I thought that there would be something here. I should search again.";
				break;
			case 2:
				pctalk "This one looks different.";
				progressbar_npc "3131FF", 1;
				pctalk "This thing... it looks like some kind of spore?? I should take it.";
				erasequest 17911;
				getitem "Ch1_Sample", 1;
				if ( isbegin_quest(17912) == 0 ) {
					setquest 17916;
				}
				break;
		}
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17911) == 1";
end;
}

ch1_dw,192,84,3	duplicate(Advanced Device#c01ms30)	Advanced Device#c01ms31	4_POINT_WHITE
ch1_dw,217,87,3	duplicate(Advanced Device#c01ms30)	Advanced Device#c01ms32	4_POINT_WHITE
ch1_dw,210,90,3	duplicate(Advanced Device#c01ms30)	Advanced Device#c01ms33	4_POINT_WHITE


ch1_gef_in,75,66,3	script	Wandering Patient#rch1_01	1_M_02,{
	if ( isbegin_quest(17921) == 1 ) {
		cutin "ch1_saram06b.png",0;
		mes "[Wandering Patient]";
		mes "Oh, hello there. Are you the volunteer who came last time?";
		mes "Are you here to volunteer again?";
		next;
		cutin "",255;
		select("I'm here to see Doctor Neumann.");
		cutin "ch1_saram06b.png",0;
		mes "[Wandering Patient]";
		mes "Oh, the doctor isn't here right now. ......Bit. I just him a while ago.";
		mes "Maybe he left in a hurry. I came to ask him something.";
		next;
		mes "[Wandering Patient]";
		mes "I think he haven't went too far yet.";
		mes "I'm planning to wait around here for a bit.";
		next;
		cutin "",255;
		select("Huh? Now that you mention it....");
		cutin "ch1_saram06b.png",0;
		emotion ET_QUESTION, getnpcid(0,"Wandering Patient#rch1_01");
		mes "[Wandering Patient]";
		mes "Hm? What is it?";
		next;
		cutin "",255;
		select("It's nothing.");
		cutin "ch1_saram06b.png",0;
		emotion ET_BLABLA,getnpcid(0, "Wandering Patient#rch1_01");
		mes "[Wandering Patient]";
		mes "Eh, you startled me for a second there. I thought something happened.";
		mes "If it's really urgent, maybe try asking one of the volunteers.";
		next;
		cutin "",255;
		emotion ET_THINK, getcharid(3);
		mes "["+strcharinfo(0)+"]";
		mes "(The door at the 9 o'clock direction that was always locked....... It seems to be slightly opened)";
		next;
		mes "["+strcharinfo(0)+"]";
		mes "(This feels strange. I've got a bad feeling.... Should I go <NAVI>[inside]<INFO>ch1_gef_in.gat,26,70,0,101,0</INFO></NAVI> and take a look?)";
		erasequest 17921;
		setquest 16940;
		close3;
	}
	cutin "ch1_saram06b.png",0;
	mes "[Wandering Patient]";
	mes "I'm walking around to build up my strength.";
	mes "Wouldn't having some stamina help me endure the disease better?";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17921) == 1";
end;
}

ch1_gef_in,26,70,0	script	#rch1_hw11	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16940) == 1 ) {
		mes "[????]";
		mes "Last time..., nothing got resolved...!";
		next;
		mes "["+strcharinfo(0)+"]";
		mes "(What's that sound?)";
		next;
		mes "[????]";
		mes "I really tried...! ...I can't take it anymore!";
		next;
		mes "["+strcharinfo(0)+"]";
		mes "(It sounds like someone is arguing inside. I can't hear it clearly, but it sounds like the same person from last time. I should quietly open the door and sneak in inside... and listen from the <NAVI>[doorway]<INFO>ch1_gef_in.gat,26,34,0,101,0</INFO></NAVI>)";
		completequest 16940;
		setquest 16941;
		close;
	}
end;
}

ch1_gef_in,26,34,3	script(CLOAKED)	Doorway#rch1_01	4_POINT_RED,{
	if ( isbegin_quest(16941) == 1 ) {
		mes "[Henry Bauer]";
		mes "Becky... bring Becky back, I kept quiet all this time for Becky's sake.";
		mes "If Becky dies... I don't know what I'll do.";
		next;
		cutin "ch1_neumann02.png",2;
		mes "[Doctor Neumann]";
		mes ".......";
		next;
		cutin "",255;
		mes "[Henry Bauer]";
		mes "Bring her back! Please, I'm begging you. I'll do anything...";
		mes "I'm willing to do something even more horrible than anything that you've asked me to do, please save my daughter.";
		next;
		cutin "ch1_sick05.png",2;
		mes "[Becky Bauer]";
		mes "......Ugh..., Da....";
		next;
		cutin "",255;
		mes "[Henry Bauer]";
		mes "Becky, Becky! Are you conscious? Do you recognize me?";
		next;
		cutin "ch1_sick05.png",2;
		mes "[Becky Bauer]";
		mes "It hurts so much..., daddy.... My body feels strange....";
		next;
		cutin "",255;
		mes "[Henry Bauer]";
		mes "Doctor Neumann, please.... Isn't there anything you can do? Anything at all?";
		mes "My child is suffering like this, how can you just stand there and watch?";
		next;
		cutin "ch1_neumann02.png",2;
		mes "[Doctor Neumann]";
		mes ".......";
		next;
		cutin "ch1_sick05.png",2;
		mes "[Becky Bauer]";
		mes "Dad... I'm scared......!";
		cloaknpc("Becky Bauer#rch1_01", true, getcharid(0));
		cloaknpc("Little Legion#rch1_0", false, getcharid(0));
		specialeffect EF_POISONHIT, AREA, "Little Legion#rch1_0";
		cloaknpc("Little Legion#rch1_0", true, getcharid(0));
		cloaknpc("Becky Bauer#rch1_01", false, getcharid(0));
		sleep2 400;
		cloaknpc("Becky Bauer#rch1_01", true, getcharid(0));
		cloaknpc("Little Legion#rch1_0", false, getcharid(0));
		specialeffect EF_POISONHIT, AREA, "Little Legion#rch1_0";
		sleep2 400;
		cloaknpc("Little Legion#rch1_0", true, getcharid(0));
		cloaknpc("Becky Bauer#rch1_01", false, getcharid(0));
		sleep2 900;
		cloaknpc("Becky Bauer#rch1_01", true, getcharid(0));
		cloaknpc("Little Legion#rch1_0", false, getcharid(0));
		specialeffect EF_POISONHIT, AREA, "Little Legion#rch1_0";
		next;
		cutin "",255;
		emotion ET_HUK, getnpcid(0,"Henry Bauer#rch1_02");
		mes "[Henry Bauer]";
		mes "No! No...! I can't lose my daughter too!";
		mes "Becky! Becky, Becky Bauer!";
		next;
		cutin "ch1_neumann03.png",2;
		mes "[Doctor Neumann]";
		mes "Tsk, After all of that......!";
		next;
		cutin "",255;
		mes "[Henry Bauer]";
		mes "What are you doing! Don't touch my daughter!";
		mes "She's my daughter, my one and only daughter...!";
		next;
		cutin "ch1_neumann04.png",2;
		mes "[Doctor Neumann]";
		mes "......I'm sorry, Henry. We can't bring her back anymore...!";
		mes "We can't bring her back anymore!";
		cloaknpc("Little Legion#rch1_0", true, getcharid(0));
		cloaknpc("Doctor Neumann#rch1_04", true, getcharid(0));
		cloaknpc("Little Legion#rch1_0-1", false, getcharid(0));
		cloaknpc("Doctor Neumann#rch1_05", false, getcharid(0));
		next;
		cutin "",255;
		mes "[Henry Bauer]";
		mes "Let go of Becky! What are you doing to her!";
		mes "Not there, it's too cold there... She'll be scared...!";
		next;
		cutin "ch1_neumann04.png",2;
		mes "[Doctor Neumann]";
		mes "This isn't the first time we've seen this, Henry Bauer...!";
		mes "I'm truly sorry. But there's nothing we can't do anymore.";
		cloaknpc("Little Legion#rch1_0-1", true, getcharid(0));
		next;
		cutin "",255;
		mes "[Henry Bauer]";
		mes "No...!";
		mes "Becky, Becky...!";
		next;
		emotion ET_ANGER, getnpcid(0,"Henry Bauer#rch1_03");
		mes "[Henry Bauer]";
		mes "You... How dare you do this to me.";
		mes "Of all people, how could you do this to me!";
		cloaknpc("Henry Bauer#rch1_02", true, getcharid(0));
		cloaknpc("Henry Bauer#rch1_03", false, getcharid(0));
		next;
		cutin "ch1_neumann03.png",2;
		mes "[Doctor Neumann]";
		mes "I tried. You know that... You know I did.";
		mes "But there was nothing else I could've done. I wanted to save her too...!";
		next;
		cutin "",255;
		emotion ET_FRET, getnpcid(0,"Henry Bauer#rch1_03");
		mes "[Henry Bauer]";
		mes "Then go save her! Bring her back now!";
		mes "Come with me, we'll take responsiblity together!";
		next;
		cutin "ch1_neumann03.png",2;
		mes "[Doctor Neumann]";
		mes "Wait, Henry...!";
		mes "Let's calm down and talk..., Ugh!";
		cloaknpc("Doctor Neumann#rch1_05", true, getcharid(0));
		cloaknpc("Henry Bauer#rch1_03", true, getcharid(0));
		next;
		cutin "",255;
		mes "["+strcharinfo(0)+"]";
		mes "(Bauer dragged Doctor Neumann into the place where the Immortal Corpss are...)";
		next;
		mes "["+strcharinfo(0)+"]";
		mes "(This is bad, wherever that is, I need to follow them through the <NAVI>[door]<INFO>ch1_gef_in.gat,29,22,0,101,0</INFO></NAVI> and save him...!)";
		completequest 16941;
		setquest 16942;
		close;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16941) == 1";
end;
}

ch1_gef_in,32,32,7	script(CLOAKED)	Henry Bauer#rch1_02	4_M_MASKMAN,{
	mes "[Henry Bauer]";
	mes "Please save my daughter...";
	close;
}

ch1_gef_in,35,30,1	script(CLOAKED)	Doctor Neumann#rch1_04	4_CH1_NEUMANN,{
	cutin "ch1_neumann04.png",2;
	mes "[Doctor Neumann]";
	mes "Calm down, calm down.";
	close3;
}

ch1_gef_in,35,32,3	script(CLOAKED)	Becky Bauer#rch1_01	4_PATIEN_KID,{
	cutin "ch1_sick05.png",2;
	mes "[Becky Bauer]";
	mes "Dad, my body feels weird....";
	close3;
}

ch1_gef_in,35,32,3	script(CLOAKED)	Little Legion#rch1_0	CH1_IMMORTAL_CORPS2,{
	end;
}

ch1_gef_in,30,24,3	script(CLOAKED)	Little Legion#rch1_0-1	CH1_IMMORTAL_CORPS2,{
	end;
}

ch1_gef_in,29,24,7	script(CLOAKED)	Doctor Neumann#rch1_05	4_CH1_NEUMANN,{
	end;
}

ch1_gef_in,29,25,5	script(CLOAKED)	Henry Bauer#rch1_03	4_M_MASKMAN,{
	end;
}

ch1_gef_in,30,27,3	script(CLOAKED)	Einhar#rch1_10	4_4JOB_EINHAR,{
	mes "[Einhar]";
	mes "I'll follow then, so please hurry back and get someone to help us.";
	close3;
}

ch1_gef_in,29,29,3	script(CLOAKED)	#rch1_w2	1_SHADOW_NPC,1,1,{
	end;
	
OnTouch:
	if ( isbegin_quest(16943) == 1 ) {
		cloaknpc("Einhar#rch1_10", true, getcharid(0));
		erasequest 16943;
		setquest 16944;
		warp "ygg_edge", 136, 172;
		end;
	}
end;
}

ygg_fruit,35,72,0	script	#rch1_hw13	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16943) == 1 ) {
		erasequest 16943;
		setquest 16944;
	}
	if ( isbegin_quest(16944) == 1 ) {
		cloaknpc("Gregor#rch1_10", false, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_1", false, getcharid(0));
	}
end;
}

ygg_fruit,41,75,3	script(CLOAKED)	Gregor#rch1_10	4259,{
	if ( isbegin_quest(16944) == 1 ) {
		cutin "4job_gregor_03.png",1;
		mes "[Gregor]";
		mes "What's going on? You suddenly came in a rush.";
		mes "Catch your breath and tell us what's happening.";
		close3;
	}
	if ( isbegin_quest(16945) == 1 ) {
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "We were just talking about a moment ago.";
		mes "About what should we do... something like that.";
		close3;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ygg_fruit,40,72,3	script(CLOAKED)	Ascetic Jeon#rch1_1	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(16944) == 1 ) {
		cutin "ex_sa_masterj03.png",2;
		emotion ET_QUESTION, getnpcid(0,"Ascetic Jeon#rch1_1");
		mes "[Ascetic Jeon]";
		mes "What's the matter? It looks like something urgent has happened.";
		mes "...Hm, from your expression, it must be something important.";
		next;
		cutin "",255;
		select("You need to come with me immediately.");
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "I don't know what's going on, but it looks like you need us.";
		mes "Shall we talk on the way?";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "No, atleast let us know what we need to prepare....";
		mes "Wait, don't drag me along!";
		next;
		cutin "",255;
		select("I'll explain on the way.");
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "Good grief, it must be really urgent.";
		mes "Fine. Let's move and talk on the way. Lead the <NAVI>[way]<INFO>ch1_gef_in.gat,29,22,0,101,0</INFO></NAVI>.";
		erasequest 16944;
		setquest 16945;
		getitem "Ch1_Root_Coin",10;
		getexp 130609489, 4000000;
		close3;
	}
	if ( isbegin_quest(16945) == 1 ) {
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "I was just talking with Gregor.";
		mes "We were discussing... on what to do now.";
		close3;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16944) == 1";
end;
}

ch1_que01,138,118,0	script	#rch1_hw14	HIDDEN_WARP_NPC,4,4,{
	end;
	
OnTouch:
	if ( isbegin_quest(16946) == 1 ) {
		cloaknpc("Gregor#rch1_11", false, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_2", false, getcharid(0));	
	}
end;
}

ch1_que01,141,114,7	script(CLOAKED)	Gregor#rch1_11	4259,{
	if ( isbegin_quest(16946) == 1 ) {
		cloaknpc("Gregor#rch1_11", false, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_2", false, getcharid(0));
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "So this is why you urgently needed someone capable.";
		mes "I'm worried since Einhar went in alone.";
		next;
		cutin "ex_sa_masterj02.png",2;
		mes "[Ascetic Jeon]";
		mes "We better hurry after him.";
		mes "...... But, which directions should we go?";
		next;
		cutin "ex_sa_masterj03.png",2;
		emotion ET_SWEAT, getnpcid(0,"Ascetic Jeon#rch1_2");
		mes "[Ascetic Jeon]";
		mes "Something feels off... it feels like someone is watching us.";
		next;
		cutin "4job_gregor_01.png",1;
		emotion ET_HUK, getnpcid(0,"Gregor#rch1_11");
		mes "[Gregor]";
		mes "Ugh, I stepped on something.......";
		mes "A shoe? It's all tattered. Why is something like this here....";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Wait, there's something else here.";
		next;
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "The one beside the shoe?";
		next;
		cutin "4job_gregor_01.png",1;
		emotion ET_QUESTION, getnpcid(0,"Gregor#rch1_11");
		mes "[Gregor]";
		mes "It looks like a mark that someone left behind intentionally.";
		mes "I'd say it was made just a while ago since the mark is still fresh.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "A simple arrow mark. Well, it's not like it needs to be more advanced.";
		mes "It must be a mark that Einhar left for us.";
		next;
		cutin "ex_sa_masterj02.png",2;
		mes "[Ascetic Jeon]";
		mes "It seems so. I can also feel a divine energy coming from it.";
		mes "He's telling us to go that way. ...Oh, wait!";
		next;
		cutin "ex_sa_masterj03.png",2;
		cloaknpc("Immortal Corps#rch1-1", false, getcharid(0));
		cloaknpc("Immortal Corps#rch1-2", false, getcharid(0));
		cloaknpc("Immortal Corps#rch1-3", false, getcharid(0));
		cloaknpc("Immortal Corps#rch1-4", false, getcharid(0));
		mes "[Ascetic Jeon]";
		mes "Where did they come from?";
		mes "There's a lot of them. Everyone, be careful.";
		specialeffect EF_BEGINSPELL, AREA, "Ascetic Jeon#rch1_2";
		specialeffect EF_FIRESPLASHHIT, AREA, "Immortal Corps#rch1-1";
		specialeffect EF_FIRESPLASHHIT, AREA, "Immortal Corps#rch1-2";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Tch, they're slowing us down.......";
		mes "Things are as urgent as is.";
		specialeffect EF_BEGINSPELL2, AREA, "Gregor#rch1_11";
		specialeffect EF_WINDHIT, AREA, "Immortal Corps#rch1-3";
		specialeffect EF_WINDHIT, AREA, "Immortal Corps#rch1-4";
		next;
		cutin "ex_sa_masterj02.png",2;
		mes "[Ascetic Jeon]";
		mes "It won't take long to deal with them.";
		mes "Let's keep going.";
		cloaknpc("Immortal Corps#rch1-1", true, getcharid(0));
		cloaknpc("Immortal Corps#rch1-2", true, getcharid(0));
		cloaknpc("Immortal Corps#rch1-3", true, getcharid(0));
		cloaknpc("Immortal Corps#rch1-4", true, getcharid(0));
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "The <NAVI>[trail]<INFO>ch1_que01.gat,126,57,0,101,0</INFO></NAVI> is still crisp, we should catch up soon.";
		mes "Hurry, let's move.";
		completequest 16946;
		setquest 16947;
		close2;
		cutin "",255;
		cloaknpc("Gregor#rch1_11", true, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_2", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16946) == 1";
end;
}

ch1_que01,144,116,1	script(CLOAKED)	Ascetic Jeon#rch1_2	4_EXJOB_MASTER_J,{
	cutin "ex_sa_masterj03.png",2;
	mes "[Ascetic Jeon]";
	mes "There seems to be something here. Let's take a closer look before we move on.";
	close3;
}

ch1_que01,143,119,3	script(CLOAKED)	Immortal Corps#rch1-1	CH1_IMMORTAL_CORPS1,{
	end;
}

ch1_que01,146,116,3	script(CLOAKED)	Immortal Corps#rch1-2	CH1_IMMORTAL_CORPS2,{
	end;
}

ch1_que01,144,118,3	script(CLOAKED)	Immortal Corps#rch1-3	CH1_IMMORTAL_CORPS1,{
	end;
}

ch1_que01,146,119,3	script(CLOAKED)	Immortal Corps#rch1-4	CH1_IMMORTAL_CORPS2,{
	end;
}

ch1_que01,131,57,0	script	#rch1_hw15	HIDDEN_WARP_NPC,6,6,{
	end;
	
OnTouch:
	if ( isbegin_quest(16947) == 1 ) {
		cloaknpc("Gregor#rch1_12", false, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_3", false, getcharid(0));
	}
end;
}

ch1_que01,126,57,3	script(CLOAKED)	Ascetic Jeon#rch1_3	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(16947) == 1 ) {
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "Hm? I can also sense a divine energy here.......";
		mes "Ah, wait, I also sense a monster nearby.";
		next;
		cutin "4job_gregor_02.png",1;
		cloaknpc("Immortal Corps#rch1-5", false, getcharid(0));
		cloaknpc("Immortal Corps#rch1-6", false, getcharid(0));
		cloaknpc("Immortal Corps#rch1-7", false, getcharid(0));
		specialeffect EF_BEGINSPELL2, AREA, "Gregor#rch1_12";
		specialeffect EF_POISONHIT, AREA, "Immortal Corps#rch1-5";
		specialeffect EF_POISONHIT, AREA, "Immortal Corps#rch1-6";
		mes "[Gregor]";
		mes "They just keep coming....";
		mes "What a nuisance. What is even this place?";
		next;
		cutin "ex_sa_masterj03.png",2;
		specialeffect EF_BEGINSPELL, AREA, "Ascetic Jeon#rch1_3";
		specialeffect EF_CRITICALWOUND, AREA, "Immortal Corps#rch1-7";
		mes "[Ascetic Jeon]";
		mes "These is troublesome.";
		mes "Now... get out of my face!";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "...This is the first time I've seen a ascetic this annoyed.";
		mes "Though I can't speak otherwise.";
		cloaknpc("Immortal Corps#rch1-5", true, getcharid(0));
		cloaknpc("Immortal Corps#rch1-6", true, getcharid(0));
		cloaknpc("Immortal Corps#rch1-7", true, getcharid(0));
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Aside our battle, there are signs that there was a fight nearby.";
		mes "Looks like it wasn't an easy one.";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "And looks like he left another arrow using blood.";
		mes "......It's concerning whose blood it is. Let's hurry and follow the <NAVI>[trail]<INFO>ch1_que01.gat,51,46,0,101,0</INFO></NAVI>.";
		erasequest 16947;
		setquest 16948;
		close2;
		cutin "",255;
		cloaknpc("Gregor#rch1_12", true, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_3", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16947) == 1";
end;
}

ch1_que01,123,58,3	script(CLOAKED)	Gregor#rch1_12	4259,{
	cutin "4job_gregor_01.png",1;
	mes "[Gregor]";
	mes "Let's hurry and move quickly.";
	mes "...Hm? Have you found something? Looks like we missed it. Let's take a look.";
	close3;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_que01,122,53,7	script(CLOAKED)	Immortal Corps#rch1-5	CH1_IMMORTAL_CORPS1,{
	end;
}

ch1_que01,125,53,7	script(CLOAKED)	Immortal Corps#rch1-6	CH1_IMMORTAL_CORPS2,{
	end;
}

ch1_que01,123,54,7	script(CLOAKED)	Immortal Corps#rch1-7	CH1_IMMORTAL_CORPS1,{
	end;
}


ch1_que01,55,44,0	script	#rch1_hw16	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16948) == 1 ) {
		cloaknpc("Gregor#rch1_13", false, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_4", false, getcharid(0));
	}
end;
}

ch1_que01,51,46,7	script(CLOAKED)	Ascetic Jeon#rch1_4	4_EXJOB_MASTER_J,{
	if ( isbegin_quest(16948) == 1 ) {
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "This mark, what in the world does it mean.";
		mes "It's just a single line drawn across.";
		next;
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "It didn't carry the same amount of divine power unlike before, I almost missed it.";
		mes "Leaving such an incomplete trace like this....";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "I doubt it was out of laziness.";
		mes "...... He was probably in a hurry like we are now.";
		cloaknpc("Immortal Corps#rch1-8", false, getcharid(0));
		cloaknpc("Immortal Corps#rch1-9", false, getcharid(0));
		cloaknpc("Immortal Corps#rch1-10", false, getcharid(0));
		next;
		cutin "ex_sa_masterj03.png",2;
		specialeffect EF_BEGINSPELL, AREA, "Ascetic Jeon#rch1_4";
		specialeffect EF_FIREHIT2, AREA, "Immortal Corps#rch1-8";
		specialeffect EF_FIREHIT2, AREA, "Immortal Corps#rch1-9";
		mes "[Ascetic Jeon]";
		mes "We've had to fight our way here.......";
		mes "Where are all of these monsters coming from?";
		next;
		cutin "4job_gregor_02.png",1;
		specialeffect EF_BEGINSPELL2, AREA, "Gregor#rch1_13";
		mes "[Gregor]";
		mes "......Looking at the monsters, have you noticed something?";
		mes "The clothes are tattered... something is hanging from its body.";
		next;
		cutin "",255;
		select("I see it too.");
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "...We need to look into it more....";
		mes "What a horrifying situation.";
		next;
		cutin "ex_sa_masterj03.png",2;
		specialeffect EF_STIN4, AREA, "Immortal Corps#rch1-10";
		mes "[Ascetic Jeon]";
		mes "I hope that inquisitor is safe.";
		mes "He went through all of these monsters alone.";
		next;
		cutin "ex_sa_masterj01.png",2;
		cloaknpc("Immortal Corps#rch1-8", true, getcharid(0));
		cloaknpc("Immortal Corps#rch1-9", true, getcharid(0));
		cloaknpc("Immortal Corps#rch1-10", true, getcharid(0));
		mes "[Ascetic Jeon]";
		mes "We've finally cleared the area up.......";
		mes "Let's hurry and catch up.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Yes, let's move quickly and find <NAVI>[Einhar]<INFO>ch1_que01.gat,67,128,0,101,0</INFO></NAVI>.";
		erasequest 16948;
		setquest 16949;
		close2;
		cutin "",255;
		cloaknpc("Gregor#rch1_13", true, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_4", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16948) == 1";
end;
}

ch1_que01,53,49,5	script(CLOAKED)	Gregor#rch1_13	4259,{
	cutin "4job_gregor_02.png",1;
	mes "[Gregor]";
	mes "Woah, you spotted something like that.";
	mes "Master Jeon, you really have sharp eyes.";
	close3;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_que01,56,50,3	script(CLOAKED)	Immortal Corps#rch1-8	CH1_IMMORTAL_CORPS2,{
	end;
}

ch1_que01,57,48,3	script(CLOAKED)	Immortal Corps#rch1-9	CH1_IMMORTAL_CORPS1,{
	end;
}

ch1_que01,56,45,3	script(CLOAKED)	Immortal Corps#rch1-10	CH1_IMMORTAL_CORPS2,{
	end;
}

ch1_que01,59,120,0	script	#rch1_hw17	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16949) == 1 ) {
		cloaknpc("Gregor#rch1_14", false, getcharid(0));
		cloaknpc("Ascetic Jeon#rch1_5", false, getcharid(0));
		cloaknpc("Einhar#rch1_11", false, getcharid(0));
		cloaknpc("Corpse#rch1_01", false, getcharid(0));
		cloaknpc("Corpse#rch1_02", false, getcharid(0));
	}
end;
}

ch1_que01,67,128,7	script(CLOAKED)	Einhar#rch1_11	4_4JOB_EINHAR,{
	if ( isbegin_quest(16949) == 1 ) {
		cutin "4job_einhar_01.png",2;
		emotion ET_THINK, getnpcid(0,"Einhar#rch1_11");
		mes "[Einhar]";
		mes ".......";
		mes "You're late.";
		next;
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "Hm, what is this.......";
		mes "What happened here. Were we too late?";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Oh, I'm not blaming you. I was late too.";
		mes "I misjudged things.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "I didn't know what the best course of action was.......";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "Calm down and explain.";
		mes "You can explain later.";
		next;
		cutin "ex_sa_masterj03.png",2;
		emotion ET_SWEAT, getnpcid(0,"Ascetic Jeon#rch1_5");
		mes "[Ascetic Jeon]";
		mes "Honestly, you always talk like that....";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "No. You're right. There's plenty of time to remorse later.";
		mes "I followed Doctor Neumann and Bauer, but lost them along the way.";
		next;
		mes "[Einhar]";
		mes "There were too many monsters, I tried to avoid fighting and focused everything on tracking them, but it wasn't enough.";
		next;
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "So, it was already like this when you found them.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "It must've been the monsters here.";
		mes "They weren't skilled on combat after all.";
		next;
		cutin "4job_gregor_02.png",1;
		mes "[Gregor]";
		mes "Weren't there signs of combat?";
		mes "Einhar, are you the one who left those marks?";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "...No.";
		mes "For some reason... there was an Immortal Corps that seemed to be fighting the others.";
		next;
		mes "[Einhar]";
		mes "It was alone and much smaller than others... but it ran away when it saw me.";
		next;
		cutin "",255;
		select(".......");
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "Judging by you reaction, you have some idea?";
		next;
		cutin "",255;
		select("No.", "I do, but it doesn't matter.");
		cutin "ex_sa_masterj01.png",2;
		mes "[Ascetic Jeon]";
		mes "......Is that so. Let's leave it at that.";
		next;
		cutin "ex_sa_masterj01.png",2;
		specialeffect EF_POTION8, AREA, "Ascetic Jeon#rch1_5";
		mes "[Ascetic Jeon]";
		mes "Let's take care of the corpses. Their souls deserved to be in peace.";
		mes "I hope they find rest in a peaceful place.";
		next;
		cutin "4job_gregor_01.png",1;
		specialeffect EF_POTION8, AREA, "Corpse#rch1_01";
		specialeffect EF_POTION8, AREA, "Corpse#rch1_02";
		mes "[Gregor]";
		mes "......We shouldn't stay here for too long.";
		mes "No one knew this place existed until now.";
		cloaknpc("Corpse#rch1_01", true, getcharid(0));
		cloaknpc("Corpse#rch1_02", true, getcharid(0));
		next;
		mes "[Gregor]";
		mes "We need to figure out what's going on.......";
		mes "For now, let's leave. We already found the ones we're looking for.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "We should do that. For the other people's sake... we can't be late.";
		mes "We need to find a solution, for this place and for the disease. Let's <NAVI>[move]<INFO>ch1_gef_in.gat,69,33,0,101,0</INFO></NAVI>.";
		erasequest 16949;
		setquest 16950;
		close3;
	}
	cutin "4job_einhar_01.png",2;
	mes "[Einhar]";
	mes "I'm a bit drained for a conversation right now. Sorry....";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16949) == 1";
end;
}

ch1_que01,63,125,7	script(CLOAKED)	Ascetic Jeon#rch1_5	4_EXJOB_MASTER_J,{
	cutin "ex_sa_masterj01.png",2;
	mes "[Ascetic Jeon]";
	mes "We finally caught up!";
	mes "Einhar, are you alright?";
	close3;
}

ch1_que01,70,131,3	script(CLOAKED)	Corpse#rch1_01	4_SOIL,{
	mes "[Corpse]";
	mes "(The tattered remains of their clothes are the only hint that they were once alive...)";
	close;
}

ch1_que01,71,129,3	script(CLOAKED)	Corpse#rch1_02	4_M_DSTMANDEAD,{
	mes "[Corpse]";
	mes "(This one has drawn their last breath... it can't speak no more. And perhaps, there's no more sorrow left to feel.)";
	close;
}

ch1_que01,63,129,7	script(CLOAKED)	Gregor#rch1_14	4259,{
	cutin "4job_gregor_01.png",1;
	mes "[Gregor]";
	mes "Oh, there's Einhar.";
	mes "But... it looks like he's alone.";
	close3;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ch1_gef_in,67,33,0	script	#rch1_hw18	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(16950) == 1 ) {
		cloaknpc("Maxim#rch1_1", false, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,69,33,3	script(CLOAKED)	Maxim#rch1_1	4_M_RUSBALD,{
	if ( isbegin_quest(16950) == 1 ) {
		cutin "ch1_maxim.png",2;
		emotion ET_PROFUSELY_SWAT, getnpcid(0,"Maxim#rch1_1");
		mes "[Maxim]";
		mes "I noticed that this place is locked all the time and came in, what happened here?";
		mes "Adventurer, and all of you... Why do you look all exhausted!";
		next;
		cutin "ex_sa_masterj03.png",2;
		mes "[Ascetic Jeon]";
		mes "Yes, we went through something extremely tiring.";
		mes "We'll probably be even more tired going forward.";
		next;
		cutin "4job_gregor_01.png",1;
		mes "[Gregor]";
		mes "It's not entirely bad news.";
		mes "What "+strcharinfo(0)+" witnessed might help us.";
		next;
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "What did you witness?";
		next;
		cutin "",255;
		switch ( select("About Neumann's secret", "About the secret underground", "About the secret of Immortal Corps") ) {
			case 1:
				cutin "ch1_maxim.png",2;
				mes "[Maxim]";
				mes "Are you telling me that Neumann is doing terrible things?";
				mes "Wasn't he the doctor that's protecting us all!";
				next;
				cutin "4job_gregor_02.png",1;
				mes "[Gregor]";
				mes "People don't just have one side to them.";
				mes "It's not always easy to understand someone's true intention.";
				break;
			case 2:
				cutin "ch1_maxim.png",2;
				mes "[Maxim]";
				mes "So there's a huge underground cavern down there?";
				mes "And the monsters down there is acting strange...?";
				next;
				cutin "ex_sa_masterj03.png",2;
				mes "[Ascetic Jeon]";
				mes "They seem like beings who haven't found peace.";
				mes "We don't know what exactly happened though.......";
				break;
			case 3:
				cutin "ch1_maxim.png",2;
				mes "[Maxim]";
				mes "The Immortal Corps, so they're below this city...!";
				mes "Why are things like that under our city?";
				next;
				cutin "4job_einhar_01.png",2;
				mes "[Einhar]";
				mes "......All of this thing happened with a reason or a cause.";
				mes "We need to uncover the reason.";
				break;
		}
		next;
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Honestly, I don't understand much of it! But, even I can tell that something is about to happen.......";
		mes "And you people are the one going to uncover it.";
		next;
		mes "[Maxim]";
		mes "Okay, I'll take responsiblity for taking care of the patients here!";
		mes "So, please find out what's going on.......";
		next;
		mes "[Maxim]";
		mes "Just so the others can understand what's happening, can you make a detailed report after your investigation?";
		mes "We can't just sit around clueless about things!";
		next;
		cutin "",255;
		select("We will.");
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Thank you very much!";
		mes "You said it's dangerous down here. I'll keep it locked so no one wanders in. I'll open it in when you return.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "......I suddenly remember something that we need to investigate.";
		mes "There's the locked journal we left with Professor Telcos in Ashridge.";
		next;
		cutin "",255;
		select("The one we found in Doctor Neumann's house.......");
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Yes, we made a copy of it and I returned the original at Doctor Neumann's home.";
		mes "The result should be ready now.";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Let's go to <NAVI>[Professor Telcos]<INFO>ygg_edge.gat,209,209,0,101,0</INFO></NAVI>. He might have found something already.";
		mes "All of you... you also don't look well. Let's go back and regroup for now.";
		erasequest 16950;
		setquest 16951;
		getitem "Ch1_Root_Coin",10;
		getexp 130609489, 4000000;
		close2;
		cutin "",255;
		cloaknpc("Maxim#rch1_1", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16950) == 1";
end;
}

ygg_edge,209,209,5	script	Professor Telcos#C1DY	4_M_COSTELL,2,2,{
	if ( isbegin_quest(16951) == 1 ) {
		mes "[Telcos]";
		mes "Oh? You're that adventurer?";
		mes "Do you want to enter the virtual record that Einhar asked me to create?";
		next;
		mes "[Telcos]";
		mes "Well.. sure, you can go in.";
		mes "But let me warn you in advance, the record is a bit lacking. It's not perfectly recreated. So don't get your hopes up too much.";
		next;
		mes "[Telcos]";
		mes "But, you can experience the important events, and that alone should be more than enough.";
		next;
		mes "[Telcos]";
		mes "Ah, and only one person can enter at a time~? Actually, I made it.. so that is not necessary!";
		mes "There's no limit on the number of people. You can go in together or alone, it's up to you.";
		mes "Just don't fight with other participants inside!";
		next;
		mes "[Telcos]";
		mes "But hey, just so I don't get dragged into anything troublesome, let me say one last thing.";
		next;
		mes "[Telcos]";
		mes "Everything that happens in there is already the 「past」.";
		mes "So don't start spouting nonsense like 「I have to change things!」.";
		next;
		mes "[Telcos]";
		mes "Even if it feels like the people inside are talking to you?";
		mes "They're actually talking to the owner of the log, not to you. So don't get too into it, getting all emotional and stuff, just observe and come back out.";
		next;
		cloaknpc("Illusion Book#C1DY", false, getcharid(0));
		erasequest 16951;
		setquest 19225;
		mes "[Telcos]";
		mes "Well, that's all the advice I've got. If you're ready, go through the Book of Illusions over there.";
		next;
		mes "[Telcos]";
		mes "..And let me say it one more time, don't get too immersed. It'll just cause trouble for me. And go to the fountain to get out.";
		close;
	}
	if ( isbegin_quest(19225) == 1 ) {
		cloaknpc("Illusion Book#C1DY", false, getcharid(0));
		mes "[Telcos]";
		mes "If you're ready, go through the Illusion Book over there.";
		next;
		mes "[Telcos]";
		mes "..And let me say it one more time, don't get too immersed. It'll just cause trouble for me. And go to the fountain to get out.";
		close;
	}
	if ( isbegin_quest(19225) == 2 ) {
		cloaknpc("Illusion Book#C1DY", false, getcharid(0));
	}
	if ( isbegin_quest(19226) == 1 ) {
		mes "[Telcos]";
		mes "How was the record experience?";
		mes "Well, did you at least get some useful information out of it?";
		mes "Ah, but that's not really my concern. It was just my job to let you in.";
		next;
		mes "You explained what happened inside the Illusion Book.";
		next;
		mes "[Telcos]";
		mes "What kind of nonsense is that?";
		mes "That's the most ridiculous thing I've heard in the past year. It's impossible. You must've been seeing things.";
		next;
		mes "[Telcos]";
		mes "Anyway! Enough of that nonsense.";
		next;
		mes "[Telcos]";
		mes "Ah, right. Someone named Maxim asked me to pass along a message. What was it again..? He said that he'll be waiting at the decayed Geffen's city hall ..something like that. Looks like you've got more to do over there, eh?";
		next;
		mes "[Telcos]";
		mes "And hey, since the Illusion Book has already been created, I won't stop you if you want to go explore it again.";
		next;
		erasequest 19226;
		setquest 23271;
		getitem "Ch1_Root_Coin",10;
		getexp 195914233, 6000000;
		mes "[Telcos]";
		mes "Whether you go again or not, it's up to you. Not like I'll be bothered. So off you go~! Take care, come back or don't, whatever~!";
		close;
	}
	if ( isbegin_quest(23271) == 1 ) {
		mes "[Telcos]";
		mes "Maxim is waiting for you at the decayed Geffen's city hall. Go if you want to.";
		next;
		mes "[Telcos]";
		mes "And since the Illusion Book is already made, I won't stop you if you want to explore it again. Just talk to me if you do.";
		close;
	}
	mes "[Telcos]";
	mes "Hey, hey, Ah. Forget it.";
	mes "No matter what you say, I know you have no reason to be here. It's true. Really, Do you think I won't notice?";
	next;
	mes "[Telcos]";
	mes "Just look at your face ‘Uhh.. What am I supposed to do here?', don't you know? Are you getting what I mean? Now, if you're getting me, just turn around and go back. Okay? Please?";
	close;
	
OnTouch:
	if ( isbegin_quest(19225) == 2 ) {
		cloaknpc("Illusion Book#C1DY", false, getcharid(0));
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16951) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(19225) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(19226) == 1";
end;
}

ygg_edge,211,211,5	script(CLOAKED)	Illusion Book#C1DY	4_VR_BOOK_WHITE,{
	if ( isbegin_quest(19225) > 0 ) {
		mes "Illusion Book-????";
		mes "Interactive Illusion Book";
		setdialogalign(DIALOG_ALIGN_CENTER);
		mes "<FONT SIZE = 15><B>A certain doctor's record..</B></FONT>";
		mes "<FONT SIZE = 15><B>Geffen Recreation</B></FONT>";
		mes " ";
		mes "^4d4dffUnknown record, recreated by Telcos^000000";
		next;
		select("Enter the Record");
		warp "ch1_vrgef2", 162, 65;
		end;
	}
	end;
}

ch1_vrgef2,139,199,0	script	ON_OFF#mj_1	HIDDEN_WARP_NPC,10,10,{
	end;
	
OnTouch:
	if ( isbegin_quest(19225) == 1 && isbegin_quest(19227) == 0 ) {
		cloaknpc("Ziegler#p11_1", false, getcharid(0));
	}
end;
}

ch1_vrgef2,139,199,0	script(CLOAKED)	Ziegler#p11_1	VR_UNKOWN_MAN,{
	if ( isbegin_quest(19225) == 1 && isbegin_quest(19227) == 0 ) {
		mes "[Ziegler]";
		mes "You're someone I haven't seen around the village? Ah!";
		next;
		mes "[Ziegler]";
		mes "So, you must have stopped by Geffen during your travels and got stuck in here? It quite an unfortunate event.";
		next;
		mes "[Ziegler]";
		mes "Maybe it's fate that we've met like this. Should we introduce ourselves?";
		next;
		mes "[Ziegler]";
		mes "I'm Matthias Ziegler. Nice to meet you.";
		next;
		mes "[Ziegler]";
		mes "I work in clinic here in the village, though it's more like dealing with the sick elderly every day than actual medicine.";
		next;
		mes "[Ziegler]";
		mes "Ah! You're an adventurer, you say? So, not just a simple traveler?";
		next;
		mes "[Ziegler]";
		mes "An adventurer... how fantastic. It was my dream to become an adventurer when I was young.";
		next;
		mes "[Ziegler]";
		mes "By the way, are you feeling anything anywhere?";
		emotion ET_QUESTION, getcharid(3);
		sleep2 1000;
		emotion ET_SMILE, getnpcid(0,"Ziegler#p11_1");
		mes "Haha, don't look at me so strangely. It's a professional deformation. Professional deformation!";
		next;
		mes "[Ziegler]";
		mes "And, there's an plague going around in the village.. given my profession, I just had to ask.";
		next;
		emotion ET_CRY, getnpcid(0,"Ziegler#p11_1");
		mes "[Ziegler]";
		mes "After seeing nothing but sick people every day, it's been a while since I've met someone so healthy..";
		next;
		mes "[Ziegler]";
		mes "Oh no! It's already that time. I'm helping out with something the village is preparing right now.";
		next;
		setquest 19227;
		mes "[Ziegler]";
		mes "It was nice meeting you. If we get the chance, we'll meet again.";
		close2;
		cloaknpc("Ziegler#p11_1", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_NONE, "isbegin_quest(19225) == 1 && isbegin_quest(19227) == 0";
end;
}

ch1_vrgef2,190,137,0	script	ON_OFF#mj_2	HIDDEN_WARP_NPC,10,10,{
	end;
	
OnTouch:
	if ( isbegin_quest(19227) == 1 ) {
		cloaknpc("Ziegler#p11_2", false, getcharid(0));
	}
end;
}

ch1_vrgef2,190,137,0	script(CLOAKED)	Ziegler#p11_2	VR_UNKOWN_MAN,{
	if ( isbegin_quest(19227) == 1 ) {
		emotion ET_HUK, getnpcid(0,"Ziegler#p11_2");
		mes "[Ziegler]";
		mes "Oh?! It's you again? What a coincidence meeting you here again.";
		next;
		mes "[Ziegler]";
		mes "Ah- This is the second time we've met, we should at least have a cup of tea together.";
		next;
		mes "[Ziegler]";
		mes "Unfortunately, I'm already late for work.";
		next;
		mes "[Ziegler]";
		mes "In this huge village, there's a lot of injured people and the plague is spreading..";
		next;
		mes "[Ziegler]";
		mes "There are so few nurses, if I don't go, my young and inexperienced doctor friend who's staying at the clinic will struggle.";
		next;
		emotion ET_CRY, getnpcid(0,"Ziegler#p11_2");
		mes "[Ziegler]";
		mes "Ugh- that friend was supposed to attend the conference, and now look at the mess we're in.";
		next;
		completequest 19227;
		setquest 19228;
		mes "[Ziegler]";
		mes "Ah! This isn't the time for this.. I'll see you again later!";
		close2;
		cloaknpc("Ziegler#p11_2", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_NONE, "isbegin_quest(19227) == 1";
end;
}

ch1_vrgef2,50,93,0	script	ON_OFF#mj_3	HIDDEN_WARP_NPC,10,10,{
	end;
	
OnTouch:
	if ( isbegin_quest(19228) == 1 || isbegin_quest(19238) == 1 ) {
		cloaknpc("Ziegler#p11_3", false, getcharid(0));
	}
end;
}

ch1_vrgef2,50,93,3	script(CLOAKED)	Ziegler#p11_3	VR_UNKOWN_MAN,{
	if ( isbegin_quest(19228) == 1 ) {
		mes "[Ziegler]";
		mes "Oh? Yet again?";
		next;
		mes "[Ziegler]";
		mes "They say meeting someone three times is fate.. haha, sorry, but I'm already taken. Hahaha! How ridiculous, how ridiculous!";
		next;
		mes "[Ziegler]";
		mes "Ah! What am I doing here, you ask? I'm not sure if I should be saying this, but..";
		next;
		emotion ET_STARE_ABOUT, getnpcid(0,"Ziegler#p11_3");
		mes "[Ziegler]";
		mes "Ah, whatever- if there's a problem, come find me.";
		next;
		mes "[Ziegler]";
		mes "Right now, there's a quarantine and treatment center set up in an old inn where we are treating patients..";
		next;
		mes "[Ziegler]";
		mes "Before the treatment center was established, a patient who came to me with symptoms similar to the plague suddenly disappeared..";
		next;
		mes "[Ziegler]";
		mes "It's annoying, but I had to go all the way to the patient's house.";
		next;
		mes "[Ziegler]";
		mes "Ugh, how annoying- why does someone disappear and make a busy person come all the way here!";
		next;
		mes "[Ziegler]";
		mes "Anyway! This is the patient's house. When they disappeared, they didn't even lock the door, ugh- there's been a robbery and everything's a mess.";
		next;
		mes "[Ziegler]";
		mes "Well, I guess it doesn't matter to someone who's going to disappear..";
		emotion ET_SURPRISE, getnpcid(0,"Ziegler#p11_3");
		mes "By the way! You said you're an adventurer, right?";
		next;
		mes "[Ziegler]";
		mes "Would you go into that house with me? I need someone to look around the house from someone else's perspective..";
		next;
		erasequest 19228;
		setquest 19229;
		mes "[Ziegler]";
		mes "Ah! You'll do it?! Alright, they say strike while the iron's hot, let's go in quickly!";
		close2;
		cloaknpc("Ziegler#p11_3", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(19238) == 1 ) {
		mes "[Ziegler]";
		mes "Oh- you're out?";
		mes "Sigh- poor house owner.";
		next;
		mes "[Ziegler]";
		mes "I came here because the owner was suspected they disappeared while being a possible carrier of the plague..";
		next;
		mes "[Ziegler]";
		mes "To think they went missing because of a monster.. sigh.. I wish they had just run away because they were fed up with the village.";
		next;
		emotion ET_OHNO,getnpcid(0, "Ziegler#p11_3");
		mes "[Ziegler]";
		mes "... ... ... ...";
		next;
		emotion ET_WRAP, getnpcid(0,"Ziegler#p11_3");
		npctalk "Clap!", "Ziegler#p11_3", BC_SELF;
		mes "[Ziegler]";
		mes "Anyway! Thanks for your help.";
		mes "I'll be holed up for a while doing research, treating people, and writing reports.. who knows when I'll see sunlight again..";
		next;
		mes "[Ziegler]";
		mes "Well, not that you can see the sun anymore with all the thick fog since the incident anyway..";
		next;
		erasequest 19238;
		setquest 19239;
		mes "[Ziegler]";
		mes "I'm just complaining right now. If we get the chance to meet again, let's have a chat! Bye-!";
		close2;
		cloaknpc("Ziegler#p11_3", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_NONE, "isbegin_quest(19228) == 1";
	questinfo QTYPE_QUEST, QMARK_NONE, "isbegin_quest(19238) == 1";
end;
}

ch1_vrgef1,68,141,0	script	ON_OFF#mj_4	HIDDEN_WARP_NPC,7,7,{
	end;
	
OnTouch:
	if ( isbegin_quest(19229) == 1 || isbegin_quest(19230) == 1 ) {
		cloaknpc("Ziegler#p11_4", false, getcharid(0));
		end;
	}
end;
}

function	script	CH1_VRGEF_CHECK	{
	for ( .@i = 19231; .@i <= 19237; ++.@i ) {
		if ( isbegin_quest(.@i) < 2 )
			return false;
	}
	return true;
}

ch1_vrgef1,66,137,7	script(CLOAKED)	Ziegler#p11_4	VR_UNKOWN_MAN,{
	if ( isbegin_quest(19229) == 1 ) {
		mes "[Ziegler]";
		mes "Oh? We meet again?";
		mes "Hahaha, just kidding! I told you to come by. Of course I remember everything.";
		next;
		mes "[Ziegler]";
		mes "I'm really joking. Don't look at me like that.";
		next;
		mes "[Ziegler]";
		mes "Alright! I'll say what I need. I'll be checking around the room, can you also please check around the room.";
		next;
		erasequest 19229;
		setquest 19230;
		mes "[Ziegler]";
		mes "Tell me if you found something strange. Now? let's start!";
		close;
	}
	if ( isbegin_quest(19230) == 1 && CH1_VRGEF_CHECK() ) {
		mes "[Ziegler]";
		mes "Oh! adventurer, I was about to ask you..";
		next;
		mes "[Ziegler]";
		mes "You spoke to me in perfect timing with my thinking- It truly must be fate!";
		next;
		mes "[Ziegler]";
		mes "Whoa.. whoa.. calm down.. I'm joking! Just joking! I'm taken, you know.";
		next;
		mes "[Ziegler]";
		mes "Alright-alright! Back to business!";
		mes "I came here to gather the information I needed, and now I'm thinking of leaving..";
		next;
		mes "[Ziegler]";
		mes "What about you? Do you want to look around more? Or should we stop and leave?";
		next;
		select("Leave.", "I saw everything, so let's leave.", "Stop looking and quickly leave.");
		for ( .@i = 19230; .@i <= 19237; ++.@i ) {
			erasequest .@i;
		}
		setquest 19238;
		mes "[Ziegler]";
		mes "Alright, let's go. I'll lock the door.";
		next;
		mes "[Ziegler]";
		mes "No one's here, so I'll go lock it for them. Let's meet outside.";
		close2;
		cloaknpc("Ziegler#p11_4", true, getcharid(0));
		end;
	} else {
		mes "[Ziegler]";
		mes "Tell me if you found something strange";
		close;
	}
	end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_NONE, "isbegin_quest(19229) == 1";
	questinfo QTYPE_QUEST, QMARK_NONE, "isbegin_quest(19230) == 1 && CH1_VRGEF_CHECK()";
end;
}

ch1_vrgef1,75,140,0	script	Bed#DM_01	HIDDEN_NPC,{
	if ( isbegin_quest(19230) == 1 && isbegin_quest(19231) == 0 ) {
		mes "It's an untidy bed.";
		mes "It seems like it hasn't been used in a long time. It gives off a disorganized feeling.";
		next;
		while ( true ) {
			mes "The bed is placed with the headrest against the wall.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "Doesn't it usually look like that? Keep looking around.";
				next;
			}
			mes "The bed frame seems to be made of stone.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "A stone bed is more expensive than a wooden one, but it's not that strange, right? Keep looking around.";
				next;
			}
			mes "The blanket is red with vertical stripes. It seems a bit faded, probably due to long use.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "Everyone uses their blanket for a long time, no? It's not strange for it to be worn out. Keep looking around.";
				next;
			}
			mes "The pillow is covered in dried sweat, probably from a lot of sweating from when it was last used.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "It's possible the owner suffered from a high fever. though it's not the main symptom of the plague..";
				next;
				setquest 19231;
				completequest 19231;
				mes "[Ziegler]";
				mes "It seems they were really sick. Good job. Keep looking around for other things.";
				close;
			}
		}
	}
	mes "It's an untidy bed.";
	mes "It seems like it hasn't been used in a long time. It gives off a disorganized feeling.";
	next;
	mes "The bed has been thoroughly examined.";
	close;
}

ch1_vrgef1,62,140,0	script	Water Container#DM_02	HIDDEN_NPC,{
	if ( isbegin_quest(19230) == 1 && isbegin_quest(19232) == 0 ) {
		mes "A dried up water container.";
		mes "It's completely empty.";
		next;
		while ( true ) {
			mes "The water container is made of wood.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "A lot of people use willow wood for their container. Because it's cheap.. keep looking around.";
				next;
			}
			mes "There's a white ring where the water has dried along the edge of the water container.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "Looks like the water container was left open for a while? Maybe the house has been empty for a long time? Keep looking around.";
				next;
			}
			mes "Could they have drank water in a hurry? There are traces of water splashed around the water container.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "Mild thirst! That's the most common symptom of the plague.";
				next;
				setquest 19232;
				completequest 19232;
				mes "[Ziegler]";
				mes "It's looking more likely that the owner's illness was the plague. Good job. Keep looking around for other things.";
				close;
			}
		}
	}
	mes "A dried up water container.";
	mes "It's completely empty.";
	next;
	mes "I already examined the water container thoroughly.";
	close;
}

ch1_vrgef1,62,145,0	script	Clock#DM_03	HIDDEN_NPC,{
	if ( isbegin_quest(19230) == 1 && isbegin_quest(19233) == 0 ) {
		mes "An antique clock.";
		mes "The owner is gone, but the clock is still moving.";
		next;
		setquest 19233;
		completequest 19233;
		mes "[Ziegler]";
		mes "Whoa, whoa.. that clock is expensive. Let's not touch it carelessly. The owner probably only looked at it. Keep checking around.";
		close;
	}
	mes "An antique clock.";
	mes "The owner is gone, but the clock is still moving.";
	next;
	mes "Let's leave the clock alone.";
	close;
}

ch1_vrgef1,76,144,0	script	Desk#DM_04	HIDDEN_NPC,{
	if ( isbegin_quest(19230) == 1 && isbegin_quest(19234) == 0 ) {
		mes "A wooden chair and desk.";
		next;
		while ( true ) {
			mes "The wood on the waist and back areas is a different color. The backrest uses willow, but what kind of wood is used on the waist side?";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "That's just paint. It's not made from different wood. Keep looking around.";
				next;
			}
			mes "On the right side of the desk, there are books placed. The desk is clean, suggesting it hasn't been used that much.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "That book's a good one. It was trendy in Geffen for a while. Keep checking around.";
				next;
			}
			mes "There's a small piece of skin between the desk and the chair.";
			next;
			if ( select("Call Ziegler", "Keep examining.") == 1 ) {
				mes "[Ziegler]";
				mes "A piece of skin? Is the color close to RGB B37B5F? Ah.. damn.. it looks like it belonged to the house owner..";
				next;
				mes "[Ziegler]";
				mes "Tsk.. there are rumors lately that monsters eat people thoroughly, and you're considered lucky if even a single piece of skin got left behind..";
				next;
				setquest 19234;
				completequest 19234;
				mes "[Ziegler]";
				mes "Damn it.. looks like the owner may have been eaten by a monster. Good job. It's grim, but keep checking around.";
				close;
			}
		}
	}
	mes "A wooden chair and desk.";
	next;
	mes "I have examined this area thoroughly.";
	close;
}

ch1_vrgef1,77,137,0	script	Wardrobe#DM_05	HIDDEN_NPC,{
	if ( isbegin_quest(19230) == 1 && isbegin_quest(19235) == 0 ) {
		mes "A small solid wood wardrobe.";
		mes "There's no clothes inside it, perhaps someone rummaged through it while the owner was away.";
		next;
		setquest 19235;
		completequest 19235;
		mes "[Ziegler]";
		mes "There's nothing left there. Check something else.";
		close;
	}
	mes "A small solid wood wardrobe.";
	mes "There's no clothes inside it, perhaps someone rummaged through it while the owner was away.";
	next;
	mes "It's completely empty.";
	close;
}

ch1_vrgef1,63,137,0	script	Barrels#DM_06	HIDDEN_NPC,{
	if ( isbegin_quest(19230) == 1 && isbegin_quest(19236) == 0 ) {
		mes "Small empty barrels.";
		mes "It's unclear why they weren't thrown out and were kept instead.";
		next;
		setquest 19236;
		completequest 19236;
		mes "[Ziegler]";
		mes "Some people just can't throw things away. Leave that and check something else.";
		close;
	}
	mes "Small empty barrels.";
	mes "It's unclear why they weren't thrown out and were kept instead.";
	next;
	mes "There's a lot. That's all there is to it.";
	close;
}

ch1_vrgef1,67,145,0	script	Shelf#DM_07	HIDDEN_NPC,{
	if ( isbegin_quest(19230) == 1 && isbegin_quest(19237) == 0 ) {
		mes "A shelf with all sorts of things placed on it.";
		mes "A wide variety of items are piled on the shelf.";
		next;
		setquest 19237;
		completequest 19237;
		mes "[Ziegler]";
		mes "Looks like the house owner didn't take the medicine I prescribed. Leave that and check something else.";
		close;
	}
	mes "A shelf with all sorts of things placed on it.";
	mes "A wide variety of items are piled on the shelf.";
	next;
	mes "Looking at it too long makes your head feel scattered.";
	close;
}


ch1_vrgef2,173,64,0	script	ON_OFF#mj_5	HIDDEN_WARP_NPC,7,7,{
	end;
	
OnTouch:
	if ( isbegin_quest(19240) == 1 ) {
		cloaknpc("Ziegler#p11_5", false, getcharid(0));
	}
end;
}

ch1_vrgef2,173,64,5	script(CLOAKED)	Ziegler#p11_5	VR_UNKOWN_MAN,{
	if ( isbegin_quest(19240) == 1 ) {
		mes "[Ziegler]";
		mes "Yo- adventurer- long time no see-";
		next;
		mes "["+strcharinfo(0)+"]";
		mes "We just met a moment ago..";
		next;
		mes "[Ziegler]";
		mes "Ah- that was me.. and yet not me? Maybe it's my final moment?";
		next;
		mes "[Ziegler]";
		mes "Ah! I already realized that this world was created by someone, am I right?";
		next;
		mes "[Ziegler]";
		mes "That must be it. Otherwise, there's no way to explain what's happening now.";
		next;
		mes "[Ziegler]";
		mes "I can't explain it clearly, but it feels like this world is turning and repeating because of me?";
		next;
		mes "[Ziegler]";
		mes "I don't know what purpose this world was created for..";
		next;
		mes "[Ziegler]";
		mes "But if there's a purpose to its creation, then you entering this world must have a reason too.";
		next;
		mes "[Ziegler]";
		mes "Haha- it's time for this simulated world to reset.";
		next;
		mes "[Ziegler]";
		mes "And I'll forget everything, meet someone else, and greet them with the same action.";
		next;
		mes "[Ziegler]";
		mes "I really feel like I should say goodbye, but I don't know when this simulation wiull end.";
		next;
		completequest 19225;
		erasequest 19227;
		erasequest 19240;
		setquest 19226;
		mes "[Ziegler]";
		mes "It was nice meeting you, and let's not meet again! Goodbye! Take care!";
		close;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_NONE, "isbegin_quest(19240) == 1";
	questinfo QTYPE_QUEST, QMARK_NONE, "isbegin_quest(19230) == 1 && CH1_VRGEF_CHECK()";
end;
}

function	script	CH1_Resident_Check	{
	for ( .@i = 23272; .@i <= 23274; ++.@i ) {
		if ( isbegin_quest(.@i) != 2 )
			return;
	}
	next;
	erasequest 23272;
	erasequest 23273;
	erasequest 23274;
	mes "It seems I've heard enough about the doctor, so let's return to <NAVI>[Maxim]<INFO>ch1_gef_in,172,187,0,101,0</INFO></NAVI>.";
	setquest 23275;
	close;
}

ch1_geffen,183,161,3	script	Geffen Resident#ch1_t_1	1_F_02,{
	if ( isbegin_quest(23272) == 1 ) {
		mes "[Geffen Resident]";
		mes "You want to know what kind of person the doctor was? That's strange thing to be curious about.";
		next;
		mes "[Geffen Resident]";
		mes "What about him? He definitely wasn't originally from Geffen. I just remember him suddenly showing up one day and starting to treat people.";
		next;
		mes "[Geffen Resident]";
		mes "Even now during this tough times, but it was even worse back then, so I think we were just grateful that someone was willing to help us.";
		next;
		mes "[Geffen Resident]";
		mes "We never wondered who he was, where he came from, or why he was helping us. We just assumed it was natural because he was a doctor.";
		next;
		mes "[Geffen Resident]";
		mes "I'm not sure why, but he was pretty dedicated. It's not easy to work for others in a place where you have no ties.";
		next;
		mes "[Geffen Resident]";
		mes "Now that I think about it, maybe the doctor just came to Geffen driven by a noble purpose to save lives?";
		next;
		mes "[Geffen Resident]";
		mes "He treated people here for quite a long time, and yet I barely know anything about him. Now that I think about it, I feel kind of bad...";
		completequest 23272;
		CH1_Resident_Check();
		close;
	}
	if ( isbegin_quest(23272) == 2 ) {
		mes "[Geffen Resident]";
		mes "Sorry, even after thinking it over, I still don't know much about the doctor. If anything comes to mind, I'll let you know.";
		close;
	}
	mes "[Geffen Resident]";
	mes "The weather is a bit gloomy, it's possible that it might rain...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23272) == 1";
end;
}

ch1_geffen,61,92,3	script	Geffen Resident#ch1_t_2	4_M_03,{
	if ( isbegin_quest(23273) == 1 ) {
		mes "[Geffen Resident]";
		mes "What's this all of a sudden? You've got something to ask me? That's unusual... I'm not sure if I know anything useful for an adventurer like you.";
		next;
		mes "[Geffen Resident]";
		mes "So, you want to know about the doctor? Hah, just as I thought. I'll probably only end up giving you some meaningless information.";
		next;
		mes "[Geffen Resident]";
		mes "I don't really have anything specific to tell you, I can offer a lot if you're up for some idle talk.";
		next;
		mes "[Geffen Resident]";
		mes "I don't think the doctor was originally from Geffen. As you know, he just showed up one day out of nowhere.";
		next;
		mes "[Geffen Resident]";
		mes "He never talked about where he was from, and he never said he had any ties to Geffen. If he had relatives here, I'm sure he would've mentioned them.";
		next;
		mes "[Geffen Resident]";
		mes "Before the Geffen Tower collapsed, people from outside would visit all the time for the academic conferences that were oftenly held here.";
		next;
		mes "[Geffen Resident]";
		mes "Back before the world turned out like this, it wasn't strange at all to see non-Geffen folks staying here.";
		next;
		mes "[Geffen Resident]";
		mes "Maybe he got stuck in Geffen after attending a conference, or maybe he came back when fleeing from a monster outside?";
		next;
		mes "[Geffen Resident]";
		mes "Then he decided to stay and do what he could. I think he's a remarkable person person";
		completequest 23273;
		CH1_Resident_Check();
		close;
	}
	if ( isbegin_quest(23273) == 2 ) {
		mes "[Geffen Resident]";
		mes "Haha. These days, all I can offer are small stories to pass the time. Would you care to hear one anyway?";
		close;
	}
	mes "[Geffen Resident]";
	mes "Is there any good news?";
	next;
	mes "[Geffen Resident]";
	mes "Geffen's atmosphere is gloomy... I want to hear something new.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23273) == 1";
end;
}

ch1_geffen,129,197,3	script	Geffen Resident#ch1_t3	1_F_03,{
	if ( isbegin_quest(23274) == 1 ) {
		mes "[Geffen Resident]";
		mes "The doctor, you say? Even if you ask me, I don't really know much. I wasn't personally close to him.";
		next;
		mes "[Geffen Resident]";
		mes "But actually, there was something I'd always been curious about too. I just kept it to myself, thinking it wasn't something I should pry into.";
		next;
		mes "[Geffen Resident]";
		mes "Can someone really devote all their time and effort for others purely out of the desire to save lives?";
		next;
		mes "[Geffen Resident]";
		mes "I don't have any medical knowledge, so I can't imagine how hard it must've been, but even if I were a doctor, I don't know if I could've done what he did.";
		next;
		mes "[Geffen Resident]";
		mes "What could've been his driving force to dedicate his time like that.";
		completequest 23274;
		CH1_Resident_Check();
		close;
	}
	if ( isbegin_quest(23274) == 2 ) {
		mes "[Geffen Resident]";
		mes "Even thinking it over again, I still can't figure it out......";
		close;
	}
	mes "[Geffen Resident]";
	mes "How am I supposed to live from now? I need to find a way to survive.";
	close;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23274) == 1";
end;
}

ch1_gef_in,142,143,3	script	Nurse Margot#ch1_margot1	4_F_PATIENT01,{
	if ( isbegin_quest(23276) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Neumann?";
		next;
		mes "[Margot]";
		mes "......What? Who are you?";
		npctalk "Margot: I thought you were Neumann...", "", BC_SELF;
		next;
		mes "[Margot]";
		mes "I don't know what's going on, but it seems you've entered the wrong room. Don't tell me you're going to give me that old excuse about visiting a patient you don't even know?";
		next;
		mes "[Margot]";
		mes "If you leave now, I won't hold it against you for barging into someone else's room.";
		next;
		select("I came here to see you Mrs. Margot");
		mes "[Margot]";
		mes "...Do you have business with me? What possible reason would an adventurer have to visit a patient they've never met...";
		next;
		select("I want to know why Mr. Neumann treated people.");
		mes "[Margot]";
		mes "......";
		next;
		mes "[Margot]";
		mes "Doctor treats people, isn't that a natural thing to do? Please leave if you're just going to ask pointless questions.";
		next;
		mes "[Margot]";
		mes "I'm not in good shape and can't talk for long. I don't want to waste my strength on a useless conversation like this.";
		completequest 23276;
		setquest 23277;
		close3;
	}
	if ( isbegin_quest(23277) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Didn't I ask you to leave? There's a limit on how rude you can be... Is it because I'm bedridden that you're continuing to do this?";
		next;
		select("I know Mr. Neumann isn't a real doctor.");
		mes "[Margot]";
		mes "I see, so you came all the way here just to confirm something everyone already knows? How thoughtful of you.";
		npctalk "Margot: Ha!", "", BC_SELF;
		next;
		mes "[Margot]";
		mes "What else do you want? You already figured out Neumann wasn't a real doctor just fine without me. If you want to know more, go find it somewhere else like how you found it.";
		next;
		mes "[Margot]";
		mes "I'm a patient as you can see. I'm tired, I need to rest and only sit up to take medications, just leave.";
		next;
		mes "[Margot]";
		mes "Make sure to shut the door tightly on your way out. Even getting up from bed to close it is too exhausting for me.";
		next;
		mes "[Margot]";
		mes "Wasting my breath on a useless conversation with you is more tiring.";
		next;
		mes "[Margot]";
		mes "I think I've made myself clear. I won't see you out, goodbye.";
		erasequest 23277;
		setquest 23278;
		close3;
	}
	if ( isbegin_quest(23278) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Why are you still standing there? Didn't you hear what I said?  I told you I have nothing to say to you, so just leave!!";
		next;
		mes "[Margot]";
		mes "How much more do you have to torment me before you're satisfied? If you want to know about Neumann, ask the ones who took him!";
		next;
		mes "[Margot]";
		mes "Cough, cough...";
		next;
		mes "[Margot]";
		mes "There were others before you who snooped around the house. They didn't come into this room, but some of Neumann's records were missing after they left.";
		next;
		mes "[Margot]";
		mes "They already took what you're looking for. You're also with them too, aren't you? I don't get why you're even here talking to me...";
		next;
		mes "[Margot]";
		mes "Whatever you're looking for, haven't you got it already? So just go bother someone else..";
		next;
		mes "[Margot]";
		mes "Stop harassing innocent people and just leave me alone. I've said enough, I believe you understand it now.";
		erasequest 23278;
		setquest 23279;
		close3;
	}
	if ( isbegin_quest(23279) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "So persistent... Are you going to stay here until you get what you want?";
		next;
		mes "[Margot]";
		mes "How exhausting, really...";
		next;
		mes "[Margot]";
		mes "Fine, I'm at the end of my life anyway, what's the point of hiding anything now? I'll tell you what you want to hear, so listen carefully. And when I'm done, I want you to leave me alone for good.";
		next;
		mes "[Margot]";
		mes "...I'm not originally from Geffen. I was one of the refugees who ended up here while fleeing.";
		next;
		mes "[Margot]";
		mes "I figured I could make money by stealing and selling medical supplies, so I snuck into the hospital. And that's where I met Neumann.";
		next;
		mes "[Margot]";
		mes "He started pretending to be a doctor with the same idea as mine, and we were both caught off guard when we ran into each other.";
		next;
		mes "[Margot]";
		mes "I panicked and made up a sloppy lie, saying I was the new nurse starting today and asked if he hadn't gotten the message.";
		next;
		mes "[Margot]";
		mes "If Neumann was a real doctor that story wouldn't have worked, but he was a fake too.";
		next;
		mes "[Margot]";
		mes "Afraid of being exposed, he pretended to play it cool and said he'd simply forgotten because he was so busy. Then he hired me right on the spot.";
		next;
		mes "[Margot]";
		mes "And that's how the two of us ended up pretending to be a doctor and nurse, roles we were never meant to play.";
		next;
		mes "[Margot]";
		mes "Looking back, we were both laughably clumsy. At the time we were too overwhelmed just trying to act the part, so we never noticed that one or other was a fake. Were we just lucky?";
		next;
		mes "[Margot]";
		mes "...Talking this much all of a sudden is making my throat hurt. If you want to hear more, could you bring me some water?";
		next;
		mes "[Margot]";
		mes "There should be a kettle downstairs. I'd appreciate it if you could warm it to a lukewarm temperature. Cold water will make my condition worse.";
		erasequest 23279;
		setquest 23280;
		close3;
	}
	if ( isbegin_quest(23280) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Talking so much all of a sudden has made my throat sore... If you want to hear more, could you bring me some water from downstairs?";
		close3;
	}
	if ( isbegin_quest(23281) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Thank you. The temperature's just right.";
		next;
		mes "[Margot]";
		mes "...Where was I? Oh, right, I was talking about how we ended up settling in Geffen together.";
		next;
		mes "[Margot]";
		mes "We were both fakes, but maybe because of that, we managed to fool each other for quite a while. If even one of us had been real, our cover would've been exposed quickly.";
		next;
		mes "[Margot]";
		mes "If that incident hadn't happened, maybe we never would've known...";
		next;
		mes "[Margot]";
		mes "No one had any medical knowledge back then, so whether or not we were treating people properly didn't matter much. But since we weren't really treating them.";
		next;
		mes "[Margot]";
		mes "Naturally, one of our patients died. But then....";
		next;
		mes "[Margot]";
		mes "Back then, people in Geffen would burn the bodies of those who died from the plague. But Neumann and I both knew that the bodies must not be burned.";
		next;
		mes "[Margot]";
		mes "We both said what we knew... knew that the bodies shouldn't be burned, but that's then we realized something, how did we know that?";
		next;
		mes "[Margot]";
		mes "Only the refugees knew that burning the bodies was dangerous.";
		next;
		mes "[Margot]";
		mes "So we both realized that we had come to Geffen for the same reason and ended up pretending to be a doctor and a nurse. That night, we talked to each other for a long time about our pasts.";
		next;
		mes "[Margot]";
		mes "Do you mind if I take a short break? I'm out of breath...";
		erasequest 23281;
		setquest 23282;
		close3;
	}
	if ( isbegin_quest(23282) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "At first, I started this all for my own gain, do you know what the saying that actions shapes the person?";
		next;
		mes "[Margot]";
		mes "Pretending to care for the villagers made us slowly begin to truly want to help them.";
		next;
		mes "[Margot]";
		mes "We didn't have much medical knowledge, but as refugees, we knew a lot about the plague. So we even thought maybe we were the right people for this.";
		next;
		mes "[Margot]";
		mes "So even after we learned each other's true identity, we continued treating people just like before.";
		next;
		mes "[Margot]";
		mes "The only thing that changed was that we fell in love.";
		next;
		mes "[Margot]";
		mes "...And that I caught the disease.";
		next;
		mes "[Margot]";
		mes "......";
		next;
		select("Do you want to rest now?");
		mes "[Margot]";
		mes "It's not that. I'm just not sure if it's okay for me to keep talking...";
		next;
		mes "[Margot]";
		mes "I can't quite remember the details, it was something Neumann handled on his own, so I can't explain everything.";
		next;
		mes "[Margot]";
		mes "There's a record that Neumann wrote. Could you find it for me? I think I need to read it to decide how to continue the story.";
		next;
		mes "[Margot]";
		mes "It's hidden between the mattress <NAVI>[downstairs]<INFO>ch1_gef_in,112,175,0,101,0</INFO></NAVI>. If no one has taken it yet, there should be another one there.";
		erasequest 23282;
		setquest 23283;
		close3;
	}
	if ( isbegin_quest(23283) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "There's a record that Neumann wrote. Could you find it for me? I think I need to read it to decide how to continue the story.";
		next;
		mes "[Margot]";
		mes "It's hidden between the mattress <NAVI>[downstairs]<INFO>ch1_gef_in,112,175,0,101,0</INFO></NAVI>. If no one has taken it yet, there should be another one there.";
		close3;
	}
	if ( isbegin_quest(23284) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Ah, you found it. I'll read through it, can you please wait a moment?";
		next;
		mes "[Margot]";
		mes "Oh, so that's how it was...";
		next;
		select("Can you understand it?");
		mes "[Margot]";
		mes "Of course. Others couldn't because Neumann had terrible handwriting, but also because he used his real name here.";
		next;
		mes "[Margot]";
		mes "I see, after I got infected, he started using other patients as test subjects. It's not the kind of thing he could say openly, so he had no choice.";
		next;
		mes "[Margot]";
		mes "I was grateful that he worked day and night to save me, but at the same time, I felt sorry for the people he used as test subjects.";
		next;
		mes "[Margot]";
		mes "But, who can calmly accept their own death in a situation like this? I just kept telling myself that if it could lead to a cure, it would be all worth it.";
		next;
		mes "[Margot]";
		mes "I wonder I had stopped Neumann even once, would things turn out differently...";
		next;
		mes "[Margot]";
		mes "Now that Neumann is gone, staying silent about it feels meaningless.";
		next;
		mes "[Margot]";
		mes "I can't say for sure that Neumann's experiment was a success, but my disease definitely progressed slower compared to other patients.";
		next;
		mes "[Margot]";
		mes "Unless I was just born with a rare condition and got lucky, I think Neumann's methods must have helped to some extent.";
		next;
		mes "[Margot]";
		mes "The journal says Neumann primarily used the patient's blood for his research. Maybe using my blood for research would lead to a cure.";
		next;
		mes "[Margot]";
		mes "...Whether or not you'll use my blood is your decision. I need to rest while your decision.";
		next;
		mes "Margot has agreed to let her blood be used for research. Let's head to <NAVI>[Ashridge]<INFO>ygg_fruit,103,73,0,101,0</INFO></NAVI> and share the news.";
		erasequest 23284;
		setquest 23285;
		getitem "Ch1_Root_Coin",10;
		getexp 130609489, 4000000;
		close3;
	}
	if ( isbegin_quest(23285) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Whether or not you'll use my blood is your decision. I need to rest while you make your decision.";
		next;
		mes "Margot has agreed to let her blood be used for research. Let's head to <NAVI>[Ashridge]<INFO>ygg_fruit,103,73,0,101,0</INFO></NAVI> and share the news.";
		close3;
	}
	if ( isbegin_quest(23286) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Have you decided whether or not to use my blood to develop a cure?";
		mes "...Who's the person next to you? It's my first time seeing him, I'm sure that he's not from Geffen.";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "Hello? I'm an assistant of Doctor Gregor, the one working on a cure in Ashridge. I heard that you are willing to cooperate by giving a blood sample. Thank you so much.";
		next;
		mes "[Baily]";
		mes "I also heard your husband was researching a cure using the blood of infected patients. He may not have been a licensed doctor, but I believe his research has a real potential.";
		next;
		mes "[Baily]";
		mes "Letting us use your blood for research will be a big help. Thank you for making such a difficult decision.";
		next;
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "There's not much I can do anymore...";
		next;
		mes "[Margot]";
		mes "......even if just a little, I hope this will help ease the people's resentment toward Neumann.";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "We'll do our best to make that happen. Once the cure is complete, We will let the people of Geffen that Doctor Neumann truly did his best.";
		next;
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Thank you. I'm counting on you.";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "Alright, I'll draw a blood sample now. I'm going to tie a band around your arm, please let me know if it hurts.";
		next;
		mes "[Baily]";
		mes "Adventurer, would you mind waiting a bit while I draw the blood? It won't take long. I'm confident when it comes to this sort of thing.";
		erasequest 23286;
		setquest 23287;
		close;
	}
	if ( isbegin_quest(23287) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "...Did you say something? I didn't catch it.";
		next;
		mes "[Margot]";
		mes "Sorry, I think I feel a little dizzy, probably because of the blood draw. Let me rest for a bit.";
		close3;
	}
	if ( isbegin_quest(23291) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "......";
		next;
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Her condition has worsened to the point where talking is difficult. We'd better administer the cure right away. I just hope it works, I'm worried about her condition.";
		next;
		mes "[Maxim]";
		mes "I assume you know how to use the cure since you brought it? Show me how it's done first. That way, I can just follow your example when giving it to others.";
		next;
		select("Got it.");
		progressbar_npc "FFFF00", 3;
		mes "[Maxim]";
		mes "Ho... so that's how it's used. I see. It's simple enough, but I should write it down to make sure I don't forget. This cure is scarce, we have to use it with care.";
		next;
		mes "[Maxim]";
		mes "Shake it lightly before using...";
		mes "The proper dose is about this much...";
		next;
		mes "[Maxim]";
		mes "Alright, I've got it now. No problem even without you here. Don't worry about a thing. Hahaha";
		next;
		mes "[Maxim]";
		mes "Let's make sure Margot gets some rest first. No matter how good a cure is, it takes time for the effects to show.";
		next;
		mes "[Maxim]";
		mes "Now all that's left is to hope she recovers. And that's not something worrying will help, so let's just keep a positive mind and try to relax a bit.";
		next;
		mes "[Maxim]";
		mes "Come to think of it, you must've gone through a lot coming all the way from Ashridge to here. I'm truly grateful for everything you've done for Geffen.";
		next;
		mes "[Maxim]";
		mes "Actually, there's something I've been meaning to ask you, it feels a bit awkward to bring it up right away. Once you've caught your breath, come talk to me. It's nothing serious, so don't feel pressured!";
		erasequest 23291;
		setquest 23292;
		close3;
	}
	if ( isbegin_quest(23292) == 1 ) {
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "......";
		close3;
	}
	cutin "ch1_margot01.png",2;
	mes "[Margot]";
	mes "......";
	next;
	cutin "ch1_margot01.png",2;
	mes "Margot looks exhausted. Let's not disturb her.";
	close3;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23276) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23277) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23278) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23279) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23281) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23282) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23284) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23286) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23291) == 1";
end;
}

ch1_gef_in,107,173,0	script	#ch1_hw5	HIDDEN_WARP_NPC,4,4,{
	end;
	
OnTouch:
	if ( isbegin_quest(23280) == 1 ) {
		cloaknpc("Water Kettle#ch1_t_w", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(23283) == 1 ) {
		cloaknpc("Neumann's Record#ch1_t_nn", false, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,104,173,3	script(CLOAKED)	Water Kettle#ch1_t_w	4_POINT_WHITE,{
	if ( isbegin_quest(23280) == 1 ) {
		mes "It's a water kettle filled with water.";
		mes "Let's warm it to a suitable temperature and bring it.";
		next;
		progressbar_npc "FFFF00",3;
		mes "It's now warm enough to drink.";
		mes "Let's bring this water to <NAVI>[Margot]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI>.";
		erasequest 23280;
		setquest 23281;
		close;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23280) == 1";
end;
}

ch1_gef_in,112,175,3	script(CLOAKED)	Neumann's Record#ch1_t_nn	4_POINT_WHITE,{
	if ( isbegin_quest(23283) == 1 ) {
		mes "It looks this bed hasn't been used in a long time.";
		next;
		while ( true ) {
			switch ( select("Pull back the blanket", "Lift the mattress", "Move the pillow") ) {
				case 1:
					mes "Dust filled the room as you pull back the blanket.";
					mes "There's nothing under it.";
					break;
				case 2:
					mes "The mattress is worn and sunken from long use.";
					mes "When you lift it with force, several sheets filled with messy handwriting came out.";
					next;
					mes "It's impossible to tell what's written on them.";
					mes "There's probably no point in staring at them, let's bring them to <NAVI>[Margot]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI>.";
					erasequest 23283;
					setquest 23284;
					close;
				case 3:
					mes "You moved the pillow. The pillowcase looks like it hasn't been washed in a long time.";
					mes "There's nothing under the pillow.";
					break;
			}
			next;
		}
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23283) == 1";
end;
}

ygg_fruit,102,72,0	script	#ch1_hw4	HIDDEN_WARP_NPC,4,4,{
	end;
	
OnTouch:
	if ( isbegin_quest(23288) == 1 ) {
		cloaknpc("Assistant Baily#ch1_t_baily", true, getcharid(0));
		cloaknpc("Baily's Shelf#ch1_t_bs", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(23289) == 1 ) {
		cloaknpc("Gregor#ch1_t_pro", false, getcharid(0));
		end;
	}
end;
}

ygg_fruit,109,76,3	script(CLOAKED)	Baily's Shelf#ch1_t_bs	4_POINT_WHITE,{
	if ( isbegin_quest(23288) == 1 ) {
		mes "Baily's Shelf. It feels cold as if it was enchanted.";
		next;
		if ( select("Place the special blood sample", "Take a look inside") == 2 ) {
			mes "There's an empty spot that looks perfect for storing the special blood sample. This should be the place.";
			next;
		}
		mes "You placed the special blood sample on Baily's shelf.";
		next;
		cloaknpc("Gregor#ch1_t_pro", false, getcharid(0));
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "Baily, did we have any clean water supplies stocked here? I'm sure I saw them in the other lab, but I just can't find them no matter how hard I look. This is really strange...";
		next;
		mes "[Gregor]";
		mes "Baily? My assistant isn't here and you are here, that's strange. What brings you here?";
		next;
		select("I've collected a special blood sample.");
		mes "[Gregor]";
		mes "What? Aren't you fast? Who was it? Mind explaining what happened from the start?";
		next;
		mes "[Gregor]";
		mes "Hmm, I see. So he did all that research on his own to try to treat his wife's disease.";
		next;
		mes "[Gregor]";
		mes "Sounds like his efforts actually showed some results. If he had had someone working with him, maybe we could've developed the cure faster. It's a bit of a bitter story when you know the full picture.";
		next;
		mes "[Gregor]";
		mes "we need to push forward even harder with the cure's development to honor those who were lost. It's a heavy burden, but what else can we do? It's on our hands now.";
		next;
		mes "[Gregor]";
		mes "Oh, I almost forgot that I have something to tell you. To keep it short, we've decided to investigate the distorted Brimir next.";
		next;
		mes "[Gregor]";
		mes "We figured that the more places we gather clues, the better. So no complainining and just head down to Brimir this instant.";
		next;
		mes "[Gregor]";
		mes "We'll be focusing all our effort on developing the cure now, you'll hear more of the details after you regroup with the others in Brimir!";
		erasequest 23288;
		setquest 8955;
		getitem "Ch1_Root_Coin",10;
		getexp 130609489, 4000000;
		close3;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23288) == 1";
end;
}

ygg_fruit,105,73,3	script(CLOAKED)	Gregor#ch1_t_pro	4259,{
	if ( isbegin_quest(8955) == 1 ) {
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "We'll be focusing all our effort on developing the cure now, you'll hear more of the details after you regroup with the others in Brimir!";
		close3;
	}
	if ( isbegin_quest(23289) == 1 ) {
		cutin "4job_gregor_01.png",2;
		mes "[Gregorr]";
		mes "It feels like I'm already near the answer yet far, it's frustrating...";
		close3;
	}
	end;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 4259;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_HEADTOP, 402;
	setunitdata getnpcid(0), UNPC_HEADMIDDLE, 1086;
end;
}

ygg_fruit,103,73,5	script	Assistant Baily#ch1_t_baily	1_F_04,{
	if ( isbegin_quest(23285) == 1 ) {
		mes "[Baily]";
		mes "Oh, what brings you here? I'm the only one here right now... I'm just an assistant, so there's not much I can do to help. I'm sorry.";
		next;
		mes "[Baily]";
		mes "But if you have a message for the doctor, you can tell me. I can at least pass it along. And if it's important, feel free to come back later too.";
		next;
		mes "[Baily]";
		mes "Ah, you're talking about the patient in Geffen? I've heard about it, so I have a general idea of what's going on.";
		next;
		mes "[Baily]";
		mes "I see, so the person working as a doctor in Geffen was conducting experiments on his own? From what I gather, it seems like he achieved some degree of success.";
		next;
		mes "[Baily]";
		mes "It definitely needs further investigation, but it sounds promising so I'll make a note of it for now. You came all the way here just to share this? Thank you so much.";
		next;
		mes "[Baily]";
		mes "I see. So the patient whose disease's development was slowed down actually agreed to have their blood drawn? We were just about to discuss whether to request Geffen's cooperation once the doctor returned, what a perfect timing!";
		next;
		mes "[Baily]";
		mes "The blood from that patient sounds like it could be incredibly valuable for developing a cure. I don't think I can wait until the doctor gets back!";
		next;
		mes "[Baily]";
		mes "I'm capable of basic treatment and drawing blood, can we head to <NAVI>[Geffen]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI> right away? I'll grab the tools I need and follow you immediately.";
		erasequest 23285;
		setquest 23286;
		close;
	}
	if ( isbegin_quest(23286) == 1 ) {
		mes "[Baily]";
		mes "I'm capable of basic treatment and drawing blood, can we head to <NAVI>[Geffen]<INFO>ch1_gef_in,138,162,0,101,0</INFO></NAVI> right away? I'll grab the tools I need and follow you immediately.";
		close;
	}
	if ( isbegin_quest(23289) == 1 ) {
		mes "[Baily]";
		mes "Hello, Adventurer? We're right in the middle of making a prototype for the cure. After much discussion, we've managed to create a fairly decent test version.";
		next;
		mes "[Baily]";
		mes "We've been experimenting by slightly adjusting the proportions based on the established recipe, but the efficiency is inconsistent. It's not easy finding that perfect balance...";
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "Feels like we're so close to the answer, but we just can't pinpoint what's wrong. I'm getting frustrated about it and can't figure it out. My back also hurts.";
		next;
		select("Show the special Hal.");
		cutin "4job_gregor_02.png",2;
		mes "[Gregor]";
		mes "A special Hal? I'd like to hear more. The fact you brought it here must mean you believe it'll help with the cure, right? Very intriguing.";
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "Hmm, hmm. I see. It holds multiple energy but leans toward none, meaning it's almost like a non-elemental substance?";
		next;
		mes "[Gregor]";
		mes "With that power, we can balance magic, holy power, and medicinal potions! Alright, I completely get it. This feels right!";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "Professor, I'm not quite sure I follow yet...";
		next;
		cutin "4job_gregor_02.png",2;
		mes "[Gregor]";
		mes "Don't worry. We've already figured out all the formulas in our heads! Bailey, just keep doing what you've been doing and be our hands and feet.";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "Yes, I'll do exactly as you say, so please give the instructions. Should we start with the measurements?";
		next;
		mes "[Baily]";
		mes "Hehe, tasks like these are always the assistant's job. Please wait just a moment while we prepare the cure... it won't take long.";
		progressbar_npc "FFFF00",4;
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "Alright, proportions are exact. We've waited long enough after mixing for stabilization... now, if I drop the cure right here...";
		next;
		mes "[Gregor]";
		mes "......";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "......";
		next;
		mes "[Baily]";
		mes "Professor, I think... I think it worked. Am I just making a mistake because of my weak will and the desire to finally stop this long experiment?";
		next;
		cutin "4job_gregor_02.png",2;
		mes "[Gregor]";
		mes "No, Baily. After carefully checking it. I think we made the cure!!!!";
		next;
		mes "[Gregor]";
		mes "Adventurer, now's not the time to sit around. Didn't you bring more of that special Hal? We can make as much of the cure as we need!";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "Oh, the professor's really excited. It's understandable. Everyone gave it their all to make this cure.";
		next;
		mes "[Baily]";
		mes "I really want to see the effects myself! Honestly, I'd love to go to Geffen with you and witness it with my own eyes, but right now making even more is top priority.";
		next;
		cutin "4job_gregor_01.png",2;
		mes "[Gregor]";
		mes "Baily, once the special Hal arrives, let's start measuring immediately so we can complete the next batch. We can't waste a moment!";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "Looks like we won't be able to go ourselves. We'll give you everything we've made so far, so could you take the cure to Geffen and administer it to the patients?";
		next;
		mes "[Baily]";
		mes "Of course you'll want to share it with as many people as possible, but we can't guarantee its effect if the given dosage is wrong. We'll keep making more here, so please use it for the most urgent cases first.";
		next;
		mes "[Baily]";
		mes "I know you're busy, but please head straight to <NAVI>[Geffen]<INFO>ch1_gef_in,172,187,0,101,0</INFO></NAVI>! Thank you so much!";
		erasequest 23289;
		setquest 23290;
		close;
	}
	if ( isbegin_quest(23290) == 1 ) {
		mes "[Baily]";
		mes "I know you're busy, but please head straight to <NAVI>[Geffen]<INFO>ch1_gef_in,172,187,0,101,0</INFO></NAVI>! Thank you so much!";
		close;
	}
	mes "[Baily]";
	mes "When will the doctor be back...";
	mes "I've finished everything I was told to do...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23285) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23289) == 1";
end;
}

ch1_gef_in,138,145,0	script	#ch1_hw3	HIDDEN_WARP_NPC,4,4,{
	end;
	
OnTouch:
	if ( isbegin_quest(23286) == 1 || isbegin_quest(23287) == 1 ) {
		cloaknpc("Assistant Baily#ch1_t_baily_gef", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(23291) == 1 || isbegin_quest(23292) == 1 || isbegin_quest(23294) == 1 ) {
		cloaknpc("Maxim#ch1_t_maxim2", false, getcharid(0));
		end;
	}
end;
}

ch1_gef_in,139,142,7	script(CLOAKED)	Maxim#ch1_t_maxim2	4_M_RUSBALD,{
	if ( isbegin_quest(23291) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Adventurer, welcome! I'm dying to see if the cure works!";
		close3;
	}
	if ( isbegin_quest(23292) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Now all that's left is to observe if the cure works. Fortunately, I managed to record Margot's condition while you were away.";
		next;
		mes "[Maxim]";
		mes "That makes it perfect for comparing any signs of recovery! Keeping records really does come in handy. I'm glad I checked on her from time to time.";
		next;
		mes "[Maxim]";
		mes "So I think it's best if I'm the one to monitor the effects of the cure. There are things you just can't tell from records alone.";
		next;
		mes "[Maxim]";
		mes "I used to just stop by when I had the time,this time I'll stay by her side and monitor here closely without leaving.";
		next;
		mes "[Maxim]";
		mes "Margot's condition has worsened lately. We don't know how quickly things could change, we have to be cautious of side effects too now that we've used the cure.";
		next;
		mes "[Maxim]";
		mes "Don't worry! With the hope this cure has brought us, there's plenty I can do. Even if it doesn't work, I'm sure it will help easy the symptoms somehow.";
		next;
		mes "[Maxim]";
		mes "I'll handle everything here, so put your mind at ease.";
		mes "But... before you take a break, could you do me a small favor?";
		next;
		mes "[Maxim]";
		mes "It's nothing much, I'd like you to bring me the journal where I recorded Margo's condition. I forgot to grab it because I was in a rush. Haha!";
		next;
		mes "[Maxim]";
		mes "I don't think anything drastic will happen while I step away for a moment, but you never know, right? It would be best to be prepared for any situation.";
		next;
		mes "[Maxim]";
		mes "Since I've been the one watching over Margo all this time, I think I'd notice any changes more clearly. So I'm counting on you.";
		next;
		mes "[Maxim]";
		mes "You can find the <NAVI>[journal]<INFO>ch1_gef_in,175,186,0,101,0</INFO></NAVI> on the city hall where I was staying. There's no need to rush.";
		erasequest 23292;
		setquest 23293;
		close3;
	}
	if ( isbegin_quest(23293) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "You can find the <NAVI>[journal]<INFO>ch1_gef_in,175,186,0,101,0</INFO></NAVI> on the city hall where I was staying. There's no need to rush.";
		close3;
	}
	if ( isbegin_quest(23294) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Adventurer, you're back! I have some wonderful news. You have no idea how long I've been waiting for you to return.";
		next;
		mes "[Maxim]";
		mes "In that short time, Margo's condition has improved greatly. Her fever and cough have lessened, and I feel like her complexion looks much better too.";
		next;
		mes "[Maxim]";
		mes "Hand me the journal that you've brought quickly. I need to double-check and make sure it's not just my hopeful imagination getting the best of me.";
		next;
		mes "[Maxim]";
		mes "......";
		next;
		mes "[Maxim]";
		mes "......";
		mes "......";
		next;
		mes "[Maxim]";
		mes "......";
		mes "......";
		mes "......";
		next;
		mes "[Maxim]";
		mes "Just as I thought! Her temperature has definitely gone down, and the frequency of her coughing has noticeably decreased. We can safely say the cure is effective.";
		next;
		mes "[Maxim]";
		mes "Things had only been getting worse ever since I began recording her condition, that made me anxious, but now I can finally be at ease. I'm so glad the cure was completed before it was too late.";
		next;
		mes "[Maxim]";
		mes "I think we can confidently say the cure works. Let's spread this joyful news throughout Geffen. I can't wait to see the smiles on everyone's faces.";
		next;
		mes "[Maxim]";
		mes "Though I should be the one administering the cure as thanks for all your hard work, I trust you'll understand how eager I am to spread hope to as many people as quickly as possible.";
		next;
		mes "[Maxim]";
		mes "So let's divide the areas and begin distributing the cure. I've written down the areas that you'll be in charge of. I'm counting on you.";
		erasequest 23294;
		setquest 23295;
		setquest 23296;
		setquest 23297;
		close2;
		cutin "",255;
		open_quest_ui 23295;
		end;
	}
	if ( isbegin_quest(23295) == 1 ) {
		cutin "ch1_maxim.png",2;
		mes "[Maxim]";
		mes "Though I should be the one administering the cure as thanks for all your hard work, I trust you'll understand how eager I am to spread hope to as many people as quickly as possible.";
		next;
		mes "[Maxim]";
		mes "So let's divide the areas and begin distributing the cure. I've written down the areas that you'll be in charge of. I'm counting on you.";
		close2;
		cutin "",255;
		open_quest_ui 23295;
		end;
	}
	end;

	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23292) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23294) == 1";
end;
}

ch1_gef_in,140,141,7	script(CLOAKED)	Assistant Baily#ch1_t_baily_gef	1_F_04,{
	if ( isbegin_quest(23286) == 1 ) {
		mes "[Baily]";
		mes "Aren't you quick? I'm still preparing for the blood sample drawing. It won't take long until I get ready, so please wait a moment.";
		close;
	}
	if ( isbegin_quest(23287) == 1 ) {
		mes "[Baily]";
		mes "The blood drawing went well. Since she's been bedridden for a long time, I was worried something might go wrong, but thankfully nothing happened. Now we just need to take it back for research.";
		next;
		mes "[Baily]";
		mes "Thanks to Mrs. Margot's patience, we were able to finish in one go. It must've been difficult to give that much blood, but she handled it so well. Maybe it's because she used to be a nurse.";
		next;
		cutin "ch1_margot01.png",2;
		mes "[Margot]";
		mes "Oh, come on... it's not a big of a deal. I already said from the start that I was okay with giving my blood, enduring this much is nothing. Please make good use of it.";
		next;
		cutin "",255;
		mes "[Baily]";
		mes "Um, by the way... Mrs. Margot seems to be in worse condition than the other patients. I think it would be best if I stayed here and monitored her in case of an emergency.";
		next;
		mes "[Baily]";
		mes "And since I came all the way to Geffen, I'd like to do what I can to help not just Mrs. Margot but also the other patients here.";
		next;
		mes "[Baily]";
		mes "Besides, the doctor is away right now, so even if I went back to Ashridge alone with the blood, we wouldn't be able to start making the cure right away.";
		next;
		mes "[Baily]";
		mes "I'm thinking of staying in Geffen a little longer, but I'm also worried the blood we just drew might spoil while I'm here...";
		next;
		mes "[Baily]";
		mes "Would it be alright if I entrusted you with the blood, adventurer? I'd appreciate it if you could deliver Margot's blood to Ashridge.";
		next;
		mes "[Baily]";
		mes "I'm not planning to stay here so long that it would delay the medicine development, so you don't need to worry too much. There's only so much I can do anyway.";
		next;
		mes "[Baily]";
		mes "Do you remember the office where I was last time? We have a special <NAVI>[shelf]<INFO>ygg_fruit,109,76,0,101,0</INFO></NAVI>. Please place the blood over there. I'm counting on you.";
		erasequest 23287;
		setquest 23288;
		close;
	}
	if ( isbegin_quest(23288) == 1 ) {
		mes "[Baily]";
		mes "Do you remember the office where I was last time? We have a special <NAVI>[shelf]<INFO>ygg_fruit,109,76,0,101,0</INFO></NAVI>. Please place the blood over there. I'm counting on you.";
		close;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23287) == 1";
end;
}

uknw_ruin,264,237,0	script	#cpt1ground1	HIDDEN_WARP_NPC,2,2,{
	end;
	
OnTouch:
	if ( isbegin_quest(8955) == 1 ) {
		cloaknpc("Elise Isha#cpt0", false, getcharid(0));
		cloaknpc("Maura#cpt0", false, getcharid(0));
		end;
	}
end;
}

ch1_sf02,190,49,0	script	#sf02onoff01-1	HIDDEN_WARP_NPC,2,2,{
	end;
	
OnTouch:
	if ( isbegin_quest(8955) == 1 ) {
		cloaknpc("Elise Isha#cpt1", false, getcharid(0));
		cloaknpc("Maura#cpt1", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(8956) == 1 || isbegin_quest(8957) == 1 ) {
		cloaknpc("Elise Isha#cpt1", false, getcharid(0));
		cloaknpc("Maura#cpt1", false, getcharid(0));
		cloaknpc("One#cpt1", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(8958) == 1 ) {
		cloaknpc("Elise Isha#cpt1", false, getcharid(0));
		cloaknpc("Maura#cpt1", false, getcharid(0));
		cloaknpc("One#cpt1", false, getcharid(0));
		cloaknpc("Zero#cpt1", false, getcharid(0));
		end;
	}
end;
}

ch1_sf02,190,40,0	duplicate(#sf02onoff01-1)	#sf02onoff01-2	HIDDEN_WARP_NPC,2,2

uknw_ruin,256,238,5	script(CLOAKED)	Elise Isha#cpt0	4_F_ACOLYTE,{
	if ( isbegin_quest(8955) == 1 ) {
	OnEvent:
		cutin "acact_04.bmp",2;
		mes "[Elise]";
		mes "Ah, you're finally here.";
		mes strcharinfo(0)+", ...Uhm...";
		next;
		cutin "4job_maura_04.png",2;
		mes "[Maura]";
		mes "Let's move to a safe place first.";
		close2;
		warp "ch1_sf02", 190, 39;
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8955) == 1";
end;
}

uknw_ruin,258,234,7	script(CLOAKED)	Maura#cpt0	4_4JOB_MAURA,{
	if ( isbegin_quest(8955) == 1 ) {
		doevent "Elise Isha#cpt0::OnEvent";
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8955) == 1";
end;
}

ch1_sf02,185,40,7	script(CLOAKED)	Elise Isha#cpt1	4_F_ACOLYTE,{
	if ( isbegin_quest(8955) == 1 ) {
	OnEvent:
		cutin "acact_04.bmp",2;
		mes "[Elise]";
		mes  strcharinfo(0)+", ...Uhm...";
		next;
		select("You look like you're in trouble.");
		cutin "4job_maura_04.png",2;
		mes "[Maura]";
		mes strcharinfo(0)+"? So that's your name.";
		mes "Does Elise always ends up in situations like this?";
		mes "But I'm glad Elise still came here with me...";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "Oh, I'm Meister Maura Lagrange.";
		mes "Me and Elise has done bit of exploration here together and have already written a brief report.";
		next;
		select("What do you mean by this situation?");
		cutin "acact_04.bmp",2;
		mes "[Elise]";
		mes "I can barely use any divine power here...";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "Aside from that, there's nothing really wrong here.";
		next;
		cutin "4job_maura_04.png",2;
		mes "[Maura]";
		mes "Hey, that's a big deal for me.";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "How about you, Maura?";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "Well, I don't really use divine powers...";
		mes "But something feels strange here. There's no light source, yet there's light... and shadows...";
		next;
		mes "[Maura]";
		mes "When I looked around, I came to a conclusion that this place time is flowing at the speed of light... but something just feels off.";
		mes "Maybe I should return to Ashridge and file a report, also should I bring back my spectrophotometer...?";
		next;
		erasequest 8955;
		setquest 8956;
		cloaknpc("One#cpt1", false, getcharid(0));
		specialeffect EF_BEGINSPELL3, AREA, "One#cpt1";
		cutin "ch1_valkyrie04.png",12;
		mes "[Valkyrie?]";
		mes "The time here is flowing steadily.";
		next;
		cutin "acact_04.bmp",14;
		emotion ET_HUK, getnpcid(0,"Elise Isha#cpt1");
		mes "[Elise]";
		mes "That startled me!";
		cutin "ch1_valkyrie04.png",11;
		close3;
	}
	if ( isbegin_quest(8956) == 1 ) {
		cutin "acact_04.bmp",2;
		mes "[Elise]";
		mes "I'm okay... I was just startled...";
		close3;
	}
	if ( isbegin_quest(8957) == 1 ) {
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "Valkyrie.";
		mes "Why can't I use my divine power here...?";
		next;
		mes "[Elise]";
		mes "The blessings that I normally use also doesn't work. It feels like...";
		next;
		cutin "ch1_valkyrie04.png",0;
		mes "[One]";
		mes "Being underwater? Someone said that before.";
		next;
		mes "[One]";
		mes "You're not a being of this dimension, so it will take a little more time for you to adjust to it. Don't worry.";
		mes "Is there a reason why you need to use divine power here?";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "Not really, maybe?";
		mes "Alright... I'll just wait for it.";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "How did you know that we came from another dimension?";
		mes "Like you knew it from the start...";
		next;
		cutin "",255;
		cloaknpc("Zero#cpt1", false, getcharid(0));
		specialeffect EF_BEGINSPELL3, AREA, "One#cpt1";
		cutin "ch1_valkyrie01.png",2;
		erasequest 8957;
		setquest 8958;
		mes "[Valkyrie?]";
		mes "Because you don't have wings.";
		npctalk "Elise : Ah, again!", "Elise Isha#cpt1", BC_SELF;
		close3;
	}
	if ( isbegin_quest(8958) == 1 ) {
		cutin "acact_04.bmp",2;
		mes "[Elise]";
		mes "Is surprising people a local tradition here?";
		close3;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8955) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8957) == 1";
end;
}

ch1_sf02,187,45,7	script(CLOAKED)	Maura#cpt1	4_4JOB_MAURA,{
	if ( isbegin_quest(8955) == 1 ) {
		doevent "Elise Isha#cpt1::OnEvent";
		end;
	}
	cutin "4job_maura_04.png",2;
	mes "[Maura]";
	mes "While I was exploring with Elise, we didn't encounter any being that's capable of communicating.";
	mes "Did he only show up because you're here? I really should've brought my spectrophotometer...";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8955) == 1";
end;
}

ch1_sf02,184,41,5	script(CLOAKED)	One#cpt1	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8956) == 1 ) {
		cutin "ch1_valkyrie04.png",0;
		mes "[Valkyrie?]";
		mes "Time is flowing, there is light. There is no sun, but something is shining.";
		mes "What that is, neither me nor you need to know.";
		next;
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "Valkyrie...? Should I call you that?";
		mes "Those wings...";
		next;
		cutin "ch1_valkyrie01.png",0;
		mes "[One]";
		mes "One.";
		mes "Call me One(1).";
		next;
		mes "[One]";
		mes "This place is a dumpster for the gods, it's where they swept their unfinished business.";
		next;
		cutin "ch1_valkyrie02.png",0;
		mes "[One]";
		mes "Clearing out the trash takes a lot of effort. The gods didn't want to bothered with such effort, so they just left it like this.";
		next;
		cutin "ch1_valkyrie03.png",0;
		mes "[One]";
		mes "It hasn't completely collapsed yet, but it's slowly breaking down on its own.";
		mes "It'll vanish without a trace after some time.";
		next;
		cutin "ch1_valkyrie02.png",0;
		mes "[One]";
		mes "There's nothing here worth picking up and taking to another dimension.";
		mes "You should leave now.";
		erasequest 8956;
		setquest 8957;
		close3;
	}
	if ( isbegin_quest(8957) == 1 ) {
		cutin "ch1_valkyrie02.png",0;
		mes "[One]";
		mes "There's nothing here worth picking up and taking to another dimension.";
		mes "You should leave now.";
		close3;
	}
	if ( isbegin_quest(8958) == 1 ) {
		cutin "ch1_valkyrie02.png",0;
		mes "[One]";
		mes "I said that you didn't need to be here.";
		close3;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8956) == 1";
end;
}

ch1_sf02,189,44,5	script(CLOAKED)	Zero#cpt1	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8958) == 1 ) {
		cutin "ch1_valkyrie04.png",2;
		mes "[Zero]";
		mes "You.";
		mes "You have no wings.";
		mes "We are not the same.";
		mes "I'm Zero(0).";
		mes "...";
		mes "That's what we are called.";
		next;
		cutin "4job_maura_01.png",2;
		emotion ET_AHA, getnpcid(0,"Maura#cpt1");
		mes "[Maura]";
		mes "Ah...";
		mes "Now I understand what Elise meant earlier when she said she couldn't feel any divine power.";
		next;
		mes "[Maura]";
		mes "One and Zero look the same to our eyes, but we can't just perceive their colors or features properly. Maybe their hair color is different?";
		next;
		mes "[Maura]";
		mes "Losing the ability to see that, so that's what it feels like to lose your divine power.";
		next;
		cutin "ch1_valkyrie04.png",2;
		mes "[Zero]";
		mes "...";
		mes "I'm glad.";
		mes "That you noticed.";
		next;
		cutin "4job_maura_01.png",2;
		mes "[Maura]";
		mes "This world is collapsing with all of its beings.";
		mes "So, "+strcharinfo(0)+". These people must have their distinct features... But this world already lacks the powers that can describe them.";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "There are no fairies, spirits, and humans...";
		next;
		cutin "ch1_valkyrie04.png",0;
		mes "[One]";
		mes "This world never had such things to begin with.";
		mes "Like I told you.";
		mes "You don't have wings.";
		next;
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "Oh, um...";
		mes "Are you two the only living beings here?";
		next;
		cutin "ch1_valkyrie04.png",2;
		mes "[Zero]";
		mes "Sometimes we see others.";
		mes "or...";
		mes "I don't know how to say it. One.";
		next;
		cutin "ch1_valkyrie02.png",0;
		mes "[One]";
		mes "There are others scattered around here.";
		mes "But their existence is fading away.";
		mes "Like one of the flickering stars in the sky, shining briefly, and fades away...";
		next;
		cutin "ch1_valkyrie01.png",0;
		mes "[One]";
		mes "...";
		mes "If you see someone who's wish is for you to end them, do honor their wish.";
		mes "We don't have authority to end ourselves, we are only granted time.";
		next;
		mes "[One]";
		mes "I'll be in the plaza, come find me if you need me.";
		next;
		cutin "ch1_valkyrie03.png",2;
		mes "[Zero]";
		mes "That's not for them to know.";
		mes "Not yet. You.";
		mes "Still have time.";
		next;
		specialeffect EF_BEGINSPELL3, AREA, "Zero#cpt1";
		specialeffect EF_BEGINSPELL3, AREA, "One#cpt1";
		cloaknpc("One#cpt1", true, getcharid(0));
		cloaknpc("Zero#cpt1", true, getcharid(0));
		cutin "4job_maura_01.png",2;
		mes "[Maura]";
		mes "The monsters around here, might have been valkyries.";
		mes "After all... it had colors.";
		mes "Let's go around and take another look.";
		mes "I want to hear your thoughts too.";
		completequest 8958;
		setquest 8959;
		close2;
		cutin "",255;
		cloaknpc("Elise Isha#cpt1", true, getcharid(0));
		cloaknpc("Maura#cpt1", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8958) == 1";
end;
}

function	script	CH1_RuinE_Check	{
	if ( isbegin_quest(8960) == 2 && isbegin_quest(8961) == 2 && isbegin_quest(8962) == 2 ) {
		return true;
	}
	return false;
}

uknw_ruin,150,150,0	script	#cpt1ground2	HIDDEN_WARP_NPC,5,5,{
	end;
	
OnTouch:
	if ( isbegin_quest(8965) == 2 ) {
		cloaknpc("Two#uknw1", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(8958) == 2 ) {
		cloaknpc("Dry Branch#cpt1", false, getcharid(0));
		end;
	}
end;
}

uknw_ruin,149,150,3	script(CLOAKED)	Dry Branch#cpt1	CH1_TULIP,{
	mes "You can see a hidden path behind the shriveled and motionless monster..";
	next;
	if ( select("Enter.", "Leave.") == 2 ) {
		mes "Let's come back when needed.";
		close;
	}
	warp "ch1_sf02", 110, 45;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8959) == 1 && isbegin_quest(8960) == 0";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8965) == 1";
end;
}

uknw_ruin,149,150,3	script(CLOAKED)	Two#uknw1	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8965) == 2 ) {
		mes "Let's go to a safe place.";
		next;
		if ( select("Enter.", "Leave.") == 1 ) {
			warp "ch1_sf02", 110, 45;
		}
		end;
	}
	
OnInit:
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8970) == 0 && checkquest(8971, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8970) == 0 && checkquest(8971, PLAYTIME) == 2";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8972) == 0 && isbegin_quest(8972) == 0 && checkquest(8971, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8972) == 0 && isbegin_quest(8972) == 0 && checkquest(8971, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8970, HUNTING) == 2 && countitem(\"aegis_1001975\") >= 20";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8972, HUNTING) == 2 && countitem(\"aegis_1001975\") >= 20";
end;
}

ch1_sf02,109,45,0	script	#sf02onoff02-2	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(8965) == 2 ) {
		cloaknpc("Two#cpt2", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(8958) == 2 ) {
		cloaknpc("!@#cpt1", false, getcharid(0));
		end;
	}
end;
}

ch1_sf02,114,46,3	script(CLOAKED)	!@#cpt1	CH1_TULIP,{
	if ( isbegin_quest(8959) == 1 && isbegin_quest(8960) == 0 ) {
		cloaknpc("Elise Isha#cpt2", false, getcharid(0));
		cloaknpc("Maura#cpt2", false, getcharid(0));
		mes "[!@]";
		mes "??? ?????.";
		mes "?? ?? ?????? ???";
		mes "??...";
		next;
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "'Witnessing this place abandoned by their god, made me realize how much our god love us...'";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "Oh...";
		next;
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "...things are complicated.";
		next;
		setquest 8960;
	}
	if ( isbegin_quest(8960) == 1 ) {
		cutin "",255;
		mes "[!@]";
		mes "...??...";
		next;
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "What is it?";
		next;
		cutin "",255;
		mes "[!@]";
		mes "...?????... ?????????.";
		mes "........???????....";
		next;
		select("What is it saying?");
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "...If I didn't mishear it, it's saying it wants to completely destroy and seal away this world, it's asking us to break its chains.";
		next;
		completequest 8960;
		if ( !CH1_RuinE_Check() ) {
			cutin "4job_maura_01.png",2;
			mes "[Maura]";
			mes "I feel like there are a few more of these beings around this place...";
			mes strcharinfo(0)+", let's find the next location.";
		} else {
			cutin "4job_maura_03.png",2;
			mes "[Maura]";
			mes "Maybe when Zero said not yet, he meant for us to find out and ask him?";
			mes "Thank you, Elise.";
			mes "Hey, "+strcharinfo(0)+". Let's go to the plaza.";
			next;
			select("I was about to go there anyway.");
			erasequest 8959;
			erasequest 8960;
			erasequest 8961;
			erasequest 8962;
			setquest 8963;
			cutin "4job_maura_02.png",2;
			mes "[Maura]";
			if ( Class != JOB_MEISTER ) {
				mes "Wow, with how decisive you are, I'm starting to wonder why you didn't become a Meister?";
			} else {
				mes "Wow, with how decisive you are, no wonder you are a Meister.";
			}
			mes "You are very dependable.";
			mes "Alright, lead the way!";
		}
		close2;
		cutin "",255;
		cloaknpc("Elise Isha#cpt2", true, getcharid(0));
		cloaknpc("Maura#cpt2", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(8965) == 1 ) {
		cloaknpc("Elise Isha#cpt2", false, getcharid(0));
		cloaknpc("Maura#cpt2", false, getcharid(0));
		mes "[!@]";
		mes "?? ??...";
		next;
		cloaknpc("!@#cpt1", true, getcharid(0));
		cloaknpc("Two#cpt2", false, getcharid(0));
		specialeffect EF_BEGINSPELL, AREA, "Two#cpt2";
		mes "[Two]";
		mes "Thank you.";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "It can speak now.";
		next;
		cutin "4job_maura_01.png",2;
		mes "[Maura]";
		mes "Yeah. I can head them now.";
		mes "And...";
		next;
		select("Is this your original form?");
		cutin "",255;
		mes "[Two]";
		mes "Yes.";
		next;
		cutin "4job_maura_01.png",2;
		mes "I thought breaking the chain would turn them into Tulips.";
		next;
		cutin "",255;
		mes "[Two]";
		mes "If I remain bounded and gradually lose consciousness, I would already been a Tulip.";
		mes "I had a little bit of my consciousness remained.";
		next;
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "The chains... who bounded them?";
		next;
		cutin "",255;
		mes "[Two]";
		mes "When the gods abandoned this place, they turned the others into a jailer.";
		mes "The rest were all labeled as sinners.";
		mes "Of course, even the jailers are sinners.";
		next;
		mes "[Two]";
		mes "A jailer's duty is to imprison sinners.";
		mes "That's all there is to it.";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "The valkyries were bounded into chains... and if left over time, you turn into flowers?";
		next;
		cutin "",255;
		mes "[Two]";
		mes "Wouldn't it be better for the world to be covered in flowers when everything ends?";
		next;
		cutin "acact_04.bmp",2;
		mes "[Elise]";
		mes "I feel like we're not quite speaking the same language anymore...";
		next;
		select("I think so too.");
		cutin "4job_maura_04.png",2;
		mes "[Maura]";
		mes "Alright! Next! Let's go to the next.";
		completequest 8965;
		setquest 8966;
		cloaknpc("Elise Isha#cpt2", true, getcharid(0));
		cloaknpc("Maura#cpt2", true, getcharid(0));
		close3;
	}
	mes "[!@]";
	mes "...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8959) == 1 && isbegin_quest(8960) == 0";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8960) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8965) == 1";
end;
}

ch1_sf02,114,46,3	script(CLOAKED)	Two#cpt2	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8968) < 2 ) {
		end;
	}
	switch ( checkquest(8971, PLAYTIME) ) {
		case -1:
			break;
		case 0:
		case 1:
			mes "[Two]";
			mes "I've prayed enough for now.";
			mes "To whom you ask?... I'm not sure.";
			close;
		case 2:
			erasequest 8971;
			break;
	}
	if ( isbegin_quest(8970) == 0 ) {
		mes "[Two]";
		mes "...";
		mes "I have a request.";
		mes "Can you bring me... Gray Petals.";
		next;
		mes "[Two]";
		mes "Destroy 50 Tulips and Calendulas.";
		mes "Then collect 20 Gray Petals from them.";
		mes "Bring them to me...";
		mes "I'll reward you for it.";
		next;
		if ( select("Accept.", "I have to go") == 2 ) {
			mes "[Two]";
			mes "I see.";
			mes "It's time for you to return to the wingless dimension.";
			close;
		}
		mes "[Two]";
		mes "How kind of you, wingless one.";
		setquest 8970;
		close;
	}
	if ( isbegin_quest(8970) == 1 ) {
		if ( checkquest(8970, HUNTING) < 2 || countitem("aegis_1001975") < 20 ) {
			mes "[Two]";
			mes "Destroy 50 Tulips and Calendulas.";
			mes "Then collect 20 Gray Petals from them.";
			mes "Bring them to me...";
			mes "I'll reward you for it.";
			close;
		}
		mes "[Two]";
		mes "...What kind of prayer should I offer for the Tulips who were remnants of once my friends?";
		mes "Rather, who should I offer my prayer...?";
		delitem "aegis_1001975", 20;
		completequest 8970;
		setquest 8971;
		getitem "Ch1_Root_Coin",10;
		getexp 82232200, 5724800;
		close;
	}
	
	//Don't ask me why kRO uses 2 quest ID
	if ( isbegin_quest(8972) == 0 ) {
		mes "[Two]";
		mes "...";
		mes "I have a request.";
		mes "Can you bring me... Gray Petals.";
		next;
		mes "[Two]";
		mes "Destroy 50 Tulips and Calendulas.";
		mes "Then collect 20 Gray Petals from them.";
		mes "Bring them to me...";
		mes "I'll reward you for it.";
		next;
		if ( select("Accept.", "I have to go") == 2 ) {
			mes "[Two]";
			mes "I see.";
			mes "It's time for you to return to the wingless dimension.";
			close;
		}
		mes "[Two]";
		mes "How kind of you, wingless one.";
		setquest 8972;
		close;
	}
	if ( isbegin_quest(8972) == 1 ) {
		if ( checkquest(8972, HUNTING) < 2 || countitem("aegis_1001975") < 20 ) {
			mes "[Two]";
			mes "Destroy 50 Tulips and Calendulas.";
			mes "Then collect 20 Gray Petals from them.";
			mes "Bring them to me...";
			mes "I'll reward you for it.";
			close;
		}
		mes "[Two]";
		mes "...What kind of prayer should I offer for the Tulips who were remnants of once my friends?";
		mes "Rather, who should I offer my prayer...?";
		delitem "aegis_1001975", 20;
		erasequest 8972;
		setquest 8971;
		getitem "Ch1_Root_Coin",10;
		getexp 82232200, 5724800;
		close;
	}
	end;
	
OnInit:
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8970) == 0 && checkquest(8971, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8970) == 0 && checkquest(8971, PLAYTIME) == 2";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8972) == 0 && isbegin_quest(8972) == 0 && checkquest(8971, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8972) == 0 && isbegin_quest(8972) == 0 && checkquest(8971, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8970, HUNTING) == 2 && countitem(\"aegis_1001975\") >= 20";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8972, HUNTING) == 2 && countitem(\"aegis_1001975\") >= 20";
end;
}

uknw_ruin,252,59,0	script	#cpt1ground3	HIDDEN_WARP_NPC,5,5,{
	end;
	
OnTouch:
	if ( isbegin_quest(8966) == 2 ) {
		cloaknpc("Three#uknw1", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(8958) == 2 ) {
		cloaknpc("Broken Branch#cpt1", false, getcharid(0));
		end;
	}
end;
}

uknw_ruin,252,59,3	script(CLOAKED)	Broken Branch#cpt1	CH1_CALENDULA,{
	mes "You can see a hidden path behind the broken and motionless monster.";
	next;
	if ( select("Enter.", "Leave.") == 2 ) {
		mes "Let's come back when needed.";
		close;
	}
	warp "ch1_sf02", 123, 271;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8959) == 1 && isbegin_quest(8961) == 0";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8966) == 1";
end;
}

uknw_ruin,252,59,3	script(CLOAKED)	Three#uknw1	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8965) == 2 ) {
		mes "To a quiet place.";
		mes "Let's move.";
		next;
		if ( select("Enter.", "Leave.") == 1 ) {
			warp "ch1_sf02", 123, 271;
		}
		end;
	}
	
OnInit:
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8973) == 0 && checkquest(8974, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8973) == 0 && checkquest(8974, PLAYTIME) == 2";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8973) == 2 && isbegin_quest(8975) == 0 && checkquest(8974, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8973) == 2 && isbegin_quest(8975) == 0 && checkquest(8974, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8973, HUNTING) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8975, HUNTING) == 2";
end;
}

ch1_sf02,111,271,0	script	#sf02onoff03-1	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(8967) == 2 ) {
		cloaknpc("Three#cpt2", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(8958) == 2 ) {
		cloaknpc("%^#cpt1", false, getcharid(0));
		end;
	}
end;
}

ch1_sf02,111,271,5	script(CLOAKED)	%^#cpt1	CH1_CALENDULA,{
	if ( isbegin_quest(8959) == 1 && isbegin_quest(8979) == 0 ) {
		cloaknpc("Elise Isha#cpt3", false, getcharid(0));
		cloaknpc("Maura#cpt3", false, getcharid(0));
		mes "[%^]";
		mes "??? ?????.";
		mes "?? ?? ?????? ???";
		mes "??...";
		next;
		cutin "4job_maura_01.png",2;
		mes "[Maura]";
		mes "That sound. Can you hear it?";
		mes "There's a faint sound ringing in my ear...";
		mes "I thought it's just a decaying sound but it's probably not.";
		next;
		cutin "4job_maura_02.png",2;
		mes "[Maura]";
		mes "Elise.";
		mes "How is your divine power?";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "...Ah!";
		mes "It's much better now.";
		mes "How do I describe it...?";
		mes "It's like when your ears pop and you started to regain your hearing again...";
		next;
		cutin "4job_maura_02.png",2;
		mes "[Maura]";
		mes "Great.";
		mes "Then use those ears to listen to what that monster or resident... is saying.";
		next;
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "...!!";
		mes "I'll give it a try.";
		next;
		mes "[Elise]";
		mes "...";
		mes "...";
		next;
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "...";
		mes "...";
		mes "I can hear it.";
		mes "It's a very faint sound... but it sounds like a prayer.";
		next;
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "...They're alive.";
		mes "And just like the Valkyries that we've met earlier, they know that we're not from this world.";
		mes "...";
		setquest 8979;
		completequest 8979;
		next;
	}
	if ( isbegin_quest(8959) == 1 && isbegin_quest(8979) == 2 && isbegin_quest(8961) == 0 ) {
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "'There is no trace of the gods here. But I remain to exist. This is the proof that I am a servant of god.'";
		next;
		mes "[Elise]";
		mes "'What an honor. With such little power remains, what woul I do... shall I offer it to the gods?'";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "Oh no...";
		next;
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "...I'm feeling complicated about it.";
		next;
		setquest 8961;
		completequest 8961;
		if ( !CH1_RuinE_Check() ) {
			cutin "4job_maura_01.png",2;
			mes "[Maura]";
			mes "I feel like there are a few more of these beings around this place...";
			mes strcharinfo(0)+", let's find the next location.";
		} else {
			cutin "4job_maura_03.png",2;
			mes "[Maura]";
			mes "Maybe when Zero said not yet, he meant for us to find out and ask him?";
			mes "Thank you, Elise.";
			mes "Hey, "+strcharinfo(0)+". Let's go to the plaza.";
			next;
			select("I was about to go there anyway.");
			erasequest 8959;
			erasequest 8960;
			erasequest 8961;
			erasequest 8962;
			setquest 8963;
			cutin "4job_maura_02.png",2;
			mes "[Maura]";
			if ( Class != JOB_MEISTER ) {
				mes "Wow, with how decisive you are, I'm starting to wonder why you didn't become a Meister?";
			} else {
				mes "Wow, with how decisive you are, no wonder you are a Meister.";
			}
			mes "You are very dependable.";
			mes "Alright, lead the way!";
		}
		close2;
		cutin "",255;
		cloaknpc("Elise Isha#cpt3", true, getcharid(0));
		cloaknpc("Maura#cpt3", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(8961) == 2 ) {
		mes "Let's go find another resident.";
		close;
	}
	if ( isbegin_quest(8966) == 1 ) {
		cloaknpc("Elise Isha#cpt3", false, getcharid(0));
		cloaknpc("Maura#cpt3", false, getcharid(0));
		mes "[%^]";
		mes "??... ??...?";
		next;
		cloaknpc("%^#cpt1", true, getcharid(0));
		cloaknpc("Three#cpt2", false, getcharid(0));
		specialeffect EF_BEGINSPELL, AREA, "Three#cpt2";
		mes "[Three]";
		mes "...I have returned.";
		next;
		select("Is this your original form?");
		mes "[Three]";
		mes "Yes.";
		next;
		mes "[Three]";
		mes "It's a little different.";
		mes "My color isn't what it used to be.";
		next;
		cutin "acact_03.bmp",2;
		mes "[Elise]";
		mes "Do you remember your name?";
		next;
		cutin "",255;
		mes "[Three]";
		mes "I'm not sure if names even matter anymore.";
		mes "Feel free to choose me anything.";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "Oh... then we'll call you Three.";
		mes "...";
		mes "How do you feel?";
		next;
		cutin "",255;
		mes "[Three]";
		mes "If you ask me... I'd just kneel down and pray, but someone came earlier.";
		next;
		mes "[Three]";
		mes "Asking me how to end this world faster.";
		next;
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "Atleast, you look better than before...";
		next;
		select("I agree");
		cutin "4job_maura_02.png",2;
		mes "[Maura]";
		mes "Well, Alright. Then! Let's move on.";
		completequest 8966;
		setquest 8967;
		close2;
		cutin "",255;
		cloaknpc("Elise Isha#cpt3", true, getcharid(0));
		cloaknpc("Maura#cpt3", true, getcharid(0));
		end;
	}
	mes "[%^]";
	mes "...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8959) == 1 && isbegin_quest(8961) == 0";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8966) == 1";
end;
}

ch1_sf02,111,271,5	script(CLOAKED)	Three#cpt2	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8968) < 2 ) {
		end;
	}
	switch ( checkquest(8974, PLAYTIME) ) {
		case -1:
			break;
		case 0:
		case 1:
			mes "[Three]";
			mes "I think I can make it through the day...";
			mes "Even if I have claw scratches.";
			close;
		case 2:
			erasequest 8974;
			break;
	}
	if ( isbegin_quest(8973) == 0 ) {
		mes "[Three]";
		mes "...";
		mes "I have a request.";
		mes "To you from another dimension.";
		next;
		mes "[Three]";
		mes "When I kneel here praying, they keep stepping on me... scratching me...";
		mes "Even if I have no will left, this I can't endure.";
		mes "Please destroy Nyaises.";
		mes "20 of it. Please.";
		next;
		if ( select("Accept.", "Sorry, I have to go.") == 2 ) {
			mes "[Three]";
			mes "I've forgotten what it feels like to be pressed for time.";
			close;
		}
		mes "[Three]";
		mes "If only they are just cute beings.";
		setquest 8973;
		close;
	}
	if ( isbegin_quest(8973) == 1 ) {
		if ( checkquest(8973, HUNTING) < 2 ) {
			mes "[Three]";
			mes "When I kneel here praying, they keep stepping on me... scratching me...";
			mes "Even if I have no will left, this I can't endure.";
			mes "Please destroy Nyaises.";
			mes "20 of it. Please.";
			close;
		}
		mes "[Three]";
		mes "Cats? I see they are considered cute in your dimension.";
		mes "I wish it was the same here.";
		mes "I'd like meet one someday.";
		completequest 8973;
		setquest 8974;
		getitem "Ch1_Root_Coin",5;
		getexp 16364720, 11449600;
		close;
	}
	//Don't ask me why kRO uses 2 quest id
	if ( isbegin_quest(8975) == 0 ) {
		mes "[Three]";
		mes "...";
		mes "I have a request.";
		mes "To you from another dimension.";
		next;
		mes "[Three]";
		mes "When I kneel here praying, they keep stepping on me... scratching me...";
		mes "Even if I have no will left, this I can't endure.";
		mes "Please destroy Nyaises.";
		mes "20 of it. Please.";
		next;
		if ( select("Accept.", "Sorry, I have to go.") == 2 ) {
			mes "[Three]";
			mes "I've forgotten what it feels like to be pressed for time.";
			close;
		}
		mes "[Three]";
		mes "If only they are just cute beings.";
		setquest 8975;
		close;
	}
	if ( isbegin_quest(8975) == 1 ) {
		if ( checkquest(8975, HUNTING) < 2 ) {
			mes "[Three]";
			mes "When I kneel here praying, they keep stepping on me... scratching me...";
			mes "Even if I have no will left, this I can't endure.";
			mes "Please destroy Nyaises.";
			mes "20 of it. Please.";
			close;
		}
		mes "[Three]";
		mes "Cats? I see they are considered cute in your dimension.";
		mes "I wish it was the same here.";
		mes "I'd like meet one someday.";
		erasequest 8975;
		setquest 8974;
		getitem "Ch1_Root_Coin",5;
		getexp 16364720, 11449600;
		close;
	}
	end;

OnInit:
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8973) == 0 && checkquest(8974, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8973) == 0 && checkquest(8974, PLAYTIME) == 2";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8973) == 2 && isbegin_quest(8975) == 0 && checkquest(8974, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8973) == 2 && isbegin_quest(8975) == 0 && checkquest(8974, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8973, HUNTING) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8975, HUNTING) == 2";
end;
}

uknw_ruin,93,174,0	script	#cpt1ground4	HIDDEN_WARP_NPC,5,5,{
	end;
	
OnTouch:
	if ( isbegin_quest(8967) == 2 ) {
		cloaknpc("Four#uknw1", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(8958) == 2 ) {
		cloaknpc("Rotten Branch#cpt1", false, getcharid(0));
		end;
	}
end;
}

uknw_ruin,93,174,3	script(CLOAKED)	Rotten Branch#cpt1	CH1_SCABIOSA,{
	if ( isbegin_quest(8958) < 2 ) {
		end;
	}
	mes "You can see a hidden path behind the rotting husk of the monster..";
	next;
	if ( select("Enter.", "Leave.") == 2 ) {
		mes "Let's come back when needed.";
		close;
	}
	warp "ch1_sf02", 35, 53;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8959) == 1 && isbegin_quest(8962) == 0";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8967) == 1";
end;
}

uknw_ruin,93,174,3	script(CLOAKED)	Four#uknw1	4_CH1_GRAY_VALKYRIE,{
	mes "[Four]";
	mes "97142...Oh.";
	mes "We... should go to a safer place.";
	next;
	if ( select("Enter.", "Leave.") == 2 ) {
		end;
	}
	warp "ch1_sf02", 35, 53;
	end;
	
OnInit:
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8976) == 0 && checkquest(8977, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8976) == 0 && checkquest(8977, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8976, HUNTING) == 2";
end;
}

ch1_sf02,35,53,0	script	#sf02onoff04-1	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(8967) == 2 ) {
		cloaknpc("Four#cpt2", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(8958) == 2 ) {
		cloaknpc("~+#cpt1", false, getcharid(0));
		end;
	}
end;
}

ch1_sf02,45,54,3	script(CLOAKED)	~+#cpt1	CH1_SCABIOSA,{
	if ( isbegin_quest(8959) == 1 && isbegin_quest(8962) == 0 ) {
		cloaknpc("Elise Isha#cpt4", false, getcharid(0));
		cloaknpc("Maura#cpt4", false, getcharid(0));
		mes "[~+]";
		mes "??? ?????.";
		mes "?? ?? ?????? ???";
		mes "??...";
		next;
		mes "[Elise]";
		mes "'142553322...";
		mes "142553323...";
		mes "142553324...'";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "Is it counting something...?";
		next;
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "Maybe... it's counting time.";
		next;
		setquest 8962;
		completequest 8962;
		if ( !CH1_RuinE_Check() ) {
			cutin "4job_maura_01.png",2;
			mes "[Maura]";
			mes "I feel like there are a few more of these beings around this place...";
			mes strcharinfo(0)+", let's find the next location.";
		} else {
			cutin "4job_maura_03.png",2;
			mes "[Maura]";
			mes "Maybe when Zero said not yet, he meant for us to find out and ask him?";
			mes "Thank you, Elise.";
			mes "Hey, "+strcharinfo(0)+". Let's go to the plaza.";
			next;
			select("I was about to go there anyway.");
			erasequest 8959;
			erasequest 8960;
			erasequest 8961;
			erasequest 8962;
			setquest 8963;
			cutin "4job_maura_02.png",2;
			mes "[Maura]";
			if ( Class != JOB_MEISTER ) {
				mes "Wow, with how decisive you are, I'm starting to wonder why you didn't become a Meister?";
			} else {
				mes "Wow, with how decisive you are, no wonder you are a Meister.";
			}
			mes "You are very dependable.";
			mes "Alright, lead the way!";
		}
		close2;
		cloaknpc("Elise Isha#cpt4", true, getcharid(0));
		cloaknpc("Maura#cpt4", true, getcharid(0));
		cutin "",255;
		end;
	}
	if ( isbegin_quest(8962) == 1 ) {
		mes "Let's go find another resident.";
		close;
	}
	if ( isbegin_quest(8967) == 1 ) {
		cloaknpc("Elise Isha#cpt4", false, getcharid(0));
		cloaknpc("Maura#cpt4", false, getcharid(0));
		mes "[~+]";
		mes "??? ??...!";
		next;
		cloaknpc("~+#cpt1", true, getcharid(0));
		cloaknpc("Four#cpt2", false, getcharid(0));
		specialeffect EF_BEGINSPELL, AREA, "Four#cpt2";
		mes "[Four]";
		mes "142553347...";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "Hello.";
		mes "Do you remember your name?";
		next;
		cutin "",255;
		mes "[Four]";
		mes "142553348...";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "...Let's just call you Four.";
		next;
		cutin "",255;
		mes "[Four]";
		mes "I agree.";
		next;
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "It can talk now.";
		next;
		cutin "4job_maura_01.png",2;
		mes "[Maura]";
		mes "What was the random number for?";
		next;
		cutin "",255;
		mes "[Four]";
		mes "I am counting to avoid losing my mind...";
		mes "If I stopped for a moment, I'd probably be a Scabiosa already.";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "...I see.";
		next;
		cutin "4job_maura_01.png",2;
		mes "[Maura]";
		mes "One said that they appear and disappear, right? We've helped everyone who's visible to us right now.";
		next;
		if ( select("Let's return to One", "What about the survey?") == 2 ) {
			cutin "4job_maura_02.png",2;
			mes "[Maura]";
			mes "Yeah. About that...";
			mes "There's not much anything to survey in here, no?";
			mes "I already collected the necessary samples.";
			next;
		}
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "Let's head back to the plaza.";
		completequest 8967;
		setquest 8968;
		close2;
		cutin "",255;
		cloaknpc("Elise Isha#cpt4", true, getcharid(0));
		cloaknpc("Maura#cpt4", true, getcharid(0));
		end;
	}
	mes "[~+]";
	mes "...";
	close;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8959) == 1 && isbegin_quest(8962) == 0";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8967) == 1";
end;
}

ch1_sf02,45,54,3	script(CLOAKED)	Four#cpt2	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8968) < 2 ) {
		end;
	}
	switch( checkquest(8977,PLAYTIME) ) {
		case -1:
			break;
		case 0:
		case 1:
			mes "[Four]";
			mes "I can't even use my own hand.";
			mes "Does existing like this have meaning?";
			mes "...";
			mes "76114325... 76114326...";
			close;
		case 2:
			erasequest 8977;
			break;
	}
	if ( isbegin_quest(8976) == 0 ) {
		mes "[Four]";
		mes "18551244... 18551245...";
		mes "You're here.";
		mes "I figured you'd come soon.";
		mes "I have a request.";
		next;
		mes "[Four]";
		mes "To keep myself from becoming a Scabiosa again, I need to...";
		mes "eliminate my kin who've become shadow jailers, and take their strength.";
		mes "They can no longer be restored.";
		mes "Please eliminate 10 of them.";
		next;
		if ( select("Accept.", "But, I have to go") == 2 ) {
			mes "[Four]";
			mes "...I'll try to hold on.";
			close;
		}
		mes "[Four]";
		mes "To think I have to borrow someone else's hands just to ask for the end of those who were once my kin...";
		setquest 8976;
		close;
	}
	if ( isbegin_quest(8976) == 1 ) {
		if ( checkquest(8976, HUNTING) < 2 ) {
			mes "[Four]";
			mes "To keep myself from becoming a Scabiosa again, I need to...";
			mes "eliminate my kin who've become shadow jailers, and take their strength.";
			mes "They can no longer be restored.";
			mes "Please eliminate 10 of them.";
			close;
		}
		mes "[Four]";
		mes "...Of course I felt it. The husks of my kin whose consciousness had already faded and drifted far away, shattered, leaving me the power that got left behind.";
		mes "Thank you.";
		mes "...I am truly thankful.";
		setquest 8977;
		erasequest 8976;
		getitem "Ch1_Root_Coin",5;
		getexp 8528760, 5955240;
		close;
	}
	end;
	
OnInit:
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8976) == 0 && checkquest(8977, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(8968) == 2 && isbegin_quest(8976) == 0 && checkquest(8977, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8976, HUNTING) == 2";
end;
}

ch1_sf02,111,49,5	script(CLOAKED)	Elise Isha#cpt2	4_F_ACOLYTE,{
	end;
}

ch1_sf02,116,51,3	script(CLOAKED)	Maura#cpt2	4_4JOB_MAURA,{
	end;
}

ch1_sf02,112,265,1	script(CLOAKED)	Elise Isha#cpt3	4_F_ACOLYTE,{
	end;
}

ch1_sf02,116,267,3	script(CLOAKED)	Maura#cpt3	4_4JOB_MAURA,{
	end;
}

ch1_sf02,43,55,5	script(CLOAKED)	Elise Isha#cpt4	4_F_ACOLYTE,{
	end;
}

ch1_sf02,44,50,1	script(CLOAKED)	Maura#cpt4	4_4JOB_MAURA,{
	end;
}

uknw_ruin,160,160,0	script	#cpt1ground5	HIDDEN_WARP_NPC,5,5,{
	end;
	
OnTouch:
	if ( isbegin_quest(8963) > 0 ) {
		cloaknpc("One#uknw2", false, getcharid(0));
	}
end;
}

uknw_ruin,160,160,3	script(CLOAKED)	One#uknw2	4_CH1_GRAY_VALKYRIE,{
	cutin "ch1_valkyrie04.png",2;
	mes "[One]";
	mes "Do you have something to say?";
	mes "This place is dangerous, let's move somewhere safe.";
	next;
	if ( select("Enter.", "Leave.") == 2 ) {
		mes "[One]";
		mes "...";
		close3;
	}
	warp "ch1_sf02", 155, 166;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8963) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8968) == 1";
end;
}

ch1_sf02,154,166,0	script	#sf02onoff00-2	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	if ( isbegin_quest(8963) > 0 ) {
		cloaknpc("Zero#cpt2", false, getcharid(0));
		cloaknpc("One#cpt2", false, getcharid(0));
	}
	if ( isbegin_quest(8968) == 2 ) {
		cloaknpc("Fading Resident#cpt1", false, getcharid(0));
		cloaknpc("Nameless Resident#cpt1", false, getcharid(0));
		cloaknpc("Sitting Resident#cp", false, getcharid(0));
	}
end;
}

ch1_sf02,160,160,3	script(CLOAKED)	One#cpt2	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8963) == 1 ) {
		cloaknpc("Elise Isha#cpt5", false, getcharid(0));
		cloaknpc("Maura#cpt5", false, getcharid(0));
		cutin "ch1_valkyrie04.png",2;
		mes "[One]";
		mes "I see that you've regained your ability to sense divine powers again, acolyte.";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "Yes, thankfully...";
		next;
		select("Elise, can't you restore the valkyries?");
		cutin "ch1_valkyrie02.png",0;
		mes "[One]";
		mes "You only draw power from divinity, we are the very essence of divine authority in this dimension.";
		mes "It's impossible.";
		next;
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "I see... I expected as much.";
		next;
		mes "[Elise]";
		mes "But I realized something just now.";
		mes "It may sound blasphemous, but we can destroy a divine authority.";
		next;
		cutin "ch1_valkyrie02.png",0;
		mes "[One]";
		mes "Correct.";
		next;
		mes "[One]";
		mes "Didn't you meet one who was exactly wishing for that?";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "None of them asked to be destroyed... atleast not directly, I think?";
		next;
		cutin "4job_maura_01.png",2;
		mes "[Maura]";
		mes "We also met one who said they wanted to destroy this dimension and seal it, asking us to break the chain.";
		next;
		cutin "ch1_valkyrie01.png",0;
		mes "[One]";
		mes "...";
		next;
		cutin "ch1_valkyrie02.png",0;
		mes "[One]";
		mes "It has been too long, everyone already went mad.";
		npctalk "So do we.", "Zero#cpt2", BC_SELF;
		next;
		cutin "ch1_valkyrie04.png",0;
		mes "[One]";
		mes "The dimension can't be destroyed.";
		mes "But there is a way to break the chains.";
		mes "I don't know if that is truly their wish, but I will tell you.";
		next;
		mes "[One]";
		mes "Destroy the Shadow Jailers. If some divine authority still lingers within them, it's possible that you will obtain a Broken Petal.";
		mes "This place is full of flower heads, so you'll immediately recognize which one is it.";
		next;
		cutin "ch1_valkyrie03.png",0;
		mes "[One]";
		mes "We were cast into this dump for sins that displeased the gods.";
		mes "When there are sinners, there are jailers.";
		next;
		mes "[One]";
		mes "That's not important anyways.";
		next;
		mes "[One]";
		mes "Maybe you can find something worth taking?";
		next;
		cutin "4job_maura_02.png",2;
		mes "[Maura]";
		mes "If we're going to collect Broken Petals anyway, how about we also survey the area?";
		next;
		mes "[Maura]";
		mes "What do you think, Elise? "+strcharinfo(0)+", are you coming too?";
		npctalk "Elise : I'd love to.", "Elise Isha#cpt5", BC_SELF;
		next;
		select("Of course");
		cutin "ch1_valkyrie01.png",0;
		mes "[One]";
		mes "Very well.";
		next;
		mes "[One]";
		mes "We cannot destroy them ourselves.";
		mes "Destroy 10 Shadow Jailers.";
		mes "And bring me 5 Broken Petals and I will handle the rest.";
		mes "May your gods bless you.";
		completequest 8963;
		setquest 8964;
		close2;
		cutin "",255;
		cloaknpc("Elise Isha#cpt5", true, getcharid(0));
		cloaknpc("Maura#cpt5", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(8964) == 1 ) {
		if ( countitem("Ch1_Broken_Petal") < 5 || checkquest(8964, HUNTING) < 2 ) {
			cutin "ch1_valkyrie01.png",0;
			mes "[One]";
			mes "We cannot destroy them ourselves.";
			mes "Destroy 10 Shadow Jailers.";
			mes "And bring me 5 Broken Petals and I will handle the rest.";
			mes "May your gods bless you.";
			close3;
		}
		cloaknpc("Elise Isha#cpt5", false, getcharid(0));
		cloaknpc("Maura#cpt5", false, getcharid(0));
		cutin "4job_maura_01.png",2;
		mes "[Maura]";
		mes "We didn't get any information after surveying the area. It's completely different from our dimension.";
		next;
		cutin "4job_maura_03.png",2;
		mes "[Maura]";
		mes "Anyway, if the ones with flowers for heads are the prisoners...";
		mes "What are you?";
		next;
		cutin "ch1_valkyrie01.png",0;
		mes "[One]";
		mes "In your own word, you are right to call us the residents.";
		mes "Our existence has faded, so you can't see our original forms.";
		mes "All of us were once residents.";
		next;
		mes "[One]";
		mes "...Now, these broken petals that you've gathered";
		mes "When you put this on the head of my friends who is yet to become a full flower...";
		mes "If you attach it to them bit by bit, the chain will be released.";
		next;
		mes "[One]";
		mes "You'll know where to place them when you look closely.";
		mes "Now then, do as you wish.";
		delitem "Ch1_Broken_Petal", 5;
		erasequest 8964;
		setquest 8965;
		close2;
		cutin "",255;
		cloaknpc("Elise Isha#cpt5", true, getcharid(0));
		cloaknpc("Maura#cpt5", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(8968) == 0 ) {
		cutin "ch1_valkyrie01.png",0;
		mes "[One]";
		mes "It's been a long time since I've seen a resident ask for something";
		mes "I'm also curious why you want to grant their requests.";
		mes "May your gods bless you.";
		close3;
	}
	if ( isbegin_quest(8968) == 1 ) {
		cloaknpc("Elise Isha#cpt5", false, getcharid(0));
		cloaknpc("Maura#cpt5", false, getcharid(0));
		cutin "ch1_valkyrie01.png",1;
		mes "[One]";
		mes "...";
		mes "Do you have some concerns?";
		next;
		select("Who released you?");
		cutin "ch1_valkyrie02.png",1;
		mes "[One]";
		mes "I was never chained.";
		mes "But I am the one who released Zero's chain.";
		next;
		mes "[One]";
		mes "My color simply faded while everyone else was chained and turned into flowers.";
		mes "But I couldn't destroy the authority myself even If I wanted to.";
		mes "I got freed because a passing Nyaise tore apart my friend who became a jailer.";
		npctalk "Elise : How... did a Nyaise did that?", "Elise Isha#cpt5", BC_SELF;
		npctalk "Maura : A beast is still a beast.", "Maura#cpt5", BC_SELF;
		next;
		mes "[One]";
		mes "Thank you for saving my friends.";
		mes "I assume your investigation is complete now?";
		next;
		cutin "4job_maura_02.png",2;
		mes "[Maura]";
		mes "...For now.";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "...Yes, for now.";
		next;
		select("Should we return?", "Is there nothing left to do?");
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "The time flow of this place is the same, but the sense of time feels different.";
		mes "I don't think we'll come up with a solution in just a day or two...";
		next;
		cutin "acact_02.bmp",2;
		mes "[Elise]";
		mes "There's something bothering me, so maybe we should come back later?";
		next;
		cutin "4job_maura_02.png",2;
		mes "[Maura]";
		mes "I was thinking the same thing.";
		mes "We really are in sync, huh?";
		next;
		cutin "4job_maura_02.png",2;
		mes "[Elise]";
		mes "Hehe. Yes we are.";
		mes "Also... the adventurer also have other things to do.";
		next;
		cutin "acact_01.bmp",2;
		mes "[Elise]";
		mes "Then, let's return to Ashridge.";
		mes "Valkyrie, may the gods bless with you until we meet again.";
		next;
		cutin "ch1_valkyrie01.png",1;
		mes "[One]";
		mes "Until we meet again...";
		mes "Ahaha.";
		mes "It's been a long time since I've heard that phrase.";
		next;
		cutin "ch1_valkyrie04.png",1;
		mes "[One]";
		mes "I'll be waiting for the day you return.";
		completequest 8968;
		setquest 8969;
		getitem "Ch1_Root_Coin",10;
		getexp 130609489, 6000000;
		close2;
		cutin "",255;
		cloaknpc("Elise Isha#cpt5", true, getcharid(0));
		cloaknpc("Maura#cpt5", true, getcharid(0));
		end;
	}
	cutin "ch1_valkyrie04.png",1;
	mes "[One]";
	mes "I'll be waiting for the day you return.";
	close3;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8963) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(8968) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(8964, HUNTING) == 2 && countitem(\"Ch1_Broken_Petal\") >= 5";
end;
}

ch1_sf02,162,165,7	script(CLOAKED)	Zero#cpt2	4_CH1_GRAY_VALKYRIE,{
	if ( isbegin_quest(8968) < 2 ) {
		cutin "ch1_valkyrie01.png",2;
		mes "[Zero]";
		mes "...Huff.";
		mes "There is no divine powers in this dimension.";
		mes "What remain is to destroy and absorb one.";
		mes "But that is beyond our authority.";
		mes "It's impossible for us.";
		next;
		mes "[Zero]";
		mes "But since you are not a resident of this place, you can destroy them.";
		mes "Someone like me or One, would absorb what remains.";
		next;
		mes "[Zero]";
		mes "Beside from the residents that you've seen, there might be others.";
		mes "They appear and disappear.";
		mes "You might see more in time.";
		next;
		mes "[Zero]";
		mes "This is all I can say... for now.";
		mes "I'm tired.";
		mes "I'll rest now.";
		close3;
	}
	mes "[Zero]";
	mes "One was waiting.";
	mes "But not for long.";
	close3;
}

ch1_sf02,156,161,5	script(CLOAKED)	Elise Isha#cpt5	4_F_ACOLYTE,{
	end;
}

ch1_sf02,158,157,1	script(CLOAKED)	Maura#cpt5	4_4JOB_MAURA,{
	end;
}

ch1_sf02,144,161,1	script(CLOAKED)	Nameless Resident#cpt1	4_CH1_GRAY_VALKYRIE,{
	mes "[Valkyrie]";
	mes "Was it you? The one who destroyed the Nyaises.";
	mes "Thank you.";
	mes "I don't want to break again.";
	mes "Thank you.";
	close;
}

ch1_sf02,150,150,7	script(CLOAKED)	Fading Resident#cpt1	4_CH1_GRAY_VALKYRIE,{
	mes "[Valkyrie]";
	mes "Can you see me clearly?";
	mes "That's right.";
	mes "I can't see your wings either.";
	mes "You're someone I've never seen before.";
	mes "Do your best to stay alive.";
	close;
}

ch1_sf02,156,149,1	script(CLOAKED)	Sitting Resident#cp	4_CH1_GRAY_VALKYRIE,{
	mes "[Valkyrie]";
	mes "Hah...";
	mes "How am I even alive like this? Who am I?";
	mes "Where is this place?";
	mes "What happened to the world?";
	close;
}

mu_fild01,95,154,3	script	Survey Point#ch01_mu	4_POINT_BLUE,2,2,{
	end;
	
OnTouch:
	if ( isbegin_quest(18376) == 1 ) { 
		warp "ch1_sf01", 191, 210;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18376) == 1";
end;
}

ch1_sf01,190,214,4	script	Chez#ch01_mu1	4_F_FAIRYKID4,{
	if ( isbegin_quest(18376) == 1 ) {
		cutin "ch1_chez01.png",2;
		mes "[Chez]";
		mes "You're here?";
		mes strcharinfo(0)+", we were waiting for you before we explore this place!";
		cloaknpc("Magi#ch01_mu1", false, getcharid(0));
		cloaknpc("Mark#ch01_mu1", false, getcharid(0));
		cloaknpc("Dew#ch01_mu1", false, getcharid(0));
		next;
		mes "[Chez]";
		mes "It's not as hot as last time, right?";
		mes "Looks like the <B>Temperature Control Device</B> is working well.";
		mes "I heard that a wind fairy was deployed with the advanced party and they said that it was <B>very effective</B>... Personally, I think they're not bad.";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "But of course!";
		mes "If you were with someone like me a water fairy, we could make it even cooler!";
		mes "Isn't that the reason they sent me here?";
		mes "<FONT SIZE = 16><B>Mwahaha!</B></FONT>";
		next;
		cutin "ep18_dew_05.png",2;
		mes "[Dew]";
		mes "Oh!";
		mes "It's not hot?";
		mes "It's not hot!";
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "<FONT SIZE = 16><B>Hey!</B></FONT>";
		mes "I told you not to rush in like that!";
		mes "You almost got melted last time!";
		next;
		cutin "ep18_dew_03.png",2;
		mes "[Dew]";
		mes "It's not that hot right now, so it's fine!";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "If you fall into a lava, you'll met before we even get a chance to heal you.";
		next;
		cutin "ep18_dew_01.png",2;
		mes "[Dew]";
		mes "Hah!";
		mes "Don't underestimate me!";
		mes "I came fully prepared this time!";
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "Can you just be careful... Sigh.";
		mes "Now, tell me the preparation that you made.";
		next;
		cutin "ep18_dew_05.png",2;
		mes "[Dew]";
		mes "I have a <B>Fly Wing</B> for emergencies";
		mes "An <B>Yggdrasil Leaf</B> just in case!";
		mes "And the ultimate backup a <B>Token of Zieg...</B>";
		next;
		cutin "ep18_mark_04.png",2;
		mes "[Mark]";
		mes "You idiot!";
		emotion ET_FRET, getnpcid(0,"Mark#ch01_mu1");
		next;
		cutin "ep18_dew_02.png",2;
		mes "[Dew]";
		mes "<FONT SIZE = 16><B>Ah!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Why did you hit me!</B></FONT>";
		npctalk "Dew : That hurts!!!", "Dew#ch01_mu1", BC_SELF;
		next;
		cutin "ep18_mark_03.png",2;
		mes "[Mark]";
		mes "Chez, I don't think there's any hope for this guy, can you cast an additional fire protection spell on him?";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Sure.";
		mes "I mean...";
		mes "I'd rather not see what could happen to him.";
		next;
		cutin "ch1_chez02.png",2;
		mes "[Chez]";
		mes "I'll freeze you nice and solid!";
		emotion ET_BEST, getnpcid(0,"Chez#ch01_mu1");
		next;
		cutin "ep18_dew_02.png",2;
		mes "[Dew]";
		mes "Aaaaagh!!!";
		specialeffect EF_HYOUSENSOU, AREA, "Dew#ch01_mu1";
		npctalk "Dew : Aaaaagh!!!!", "Dew#ch01_mu1", BC_SELF;
		next;
		cutin "ep18_mark_02.png",2;
		mes "[Mark]";
		mes "Phew~";
		mes "Perfect!";
		mes "Now we're good to go.";
		next;
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Now, should we leave and find the scout party?";
		mes "We just need to follow the path.";
		npctalk "Magi : Okay...", "Magi#ch01_mu1", BC_SELF;
		completequest 18376;
		setquest 24086;
		close2;
		removespecialeffect EF_HYOUSENSOU, AREA, "Dew#ch01_mu1";
		cutin "",255;
		cloaknpc("Magi#ch01_mu1", true, getcharid(0));
		cloaknpc("Mark#ch01_mu1", true, getcharid(0));
		cloaknpc("Dew#ch01_mu1", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(24086) == 1 ) {
		cloaknpc("Mark#ch01_mu1", false, getcharid(0));
		cutin "ep18_mark_01.png",2;
		mes "[Mark]";
		mes "Now, should we leave and find the scout party?";
		mes "We just need to follow the path.";
		close2;
		cutin "",255;
		cloaknpc("Mark#ch01_mu1", true, getcharid(0));
		end;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18376) == 1";
end;
}

ch1_sf01,187,213,5	script(CLOAKED)	Magi#ch01_mu1	4_F_FAIRYKID4,{
	end;
}

ch1_sf01,187,211,5	script(CLOAKED)	Mark#ch01_mu1	4_EP18_MARK,{
	end;
}

ch1_sf01,193,215,3	script(CLOAKED)	Dew#ch01_mu1	4_EP18_DEW,{
	end;
}

mu_fild01,151,221,5	script	Harban#ch01_1	4_CH1_KAL,{
	if ( isbegin_quest(24086) == 0 ) {
		end;
	}
	if ( isbegin_quest(24086) == 1 ) {
		warp "ch1_sf01", 54, 210;
		end;
	}
	if ( isbegin_quest(24087) == 1 ) {
		mes "[Harban]";
		mes "Well... It's an obvious thing for me, so it's hard to explain.";
		mes "I'm not really good at explaining to begin with, but the Hal overseers <B>further inside</B> would probably know better, so go ask them.";
		next;
		mes "[Harban]";
		mes "Or you can ask your friends.";
		mes "I mean there's gotta be something good about it or you won't harvest it, right?";
		close2;
		navigateto("mu_fild01", 222, 244);
		end;
	}
	mes "[Harban]";
	mes "A bunch of folks like you suddenly showed up and started harvesting <B>Hal</B>.";
	mes "Young people these days don't really like to work, so having workers around lessen the burden for us.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24086) == 1";
end;
}

ch1_sf01,55,214,5	script	Harban#ch01_1-1	4_CH1_KAL,{
	if ( isbegin_quest(24086) == 1 ) {
		mes "[Harban]";
		mes "Oh-";
		mes "A new worker?";
		next;
		select("Worker?");
		mes "[Harban]";
		mes "A bunch of folks like you suddenly showed up and started harvesting <B>Hal</B>.";
		mes "Young people these days don't really like to work, so having workers around lessen the burden for us.";
		next;
		select("Hal?");
		mes "[Harban]";
		mes "Yes.";
		mes "Hal.";
		next;
		select("Ask about Hal.");
		mes "[Harban]";
		mes "Don't you know what Hal is?";
		mes "How could you not?";
		mes "Hal is well... Hal.";
		mes "It's something that you can <B>gather</B> around here...";
		next;
		mes "[Harban]";
		mes "Well... It's an obvious thing for me, so it's hard to explain.";
		mes "I'm not really good at explaining to begin with, but the Hal overseers <B>further inside</B> would probably know better, so go ask them.";
		next;
		mes "[Harban]";
		mes "Or you can ask your friends.";
		mes "I mean there's gotta be something good about it or you won't harvest it, right?";
		completequest 24086;
		setquest 24087;
		close;
	}
	if ( isbegin_quest(24087) == 1 ) {
		mes "[Harban]";
		mes "Well... It's an obvious thing for me, so it's hard to explain.";
		mes "I'm not really good at explaining to begin with, but the Hal overseers <B>further inside</B> would probably know better, so go ask them.";
		next;
		mes "[Harban]";
		mes "Or you can ask your friends.";
		mes "I mean there's gotta be something good about it or you won't harvest it, right?";
		close;
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24086) == 1";
end;
}

mu_fild01,222,244,3	script	Tamarin#ch01	4_EP18_TAMARIN,{
	if ( isbegin_quest(24087) == 0 ) {
		end;
	}
	if ( isbegin_quest(24087) == 1 ) {
		warp "ch1_sf01", 268, 219;
		end;
	}
	if ( isbegin_quest(24088) == 1 ) {
		cutin "ep18_tamarin_02.png",2;
		mes "[Tamarin]";
		mes "For more details, you should ask the <B>advanced party's Yuvilus</B>.";
		mes "They have more information on it than I do.";
		close2;
		cutin "",255;
		navigateto("mu_fild01", 198, 186);
		end;
	}
	cutin "ep18_tamarin_01.png",2;
	mes "[Tamarin]";
	mes "It's not as hot as it used to be, but it's still too warm.";
	mes "How long are we supposed to keep gathering Hal...";
	close3;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24087) == 1";
end;
}

ch1_sf01,265,226,3	script	Tamarin#ch01-1	4_EP18_TAMARIN,{
	if ( isbegin_quest(24087) == 1 ) {
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "Ah!";
		mes "You're here?";
		cloaknpc("Chez#ch01_mu2", false, getcharid(0));
		cloaknpc("Magi#ch01_mu2", false, getcharid(0));
		cloaknpc("Mark#ch01_mu2", false, getcharid(0));
		next;
		select("Ask about Hal.");
		mes "[Tamarin]";
		mes "<B>Hal</B> is like a <B>special mass of energy</B> found in this region...";
		mes "It looks like a glass bead or a shiny mineral...";
		next;
		mes "[Tamarin]";
		mes "But it's more like... a crop,";
		mes "I heard that <B>Hal is transparent, eaten, and used in various ways</B>.";
		next;
		cutin "ep18_tamarin_02.png",2;
		mes "[Tamarin]";
		mes "For more details, you should ask the <B>advanced party's Yuvilus</B>.";
		mes "They have more information on it than I do.";
		emotion ET_HUK, getnpcid(0,"Chez#ch01_mu2");
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Eh?";
		mes "Who did you say?";
		npctalk "Chez : Did I hear it correctly?", "Chez#ch01_mu2", BC_SELF;
		next;
		cutin "ep18_tamarin_02.png",2;
		mes "[Tamarin]";
		mes "Yuvilus from the advanced party.";
		next;
		cutin "ch1_chez03.png",2;
		mes "[Chez]";
		mes "Ah... Pfft... well... are you serious?";
		mes "That blonde hair...";
		next;
		cutin "ep18_tamarin_01.png",2;
		mes "[Tamarin]";
		mes "That's right!";
		mes "Do you know him?";
		next;
		cutin "ch1_chez04.png",2;
		mes "[Chez]";
		mes "Ah... pfft";
		mes "I know them well. Very well.";
		mes "Uh... pff... sigh... we better go quickly... pff...";
		npctalk "Chez : pff... Pffhahahaha...", "Chez#ch01_mu2", BC_SELF;
		completequest 24087;
		setquest 24088;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_mu2", true, getcharid(0));
		cloaknpc("Magi#ch01_mu2", true, getcharid(0));
		cloaknpc("Mark#ch01_mu2", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(24088) == 1 ) {
		cutin "ep18_tamarin_02.png",2;
		mes "[Tamarin]";
		mes "<B>Hal</B> is like a <B>special mass of energy</B> found in this region...";
		mes "It looks like a glass bead or a shiny mineral...";
		next;
		mes "[Tamarin]";
		mes "But it's more like... a crop,";
		mes "I heard that <B>Hal is transparent, eaten, and used in various ways</B>.";
		next;
		mes "[Tamarin]";
		mes "For more details, you should ask the <B>advanced party's Yuvilus</B>.";
		mes "They have more information on it than I do.";
		close3;
	}
	end;

	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24087) == 1";
end;
}

ch1_sf01,263,227,5	script(CLOAKED)	Chez#ch01_mu2	4_F_FAIRYKID4,{
	end;
}

ch1_sf01,262,225,5	script(CLOAKED)	Magi#ch01_mu2	4_F_FAIRYKID4,{
	end;
}

ch1_sf01,264,222,3	script(CLOAKED)	Mark#ch01_mu2	4_EP18_MARK,{
	end;
}

mu_fild01,198,186,3	script	Yuvilus#ch01	4_CH1_YUP,{
	if ( isbegin_quest(24088) == 0 ) {
		end;
	}
	cutin "ch1_jb01.png",2;
	mes "[Yuvilus]";
	mes "Did Ashridge send another person?";
	mes "Hmm... This place is too dangerous, let's move to a safer location.";
	close2;
	cutin "",255;
	warp "ch1_sf01", 107, 227;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24088) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24097) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "CH1_Complete() && isbegin_quest(24108) == 0 && isbegin_quest(24109) < 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24109) == 1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24110) == 0 && checkquest(24111, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24110) == 0 && checkquest(24111, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24110) == 1 && countitem(\"aegis_1001977\") >= 10";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24112) == 0 && checkquest(24113, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24112) == 0 && checkquest(24113, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(24112, HUNTING) == 2";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24114) == 0 && checkquest(24115, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24114) == 0 && checkquest(24115, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(24114, HUNTING) == 2";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24116) == 0 && checkquest(24117, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24116) == 0 && checkquest(24117, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(24116, HUNTING) == 2";
end;
}

ch1_sf01,110,227,3	script	Yuvilus#ch01_in	4_CH1_YUP,{
	callsub OnFinishCheck;
	if ( isbegin_quest(24109) == 2 ) {
		.@s = select("Hal Gathering", "Is it edible?", "Is this important?", "Tricked by a bug!!!", "Quit.");
		switch ( .@s ) {
			case 1:
				.@qid = 24110;
				.@cid = 24111;
				cutin "ch1_jb01.png",2;
				mes "[Yuvilus]";
				mes "You can gather Hal from anywhere in Muspelheim, but the most reliable method is defeating monsters.";
				mes "Can you <B>bring me 10 Hal by hunting nearby monsters</B>?";
				break;
			case 2:
				.@qid = 24112;
				.@cid = 24113;
				cutin "ch1_jb01.png",2;
				mes "[Yuvilus]";
				mes "The Harbans can live off with just Hal, but we can't.";
				mes "That's why we're looking for anything edible in the area.";
				mes "<B>Can you hunt 20 Volcanic Clams</B>? I'd like to check them out.";
				break;
			case 3:
				.@qid = 24114;
				.@cid = 24115;
				cutin "ch1_jb01.png",2;
				mes "[Yuvilus]";
				mes "<B>Firafflesia</B>, hm... It sounds like one of those creature with mythical herbs.";
				mes "I want to check if they're edible, could you hunt <B>20</B> of them?";
				break;
			case 4:
				.@qid = 24116;
				.@cid = 24117;
				cutin "ch1_jb01.png",2;
				mes "[Yuvilus]";
				mes "Have you seen the <B>Fire Ring</B> that's roaming around in this place?";
				mes "It's actually an insect, but I don't know why it doesn't crawl around with all those legs.";
				mes "I can't stand bugs... Can you hunt <B>20</B> of them for me?";
				break;
			case 5:
				cutin "ch1_jb01.png",2;
				mes "[Yuvilus]";
				mes "Okay.";
				mes "It's not that urgent, feel free to drop by anytime.";
				close3;
		}
		if ( isbegin_quest(.@qid) == 1 ) {
			close3;
		}
		next;
		switch( checkquest(.@cid, PLAYTIME) ) {
			case -1:
				break;
			case 0:
			case 1:
				end;
			case 2:
				erasequest .@cid;
				break;
		}
		if ( select("Accept.", "Decline.") == 2 ) {
				cutin "ch1_jb01.png",2;
				mes "[Yuvilus]";
				mes "Okay.";
				mes "It's not that urgent, feel free to drop by anytime.";
				close3;
		}
		mes "[Yuvilus]";
		mes "Thanks.";
		mes "I'll be counting on you.";
		switch ( .@s ) {
			case 1:
				mes "Bring me <B>10 Hal by hunting around the Volund Valley</B>.";
				break;
			case 2:
				mes "Hunt me <B>20 Volcanic Clams</B> roaming around the Volund Valley.";
				break;
			case 3:
				mes "Hunt me <B>20 Firafflesia</B> roaming around the Volund Valley.";
				break;
			case 4:
				mes "Hunt me <B>20 Fire Ring</B> roaming around the Volund Valley.";
				break;
		}
		setquest .@qid;
		close3;
	}
	if ( isbegin_quest(24109) == 1 ) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "So it literally means farming.";
		mes "I won't question how it's done anymore.";
		next;
		mes "[Yuvilus]";
		mes "Let's start by gathering Hal.";
		mes "We'll need a lot of it, so you should prepare to start farming.";
		completequest 24109;
		getitem "1001972",10;
		getexp 65304744, 4000000;
		close3;
	}
	if ( CH1_Complete() && isbegin_quest(24108) == 0 ) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "You're here.";
		mes "How did it go?";
		next;
		select("Talk about the cure.");
		mes "[Yuvilus]";
		mes "I'm glad to hear that Hal was effective in the treatment.";
		mes "Now that the Dark Whisper is gone, we probably don't have to worry about the disease anymore. But just in case, we should keep some extra on hand...";
		next;
		mes "[Yuvilus]";
		mes "Don't you think Hal could be useful in various ways if used properly?";
		mes "Maybe you could start by figuring out what kind of Hal is needed and give <B>farming</B> a try...";
		next;
		mes "[Yuvilus]";
		mes "I can't even imagine what farming something like that would be like,";
		mes "It's best to ask the <B>Harbans</B> when it comes to farming, right?";
		setquest 24108;
		close3;
	}
	if ( isbegin_quest(24108) == 1 ) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Don't you think Hal could be useful in various ways if used properly?";
		mes "Maybe you could start by figuring out what kind of Hal is needed and give <B>farming</B> a try...";
		next;
		mes "[Yuvilus]";
		mes "I can't even imagine what farming something like that would be like,";
		mes "It's best to ask the <B>Harbans</B> when it comes to farming, right?";
		close3;
	}
	if ( isbegin_quest(24088) == 1 ) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Did Ashridge sent another person?";
		mes "The <B>Muspelheim</B> investigation is already going smoothly.";
		mes "The harbans have been quite cooperative <B>temperature control device</B> and <B>wind barrier</B>, the heat of <B>Muspelheim</B> is not a problem anymore.";
		cloaknpc("Chez#ch01_y", false, getcharid(0));
		cloaknpc("Magi#ch01_y", false, getcharid(0));
		cloaknpc("Mark#ch01_y", false, getcharid(0));
		next;
		mes "[Yuvilus]";
		mes "I'm researching a material called <B>Hal</B>, it's a <B>natural substance</B> that grows here in Muspelheim. When <B>harvested</B>, it can gain <B>unique properties</B>.";
		next;
		mes "[Chez]";
		mes "<FONT SIZE = 16><B>Puhahahahahaha?!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>Ack?! Seriously! Really!!!</B></FONT>";
		mes "<FONT SIZE = 16><B>It's really Yuvilus?!!!</B></FONT>";
		cutin "ch1_chez04.png",2;
		npctalk "Chez : Puhahahahahaha?!!!", "Chez#ch01_y", BC_SELF;
		next;
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "?... Ch-chez.";
		npctalk "Yuvilus : What are you doing here...", "Yuvilus#ch01_in", BC_SELF;
		next;
		cutin "ch1_chez04.png",2;
		mes "[Chez]";
		mes "You were the one who said you're too busy.";
		mes "And now you're here with that look.";
		mes "Agh... I laughed too much.";
		next;
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "It's not like I want to be here looking like this, so please stop laughing.";
		next;
		mes "[Yuvilus]";
		mes "I'd like to stop laughing and talk about the work now, got it?";
		emotion ET_FRET, getnpcid(0,"Yuvilus#ch01_in");
		next;
		cutin "ch1_chez04.png",2;
		mes "[Chez]";
		mes "Ha... huff...";
		mes "That tone of yours... Pf...";
		npctalk "Chez : I can't breathe...", "Chez#ch01_y", BC_SELF;
		next;
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Ahem!";
		mes "Let's go back to what's important!";
		mes "The fact that they sent more people must mean there's been some progress in the village's research, right?";
		next;
		select("Talk about the energy fusion.");
		mes "[Yuvilus]";
		mes "Hm... I see.";
		mes "Based on the information the Harbans gave us, it's not too hard to fuse Hal with one specific energy, but <B>infusing multiple energies at once</B> might be hard.";
		next;
		mes "[Yuvilus]";
		mes "Well, just because it's difficult doesn't mean it's impossible, so how about trying it out by infusing energy into a Hal yourself?";
		mes "I believe that you have a magician and an acolyte with you";
		next;
		mes "[Yuvilus]";
		mes "You should be able to get Hal by asking the workers who are currently harvesting it.";
		mes "While you're at it, it would be nice if you can share some wind energy to those <B>workers</B>.";
		next;
		mes "[Yuvilus]";
		mes "You don't need to do anything, just ask the workers here and let them handle it.";
		mes "Now go.";
		mes "I'll be preparing for an experiment in the meantime.";
		completequest 24088;
		setquest 24089;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_y", true, getcharid(0));
		cloaknpc("Magi#ch01_y", true, getcharid(0));
		cloaknpc("Mark#ch01_y", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(24089) == 1 ) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "You should be able to get Hal by asking the workers who are currently harvesting it.";
		mes "While you're at it, it would be nice if you can share some wind energy to those <B>workers</B>.";
		next;
		mes "[Yuvilus]";
		mes "You don't need to do anything, just ask the workers here and let them handle it.";
		mes "Now go.";
		mes "I'll be preparing for an experiment in the meantime.";
		close3;
	}
	if ( isbegin_quest(24097) == 1 ) {
		mes "[Yuvilus]";
		mes "Judging by the amount you brought, the Hal gathering must be going smoothly.";
		cutin "ch1_jb01.png",2;
		cloaknpc("Chez#ch01_y", false, getcharid(0));
		cloaknpc("Magi#ch01_y", false, getcharid(0));
		cloaknpc("Mark#ch01_y", false, getcharid(0));
		next;
		cutin "",255;
		mes "- Show the blemished Hal. -";
		next;
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "What's this?!";
		mes "Where did you get it?";
		next;
		mes "[Yuvilus]";
		mes "It doesn't seem to carry any energy like normal a Hal, but the color changed as if it does.";
		mes "This red part is probably fire energy and this light green part seems to be wind energy.";
		next;
		mes "[Yuvilus]";
		mes "It's displaying elemental attributes without actually changing elements...";
		mes "I've never heard of Hal in this state before, so it'd be best to ask the <B>Harbans</B> directly..";
		erasequest 24097;
		setquest 24098;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_y", true, getcharid(0));
		cloaknpc("Magi#ch01_y", true, getcharid(0));
		cloaknpc("Mark#ch01_y", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(24098) == 1 ) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "It's displaying elemental attributes without actually changing elements...";
		mes "I've never heard of Hal in this state before, so it'd be best to ask the <B>Harbans</B> directly..";
		close3;
	}
	if ( isbegin_quest(24099) == 1 ) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Just seeing it is remarkable!";
		mes "Multiple energy were abnormally absorbed, so it doesn't hold all powers equally, but it also isn't biased toward any single one, a <B>special Hal</B> has been created.";
		cloaknpc("Chez#ch01_y", false, getcharid(0));
		cloaknpc("Magi#ch01_y", false, getcharid(0));
		cloaknpc("Mark#ch01_y", false, getcharid(0));
		next;
		mes "[Yuvilus]";
		mes "With the traits of this Hal we should be able to fuse <B>magic, holy energy, and medical potions</B>.";
		next;
		mes "[Yuvilus]";
		mes "Hurry back to Ashridge and pass the information, we've found the special Hal that can eradicate the <B>Legion's parasitic spores</B>!";
		mes "I'll stay here and try to produce more <B>special Hal</B>.";
		mes "I'll send them to the village as soon as they're ready.";
		erasequest 24099;
		setquest 23289;
		getitem "Ch1_Root_Coin",10;
		getexp 130609489, 4000000;
		close2;
		cutin "",255;
		cloaknpc("Chez#ch01_y", true, getcharid(0));
		cloaknpc("Magi#ch01_y", true, getcharid(0));
		cloaknpc("Mark#ch01_y", true, getcharid(0));
		end;
	}
	if ( isbegin_quest(23289) == 1 ) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Just seeing it is remarkable!";
		mes "Multiple energy were abnormally absorbed, so it doesn't hold all powers equally, but it also isn't biased toward any single one, a <B>special Hal</B> has been created.";
		next;
		mes "[Yuvilus]";
		mes "With the traits of this Hal we should be able to fuse <B>magic, holy energy, and medical potions</B>.";
		next;
		mes "[Yuvilus]";
		mes "Hurry back to Ashridge and pass the information, we've found the special Hal that can eradicate the <B>Legion's parasitic spores</B>!";
		mes "I'll stay here and try to produce more <B>special Hal</B>.";
		mes "I'll send them to the village as soon as they're ready.";
		close3;
	}
	end;
	
OnFinishCheck:
	if ( 
		isbegin_quest(24110) == 1 && countitem("aegis_1001977") >= 10 ||
		checkquest(24112, HUNTING) == 2 ||
		checkquest(24114, HUNTING) == 2 ||
		checkquest(24116, HUNTING) == 2
	) {
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Well done.";
		mes "Thanks to you, preparations for farming are progressing smoothly.";
		if ( isbegin_quest(24110) == 1 && countitem("aegis_1001977") >= 10 ) {
			delitem "aegis_1001977", 10;
			erasequest 24110;
			setquest 24111;
			getitem "Ch1_Root_Coin", 5;
		}
		if ( checkquest(24112, HUNTING) == 2 ) {
			erasequest 24112;
			setquest 24113;
			getitem "Ch1_Root_Coin", 5;
		}
		if ( checkquest(24114, HUNTING) == 2 ) {
			erasequest 24114;
			setquest 24115;
			getitem "Ch1_Root_Coin", 5;
		}
		if ( checkquest(24116, HUNTING) == 2 ) {
			erasequest 24116;
			setquest 24117;
			getitem "Ch1_Root_Coin", 5;
		}
		close3;
	} else {
		return;
	}
end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24088) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24097) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24099) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "CH1_Complete() && isbegin_quest(24108) == 0 && isbegin_quest(24109) < 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24109) == 1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24110) == 0 && checkquest(24111, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24110) == 0 && checkquest(24111, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24110) == 1 && countitem(\"aegis_1001977\") >= 10";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24112) == 0 && checkquest(24113, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24112) == 0 && checkquest(24113, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(24112, HUNTING) == 2";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24114) == 0 && checkquest(24115, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24114) == 0 && checkquest(24115, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(24114, HUNTING) == 2";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24116) == 0 && checkquest(24117, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24109) == 2 && isbegin_quest(24116) == 0 && checkquest(24117, PLAYTIME) == 2";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "checkquest(24116, HUNTING) == 2";
end;
}

ch1_sf01,120,224,5	script	Harban#ch01_2	4_CH1_KAL,{
	if ( isbegin_quest(24108) == 1 ) {
		mes "[Harban]";
		mes "Farming?";
		mes "Don't you know how to farm?";
		mes "Don't you people grow foods where you're from?";
		mes "It's odd that you're asking how to farm.";
		next;
		mes "[Harban]";
		mes "You simply plant it, give it fertilizer, and diligently look over it.";
		mes "The joy of harvesting your own work can't be explained with words.";
		next;
		mes "[Harban]";
		mes "Anyways, if you're interested in farming, you should try raising your own Hal.";
		mes "You're not after just some common Hal, right?";
		next;
		mes "[Harban]";
		mes "If you're planning to grow something you better stock up on Hal first, expect to fail a lot, that's normal, right?";
		erasequest 24108;
		setquest 24109;
		close;
	}
	if ( isbegin_quest(24109) == 1 ) {
		mes "[Harban]";
		mes "Anyways, if you're interested in farming, you should try raising your own Hal.";
		mes "You're not after just some common Hal, right?";
		next;
		mes "[Harban]";
		mes "If you're planning to grow something you better stock up on Hal first, expect to fail a lot, that's normal, right?";
		close;
	}
	if ( isbegin_quest(24098) == 1 ) {
		mes "[Harban]";
		mes "How's the work coming?";
		mes "Farming Hal isn't something just anyone can do, everyone's really putting in the effort.";
		next;
		select("Show the blemished Hal.");
		mes "[Harban]";
		mes "Th-this thing.";
		mes "Who went and messed up a precious Hal like this?";
		next;
		mes "[Harban]";
		mes "This thing doesn't even have an attribute, yet it's soaked up magic for nothing, and it doesn't even taste like anything, it's defective.";
		mes "The only thing it's good for is helping other Hal's flavor to blend better, but isn't that no different from just eating two together?";
		mes "You just wasted perfectly good Hal!";
		next;
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Wait a moment!";
		mes "You said it can <B>blend flavors</B>?";
		mes "This is it! This is really it!";
		next;
		mes "[Yuvilus]";
		mes "Do you know how to make this?";
		next;
		cutin "",255;
		mes "[Harban]";
		mes "It's unnecessarily hard to make and doesn't even taste good.";
		mes "Basically, as long as you add just enough of any power to avoid changing the Hal's attribute, then add another energy, and another, and another... keep doing that and you end up with a Hal that has no trait but also isn't pure, just a very weird Hal.";
		next;
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Thank you!";
		mes "That gave me an idea how to solve the problem we have!";
		next;
		cutin "",255;
		mes "[Harban]";
		mes "I don't know why anyone would want to make something like that, but great for you~";
		mes "Just don't eat it. It's not a substance meant to be eaten...";
		next;
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Alright, we're short on time, let's head back to my place and talk more there.";
		erasequest 24098;
		setquest 24099;
		close3;
	}
	if ( isbegin_quest(24099) == 1 ) {
		mes "[Harban]";
		mes "I don't know why anyone would want to make something like that, but great for you~";
		mes "Just don't eat it. It's not a substance meant to be eaten...";
		next;
		cutin "ch1_jb01.png",2;
		mes "[Yuvilus]";
		mes "Alright, we're short on time, let's head back to my place and talk more there.";
		close3;
	}
	mes "[Harban]";
	mes "Farming means planting Hal in the ground, giving it fertilizer, and taking care of it with devotion.";
	mes "The joy of harvesting Hal that you've raised with care can't be explained with words.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24098) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24108) == 1";
end;
}

ch1_sf01,108,229,5	script(CLOAKED)	Chez#ch01_y	4_F_FAIRYKID4,{
	end;
}

ch1_sf01,107,230,5	script(CLOAKED)	Magi#ch01_y	4_F_FAIRYKID4,{
	end;
}

ch1_sf01,109,224,3	script(CLOAKED)	Mark#ch01_y	4_EP18_MARK,{
	end;
}

mu_fild01,237,299,3	script	Worker#ch01_1	4_M_REPAIR,{
	if ( isbegin_quest(24089) == 1 || isbegin_quest(24090) == 1 ) {
		cutin "",255;
		mes "[Worker]";
		mes "Hello.";
		mes "You're right on time.";
		mes "The temperature is starting to feel too hot.";
		next;
		mes "[Worker]";
		mes "Are you here to take the Hals?";
		mes "If it's for research, then it's all good.";
		mes "That's what we gathered it for after all.";
		mes "I hope it leads to good results!";
		if ( isbegin_quest(24089) == 1 ) {
			erasequest 24089;
			setquest 24090;
		}
		close2;
		navigateto("mu_fild01", 220, 313);
		end;
	}
	mes "[Worker]";
	mes "Hello.";
	mes "You're right on time.";
	mes "The temperature is starting to feel too hot.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24089) == 1";
end;
}

mu_fild01,220,313,3	script	Worker#ch01_2	4_M_REPAIR,{
	if ( isbegin_quest(24090) == 1 || isbegin_quest(24091) == 1 ) {
		mes "[Worker]";
		mes "Phew~";
		mes "I can finally breathe.";
		mes "It's tough for me since I'm sensitive to heat.";
		next;
		mes "[Worker]";
		mes "The Hal is over here.";
		mes "I worked really hard to gather it, you know??";
		mes "Put it to good use..";
		if ( isbegin_quest(24090) == 1 ) {
			erasequest 24090;
			setquest 24091;
		}
		close2;
		navigateto("mu_fild01", 166, 370);
	}
	mes "[Worker]";
	mes "Phew~";
	mes "I can finally breathe.";
	mes "It's tough for me since I'm sensitive to heat.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24090) == 1";
end;
}

mu_fild01,166,370,5	script	Worker#ch01_3	4_M_REPAIR,{
	if ( isbegin_quest(24091) == 1 || isbegin_quest(24092) == 1 ) {
		mes "[Worker]";
		mes "The harvest isn't bad at all.";
		mes "I didn't think I'd be able to use herbalist skills to use here.";
		mes "Hehehe";
		next;
		mes "[Worker]";
		mes "Oh!";
		mes "Are you here to collect Hal?";
		mes "There's a lot so carry it carefully.";
		mes "Some of them might've been infused with fire energy already, so make sure to sort them properly.";
		next;
		mes "- Does the Hal gets infused if exposed to a certain energy for a long time? -";
		mes "- I better keep that in mind. -";
		if ( isbegin_quest(24091) == 1 ) {
			erasequest 24091;
			setquest 24092;
		}
		close2;
		navigateto("mu_fild01", 134, 311);
		end;
	}
	mes "[Worker]";
	mes "The harvest isn't bad at all.";
	mes "I Didn't think I'd be able to use herbalist skills to use here.";
	mes "Hehehe";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24091) == 1";
end;
}

mu_fild01,134,311,5	script	Worker#ch01_4	4_M_REPAIR,{
	if ( isbegin_quest(24092) == 1 || isbegin_quest(24093) == 1 ) {
		mes "[Worker]";
		mes "Sigh...";
		mes "I want to go home.";
		mes "I signed up as a combatant, but here I am picking up stones...";
		next;
		mes "[Worker]";
		mes "What are you looking at!";
		mes "Just take those stones already and go!";
		if ( isbegin_quest(24092) == 1 ) {
			erasequest 24092;
			setquest 24093;
		}
		close2;
		navigateto("mu_fild01", 126, 306);
		end;
	}
	mes "[Worker]";
	mes "Sigh...";
	mes "I want to go home.";
	mes "I signed up as a combatant, but here I am picking up stones...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24092) == 1";
end;
}

mu_fild01,126,306,3	script	Worker#ch01_5	4_M_REPAIR,{
	if ( isbegin_quest(24093) == 1 || isbegin_quest(24094) == 1 ) {
		mes "[Worker]";
		mes "Did you know?";
		mes "Hal is very tasty.";
		next;
		mes "[Worker]";
		mes "I'll be eating the ones that changed color already.";
		mes "Take the transparent ones with you.";
		if ( isbegin_quest(24093) == 1 ) {
			erasequest 24093;
			setquest 24094;
		}
		close2;
		navigateto("mu_fild01", 35, 194);
		end;
	}
	mes "[Worker]";
	mes "Did you know?";
	mes "Hal is very tasty.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24093) == 1";
end;
}

mu_fild01,35,194,3	script	Worker#ch01_6	4_M_REPAIR,{
	if ( isbegin_quest(24094) == 1 || isbegin_quest(24095) == 1 ) {
		mes "[Worker]";
		mes "Because of Muspelheim's environment, it seems that if you gather Hal and leave it unattended, it often ends up getting infused with fire energy.";
		next;
		mes "[Worker]";
		mes "Once Hal changes like that, the quality drops and it loses its value, high quality Hal is carefully farmed and nurtured by the Harbans.";
		next;
		mes "- Farming Hal... how do they even do that? -";
		if ( isbegin_quest(24094) == 1 ) {
			erasequest 24094;
			setquest 24095;
		}
		close2;
		navigateto("mu_fild01", 91, 196);
		end;
	}
	mes "[Worker]";
	mes "Because of Muspelheim's environment, it seems that if you gather Hal and leave it unattended, it often ends up getting infused with fire energy.";
	next;
	mes "[Worker]";
	mes "Once Hal changes like that, the quality drops and it loses its value, high quality Hal is carefully farmed and nurtured by the Harbans.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24094) == 1";
end;
}

mu_fild01,91,196,5	script	Worker#ch01_7	4_M_REPAIR,{
	if ( isbegin_quest(24095) == 1 || isbegin_quest(24096) == 1 ) {
		mes "[Worker]";
		mes "Where does Hal even come from and how is it this unique?";
		mes "We gather it because it's there, but before we gather it, it's just Hal and then it changes under some kind of influence. It's really a mysterious substance.";
		if ( isbegin_quest(24095) == 1 ) {
			erasequest 24095;
			setquest 24096;
		}
		close2;
		navigateto("mu_fild01", 121, 236);
		end;
	}
	mes "[Worker]";
	mes "Where does Hal even come from and how is it this unique?";
	mes "We gather it because it's there, but before we gather it, it's just Hal and then it changes under some kind of influence. It's really a mysterious substance.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24095) == 1";
end;
}

mu_fild01,121,236,5	script	Worker#ch01_8	4_M_REPAIR,{
	if ( isbegin_quest(24096) == 1 || isbegin_quest(24097) == 1 ) {
		mes "[Worker]";
		mes "Ah!";
		mes "I failed to get a lot of Hal...";
		mes "I only have a few, I'll give you what I have.";
		next;
		mes "[Worker]";
		mes "H-huh...";
		mes "I kept it in my pouch... It turned it into this weird <B>blemished color</B>.";
		mes "I'll get stomach ache if I eat this...";
		mes "Should I throw it away?";
		next;
		mes "- Unlike the ones seen so far, this Hal has a blemished color. -";
		mes "- Let's set it aside for now. -";
		next;
		mes "- Seems like we've gathered enough Hal by now. -";
		mes "- Let's report back to <B>Yuvilus</B>. -";
		if ( isbegin_quest(24096) == 1 ) {
			erasequest 24096;
			setquest 24097;
		}
		close2;
		navigateto("mu_fild01", 198, 186);
		end;
	}
	mes "[Worker]";
	mes "Ah!";
	mes "I failed to get a lot of Hal...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24096) == 1";
end;
}

function	script	CH1_Cure_Check	{
	for ( .@i = 23295; .@i <= 23297; ++.@i ) {
		if ( isbegin_quest(.@i) == 1 )
			return;
	}
	next;
	mes "I have given all the doses of the cure. Let's return to <NAVI>[Maxim]<INFO>ch1_gef_in,172,187,0,101,0</INFO></NAVI>.";
	setquest 23298;
	close;
}

ch1_gef_in,61,76,3	script	Weak Patient#ch1_t_p1	4_M_PATIENT01,{
	if ( isbegin_quest(23295) == 1 ) {
		mes "[Weak Patient]";
		mes "Are you here to give me a medicine? Sigh, I wonder if it'll actually work this time...";
		mes "Ah, I ended up saying something discouraging without thinking. Sorry.";
		next;
		mes "[Weak Patient]";
		mes "I know that everyone's doing their best. But when you get your hopes up every time, the disappointment just hits harder and leaves you more drained than before.";
		next;
		mes "[Weak Patient]";
		mes "Fighting an unknown disease with no sign of recovery is harder than you think. I'm sure you can understand how I feel, right?";
		erasequest 23295;
		CH1_Cure_Check();
		close;
	}
	mes "[Weak Patient]";
	mes "Ugh, my whole body aches.";
	next;
	mes "[Weak Patient]";
	mes "I've been coughing so much, my chest hurts...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23295) == 1";
end;
}

ch1_gef_in,107,33,3	script	Skinny Patient#ch1_	4_M_PATIENT02,{
	if ( isbegin_quest(23296) == 1 ) {
		mes "[Skinny Patient]";
		mes "The cure's been developed? Really? So there is hope after all. You even took your time to deliver it personally, thank you so much.";
		next;
		mes "[Skinny Patient]";
		mes "I'm not even hoping for a full recovery. I just wish this ridiculous hunger would go away.";
		next;
		mes "[Skinny Patient]";
		mes "My body already feels heavy and weak. Eating doesn't restore my strength, it just makes me more sluggish. The worst part is it doesn't go away...";
		next;
		mes "[Skinny Patient]";
		mes "It might not sound like much when you just hear about it, but it's seriously annoying when you're the one dealing with it.";
		next;
		mes "[Skinny Patient]";
		mes "Ugh, just talking about it is making me hungry again...";
		erasequest 23296;
		CH1_Cure_Check();
		close;
	}
	mes "[Skinny Patient]";
	mes "Don't talk to me. The more I talk, the hungrier I get, and it puts me in a foul mood...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23296) == 1";
end;
}

ch1_gef_in,110,102,3	script	Drowsy Patient#ch1_t_p3	4_F_PATIENT01,{
	if ( isbegin_quest(23297) == 1 ) {
		mes "[Drowsy Patient]";
		mes "Is that the cure? I don't care what type it is, just give it to me quick.";
		next;
		mes "[Drowsy Patient]";
		mes "I want to fall into a deep sleep, but I can't rest properly because the cold sweats keep coming. Even when I sleep, it doesn't feel like I have rested at all...";
		next;
		mes "[Drowsy Patient]";
		mes "I can't even remember the last time I slept properly...";
		next;
		mes "[Drowsy Patient]";
		mes "Huwaaah...";
		next;
		mes "[Drowsy Patient]";
		mes "Oh, is it already over? I dozed off for a second. Sorry.";
		erasequest 23297;
		CH1_Cure_Check();
		close;
	}
	mes "[Drowsy Patient]";
	mes "Ahh, I'm so sleepy...";
	mes "I want to go home already...";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(23297) == 1";
end;
}

hem_dun01,207,246,0	script	#w_wp_01	HIDDEN_WARP_NPC,2,2,{
	end;
	
OnTouchNPC:
	unitskilluseid getattachedrid(), "AL_TELEPORT", 1;
end;
	
OnTouch:
	if ( isbegin_quest(12663) == 1 || isbegin_quest(12666) == 1 ) {
		cloaknpc("Est#wsp1", false, getcharid(0));
		end;
	}
	if ( isbegin_quest(12660) > 0 ) {
		cloaknpc("Wizard Professor#w1", false, getcharid(0));
		end;
	}
end;
}

hem_dun01,202,246,0	duplicate(#w_wp_01)	#w_wp_02	HIDDEN_WARP_NPC,0,5
hem_dun01,212,246,0	duplicate(#w_wp_01)	#w_wp_03	HIDDEN_WARP_NPC,0,5
hem_dun01,207,241,0	duplicate(#w_wp_01)	#w_wp_04	HIDDEN_WARP_NPC,5,0
hem_dun01,207,251,0	duplicate(#w_wp_01)	#w_wp_05	HIDDEN_WARP_NPC,5,0

hem_dun01,205,248,4	script(CLOAKED)	Wizard Professor#w1	9,{
	if ( isbegin_quest(12660) == 0 ) {
		end;
	}
	if ( isbegin_quest(12660) == 1 ) {
		mes "[Wizard Professor]";
		mes "Nice to meet you. A student... no, what is your name adventurer?";
		next;
		select("Tell your name.");
		mes "[Wizard Professor]";
		mes strcharinfo(0)+", Ah~ Now I remember. You're on the list that are invited to come. Welcome.";
		next;
		mes "[Wizard Professor]";
		mes "I'm one of the professors who is handling the development of the Illusion Codex. I'm the one who's developing the feel and pain receptors in the virtual environment.";
		next;
		mes "[Wizard Professor]";
		mes "The connect the person's sensation to the mana system in the virtual space, we have to do...";
		next;
		mes "^0000ff You've listened for the professor for over 45 minutes. ^000000";
		next;
		mes "[Wizard Professor]";
		mes "Anyways, regarding that giant hand when you defeated Himmelmez. We need to reconstruct the same moment when you defeated Himmelmez.";
		next;
		mes "[Wizard Professor]";
		mes "That's the reason we've recreated the Dark Whisper. The same as what Est and you,, "+strcharinfo(0)+" have reported.";
		next;
		mes "[Wizard Professor]";
		mes "I forgot to mention~ We've might have added some other stuffs. A tiny, slightly, and some personal touch by ramping up the pain receptors and feel level by a hundred times~ I hope you give it a chance.";
		next;
		mes "[Wizard Professor]";
		mes "Now then, place your hands here. That's it, the system recognized your data already. You are free to access and recreate the Dark Whisper anytime.";
		close2;
		completequest 12660;
		end;
	}
	switch ( checkquest(12661, PLAYTIME) ) {
		case -1:
			break;
		case 0:
		case 1:
			end;
		case 2:
			erasequest 12661;
			break;
	}
	.@party_id = getcharid(1);
	if ( !.@party_id ) {
		instance_warning(1);
	}
	.@md_name$ = "Simulated Dark Whisper";
	.@active = instance_live_info(ILI_NAME, instance_id(IM_PARTY)) == .@md_name$;
	if ( .@active == false && is_party_leader() == true ) {
		select("Create Simulated Dark Whisper");
		if(instance_create(.@md_name$) == -3) {
			instance_warning(2, .@md_name$);
		}
		end;
	}
	if ( .@active == true ) {
		select("Enter Simulated Dark Whisper");
		if( instance_enter(.@md_name$) != IE_OK ){
			mes "^0000ffTalk to me again after everything is ready^000000";
			close;
		}
		setquest 12661;
		if ( isbegin_quest(12662) == 1 ) {
			erasequest 12662;
		}
		setquest 12662;
	} else {
		instance_warning(4);
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(12660) == 1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "checkquest(12661, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "checkquest(12661, PLAYTIME) == 2";
	setunitdata getnpcid(0), UNPC_CLASS, 9;
	setunitdata getnpcid(0), UNPC_SEX, 0;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_CLOTHCOLOR, 3;
	setunitdata getnpcid(0), UNPC_HEADTOP, 1938;
	setunitdata getnpcid(0), UNPC_ROBE, 65;
end;
}

hem_dun01,209,248,4	script(CLOAKED)	Est#wsp1	4_F_ESTLOVELOY,{
	if ( isbegin_quest(12663) == 1 ) {
		cutin "ep162_est02.png",2;
		mes "[Est]";
		mes "I was wondering who'd they send on this operation, so it's you again.";
		next;
		select("So you're the commander, Est.");
		cutin "ep162_est01.png",2;
		mes "[Est]";
		mes "Yes, I'm in charge of the this operation. It's reassuring that you're the one participating.";
		next;
		select("Do you already have a plan?");
		cutin "ep162_est02.png",2;
		mes "[Est]";
		mes "I see that you're also curious. We will disguise ourselves as an infected. Our priority is to avoid battle as much as possible, we'll only engage in battle on unavoidable situations.";
		next;
		cutin "ep162_est01.png",2;
		mes "[Est]";
		mes "Avoid unnecessary fights and we'll focus everything on our main objective. Don't forget that it's our goal.";
		next;
		select("Where do we enter from?");
		cutin "ep162_est03.png",2;
		mes "[Est]";
		mes "We can't knock on the front door. We're going to enter from the tunnel where the infecteds are leaking out from. It'll be disgusting, but do you still want to do it?";
		next;
		select("Of course.", "Let's do this.");
		cutin "ep162_est02.png",2;
		mes "[Est]";
		mes "That's what I like to hear. Prepare your items first. Talk to me once you're ready.";
		close2;
		cutin "",255;
		erasequest 12663;
		setquest 12666;
		end;
	}
	if ( isbegin_quest(12666) == 1 || isbegin_quest(12664) == 1 ) {
		.@party_id = getcharid(1);
		if ( !.@party_id ) {
			mes "^0000ffTalk to me again after everything is ready^000000";
			close;
		}
		.@md_name$ = "Ominous Dark Whisper";
		.@active = instance_live_info(ILI_NAME, instance_id(IM_PARTY)) == .@md_name$;
		if ( .@active == false && is_party_leader() == true ) {
			select("Create Ominous Dark Whisper");
			if(instance_create(.@md_name$) == -3) {
				instance_warning(2, .@md_name$);
			}
			end;
		}
		if ( .@active == true ) {
			select("Enter Ominous Dark Whisper");
			if( instance_enter(.@md_name$) != IE_OK ){
				mes "^0000ffTalk to me again after you are ready^000000";
				close;
			}
		} else {
			mes "^0000ffTalk to me again after you are ready^000000";
		}
	}
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(12663) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(12664) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(12666) == 1";
end;
}

ygg_edge,178,185,5	script	Yvain#ch01_v	4_F_FAIRYKID2,2,2,{
	if ( !CH1_Complete() ) {
		end;
	}
	if ( isbegin_quest(24103) == 2 ) {
		switch ( checkquest(24106, PLAYTIME) ) {
			case -1:
				break;
			case 0:
			case 1:
				cutin "ch1_yvain04.png",2;
				mes "[Yvain]";
				mes "Thank you again.";
				mes "But please rest for today and come help again tomorrow.";
				mes "We don't want any more patients.";
				close3;
			case 2:
				erasequest 24106;
				break;
		}
		if ( isbegin_quest(24105) == 1 ) {
			cutin "ch1_yvain04.png",2;
			mes "[Yvain]";
			mes "We've received word that the injured have arrived safely at the infirmary.";
			mes "Thank you.";
			erasequest 24105;
			setquest 24106;
			getitem "Ch1_Root_Coin",5;
			getexp 16364720, 1000000;
			close3;
		}
		if ( isbegin_quest(24104) == 0 ) {
			cutin "ch1_yvain04.png",2;
			mes "[Yvain]";
			mes "I'm worried that some <B>researchers in the Land of Darkness</B> might be <B>injured</B>.";
			mes "I know all of them are doing their best to stay safe...";
			next;
			mes "[Yvain]";
			mes "<B>If you're going on the Land of Darkness</B>, could I ask you to check if there are injured people?";
			next;
			if ( select("Accept.", "Decline.") == 2 ) {
				cutin "ch1_yvain04.png",2;
				mes "[Yvain]";
				mes "Okay, I understand.";
				close3;
			}
			mes "[Yvain]";
			mes "Thank you.";
			mes "If you meet any <B>injured researchers in the Land of Darkness</B>, make sure to send them back to the village.";
			setquest 24104;
			close3;
		}
		if ( isbegin_quest(24104) == 1 ) {
			cutin "ch1_yvain04.png",2;
			mes "[Yvain]";
			mes "<B>If you're going on the Land of Darkness</B>, could I ask you to check if there are injured people?";
			close3;
		}
	}
	if ( isbegin_quest(24103) == 1 ) {
		cutin "ch1_yvain04.png",2;
		mes "[Yvain]";
		mes "We've received word that the injured have arrived safely at the infirmary.";
		mes "Thank you.";
		next;
		mes "[Yvain]";
		mes "It's a relief that the Land of Darkness is starting to stabilize.";
		mes "The monster amount seems to have been reduced, thanks to that we've able to identify the other locations of dimensional dimension.";
		completequest 24103;
		getitem "Ch1_Root_Coin",10;
		getexp 32652372, 2000000;
		close3;
	}
	if ( 
		isbegin_quest(24100) == 1 ||
		isbegin_quest(24101) == 1 ||
		isbegin_quest(24102) == 1
	) {
		cutin "ch1_yvain04.png",2;
		mes "[Yvain]";
		mes "I'm so worried about the researchers who went out for investigation, but I can't leave the village, it's so frustrating...";
		next;
		mes "[Yvain]";
		mes "<B>If you're going on the Land of Darkness</B>, could I ask you to check on the researchers there?";
		mes "Most of them do return safely and report from time to time, but there are times when we lose contact with a few, and that worries me a lot.";
		close2;
		cutin "",255;
		if ( isbegin_quest(24100) == 1 ) {
			navigateto("ygg_roots", 280, 99);
		}
		else if ( isbegin_quest(24101) == 1 ) {
			navigateto("ygg_roots", 208, 95);
		}
		else if ( isbegin_quest(24102) == 1 ) {
			navigateto("ygg_roots", 120, 170);
		}
		end;
	}
	if ( isbegin_quest(24100) == 0 ) {
		cutin "ch1_yvain04.png",2;
		mes "[Yvain]";
		mes "Thank goodness that you're safe.";
		mes "Do you know how things are going in the Land of Darkness?";
		next;
		select("It's the same as before.");
		mes "[Yvain]";
		mes "Is that so?";
		mes "I feel sorry for staying behind the village, while everyone is investigating such a dangerous place.";
		next;
		mes "[Yvain]";
		mes "I'm so worried about the researchers who went out for investigation, but I can't leave the village, it's so frustrating...";
		next;
		mes "[Yvain]";
		mes "<B>If you're going on the Land of Darkness</B>, could I ask you to check on the researchers there?";
		mes "Most of them do return safely and report from time to time, but there are times when we lose contact with a few, and that worries me a lot.";
		setquest 24100;
		close2;
		cutin "",255;
		navigateto("ygg_roots", 280, 99);
		end;
	}
	end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "CH1_Complete() && isbegin_quest(24100) == 0";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24103) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24105) == 1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24103) == 2 && isbegin_quest(24104) == 0 && isbegin_quest(24105) != 1 && checkquest(24106, PLAYTIME) == -1";
	questinfo QTYPE_DAILYQUEST, QMARK_YELLOW, "isbegin_quest(24103) == 2 && isbegin_quest(24104) == 0 && isbegin_quest(24105) != 1 && checkquest(24106, PLAYTIME) == 2";
end;
}

ygg_roots,280,99,5	script	Investigator#ch01_y1	4_CAT_SAILOR2,{
	if ( isbegin_quest(24100) == 1 || isbegin_quest(24101) == 1 ) {
		mes "[Investigator]";
		mes "Ah!";
		mes "I didn't notice you!";
		mes "I've gotten so used to the tremors that I don't even notice when someone is touching me.";
		next;
		mes "[Investigator]";
		mes "I'm fine, but I haven't gotten any solid results yet, that's why I'm not heading back to the village.";
		next;
		mes "[Investigator]";
		mes "I'm being extra careful not to get hurt, please help the other who really need it.";
		if ( isbegin_quest(24100) == 1 ) {
			completequest 24100;
			setquest 24101;
		}
		close2;
		navigateto("ygg_roots", 208, 95);
		end;
	}
	mes "[Investigator]";
	mes "Ah!";
	mes "I didn't notice you!";
	mes "I've gotten so used to the tremors that I don't even notice when someone is touching me.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24100) == 1";
end;
}

ygg_roots,208,95,5	script	Investigator#ch01_y2	4_M_DESERT,{
	if ( isbegin_quest(24101) == 1 || isbegin_quest(24102) == 1 ) {
		mes "[Investigator]";
		mes "!!!";
		mes "Have I been away that long?";
		mes "I got so caught up watching the newly appearing monsters that started to emerged when the Land of Darkness started recovering that I completely lost track of time.";
		next;
		mes "[Investigator]";
		mes "The yellow jelly like monsters is called an Yggring and the one with a bunch of leaf is a Leafling, aren't they adorable?";
		next;
		mes "[Investigator]";
		mes "They're not aggressive, they're just cute little things. I might consider bringing back one for myself next time.";
		mes "There's nothing worth nothing other than that, I wonder if the others discovered anything?";
		if ( isbegin_quest(24101) == 1 ) {
			erasequest 24101;
			setquest 24102;
		}
		close2;
		navigateto("ygg_roots", 120, 170);
		end;
	}
	mes "[Investigator]";
	mes "!!!";
	mes "Have I been away that long?";
	mes "I got so caught up watching the newly appearing monsters that started to emerged when the Land of Darkness started recovering that I completely lost track of time.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24101) == 1";
end;
}

ygg_roots,120,170,5	script	Researcher#ch01_y3	4_M_SITDOWN,{
	if ( isbegin_quest(24102) == 1 || isbegin_quest(24103) == 1 ) {
		mes "[Investigator]";
		mes "Ah!!";
		mes "Adventurer, perfect timing.";
		mes "I need help returning to the village.";
		next;
		mes "[Investigator]";
		mes "I twisted my ankle while investigating the area and was wondering what to do, so I'm really lucky to have run into you.";
		next;
		mes "[Investigator]";
		mes "The Land of Darkness isn't particularly dangerous, the biggest threat might be the sudden earthquakes.";
		mes "It's hard to prepare for since it happens irregularly, everyone could get injured if not careful just like me.";
		if ( isbegin_quest(24102) == 1 ) {
			erasequest 24102;
			setquest 24103;
		}
		close2;
		navigateto("ygg_edge", 178, 185);
		end;
	}
	mes "[Researcher]";
	mes "The Land of Darkness isn't particularly dangerous, the biggest threat might be the sudden earthquakes.";
	close;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24102) == 1";
end;
}

ygg_roots,148,122,5	script	Investigator#ch01_dq1	4_M_REPAIR,{
	if ( isbegin_quest(24104) == 1 ) {
		if ( rand(1,100) > 10 ) {
			mes "[Investigator]";
			mes "My investigation is going smoothly.";
			mes "There's no need to worry about me.";
		} else {
			mes "[Investigator]";
			mes "Owh, my leg...";
			mes "Please help me return to the village.";
			erasequest 24104;
			setquest 24105;
			next;
			mes "- You've found an injured person.";
			mes "Let's escort them back to the village. -";
		}
		disablenpc;
		initnpctimer;
		close;
	}
	end;
	
OnTimer60000:
	stopnpctimer;
	enablenpc;
end;

OnInit:
	enablenpc;
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(24104) == 1";
end;
}

ygg_roots,276,283,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq2	4_M_REPAIR
ygg_roots,311,177,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq3	4_M_REPAIR
ygg_roots,242,141,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq4	4_M_REPAIR
ygg_roots,163,157,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq5	4_M_REPAIR
ygg_roots,145,278,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq6	4_M_REPAIR
ygg_roots,335,263,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq7	4_M_REPAIR
ygg_roots,265,170,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq8	4_M_REPAIR
ygg_roots,320,239,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq9	4_M_REPAIR
ygg_roots,194,264,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq10	4_M_REPAIR
ygg_roots,184,146,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq11	4_M_REPAIR
ygg_roots,246,82,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq12	4_M_REPAIR
ygg_roots,357,86,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq13	4_M_REPAIR
ygg_roots,210,253,5	duplicate(Investigator#ch01_dq1)	Investigator#ch01_dq15	4_M_REPAIR

ygg_edge,175,186,4	script	Rubiel#ch1p1rubiel01	4_CH1_DELEGACY03,{
	if ( CH1_Complete() && isbegin_quest(18379) == 0 ) {
		mes "[Rubiel]";
		mes "Ah, adventurer?!";
		mes "Do you have a moment? I could really use your help...";
		next;
		if ( select("What's going on?", "I'm busy right now.") == 2 ) {
			mes "[Rubiel]";
			mes "Oh~ Is that so? You're busy! Okay, I'll make it quick!";
			mes "We've come across a rather strange place.";
			next;
		}
		mes "[Rubiel]";
		mes "You know the weird gates in the Second World?";
		mes "Have you seen one while passing by?";
		mes "I've mainly been investigating that area.";
		next;
		mes "[Rubiel]";
		mes "The resistance from dimensional magic was too strong to get close before, but I finally found a way through.";
		mes "When I peeked inside....";
		next;
		mes "[Rubiel]";
		mes "Ugh, it was a complete mess.";
		mes "But I couldn't stay long, so I've been looking for an adventurer to explore in my place.";
		next;
		select("So, me?");
		mes "[Rubiel]";
		mes "Yup! You're strong!";
		mes "It's a dangerous area, so we need someone elite!";
		mes "If you're up for it, come to the Geffen Field of the Second World.";
		mes "I'll be waiting!";
		setquest 18379;
		close;
	}
	if ( isbegin_quest(18379) == 1 ) {
		mes "[Rubiel]";
		mes "It's on the Geffen Field of the Second World.";
		mes "Some really suspicious dimensional dimension have appeared there.";
		mes "Thankfully, they're not from another dimension...";
		next;
		mes "[Rubiel]";
		mes "I need a strong adventurer like you, so make sure you take a look!";
		close2;
		navigateto("hem_fild", 180, 263);
		end;
	}
	npctalk "I'll do my best to research and be of help!", "", bc_self;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "CH1_Complete() && isbegin_quest(18379) == 0";
end;
}

hem_fild,180,263,3	script	Recon Point#ch1p1safty	4_POINT_BLUE,{
	if ( isbegin_quest(18379) < 1 ) {
		end;
	}
	mes "This is the place Rubiel mentioned.";
	next;
	if ( select("Enter.", "Leave.") == 2 ) {
		end;
	}
	warp "ch1_sf03", 123, 251;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18379) == 1";
	questinfo QTYPE_CLICKME, QMARK_YELLOW, "isbegin_quest(18379) == 2";
end;
}

ch1_sf03,122,255,4	script	Rubiel#ch1p1rubiel02	4_CH1_DELEGACY03,{
	if ( isbegin_quest(18379) < 1 ) {
		end;
	}
	if ( isbegin_quest(18379) == 1 ) {
		mes "[Rubiel]";
		mes "You really came!";
		mes "I even picked out a safe place so we can talk comfortably!";
		next;
		select("So what's the matter?");
		mes "[Rubiel]";
		mes "I found three hazy gates in this field.";
		mes "Each one is overflowing with dimensional magic.";
		mes "But I figured out a way to resist that magic.";
		next;
		mes "[Rubiel]";
		mes "Have you seen the Mana Rings sold at the Gold Root Coin Store?";
		mes "Those Mana Rings contains high-quality mana, to be precise, it's the mana of the world tree.";
		mes "I've found a way to use them to resist dimensional magic.";
		next;
		select("So Mana Rings is needed?");
		mes "[Rubiel]";
		mes "If you want to explore beyond those dimensional dimension, I can use a Mana Ring to cast a dimensional magic resistance buff on you.";
		mes "How does that sound?";
		next;
		mes "[Rubiel]";
		mes "According to my earlier research~~";
		mes "The gate at the end of the eastern bridge leads to a place called Valkyrie Realm.";
		mes "The dimensional magic there isn't too strong, so it doesn't use up that many Mana Rings.";
		next;
		mes "[Rubiel]";
		mes "The other two leads to Geffenia according Maysel...";
		mes "I don't really know what either Valkyrie Realm or Geffenia are, but they're probably locations in Midgard, right?";
		next;
		mes "[Rubiel]";
		mes "Geffenia has such strong dimensional magic that you'll need a lot more Mana Rings.";
		mes "And you'll need to be a skilled adventurer to explore it too.";
		next;
		mes "[Rubiel]";
		mes "The eastern entrance that connects to the decayed Valkyrie Realm will be managed by a friend named Minuel going forward.";
		mes "Be sure to get the dimensional resistance buff from the Mana Ring before heading in.";
		next;
		mes "[Rubiel]";
		mes "As for the corrupted Geffenia area, come find me here and I'll apply the magic resistance buff for you.";
		mes "I'll be counting on you to handle both regions.";
		mes "Got it?";
		completequest 18379;
		close;
	}
	if ( BaseLevel < 260 ) {
		mes "[Rubiel]";
		mes "If you want to enter the decayed Valkyrie Realm, ask Minuel near the gate to apply dimensional magic resistance.";
		mes "As for the Corrupted Geffenia, it's too much for you right now.";
		next;
		mes "^4d4dffThe Corrupted Geffenia is for adventurers with a level of 260 or above.^000000";
		close;
	}
	mes "[Rubiel]";
	mes "Are you going to the Corrupted Geffenia? Do you need dimensional magic resistance?";
	next;
	if ( select("Dimensional Magic Resistance - 45 Mana Rings", "I don't need it.") == 2 ) {
		mes "[Rubiel]";
		mes "Okay, I'll be counting on you.";
		close;
	}
	if ( countitem("Ch1_Mana_Ring") < 45 ) {
		mes "[Rubiel]";
		mes "You'll need atleast 45 Mana Rings to resist the dimensional magic around in Corrupted Geffenia.";
		mes "You can exchange Root Gold Coins for Mana Rings at the shop in Ashridge.";
		close;
	}
	delitem "Ch1_Mana_Ring", 45;
	specialeffect2 EF_GUARD2;
	sc_start SC_CONTENTS_38, 3600000, true;
	mes "[Rubiel]";
	mes "This dimensional magic resistance will protect you in that place.";
	close;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(18379) == 1";
	questinfo QTYPE_CLICKME, QMARK_YELLOW, "isbegin_quest(18379) == 2";
end;
}

hem_fild,329,191,3	script	Minuel#ch1p1minuel01	4_M_FAIRYSOLDIER,{
	if ( BaseLevel < 230 ) {
		mes "[Minuel]";
		mes "Wait, wait! This place is too dangerous. I don't think you're ready for it.";
		next;
		mes "^4d4dffThis area is for adventurers with a level of 230 or above.^000000";
		close;
	}
	if ( isbegin_quest(18379) < 2 ) {
		mes "[Minuel]";
		mes "Hold on, this area ahead is too dangerous, so access is restricted.";
		mes "Please stand back.";
		close;
	}
	mes "[Minuel]";
	mes "Are you here for an expedition?";
	mes "It'll be dangerous if you don't have the dimensional magic resistance buff, so make sure to come back before that happens.";
	next;
	if ( select("Dimensional Magic Resistance - 20 Mana Rings", "I'm all good.") == 2 ) {
		mes "[Minuel]";
		mes "Alright. Be careful.";
		close;
	}
	if ( countitem("Ch1_Mana_Ring") < 20 ) {
		mes "[Minuel]";
		mes "To properly resist dimensional magic, you'll need at least 20 Mana Rings.";
		mes "You can exchange Root Gold Coins for Mana Rings at the shop in Ashridge.";
		close;
	}
	delitem "Ch1_Mana_Ring", 20;
	specialeffect2 EF_GUARD2;
	sc_start SC_CONTENTS_37, 3600000, true;
	mes "[Minuel]";
	mes "This will protect you from that dangerous place.";
	close;
	
OnInit:
	questinfo QTYPE_CLICKME, QMARK_YELLOW, "isbegin_quest(18379) == 2 && BaseLevel >= 230";
end;
}

hem_fild,334,188,3	script	Dimensional Boundary#to_hem_	21813,{
	if ( BaseLevel < 230 ) {
		mes "^4d4dffThis area is for adventurers with a level of 230 or above.^000000";
		close;
	}
	if ( isbegin_quest(18379) < 2 ) {
		npctalk "Hey, adventurer, wait, wait. That area is unstable. It's too dangerous.", "Minuel#ch1p1minuel01", BC_SELF;
		end;
	}
	if ( !getstatus(SC_CONTENTS_37) ) {
		end;
	}
	npctalk "It'll be dangerous if you don't have the dimensional magic resistance buff, so make sure to come back before that happens.", "Minuel#ch1p1minuel01", BC_SELF;
	if ( select("Enter the Northern Area.", "Enter the Southern Area.") == 2 ) {
		warp "hem_dun02", 160, 51;
	} else {
		warp "hem_dun02", 160, 242;
	}
	end;
}

hem_fild,133,337,3	script	Dimensional Boundary#to_gfn3	21813,{
	if ( BaseLevel < 260 || !getstatus(SC_CONTENTS_38) || isbegin_quest(18379) < 2 ) {
		pctalk "This area is too dangerous...";
		end;
	}
	warp "ch1_gfn03", 145, 33;
	end;
	
OnInit:
	questinfo QTYPE_NONE, QMARK_YELLOW, "isbegin_quest(18379) == 2 && BaseLevel >= 260";
end;
}

hem_fild,231,299,3	script	Dimensional Boundary#to_gfn1	21813,{
	if ( BaseLevel < 260 || !getstatus(SC_CONTENTS_38) || isbegin_quest(18379) < 2 ) {
		pctalk "This area is too dangerous...";
		end;
	}
	warp "ch1_gfn01", 59, 169;
	end;
	
OnInit:
	questinfo QTYPE_NONE, QMARK_YELLOW, "isbegin_quest(18379) == 2 && BaseLevel >= 260";
end;
}

ygg_edge,167,182,3	script	Naviel#c1p1naviel	4_M_FAIRYSOLDIER,{
	cutin "minuel02.bmp",2;
	mes "[Naviel]";
	mes "Do you need help navigating Ashridge?";
	while ( true ) {
		next;
		switch ( select("▷ Hvergelmir’s Spring", "▷ Twisted Land of Darkness", "▷ Prontera", "▷ Payon", "▷ Shops", "▷ Inn", "▷ Sour Fruit", "▷ Sweet Fruit", "▷ Fresh Fruit", "▷ Long Fruit", "Remove Mini-Map Marker", "No, it's fine.") ) {
			case 1:
				mes "[Naviel]";
				mes "<NAVI>[Hvergelmir's Spring]<INFO>ygg_edge,259,245,0,101,0</INFO></NAVI>";
				mes "Follow this passage to Hvergelmir's Spring. You can return to Midgard from there.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 259, 245, 0, 0xFFCCFFFF;
				break;
			case 2:
				mes "[Naviel]";
				mes "<NAVI>[Twisted Land of Darkness]<INFO>ygg_edge,31,117,0,101,0</INFO></NAVI>";
				mes "This passage will lead you to a region where experiencing a lot of spatial distortions. Most of the adventurers are currently there.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 31, 117, 1, 0xFFAAFF00;
				break;
			case 3:
				mes "[Naviel]";
				mes "<NAVI>[Prontera]<INFO>ygg_edge,216,91,0,101,0</INFO></NAVI>";
				mes "This passage is the connecting branch to the young tree growing near Prontera in Midgard.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 216, 91, 2, 0xFFFF1493;
				break;
			case 4:
				mes "[Naviel]";
				mes "<NAVI>[Payon]<INFO>ygg_edge,193,92,0,101,0</INFO></NAVI>";
				mes "This passage is the connecting branch to the young tree growing near Payon in Midgard.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 193, 92, 3, 0xFFFF82FF;
				break;
			case 5:
				mes "[Naviel]";
				mes "<NAVI>[Shops]<INFO>ygg_edge,141,209,0,101,0</INFO></NAVI>";
				mes "Follow this passage to the shops where you can buy and sell items. A helpful place for adventurers.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 141, 209, 4, 0xFF33CC45;
				break;
			case 6:
				mes "[Naviel]";
				mes "<NAVI>[Inn]<INFO>ygg_edge,89,186,0,101,0</INFO></NAVI>";
				mes "If you need to rest, uou should stop by there.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 89, 186, 5, 0xFFFF66FF;
				break;
			case 7:
				mes "[Naviel]";
				mes "<NAVI>[Sour Fruit]<INFO>ygg_edge,166,138,0,101,0</INFO></NAVI>";
				mes "An indoor space set within the world tree's fruit.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 166, 138, 6, 0xFFAAFF00;
				break;
			case 8:
				mes "[Naviel]";
				mes "<NAVI>[Sweet Fruit]<INFO>ygg_edge,196,119,0,101,0</INFO></NAVI>";
				mes "An indoor space set within the world tree's fruit.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 196, 119, 7, 0xFFFFFF00;
				break;
			case 9:
				mes "[Naviel]";
				mes "<NAVI>[Fresh Fruit]<INFO>ygg_edge,73,152,0,101,0</INFO></NAVI>";
				mes "An indoor space set within the world tree's fruit.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 73, 152, 8, 0xFFFFDC37;
				break;
			case 10:
				mes "[Naviel]";
				mes "<NAVI>[Long Fruit]<INFO>ygg_edge,149,130,0,101,0</INFO></NAVI>";
				mes "An indoor space set within the world tree's fruit.";
				mes "The location has been marked on your mini-map.";
				mes "Click the navigation links for directions.";
				viewpoint 1, 149, 130, 9, 0xFFFF82FF;
				break;
			case 11:
				mes "[Naviel]";
				mes "All location markers on the mini-map have been removed.";
				viewpoint 2, 1, 1, 0, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 1, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 2, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 3, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 4, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 5, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 6, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 7, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 8, 0xFFFFFFFF;
				viewpoint 2, 1, 1, 9, 0xFFFFFFFF;
				break;
			case 12:
				mes "[Naviel]";
				mes "The locations of the fruits can be a bit confusing. Feel free to ask anytime.";
				close3;
		}
	}
}

ygg_fruit,168,124,3	script	Hotbear Mur#ch1t01	2_M_OLDBLSMITH,{
	if ( !CH1_Complete() ) {
		cutin "ch1_hotbeer01.png",2;
		mes "[Hotbear Mur]";
		mes "Aren't you a remarkable one?";
		mes "How did you end up here?";
		mes "Were you captured just like me?";
		mes "Hahaha! I'm kidding.";
		next;
		mes "[Hotbear Mur]";
		mes "I'm preparing something that will be useful to all of you soon.";
		mes "Just wait for it.";
		close3;
	}
	cutin "ch1_hotbeer01.png",2;
	mes "[Hotbear Mur]";
	mes "I prepared some things that can be helpful to you. Would you like to take a look?";
	next;
	switch( select("Entwined Equipments.", "Dimensional World Shoes", "Entwined Armor/Robe Enchant", "Entwined Manteau/Muffler Enchant", "Entwined Boots/Shoes Enchant", "Dimensional Greaves/Boots/Shoes Enchant", "Quit") ) {
		case 1:
			cutin "",255;
			npctalk "Take your time and look around.", "", BC_SELF;
			callshop "CH1_STORE_4", 1;
			break;
		case 2:
			cutin "",255;
			npctalk "I've created some sturdy shoes.", "", BC_SELF;
			callshop "CH1_STORE_5", 1;
			break;
		case 3:
			mes "[Hotbear Mur]";
			mes "I can enchant your Entwined Armor and Robe.";
			mes "This is the following items that I can enchant.";
			mes "^i[450440] ^i[450441]";
			close2;
			item_enchant(160);
			break;
		case 4:
			mes "[Hotbear Mur]";
			mes "I can enchant your Entwined Manteau and Muffler.";
			mes "This is the following items that I can enchant.";
			mes "^i[480545] ^i[480546]";
			close2;
			item_enchant(161);
			break;
		case 5:
			mes "[Hotbear Mur]";
			mes "I can enchant your Entwined Boots and Shoes.";
			mes "This is the following items that I can enchant.";
			mes "^i[470333] ^i[470334]";
			close2;
			item_enchant(162);
			break;
		case 6:
			mes "[Hotbear Mur]";
			mes "I can enchant your Dimensional Greaves, Boots, and Shoes.";
			mes "This is the following items that I can enchant.";
			mes "^i[470339] ^i[470337] ^i[470338] ^i[470336] ^i[470335]";
			close2;
			item_enchant(163);
			break;
		case 7:
			break;
	}
	cutin "",255;
	end;
}

ygg_edge,202,204,4	script	Repairman#ch1edge	4_M_04,{ repairmain(); }

ygg_fruit,165,126,3	script	Gold Root Coin Exchanger#ch1t01	4_CH1_DELEGACY04,{
	if ( !CH1_Complete() ) {
		mes "[Gold Root Coin Exchanger]";
		mes "There are some goods in stock, do you want to take a look?";
		close2;
		callshop "CH1_STORE_1", 1;
		end;
	} else {
		mes "[Gold Root Coin Exchanger]";
		mes "I have a lot of goods that is useful for adventurers like you~!";
		next;
		if ( select("Gold Root Coin, Mana Ring Store", "Other Materials Store") == 1 ) {
			npctalk "It's very useful~ Thank you for patronage!", "", BC_SELF;
			callshop "CH1_STORE_2", 1;
		} else {
			npctalk "I can exchange a lot of materials!", "", BC_SELF;
			callshop "CH1_STORE_3", 1;
		}
	}
	end;
}

ygg_fruit,165,115,3	shop	Cathand Merchant#ch1p1	4_CAT_DOWN,611:-1,602:-1,601:-1,23280:-1,23288:-1,645:-1,656:-1,657:-1,501:-1,502:-1,503:-1,504:-1,506:-1,610:-1,713:-1,717:-1,1771:-1

ygg_fruit,167,119,3	script	Kafra Employee#ch1	4_F_KAFRA4,{
	cutin "kafra_04.bmp",2;
	mes "[Kafra Employee]";
	mes "Welcome. The Kafra Services are always by your side.";
	mes "Thanks to the given opportunity, we are now operating in Ashridge as well.";
	mes "How can I assist you?";
	next;
	switch ( select("Use Storage", "Rent a Cart", "Quit") ) {
		case 1:
			close2;
			cutin "",255;
			openstorage; //Edit to your needs
			end;
		
		case 2:
			if ( BaseClass != Job_Merchant ) {
				mes "[Kafra Employee]";
				mes "I apologize.";
				mes "But the cart rental service is only available for merchant classes.";
				close3;
			}
			if ( checkcart() == true ) {
				mes "[Kafra Employee]";
				mes "I apologize.";
				mes "But you already have a cart, and we can only rent one cart at a time.";
				close3;
			}
			if ( getskilllv("MC_PUSHCART") == 0 ) {
				mes "[Kafra Employee]";
				mes "You can only rent a cart after learning the \"Push Cart\" skill.";
				close3;
			}
			if ( Zeny < 700 ) {
				mes "[Kafra Employee]";
				mes "I apologize.";
				mes "But you don't have enough Zeny to pay for the cart's rental fee of 700 Zeny.";
				close3;
			}
			setcart;
			mes "[Kafra Employee]";
			mes "Thank you for using the Kafra Services.";
			close3;

		case 3:
			mes "[Kafra Employee]";
			mes "At Kafra Services, we strive to provide flawless services and strict management, ensuring that our customers can use our services at peace.";
			mes "Thank you for using the Kafra Services.";
			close3;

	}
}

yggdrasil01,145,100,0	script	#to_ygg_edge	GATE_SKYBLUE,1,1,{
	end;
	
OnTouch:
	warp "ygg_edge", 255, 240;
end;
}

// Yggdrasil Edge
ygg_edge,239,225,5	script	Knight#c01ms00	4_M_UNCLEKNIGHT,{
	npctalk "This is a great opportunity to gain experience. Are you ready?", "", bc_self;
	npctalk "I was born ready!", "Knight#c01ms01", bc_self;
	end;
}

ygg_edge,242,225,5	script	Knight#c01ms01	4_M_YOUNGKNIGHT,{
	npctalk "What an odd place, am I going to be alright...? No! I can do this! I'm the best!!", "", bc_self;
	end;
}

ygg_edge,153,162,3	script	Magenta#c01ms00	4_M_GUNSLINGER2,{
	npctalk "I'm taking watch over here~", "", BC_SELF;
	end;
}

ygg_edge,153,164,3	script	Denny#c01ms00	4_M_REBELLION3,{
	npctalk "The superstar Denny! Is on the floor!!! Da-dada! Bangbangbang!", "", BC_SELF;
	end;
}

ygg_edge,149,163,5	script	Rookie#c01ms00	4_M_ROOKIE,{
	npctalk "Why am I even here? With so many incredible people around me, why me...? Did they just grab everything in sight like packing for a in the last-minute for a trip?", "", BC_SELF;
	end;
}

ygg_edge,131,168,5	script	Adventurer#c01ms14	4_TOWER_15,{
	npctalk "Huh... What am I supposed to do here? I wish someone would tell me...", "", BC_SELF;
	end;
}

ygg_edge,128,167,5	script	Adventurer#c01ms15	4_M_SWORDMAN,{
	npctalk "I'll go wherever my help is needed!", "", BC_SELF;
	end;
}

ygg_edge,91,173,3	script	Cyan#c01ms00	4_F_REBELLION3,{
	npctalk "Oh, you're here too? Looks like we'll have a lot of work to do, right?", "", BC_SELF;
	end;
}

ygg_edge,88,172,5	script	Deryl#c01ms00	4_M_REBELLION2,{
	npctalk "Dropping your weapon here would be the end of you...", "", BC_SELF;
	end;
}

ygg_edge,200,238,3	script	Debon Aire#c01ms00	4_M_DEBON,{
	npctalk "Even here, the members of Arcwand Clan will do their best. We're the best clan after all.", "", BC_SELF;
	end;
}

ygg_edge,195,239,5	script	Birman Aire#c01ms00	4_M_BIRMAN,{
	npctalk "Need any help? The ask any members of the Golden Mace Clan members. We will always stand by you.", "", BC_SELF;
	end;
}

ygg_edge,199,235,3	script	Laperm Ornphier#c01ms00	4_F_LAPERM,{
	npctalk "Hello! Isn't it a coincidence meeting you here? The Sword Clan is also working together here!", "", BC_SELF;
	end;
}

ygg_edge,193,235,5	script	Sham Rumi#c01ms00	4_F_SHAM,{
	npctalk "Hello~ Are you thinking of joining the Crossbow Clan? Sorry, but I can't sign you up here~ We've been sent here to help. If you really want to join, come find us in Prontera.", "", BC_SELF;
	end;
}

ygg_edge,196,252,5	script	Adventurer#c01ms10	4_CAT_SAILOR2,{
	npctalk "A glass of milk please!", "", BC_SELF;
	end;
}

ygg_edge,200,254,3	script	Adventurer#c01ms11	4_CAT_CHEF,{
	npctalk "Being able to cook with new ingredients in a new place, life is really full of surprises!", "", BC_SELF;
	end;
}

ygg_edge,201,251,3	script	Adventurer#c01ms12	4_M_MERCAT2,{
	npctalk "I've rested enough and ate my fill, time to start moving.", "", BC_SELF;
	end;
}

ygg_edge,164,237,5	script	Adventurer#c01ms13	4_TOWER_14,{
	npctalk "The scenery here feels familiar yet new. It's absolutely beautiful. Don't you think so?", "", BC_SELF;
	end;
}

ygg_edge,200,250,3	script	Stelo#c01ms00	4_DR_STELO,{
	npctalk "I came because they said they needed help, but it feels like I'm only exploring here.", "", BC_SELF;
	end;
}

//= Yggdrasil Root
ygg_roots,68,285,3	script	As#c01ms00	4_EP17_AS,{
	npctalk "I wonder how this place looks like before it collapsed.", "", BC_SELF;
	end;
}

ygg_roots,67,253,5	script	Mr. Seo#c01ms00	4_EXJOB_MR_SEO,{
	npctalk "Hm, this place is fascinating. Do you also think so?", "", BC_SELF;
	end;
}

ygg_roots,144,310,3	script	Adventurer#c01ms00	4_TOWER_09,{
	npctalk "I forced myself to come here, I think it was worth it. Is it?", "", BC_SELF;
	sleep2 100;
	npctalk "I'm hungry. Do we have food left?", "Adventurer#c01ms01", BC_SELF;
	end;
}

ygg_roots,142,309,5	script	Adventurer#c01ms01	4_DR_TORTEL,{
	npctalk "There's nothing here. Maybe it got destroyed?", "Adventurer#c01ms01", BC_SELF;
	sleep2 100;
	npctalk "We almost ended up getting destroyed too.", "Adventurer#c01ms01", BC_SELF;
	end;
}

ygg_roots,355,92,7	script	Adventurer#c01ms02	4_GEFFEN_03,{
	npctalk "I wonder if the others are okay?", "", BC_SELF;
	end;
}

ygg_roots,256,165,5	script	Adventurer#c01ms03	4_GEFFEN_11,{
	npctalk "Phew- You, and everyone here came all this way and worked hard. Let's keep it up!", "", BC_SELF;
	end;
}

ygg_roots,256,151,3	script	Adventurer#c01ms04	4_GEFFEN_08,{
	npctalk "Wait, I think I saw something moving over there? It it just my imagination?", "", BC_SELF;
}

ygg_roots,167,258,3	script	Adventurer#c01ms05	4_M_RACHMAN1,{
	npctalk "We came all the way here, we might as well accomplish something before we leave!", "", BC_SELF;
	end;
}

ygg_roots,197,250,3	script	Adventurer#c01ms06	4_M_HUBOY,{
	npctalk "For a place called Land of Darkness, isn't this place too beautiful...?", "", BC_SELF;
	end;
}

ygg_roots,337,283,3	script	Adventurer#c01ms07	4_M_SITDOWN,{
	npctalk "Phew, adventuring and exploring should be done in a steady pace.", "", BC_SELF;
	end;
}

ygg_roots,250,234,5	script	Adventurer#c01ms08	4_M_THAIONGBAK,{
	npctalk "Ahahaha! Why don't we just catch everything here and study it all?", "", BC_SELF;
	end;
}

ygg_roots,137,142,5	script	Adventurer#c01ms09	4_M_CHNMONK,{
	npctalk "I came here out of curiosity, I didn't expect it to be this hard.", "", BC_SELF;
	end;
}

ygg_roots,268,141,3	script	Happy Cloud#c01ms00	4_EXJOB_HAPPY_CLOUD,{
	npctalk "So, you've come too adventurer. I heard the situation, this place is really desolated...", "", BC_SELF;
	end;
}



// Geffen
ch1_geffen,113,150,3	script	Geffen Resident#c01ms01	4_M_PATIENT01,{
	npctalk "You there... can you help me up? I left the clinic... I didn't want to be there, but... I'm out of breath... ugh...", "", BC_SELF;
	end;
}

ch1_geffen,165,124,3	script	Geffen Resident#c01ms02	4_F_EINWOMAN,{
	npctalk "It's been unsettling lately, I'm too afraid to even go outside. Why does bad things happen one after another...", "", BC_SELF;
	end;
}

ch1_geffen,43,126,5	script	Geffen Resident#c01ms03	4_F_BRZ_WOMAN,{
	npctalk "Are you from around here? Look around. There's nothing good to see here.", "", BC_SELF;
	sleep2 100;
	npctalk "That's right. There's nothing good to see here, get quickly while you still have the chance.", "Geffen Resident#c01ms04", BC_SELF;
	end;
}

ch1_geffen,42,124,5	script	Geffen Resident#c01ms04	4_M_KHMAN,{
	npctalk "What to do for dinner... Why am I even worrying about food in a situation like this...", "", BC_SELF;
	end;
}

ch1_geffen,123,65,1	script	Geffen Resident#c01ms05	4_F_MALAYA,{
	npctalk "I wonder if my friend is okay? She'll be okay, right? She has to be...", "", BC_SELF;
	end;
}

ch1_geffen,107,64,5	script	Geffen Resident#c01ms06	4_M_NINJA_BLUE,{
	npctalk "Cough- cough- ack... ha... It would be bearable if this cough would stop a bit.", "", BC_SELF;
	end;
}

ch1_geffen,82,194,3	script	Geffen Resident#c01ms07	4_GEFFEN_09,{
	npctalk "There must be a cure out there somewhere. Why can't we be treated?", "", BC_SELF;
	end;
}

ch1_geffen,128,105,5	script	Geffen Resident#c01ms08	4_M_SIT_NOVICE,{
	npctalk "Phew... This situation, isn't it too much?", "", BC_SELF;
}

ch1_geffen,65,78,3	script	Geffen Resident#c01ms09	4_M_GEF_SOLDIER,{
	npctalk "We will definitely overcome this.", "", BC_SELF;
	end;
}

ch1_geffen,102,106,7	script	Geffen Resident#c01ms20	4_M_SEAMAN,{
	npctalk "When are they going to clear out the rubble from the Geffen Tower?", "", BC_SELF;
	end;
}

ch1_gef_in,28,134,6	script	Geffen Resident#c01ms21	1_ETC_01,{
	npctalk "Please, have seat wherever you're comfortable.", "", BC_SELF;
	end;
}

ch1_gef_in,29,131,1	script	Geffen Resident#c01ms22	4_M_SEAMAN,{
	npctalk "What's the fastest one I can get done?", "", BC_SELF;
	end;
}

ch1_gef_in,26,131,7	script	Geffen Resident#c01ms23	4W_F_01,{
	npctalk "There was a time when this place was packed with people...", "", BC_SELF;
	end;
}

ch1_gef_in,23,125,5	script	Geffen Resident#c01ms24	4_M_HUMAN_01,{
	npctalk "Sigh, I should get going. Thanks for the meal.", "", BC_SELF;
	end;
}

ch1_gef_in,24,123,3	script	Geffen Resident#c01ms25	4_F_SCIENCE,{
	npctalk "Breaks time's over! Time to get back to work!", "", BC_SELF;
	end;
}

ch1_gef_in,163,104,3	script	Geffen Resident#c01ms26	4_M_ARUNA_NFM1,{
	npctalk "Ah! You can't just come in like that!", "", BC_SELF;
	end;
}

ch1_gef_in,157,103,5	script	Geffen Resident#c01ms27	4_GEFFEN_05,{
	npctalk "We mages have nothing to do with this incident~ Trust us~ Please, believe us~", "", BC_SELF;
	end;
}

ch1_geffen,111,201,5	script	Geffen Resident#c01ms28	4_F_PATIENT01,{
	npctalk "The cold breeze makes it bearable. Ah! Don't tell anyone you saw me here.", "", BC_SELF;
	end;
}

ch1_geffen,58,164,1	script	Geffen Resident#c01ms29	4_GEFFEN_05,{
	npctalk "How many am I supposed to buy again...", "", BC_SELF;
}

ch1_geffen,86,176,1	script	Geffen Resident#c01ms30	4W_F_01,{
	npctalk "All of this! It's all the Geffen Tower's fault!", "", BC_SELF;
	end;
}

ch1_gef_in,61,175,5	script	Patient#rch1_1	4_M_SITDOWN,{
	npctalk "It's so warm in here, but why do I keep shivering....", "", BC_SELF;
	end;
}

ch1_gef_in,77,174,1	script	Mage#rch1_2	1_M_WIZARD,{
	npctalk "We select the cleanest linens possible and wash them daily. Cleanliness is our top priority.", "", BC_SELF;
	end;
}

ch1_gef_in,76,172,3	script	Mage#rch1_3	2_F_MAGICMASTER,{
	npctalk "We are conducting experiments, but it's all part of our effort to cure Soren Syndrome.", "", BC_SELF;
	end;
}

ch1_geffen,143,182,3	script	Villager#rch1_4	1_M_02,{
	npctalk "Why is it so cold? I'm getting chills. Is there a ghost nearby?", "", BC_SELF;
	sleep2 100;
	npctalk "Cold? Isn't it actually humid right now?", "Villager#rch1_5", BC_SELF;
	end;
}

ch1_geffen,140,180,7	script	Villager#rch1_5	1_M_WIZARD,{
	npctalk "It feels hotter than usual because it's humid. Unlucky....", "", BC_SELF;
	sleep2 100;
	npctalk "Ack! I forgot that need to go to the clinic. Wait, I need to get my mask first....", "Villager#rch1_4", BC_SELF;
	end;
}

ch1_geffen,167,163,3	script	Villager#rch1_6	1_M_SMITH,{
	npctalk "They say you might catch Soren Syndrome just by going in and out of the clinic. The volunteers there are all risking their lives to work!", "", BC_SELF;
	end;
}

ch1_geffen,196,142,3	script	Herbalist#rch1_7	4_M_ORIENT01,{
	npctalk "Fresh herbs picked from outside town is for sale! ...Sigh, the other herbalists and hunter got sick first, so no one buys them anymore.", "", BC_SELF;
	end;
}

ch1_geffen,204,129,1	script	Villager#rch1_8	4_F_GODEMOM,{
	npctalk "There's nothing available to buy in the market....", "", BC_SELF;
}

ch1_geffen,168,82,3	script	Village Kid#rch1_9	4_M_KID1,{
	npctalk "Is there anything fun to do? Mom won't even let me go outside recently, it's frustrating!", "", BC_SELF;
	sleep2 100;
	npctalk "W-were not supposed to be wandering around like this? We might get sick.", "Village Kid#rch1_10", BC_SELF;
	end;
}

ch1_geffen,164,81,5	script	Village Kid#rch1_10	4_F_KID2,{
	npctalk "Hmp... I want to go home.", "", BC_SELF;
	sleep2 100;
	npctalk "Scaredy cat. Do you think Soren Syndrome just roams on the street? Can it grab you?", "Village Kid#rch1_9", BC_SELF;
	end;
}

ch1_geffen,120,55,5	script	Village Kid#rch1_12	1_F_MARIA,{
	npctalk "They say that clear water used to flow from that fountain. I can't imagine it, but did all the adults see it before?", "", BC_SELF;
	end;
}

ch1_geffen,115,59,5	script	Villager#rch1_13	1_F_GYPSY,{
	npctalk "There's no good news on the plaza bulletin, no one is in the plaza either....", "Villager#rch1_13", BC_SELF;
	end;
}

ch1_gef_in,35,173,3	script	Bedridden Patient#rch1_01	4_M_PATIENT,{
	cutin "ch1_sick02.png",0;
	mes "[Bedridden Patient]";
	mes "Ugh... my body hurts. I have a high fever and my vision is blurry.";
	mes "But I still hope that I'll recover....";
	next;
	mes "[Bedridden Patient]";
	mes "Surely, the gods are watching over me.";
	mes "With them, I'll make it through another day.";
	close3;
}

ch1_gef_in,69,175,1	script	Grumbling Patient#rch1_01	4_M_DIEMAN,{
	cutin "ch1_sick01.png",2;
	mes "[Grumbling Patient]";
	mes "Ugh, my back hurts and my throat is so dry....";
	mes "It's cold yet it's hot, the food is tasteless... Grumbles.";
	close3;
}

//= Verus
hem_dun01,61,197,5	script	Verus Believer#c01ms40	4_M_BELIEVER01,3,3,{
	if ( isbegin_quest(17926) == 1 ) {
		pcblock(true);
		pctalk "I bring good news. This is a holy relic infused with the energy of a new human.";
		sleep2 500;
		npctalk "Is there such a thing? Thank you so much.", "", BC_SELF;
		sleep2 500;
		pctalk "Be sure to keep it with you at all times. I heard it's very effective.";
		if ( countitem("Ch1_Purifi_Amulet") )
			delitem "Ch1_Purifi_Amulet", 1;
		erasequest 17926;
		pcblock(false);
		CH1_Daily_Jeon_Check();
		end;
	}
	npctalk "I need more of it's energy!", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17926) == 1";
end;
}

hem_dun01,139,134,7	script	Verus Believer#c01ms41	4_F_BELIEVER01,{
	if ( isbegin_quest(17927) == 1 ) {
		pcblock(true);
		pctalk "Have you heard? If you have this with you, you'll receive the blessings of rebirth!";
		sleep2 500;
		npctalk "Oooh! Give me one of that!", "", BC_SELF;
		sleep2 500;
		pctalk "Here, I hope you receive the blessing!!";
		if ( countitem("Ch1_Purifi_Amulet") )
			delitem "Ch1_Purifi_Amulet", 1;
		erasequest 17927;
		pcblock(false);
		CH1_Daily_Jeon_Check();
	}
	npctalk "Somehow, I feel today is going to be good.", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17927) == 1";
end;
}

hem_dun01,76,120,3	script	Verus Believer#c01ms42	4_M_BELIEVER02,{
	if ( isbegin_quest(17928) == 1 ) {
		pcblock(true);
		pctalk "Have you already got one of these? They say it's a new relic, I got plenty of it. Here, take one.";
		sleep2 500;
		npctalk "I'm grateful! Thank you.", "", BC_SELF;
		sleep2 500;
		pctalk "Don't mention it. Be sure to keep it with you. Got it?";
		if ( countitem("Ch1_Purifi_Amulet") )
			delitem "Ch1_Purifi_Amulet", 1;
		erasequest 17928;
		pcblock(false);
		CH1_Daily_Jeon_Check();
		end;
	}
	npctalk "How can such a civilization exist...!", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17928) == 1";
end;
}

hem_dun01,151,247,3	script	Verus Believer#c01ms43	4_M_BELIEVER01,{
	if ( isbegin_quest(17929) == 1 ) {
		pcblock(true);
		pctalk "Have you seen this sacred looking relic? They say new humans used to carry  it.";
		sleep2 500;
		npctalk "Give me one! Give me!!!", "", BC_SELF;
		sleep2 500;
		pctalk "Okay, here you go. Then, have a nice day.";
		if ( countitem("Ch1_Purifi_Amulet") )
			delitem "Ch1_Purifi_Amulet", 1;
		erasequest 17929;
		pcblock(false);
		CH1_Daily_Jeon_Check();
		end;
	}
	npctalk "Amazing...", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17929) == 1";
end;
}

hem_dun01,215,202,5	script	Verus Believer#c01ms44	4_F_BELIEVER01,{
	if ( isbegin_quest(17930) == 1 ) {
		pcblock(true);
		pctalk "Take this. It's a holy relic that they're distributing over there.";
		sleep2 500;
		npctalk "They're distributing this out? How did I not hear about it?", "", BC_SELF;
		sleep2 500;
		pctalk "They only started handing out them today. Then, I'll be off...";
		if ( countitem("Ch1_Purifi_Amulet") )
			delitem "Ch1_Purifi_Amulet", 1;
		erasequest 17930;
		pcblock(false);
		CH1_Daily_Jeon_Check();
		end;
	}
	npctalk "When will it be my turn...", "", BC_SELF;
	end;
	
OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(17930) == 1";
end;
}

hem_dun01,220,154,3	script	Verus Believer#c01ms50	4_F_BELIEVER01,{
	npctalk "Will the blessing come today...", "", BC_SELF;
	end;
}

hem_dun01,206,107,3	script	Verus Believer#c01ms51	1_M_03,{
	npctalk "What was am I supposed to do again? I don't remember, I'll just look around.", "", BC_SELF;
	end;
}

hem_dun01,182,166,5	script	Verus Believer#c01ms52	4_M_BELIEVER01,{
	npctalk "So? Any news today?", "", BC_SELF;
	sleep2 100;
	npctalk "Please. I hope it would be me...", "Verus Believer#c01ms53", BC_SELF;
	end;
}

hem_dun01,184,165,1	script	Verus Believer#c01ms53	4_F_BELIEVER01,{
	npctalk "Please, I hope the blessing would come to me...", "", BC_SELF;
	end;
}

hem_dun01,100,220,5	script	Verus Believer#c01ms54	4_M_BELIEVER02,{
	npctalk "This place is paradise.", "", BC_SELF;
	end;
}

hem_dun01,99,219,5	script	Verus Believer#c01ms55	1_M_JOBGUIDER,{
	npctalk "How on earth did they make this place? I don't know what to do.", "", BC_SELF;
	end;
}

hem_dun01,131,201,3	script	Verus Believer#c01ms56	4_M_JOB_WIZARD,{
	npctalk "Ugh, it's not responding today... I'll try again tomorrow...", "", BC_SELF;
	end;
}

hem_dun01,146,170,1	script	Verus Believer#c01ms57	4_F_03,{
	npctalk "What could be beneath this place? Is there more amazing down here?", "", BC_SELF;
	end;
}

hem_dun01,154,97,1	script	Verus Believer#c01ms58	4_M_ALCHE_A,{
	npctalk "How did they move that? New humans, please answer me. Hello?", "", BC_SELF;
	end;
}

hem_dun01,118,249,3	script	Verus Believer#c01ms59	4_F_BELIEVER01,{
	npctalk "Who could this be? Is this somekind of higher being?", "", BC_SELF;
	end;
}

hem_dun01,218,186,7	script	Verus Believer#c01ms60	4_M_BELIEVER02,{
	npctalk "Do you think I'll be able to enter there someday?", "", BC_SELF;
	end;
}

hem_dun01,26,81,3	script	Verus Believer#c01ms61	4_M_BELIEVER01,{
	npctalk "I need to bring my family here, but for some reason I can't leave...", "", BC_SELF;
	end;
}

hem_dun01,51,73,3	script	Verus Believer#c01ms62	4_M_ORIENT02,{
	npctalk "Dear creator, can you hear me? I'm praying to you with all my heart!!!", "", BC_SELF;
	end;
}

hem_dun01,133,64,3	script	Verus Believer#c01ms63	4_M_NFMAN,{
	npctalk "This place... it's so scary... I shouldn't have come...", "", BC_SELF;
	end;
}

hem_dun01,135,79,7	script	Verus Believer#c01ms64	4_M_ZONDAOYAJI,{
	npctalk "Ah! If only this hadn't been destroyed!! What a shame!", "", BC_SELF;
	end;
}

hem_dun01,32,171,5	script	Verus Believer#c01ms65	4_F_BELIEVER01,{
	npctalk "How about making this place our new home?", "", BC_SELF;
	end;
}

hem_dun01,173,159,3	script	Verus Believer#c01ms66	4_M_BELIEVER01,{
	npctalk "Are you new here? Welcome. We haven't had new people lately, I wonder what changed.", "", BC_SELF;
	end;
}

hem_dun01,84,266,5	script	Verus Believer#c01ms67	4_M_REPAIR,{
	npctalk "Should we leave this decayed place as is? Should we restore it?", "", BC_SELF;
	end;
}

hem_dun01,87,266,3	script	Verus Believer#c01ms68	4_M_EINMAN,{
	npctalk "Of course we leave it. Even that is their will.", "", BC_SELF;
	end;
}

hem_dun01,36,257,5	script	Verus Believer#c01ms69	4_F_BELIEVER01,{
	npctalk "It didn't look like this when I first came...", "", BC_SELF;
	end;
}

hem_dun01,117,175,3	script	Verus Believer#c01ms70	4_M_HUMAN_01,{
	npctalk "The blessing finally came to me too! I should share this with everyone.", "", BC_SELF;
	end;
}

hem_dun01,112,174,7	script	Verus Believer#c01ms71	4_F_CAPEGIRL,{
	npctalk "Hey, stop pushing! Get in line!", "", BC_SELF;
	end;
}

hem_dun01,111,173,7	script	Verus Believer#c01ms72	4_M_MIDDLE,{
	npctalk "Want to shake hands who received the blessing? Then, you should get in line.", "", BC_SELF;
	end;
}

hem_dun01,114,173,7	script	Verus Believer#c01ms73	4_M_RACHMAN2,{
	npctalk "Ah, this time for sure!! I must be reborn too...!", "", BC_SELF;
	end;
}

hem_dun01,112,178,5	script	Verus Believer#c01ms74	4_F_BELIEVER01,{
	npctalk "Oooh, that's a body that received the blessing!!", "", BC_SELF;
	end;
}

hem_dun01,111,177,5	script	Verus Believer#c01ms75	4_M_BELIEVER02,{
	npctalk "Will I receive the blessing too, if I shake his hand? I hope I get it...", "", BC_SELF;
	end;
}

hem_dun01,171,128,3	script	Verus Believer#c01ms76	4_M_NOVELIST,{
	npctalk "If I only could get my hands on it.", "", BC_SELF;
	end;
}

hem_dun01,152,146,1	script	Verus Believer#c01ms77	4_M_ORIENT02,{
	npctalk "Someone blessed is sharing their energy there!", "", BC_SELF;
	sleep2 100;
	npctalk "Let's hurry and go!", "Verus Believer#c01ms78", BC_SELF;
	end;
}

hem_dun01,154,147,1	script	Verus Believer#c01ms78	4_GEFFEN_05,{
	npctalk "If I become one of them, will I able to learn all the secrets of this place?", "", BC_SELF;
	end;
}

hem_dun01,61,148,1	script	Verus Believer#c01ms79	4_GEFFEN_01,{
	npctalk "I have no idea how any of this is possible.", "", BC_SELF;
	end;
}

//= VR Geffen
ch1_vrgef2,173,88,5	script	Merchant#Ch1virtual	4_M_03,{
	mes "[Merchant]";
	mes "Hello there! I'm not doing so well right now.";
	next;
	mes "[Merchant]";
	mes "For someone like me who just get by day to day, this situation is hell.";
	next;
	mes "[Merchant]";
	mes "I don't know how it turned to this nightmare. I hope it ends soon..";
	close;
}

ch1_vrgef2,177,71,3	script	Aki#Ch1virtual	4_F_JOB_BLACKSMITH,{
	mes "[Aki]";
	mes "The weather's pretty bad today, huh? A day like this is perfect for holing up in my workship and do some refining..";
	next;
	mes "[Gudei]";
	mes "Young brat, you're talking nonsense again. On a day like this, the fire doesn't burn well.";
	next;
	mes "[Gudei]";
	mes "This is exactly the kind of day to strap on your bag and head up on the mountain to collect rare ores.";
	next;
	mes "[Aki]";
	mes "Ugh.. hiking.. sounds troublesome..";
	next;
	mes "[Gudei]";
	mes "You've still got a long way to go if you want to be a true blacksmith, hahahaha!";
	close;
}

ch1_vrgef2,175,71,0	script	Gudei#Ch1virtual	4_M_DWARF,{
	mes "[Gudei]";
	mes "The town's atmosphere is uneasy, I can't get my spirits up..";
	next;
	mes "[Gudei]";
	mes "A good hike would clear this uneasy feeling, believe me.";
	next;
	mes "[Gudei]";
	mes "Aki, come to think of it. How's that skill that you said you were researching?";
	next;
	mes "[Aki]";
	mes "Ugh.. this old man seriously know to hit where it hurts.. don't you have anything better to do? If not, just go help out in the square or something!";
	next;
	mes "[Gudei]";
	mes "You rookie, better watch your mouth!";
	close;
}

ch1_vrgef2,161,81,5	script	Haes#Ch1virtual	15,{
	mes "[Haes]";
	mes "Hello. I'm Haes. Welcome to the magic city of Geffen.";
	next;
	mes "[Haes]";
	mes "But things aren't really going well for a welcome right now.";
	close;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 15;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 1;
	setunitdata getnpcid(0), UNPC_CLOTHCOLOR, 2;
	setunitdata getnpcid(0), UNPC_HAIRCOLOR, 2;
	setunitdata getnpcid(0), UNPC_HEADTOP, 1885;
end;
}

ch1_vrgef2,173,55,3	script	Jerome#Ch1virtual	4_M_DWARF,{
	mes "[Jerome]";
	mes "Procyon, how are you feeling today?";
	next;
	mes "[Procyon]";
	mes "I'm feeling the absolute worst possible. Don't even talk to me.";
	next;
	mes "[Jerome]";
	mes "Oh.. sorry for talking to you..";
	next;
	mes "[Procyon]";
	mes "I said don't talk to me!";
	next;
	mes "[Jerome]";
	mes "... ... ... ...";
	close;
}

ch1_vrgef2,172,53,7	script	Procyon#Ch1virtual	4_F_EINWOMAN,5,5,{
	mes "[Procyon]";
	mes "Jerome! Is all the reserved orders done?";
	next;
	mes "[Jerome]";
	mes "Well, no.. I told you before, I can't work on them. Geffen is on a lockdown!";
	next;
	mes "[Jerome]";
	mes "No materials aren't coming in.. and the finished goods can't go out either.";
	next;
	mes "[Procyon]";
	mes "You're always like this! Always saying it impossible without trying. Did you even try?";
	next;
	mes "[Jerome]";
	mes "No.. you're right.. okay.. I'll try.. not like I have a choice anyway. Sorry.";
	close;
	
OnTouch: //No Touch Event
end;
}

ch1_vrgef2,153,70,5	script	Vending Machine#Ch1virtual	2_VENDING_MACHINE1,{
	mes "The light near the Zeny slot of the vending machine is flashing under maintenance. Looks like a product got stuck.";
	next;
	mes "[Vending Machine Manager Kidol]";
	mes "If two items comes out of the vending machine, please place the other one the top of the vending machine.";
	close;
}

ch1_vrgef2,151,68,5	script	Vending Machine Manager#Ch1virtual	VR_UNKOWN_MAN,{
	mes "[Vending Machine Manager Kidol]";
	mes "This stupid vending machine is jammed again. What is wrong with it!";
	next;
	mes "[Vending Machine Manager Kidol]";
	mes "I restock it and it jams, fix it and jams again, what is happening? Does this thing wants me to go out of business?";
	next;
	mes "[Vending Machine Manager Kidol]";
	mes "Aaaaaaaaaaaaaaaaaaaaaargh!!!!!!!!!!!!";
	close;
}

ch1_vrgef2,151,44,0	script	Young Man#Ch1virtual	4W_M_01,{
	mes "[Young Man]";
	mes "Did you know, when the Geffen Tower collapsed, the debris from it flew all the way here in front of me?";
	next;
	mes "[Young Man]";
	mes "I could've been in real danger. It was scary..";
	close;
}

ch1_vrgef2,129,49,5	script	Elemental Point Merchant#Ch1	8_F_GIRL,{
	mes "[Elemental Point Merchant]";
	mes "Sigh- I thought business would go well with the number of mages around here.. now I'm caught up in this ridiculous incident..";
	next;
	mes "[Elemental Point Merchant]";
	mes "Why are you looking me like I'm some kind of psychopath? I invested practically my entire fortune just to come here!";
	next;
	mes "[Elemental Point Merchant]";
	mes "Do you know how much the interest piles up every single day? As days goes by I feel like I'm bleeding out!";
	next;
	mes "[Elemental Point Merchant]";
	mes "Honestly I just want to sell everything at a small cost! But I can't! What do you expect me to do! Eh!";
	close;
}

ch1_vrgef2,129,46,5	script	Genetic Mingmin#Ch1Fountain	4_F_GENETIC,{
	mes "[Mingmin]";
	mes "I don't know why, but I feel strangely compelled to tell you this... I've finally discovered the way to open the entrance to Geffenia.";
	next;
	mes "[Mingmin]";
	mes "Shh! This is still a secret. It hasn't been officially announced yet, so you absolutely must not tell anyone else.";
	next;
	mes "[Mingmin]";
	mes "Geffenia... I've wanted to go there for such a long time.";
	mes "But... I haven't been able to find a reliable enough expedition partner, so I still haven't went in.";
	next;
	mes "[Mingmin]";
	mes "The Demon King's Tear shall lead to the ancient city.";
	mes "It's a line I really love. It's from one of the legends passed down about Geffenia.";
	next;
	mes "[Mingmin]";
	mes "But I figured out a way to get there without the Demon King's Tear... I really did...";
	next;
	mes "[Mingmin]";
	mes "After the Geffen Tower collapsed, the path to Geffenia stopped responding. What should I do?";
	next;
	mes "[Mingmin]";
	mes "It was my lifelong dream.";
	close;
}

ch1_vrgef2,132,38,3	script	Bard Eland#Ch1fly	1_M_BARD,{
	cutin "bard_eland01.bmp",2;
	mes "[Eland]";
	mes "I am Eland, I am but a wandering bard drifting wherever the wind takes me.";
	next;
	mes "[Eland]";
	mes "If you wish, I shall tell you all the things I have seen and heard.";
	next;
	mes "[Eland]";
	mes "I shall also play you a tune that dances with the gentle breeze.";
	next;
	if ( select("Play me a song.", "Maybe another time.") == 2 ) {
		mes "[Eland]";
		mes "Aww~ Just one song wouldn't hurt... Well then, see you next time!";
		close3;
	}
	cutin "bard_eland02.bmp",2;
	mes "[Eland]";
	mes "One song coming up~";
	mes "Ehem!";
	mes "Ah!";
	mes "Now that my voice is warmed up, shall we begin?";
	next;
	mes "[Eland]";
	mes "That tower that pierced the sky,";
	mes "was the symbol of the city and of magic~♬";
	mes "Geffen's symbol, the essence of magic,";
	mes "there, everyone followed their dreams~♬";
	next;
	cutin "bard_eland03.bmp",2;
	mes "[Eland]";
	mes "But one day the wind whispered,";
	mes "the tower no longer stands~♬";
	mes "The hand of destruction fell,";
	mes "and the symbol of all turned to ash~♬";
	next;
	mes "[Eland]";
	mes "Echoing cries resounded,";
	mes "under the crimson evening sky,";
	mes "the symbol of the mage collapsed,";
	mes "and the Mage Tower crumbled into dust~♬";
	next;
	cutin "bard_eland02.bmp",2;
	mes "[Eland]";
	mes "A name unseen, a symbol lost,";
	mes "sing for them~♬";
	mes "Breaths scattered in the wind,";
	mes "may the stars cradle their souls~♬";
	next;
	mes "[Eland]";
	mes "Though the tower is gone,";
	mes "a greater hope remains in our hearts~♬";
	mes "Even in ruins, hope will bloom,";
	mes "and from the ashes, new sprouts shall grow~♬";
	next;
	cutin "bard_eland03.bmp",2;
	mes "[Eland]";
	mes "O dear one, remember.";
	mes "Though the tower has fallen, our story,";
	mes "carried by the wind, sung through song,";
	mes "shall live on forever~♬";
	next;
	mes "[Eland]";
	mes "... ... ... ... ... ...";
	next;
	cutin "bard_eland01.bmp",2;
	mes "[Eland]";
	mes "Thank you for listening. Again, Best regards from me, Eland.";
	close3;
}

ch1_vrgef2,122,37,4	script	Kafra Promotion Staff#Ch	4_F_KAFRA2,{
	cutin "kafra_02.bmp",2;
	mes "[Kafra Promotion Staff]";
	mes "Welcome.";
	mes "Unfortunately, our services is not available at the moment. All commercial district voting is currently closed.";
	next;
	mes "[Kafra Promotion Staff]";
	mes "We apologize for not being able to provide our services smoothly.";
	close3;
}

ch1_vrgef2,117,37,6	script	Zonda Promotion Staff#Ch1	4_M_ZONDAMAN,{
	cutin "zonda_01.bmp",2;
	mes "[Kafra Promotion Staff]";
	mes "Hello.";
	mes "Unfortunately, our services is not available at the moment. All commercial district voting is currently closed.";
	next;
	mes "[Kafra Promotion Staff]";
	mes "We apologize for the inconvenience.";
	close3;
}

ch1_vrgef2,103,42,5	script	Sphoniac#Ch1Fountain	4W_M_03,{
	mes "[Sphoniac]";
	mes "Instead of loitering around here, you'd better let us go outside to hunt something, it might help our situation a little.";
	next;
	mes "[Sphoniac]";
	mes "Oh, right! The city is under lockdown, right? Sorry, just keep loitering then.";
	close;
}

ch1_vrgef2,111,48,4	script	Woman#Ch1Fountain	4W_F_01,{
	mes "[Woman]";
	mes "Are you having a good day? Because I am not having a good one. I came out because I'm feeling anxious.";
	next;
	mes "[Woman]";
	mes "I thought maybe being around people might ease the anxiety a little.";
	close;
}

ch1_vrgef2,101,53,5	script	Peco Breeder#EG07	8W_SOLDIER,{
	mes "[Peco Breeder]";
	mes "I apologize. We're currently not offering any services right now.";
	close;
}

ch1_vrgef2,103,55,5	script	Madogear Rental#EG07	8W_SOLDIER,{
	mes "[Madogear Rental]";
	mes "I apologize. We're currently not offering any services right now.";
	close;
}

ch1_vrgef2,113,64,6	script	Dolangmal#Ch1Fountain	4_CAT_DOWN,{
	mes "[Dolangmal]";
	mes "Hello~! Human~ I'm the great Dolangmal. Kyahaha~";
	next;
	mes "[Dolangmal]";
	mes "Normally, I'd personally take you to Malangdo, but it's impossible right now, so don't get your hopes up!";
	next;
	mes "[Dolangmal]";
	mes "By the way, don't you know the name of that beautiful lady next to me? She won't answer me when I talk to her.";
	close;
}

ch1_vrgef2,118,62,0	script	Geffen Guard#Ch1Fountain	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,120,62,0	script	Kafra Employee#Ch1Fountain	4_F_KAFRA3,{
	cutin "kafra_03.bmp",2;
	mes "[Kafra Employee]";
	mes "The Kafra Services is always by your side.";
	mes "Due to cooperation with the current Geffen lockdown, some services may not be available. How may I assist you?";
	next;
	switch ( select("Use Cart Service", "Check Other Information", "Quit") ) {
		case 1:
			if ( BaseClass != Job_Merchant ) {
				mes "[Kafra Employee]";
				mes "I apologize.";
				mes "But the cart rental service is only available for merchant classes.";
				close3;
			}
			if ( checkcart() == true ) {
				mes "[Kafra Employee]";
				mes "I apologize.";
				mes "But you already have a cart, and we can only rent one cart at a time.";
				close3;
			}
			if ( getskilllv("MC_PUSHCART") == 0 ) {
				mes "[Kafra Employee]";
				mes "You can only rent a cart after learning the \"Push Cart\" skill.";
				close3;
			}
			if ( Zeny < 700 ) {
				mes "[Kafra Employee]";
				mes "I apologize.";
				mes "But you don't have enough Zeny to pay for the cart's rental fee of 700 Zeny.";
				close3;
			}
			setcart;
			mes "[Kafra Employee]";
			mes "Thank you for using the Kafra Services.";
			close3;

		case 2:
			switch ( select("Check Saved Points", "카프라 위치 안내", "취소") ) {
				case 1:
					mes "[Kafra Employee]";
					mes ""+strcharinfo(0)+", you have saved 1747 points.";
					next;
					mes "[Kafra Employee]";
					mes "Saved points can be exchanged for items at our kafra headquarters.";
					mes "Use our convenient Kafra services and you can receive lots of gifts too~";
					close3;
				case 2:
					viewpoint 1,120,62,1,0xFF0000FF;
					viewpoint 1,203,123,2,0xFF0000FF;
					close3;
				case 3:
					mes "[Kafra Employee]";
					mes "At Kafra Services, we strive to provide flawless services and strict management, ensuring that our customers can use our services at peace.";
					mes "Thank you for using the Kafra Services.";
					close3;
			}
			end;
		case 3:
			mes "[Kafra Employee]";
			mes "At Kafra Services, we strive to provide flawless services and strict management, ensuring that our customers can use our services at peace.";
			mes "Thank you for using the Kafra Services.";
			close3;
	}
}

ch1_vrgef2,116,68,0	script	Mailbox#Ch1Fountain	2_POSTBOX,{
	mes "[Mailbox]";
	mes "Mail delivery is currently unavailable. We ask for your understanding.";
	close;
}

ch1_vrgef2,123,68,3	script	Royal Messenger#Ch1Fountain	4_M_ZONDAOYAJI,5,5,{
	mes "[Messenger]";
	mes "Greetings-!";
	mes "I've come to Geffen in search of the adventurer summoned by the royal family.";
	next;
	mes "[Messenger]";
	mes "Everyone's scattered all over..";
	mes "It's really hard to find a person when you're working alone. Hahaha!";
	next;
	mes "[Messenger]";
	mes "Hahahahahaha!";
	mes "If this whole situation could be resolved before I find the adventurer, I'd feel a lot better when I head home...";
	close;
	
OnTouch:
end;
}

ch1_vrgef2,115,65,5	script	Kafra Teleport Services#Ch1	4_F_DRKAFRA01,{
	cutin "kafra_do01.bmp",2;
	mes "[Kafra Employee]";
	mes "Welcome-nyan.";
	mes "The kafra teleport services is currently unavailable-nyan. That's just how it is-nyan.";
	next;
	mes "[Kafra Employee]";
	mes "More than that-nyan, the cat beside me is annoying-nyan. I wish someone would get rid of him-nyan.";
	close3;
}

ch1_vrgef2,124,65,0	script	Sign#Ch1fly	2_BULLETIN_BOARD,{
	mes "A large sheet of paper has been attached over the original notice.";
	next;
	setdialogsize(400, 420);
	mes "<FONT SIZE = 18><B>Magic City of Geffen Official Notice</B></FONT>";
	mes "Author: Geffen City Committee";
	mes "Date: xxxx-xxx-xxx";
	mes "Recipient: Citizen and adventurers of Geffen";
	mes "Subject: Notice about the lockdown and Geffen Tower collapse";
	mes "---------- ---------- ---------- ----------";
	mes "Honorable citizens and adventurers of Geffen";
	mes "The collapse of the Geffen Tower was not due to a simple structural flaw but was the result of an act of aggression.";
	mes "To ensure the safety of the city and to prevent further damage, the following emergency measures will be implemented.";
	mes "---------- ---------- ---------- ----------";
	mes "<FONT SIZE = 18><B>1. City Lockdown Measures</B></FONT>";
	mes "  ";
	mes "<B>Geffen perimeter lockdown</B>";
	mes "All entry and exit routes will be sealed, and going outside the city is strictly prohibited.";
	mes "^FF0000※ Entry of essential goods and relief supplies will only be allowed after strict inspection by the guards.^000000";
	mes "  ";
	mes "<B>Rules within the city</B>";
	mes "Free movement within the city is permitted. However, access to major facilities may be restricted.";
	mes "  ";
	mes "  ";
	mes "<FONT SIZE = 18><B>2. Temporary Suspension of Convenience Services</B></FONT>";
	mes "<B>Suspension of Kafra & Zonda services</B>";
	mes "All Kafra and Zonda-related services will be completely suspended..";
	mes "  ";
	mes "<B>Warp portal ban and restriction</B>";
	mes "The opening of warp portals by priests within the city is prohibited.";
	mes "  ";
	mes "<B>Storage service suspension</B>";
	mes "To manage the goods movement within the city, the service will be suspended until the situation is resolved.";
	mes "  ";
	mes "  ";
	mes "<FONT SIZE = 18><B>3. Request for citizens and adventurers Cooperation</B></FONT>";
	mes "To help resolve the incident and ensure the city's safety, please refrain from unnecessary fighting and cooperate with guard instructions..";
	mes "Residents with any information regarding the attack are requested to report immediately to the guards or the committee..";
	mes "As material shortages may occur in the city, please cooperate in conserving resources and refrain from hoarding..";
	mes "  ";
	mes "  ";
	mes "<FONT SIZE = 18><B>4. plague Measures</B></FONT>";
	mes "The committee has officially confirmed the outbreak of an plague in the village.";
	mes "Please pay special attention to personal hygiene such as washing hands and wearing masks.";
	mes "If the following symptoms appear, itching, thromptosis(thirsty), and severe cold.";
	mes "please visit the inn, which has been temporarily converted into a clinic, to receive appropriate treatment.";
	mes "  ";
	mes "  ";
	mes "The committee is working diligently to resolve the situation quickly and will promptly inform you of any additional announcements.";
	mes "  ";
	mes "  ";
	mes "We ask for your understanding that these measures are necessary to protect the safety of our citizens and the peace of the city.";
	mes "We sincerely hope that, through cooperation, we can overcome this difficult time together.";
	mes "  ";
	mes "---------- ---------- ---------- ----------";
	mes "  ";
	mes "Geffen City Committee";
	close;
}

ch1_vrgef2,135,64,3	script	Reckenber Promoter#Ch1frac	4_F_ZONDAGIRL,4,4,{
	mes "[Reckenber Promoter]";
	mes "Hello there~!";
	mes "Rekenber is currently recruiting talented individuals to help excavate a newly discovered dungeon in Amatsu~!";
	next;
	mes "[Reckenber Promoter]";
	mes "All the support needed for the excavation will be provided by us at Rekenber.";
	mes "If you want to join the excavation, just bring yourself there~!";
	next;
	mes "[Reckenber Promoter]";
	mes "Of course, with Geffen currently under lockdown, it won't be possible until the restrictions are lifted.";
	close;
	
OnTouch: //No ontouch
end;
}

ch1_vrgef2,132,66,3	script	Eden Teleport Officer#Ch1minute	4_F_NOVICE,{
	mes "[Eden Teleport Officer]";
	mes "Those who are wandering around this strange world with aimless eyes listen up and come to me!";
	next;
	mes "[Eden Teleport Officer]";
	mes "is what I'd like to say but, we're not providing teleport services to the Eden Group in order to cooperate with the Geffen lockdown. Sorry.";
	next;
	if ( select("Teleport to Eden Group", "End conversation") == 1 ) {
		mes "[Eden Teleport Officer]";
		mes "We're you not listening?";
		close;
	}
	mes "[Eden Teleport Officer]";
	mes "If you don't know what to do for your future, don't hesitate to tell me. Just drop by and talk to me!";
	close;
}

ch1_vrgef2,128,68,3	script	Guild Warehouse Manager#Ch1minute	4_M_HUBOY,{
	mes "[Warehouse Manager]";
	mes "How are you? We are specialized in guild warehouses. This is our ^0000cdstory^000000:";
	next;
	mes "[Warehouse Manager]";
	mes "Due to the current Geffen lockdown cooperation, we are unable to provide guild storage services. For now, please follow the guards' instructions. Thank you.";
	close;
}

ch1_vrgef2,122,77,4	script	Nia#Ch1other	8_F_GIRL,{
	mes "[Nia]";
	mes "Uh.. uh.. that.. t-the tower.. it collapsed!";
	next;
	mes "[Nia]";
	mes "This.. isn't a joke? Am I right? right.. Am I dreaming? Am I seeing things? No, there's no way..!";
	next;
	mes "[Nia]";
	mes "Master wasn't there, was he? Or.. even the other seniors?";
	next;
	mes "[Nia]";
	mes "Oh.. no, Nia! That's now whats important right now! You have to pull yourself together! Pull it together..!";
	next;
	mes "[Nia]";
	mes "This isn't the time for this kind of thinking! This place... what on earth happened here?!";
	next;
	mes "[Nia]";
	mes "Everyone's okay, right? They must be! Yeah, Nia... this is not the time to smile and brush it off.";
	next;
	mes "[Nia]";
	mes "Right now... what should I do? H-hey! No one's hurt, right? Is there anything I can help with?!";
	next;
	mes "[Nia]";
	mes "If the situation.. turns into survivors counting.. what should I do..?";
	next;
	mes "[Nia]";
	mes "No, no! Let's start with what I can do. One step at a time.. Even in uneasy times like this.. I need to make people smile..";
	next;
	mes "[Nia]";
	mes "This situation, I don't know what to do..";
	close;
}

ch1_vrgef2,115,91,8	script	Smile Helper#Ch1other	4_F_03,{
	mes "[Smile Helper]";
	mes "Wow.. that was close.. the tower collapsed while I was taking a shower..";
	close;
}

ch1_vrgef2,128,90,3	script	Continental Guard Messenger#Ch1	4_M_MOCASS1,3,3,{
	mes "[Continental Guard Messenger]";
	mes "Haha, well.. I thought I'd gained a lot of experience traveling across countless battlefields, but this is a first for me.";
	next;
	mes "[Continental Guard Messenger]";
	mes "Of all times, this had to happen while I was assigned to Geffen..";
	next;
	mes "[Continental Guard Messenger]";
	mes "I'm starting to wonder if some cursed energy from the battlefield stuck to me and caused all this.";
	next;
	mes "[Continental Guard Messenger]";
	mes "At this rate, never mind the report about what happened in Morroc, this place is turning into something bigger";
	next;
	mes "[Continental Guard Messenger]";
	mes "For now, I'll just follow the lockdown procedures here and quietly return afterward.";
	close;
	
OnTouch:
end;
}

ch1_vrgef2,128,108,2	script	Apprentice Mage#Ch1other	4_M_SAGE_C,{
	mes "[Apprentice Mage]";
	mes "Those soldiers... I mean... technically, I'm affiliated with the tower as a mage.";
	next;
	mes "[Apprentice Mage]";
	mes "But I've been with the tower for less than a week, the lowest of all mages...";
	next;
	mes "[Apprentice Mage]";
	mes "Even if you ask me about the tower, I don't know anything... They just said a representative from the tower needed to be here, so that's why I'm standing here.";
	close;
}

ch1_vrgef2,123,110,5	script	Geffen Guard#EG06_3	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,122,117,1	script	Geffen Guard#EG06_2	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Ah! The debris hasn't all been cleared yet. It's dangerous. Please don't come any closer.";
	close3;
}

ch1_vrgef2,118,121,6	script	Geffen Guard#EG06_1	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "That's strange... something feels off...";
	next;
	mes "[Geffen Guard]";
	mes "Oh! Haha... nothing strange after all. Pay me no mind.";
	close3;
}

ch1_vrgef2,115,110,4	script	Ford Leslie#EG06_1	4_M_CRU_OLD,{
	mes "[Ford Leslie]";
	mes "The accident site hasn't been cleared yet, so it's currently restricted. Please follow the guard's instructions for everyone's safety.";
	next;
	mes "[Ford Leslie]";
	mes "Thank you for your cooperation.";
	close;
}

ch1_vrgef2,146,132,3	script	Anselia#Ch1other	4_F_KHGIRL,{
	mes "[Anselia]";
	mes "That distant Glast Heim Castle... doesn't just hearing the name send chills down your spine?";
	next;
	mes "[Anselia]";
	mes "I've heard stories, supposedly the cursed of the former king of Prontera has been engulfed in the castle...";
	next;
	mes "[Anselia]";
	mes "The Geffen Tower has collapsed, is this really just a coincidence?";
	next;
	mes "[Anselia]";
	mes "Or maybe... something awakened in that castle? The more I think about it, the more it creeps me out.";
	next;
	mes "[Anselia]";
	mes "It feels like the horrors inside that dreadful castle could spill over the walls and attack the village at any moment.";
	next;
	mes "[Anselia]";
	mes "I used to think it was all just rumors...";
	next;
	mes "[Anselia]";
	mes "But now... I don't think anyone can protect us anymore.";
	next;
	mes "[Anselia]";
	mes "Even the Geffen guards still seem confused, and nothing's been sorted out yet..";
	next;
	mes "[Anselia]";
	mes "Just imagining the spirits of Glast Heim flooding toward us while we're unprepared, it's enough to stop my heart.";
	next;
	mes "[Anselia]";
	mes "Please, please let nothing like that happen... I really don't want to go through anything more terrifying...";
	close;
}

ch1_vrgef2,130,128,7	script	Geffen Guard#EG06_5	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,129,148,3	script	Arjen#Ch1other	4_M_YOYOROGUE,{
	mes "[Arjen]";
	mes "I've always seen the world as boring. But after going through this series of incidents, I've realized a lot.";
	next;
	mes "[Arjen]";
	mes "For now, I need to give my best to the present. Regret later is just regret. It means nothing.";
	next;
	mes "[Arjen]";
	mes "If you want to do something, you should do it. If not, you'll regret it later on.";
	close;
}

ch1_vrgef2,128,153,3	script	Reblo#Ch1other	4_M_ALCHE_D,{
	mes "[Reblo]";
	mes "Good grief, what a sight this is. To think... the tower would collapse like this.";
	next;
	mes "[Reblo]";
	mes "'ve always said the world is unpredictable, but I never imagined something like this would happen. Heh... heh heh... no, this isn't something to laugh about.";
	next;
	mes "[Reblo]";
	mes "There's nothing I can do. Even those great Geffen mages couldn't do anything? What could I do to fix it?";
	next;
	mes "[Reblo]";
	mes "The more I think about it, the more pathetic I feel. No news is coming in... my business is practically decayed... and now even Geffen ends up like this.";
	next;
	mes "[Reblo]";
	mes "Always carrying some risk, yet acting like everything's fine on the surface... No matter how flashy the magic or how much you boast, what's meant to fall, will fall!!";
	next;
	mes "[Reblo]";
	mes "Isn't it all over now? It's over! Heh... heh heh heh... No, what am I doing, still whining even after going through something like this?";
	next;
	mes "[Reblo]";
	mes "I was just a small-time informant, wasn't I? And now I've got no information to sell, no rumors coming in... Maybe I'm done too.";
	next;
	mes "[Reblo]";
	mes "Still, this situation... just feels too harsh. I always thought even if the world crumbled, business would go on...";
	next;
	mes "[Reblo]";
	mes "Now, even my little heh-heh laugh feels like it's got only a few days left before it gets blown away in the wind...";
	next;
	mes "[Reblo]";
	mes "Well, yeah... what can I do? Now that everything's collapsed... all that's left is to flail around in the mud.";
	next;
	mes "[Reblo]";
	mes "I don't know who I'll meet next... but I bet... I'll be complaining just the same then too. Heh... heh...";
	close;
}

ch1_vrgef2,100,138,5	script	Geffen Guard#GN_01	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "I apologize.";
	mes "The entry to the building is currently restricted.";
	next;
	mes "[Geffen Guard]";
	mes "When the tower collapsed, large debris fell in this direction..";
	next;
	mes "[Geffen Guard]";
	mes "Something terrible happened, something I can't even speak of. We are currently in recovery and in reconstruction.";
	close3;
}

ch1_vrgef2,90,130,6	script	Monster Scholar#Ch1fly	4_F_GON,{
	mes "[Monster Scholar Rein]";
	mes "I am Rein, a monster scholar from Rune-Midgarts.";
	next;
	mes "[Monster Scholar Rein]";
	mes "Until now, there were no powerful monsters found around Geffen because of the strong magical energy flowing from the tower.";
	next;
	mes "[Monster Scholar Rein]";
	mes "Thanks to that, weak or weary travelers could move safely around the city.";
	next;
	mes "[Monster Scholar Rein]";
	mes "But now, with the collapse of the Geffen Tower, that protective magic has vanished.";
	next;
	mes "[Monster Scholar Rein]";
	mes "Who knows what might happen. Be careful if you're going to wander around Geffen.";
	close;
}

ch1_vrgef2,110,126,1	script	Geffen Guard#EG06_4	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,154,143,3	script	Diqui#Ch1other	4_M_JOB_KNIGHT2,{
	mes "[Diqui]";
	mes "I know it's hard. I know that too. The tower collapsed, there's an outbreak, and life is being shaken to its core..";
	mes "But nothing will change if we also collapse here!!";
	next;
	mes "[Diqui]";
	mes "When I chose the path of a swordsman, someone once told me this. A swordsman is someone who repays with their work. And it's true.";
	next;
	mes "[Diqui]";
	mes "We live with bodies and minds like steel. A swordsman stands on their own two legs, and moves forward with their own two arms. That strength is something anyone can have.";
	next;
	mes "[Diqui]";
	mes "The hardship before you now, it's painful and frightening. But don't let it bring you down.";
	next;
	mes "[Diqui]";
	mes "We don't fall. Because within us is a strong heart.";
	next;
	mes "[Diqui]";
	mes "No matter how much the world tramples us, as long as we choose to stand up, no one can stop us!";
	next;
	mes "[Diqui]";
	mes "As a swordsman, there's one thing I want to tell you.";
	mes "Strength is in everyone. Whether you become a swordsman, a doctor, a merchant, the power you hold is yours alone.";
	next;
	mes "[Diqui]";
	mes "Believe in that strength. Even if it's hard now, you have to fight for tomorrow. Tomorrow will come. And we will be the ones to shape it!";
	next;
	mes "[Diqui]";
	mes "It is us who will rise again from the ashes of Geffen.";
	mes "Stronger than the fallen tower, more persistent than the outbreak, we are the ones who will survive!";
	next;
	mes "[Diqui]";
	mes "I will fight to the end. I won't give up until the very end. And I ask you to stand with me.";
	next;
	mes "[Diqui]";
	mes "Let's rise again together, as the strongest people in the world!";
	mes "Now, let's gather our strength and start again. Becoming strong is up to you, right here and now!";
	close;
}

ch1_vrgef2,90,67,3	script	Sailor on Vacation#Ch1virtual	4W_SAILOR,{
	mes "[Sailor on Vacation]";
	mes "Eeeek.. Phew, you scared me!";
	next;
	mes "[Sailor on Vacation]";
	mes "Since we've met like this, let's introduce ourselves. I'm Mikil. Not Mikkel, Mikil.";
	next;
	mes "[Sailor on Vacation]";
	mes "I used to think sailing was a rough job, but life in the city's no less dangerous.";
	next;
	mes "[Sailor on Vacation]";
	mes "For a vacation in the city to start with such a huge incident.. Still, at least I'm trapped in a place bigger than a ship.";
	close;
}

ch1_vrgef2,63,70,3	script	Tetsu#Ch1virtual	4_M_SEAMAN,{
	mes "[Tetsu]";
	mes "You're moving around pretty well in all this chaos. Nice to meet you.";
	next;
	mes "[Tetsu]";
	mes "I'm Tetsu, master of wind-blade shuriken crafting.";
	next;
	mes "[Tetsu]";
	mes "I'm originally from Amatsu, but my personal gambling hobby decayed my finances, so I ended up here in Geffen.";
	next;
	mes "[Tetsu]";
	mes "Don't know Amatsu? It's a nice place. Lots of beautiful flowers, lots of debt collectors.. Unlike here in Geffen, the air's even cleaner there..";
	next;
	mes "[Tetsu]";
	mes "Once this chaos dies down, you should visit it sometime.";
	close;
}

ch1_vrgef2,67,87,3	script	Aipo#Ch1virtual	8_F_GIRL,{
	mes "[Aipo]";
	mes "Hehehe.. I've finally discovered it! A terrifying magic that makes mushrooms sprout from your head!";
	next;
	mes "[Aipo]";
	mes "My magic power is still lacking, so it's not perfect yet, but one day, mushrooms will grow on everyone's heads. Just you wait.";
	close;
}

ch1_vrgef2,40,113,1	script	Small Boy#Ch1virtual	4_M_KID1,{
	mes "[Hikuk]";
	mes "Petite is so cute! Petite is the best! Especially the flying ones, they're unbelievably adorable!";
	next;
	mes "[Hikuk]";
	mes "Even their name is Petite, they must have been born just to be cute!";
	next;
	mes "[Hikuk]";
	mes "Waaah, I want to pet a Petite! I want to keep one as a pet and roll around playing with it!";
	next;
	mes "[Hikuk]";
	mes "I can go out and find one, right?";
	next;
	mes "[Hikuk]";
	mes "Uhh... if not, then I guess I can't help it.";
	close;
}

ch1_vrgef2,43,123,6	script	Young Mage#Ch1virtual	2_F_MAGICMASTER,{
	mes "[Young Mage]";
	mes "Helloo! I'm new a mage Pickee!";
	next;
	mes "[Young Mage]";
	mes "I just changed jobs to a mage a little while ago! And now the Geffen Tower collapsed!";
	next;
	mes "[Young Mage]";
	mes "Maybe I should start looking for a different job!";
	close;
}

ch1_vrgef2,59,143,0	script	Woman#Ch1virtual	4_F_02,{
	mes "[Woman]";
	mes "The Hornet, a bee that lives in the grasslands, spends its entire life gathering honey under the queen's orders. That honey is said to be very sweet, nutritious, and extremely expensive.";
	next;
	mes "[Woman]";
	mes "But compared to royal jelly, which only the queen bee gets to eat, the honey is apparently not even close.";
	next;
	mes "[Woman]";
	mes "Well, in times like these, I guess I'll never get to taste it anyway.";
	close;
}

ch1_vrgef2,46,164,1	script	Weisritter#Ch1fly	1_F_04,{
	mes "[Weisritter]";
	mes "Would you.. listen to me for a second.";
	mes "I needed a good arc wand, so I put in a request with the renowned Einbroch Blacksmith Guild.. but guess what..";
	next;
	mes "[Weisritter]";
	mes "Without even telling me!";
	mes "They went and handed the job off to someone in Alberta, saying there was a skilled person there!!";
	next;
	mes "[Weisritter]";
	mes "Geez.. if it had been made in Einbroch, I probably would've gotten it already, but then I might've ended up inside the tower and gotten caught in this disaster..";
	next;
	mes "[Weisritter]";
	mes "Since they sent it to Alberta instead, I had to leave the tower to file a complaint, and ended up avoiding the disaster, so I guess that's lucky too..";
	next;
	mes "[Weisritter]";
	mes "I don't know if I should be happy, stay angry, or be sad.. I'm all over the place right now.";
	close;
}

ch1_vrgef2,67,180,5	script	Mana#Ch1virtual	8_F,{
	mes "[Mana]";
	mes "This building was originally built as a training facility for beginner mages.";
	next;
	mes "[Mana]";
	mes "After the tower incident, it was remodeled and is now being used as a temporary cafeteria.";
	next;
	mes "[Mana]";
	mes "And the building next to it is being used as a temporary shelter where tired soldiers can rest for a bit.";
	next;
	mes "[Mana]";
	mes "Well, that's how it is for now, but who knows what purpose these buildings will serve in the future.";
	next;
	mes "[Mana]";
	mes "Have confidence in yourself. As you observe yourself, you'll come to understand yourself better.";
	close;
}

ch1_vrgef2,83,189,5	script	Seth#Ch1virtual	4_M_KID2,{
	mes "[Seth]";
	mes "My name is Seth. I like origami.";
	next;
	mes "[Seth]";
	mes "Yesterday, I folded a big piece of colored paper into a big boat and gave it to my mom, and she patted my head and said thank you. Hehe♪";
	next;
	mes "[Seth]";
	mes "My teacher taught me how to make pretty flowers!!! It's a bit hard, but suuuper fun~♪";
	next;
	mes "[Seth]";
	mes "They're small and cute, and when I put one in my hair, everyone said it looked pretty, so Seth feels really happy~♪";
	next;
	mes "[Seth]";
	mes "I hope everyone can be as happy as Seth~♪";
	close;
}

ch1_vrgef1,62,163,6	script	Geffen Guard#DB_01	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Did you just arrive?";
	next;
	mes "[Geffen Guard]";
	mes "About us? We're currently working in 3 shifts because of the emergency situation.";
	next;
	mes "[Geffen Guard]";
	mes "Haha, you sure are curious about everything.";
	close3;
}

ch1_vrgef1,62,168,6	script	Geffen Guard#DB_02	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Even if it's temporary, I'm really glad there's a space where our soldiers can rest.";
	next;
	mes "[Geffen Guard]";
	mes "With Geffen under lockdown, most of the soldiers who don't live here needed a place to stay.";
	close3;
}

ch1_vrgef1,77,166,6	script	Geffen Guard#DB_04	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Ah! I'm from the previous shift, so I'm writing the log of my shift.";
	next;
	mes "[Geffen Guard]";
	mes "Keeping records like this helps prevent unexpected situations where we might end up working multiple shifts.";
	next;
	mes "[Geffen Guard]";
	mes "Well, there's the matter of overtime pay too. Hahaha.";
	close3;
}

ch1_vrgef1,65,175,5	script	Resting Guard#DB_03	4_M_PATIENT,{
	mes "[Resting Guard]";
	mes "Hmmmm.. Hmmm..";
	close;
}

ch1_vrgef2,110,200,3	script	Sage#Ch1virtual	VR_UNKOWN_MAN,{
	mes "[Sage]";
	mes "Kaboom!! Aaaah!! So scary.. too scary..";
	next;
	mes "[Sage]";
	mes ".. .. ..";
	next;
	mes "[Sage]";
	mes "Ah.. my disciple.. how could you leave so suddenly..";
	next;
	mes "[Sage]";
	mes ".. .. ..";
	next;
	mes "[Sage]";
	mes "Ruuuumble.. aaah!! The tower's collapsing, everyone run.. heehee..";
	next;
	mes "[Sage]";
	mes ".. .. ..";
	next;
	mes "[Sage]";
	mes "It's nerve-wracking..";
	next;
	mes "[Sage]";
	mes ".. .. ..";
	next;
	mes "Something doesn't seem quite right.";
	close;
}

ch1_vrgef2,119,213,0	script	geffen12oclock_gef_fild04	WARPNPC,3,2,{
	end;
	
OnTouch:
	npctalk "Guard: Leaving Geffen is prohibited. Please follow the rules.", "Geffen Guard#Ch1_12_1", BC_SELF;
	npctalk "Guard: Leaving Geffen is prohibited. Please follow the rules.", "Geffen Guard#Ch1_12_2", BC_SELF;
end;
}

ch1_vrgef2,118,210,4	script	Geffen Guard#Ch1_12_1	VR_UNKOWN_MAN,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,121,210,4	script	Geffen Guard#Ch1_12_2	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,29,118,6	script	Geffen Guard#Ch1_09_2	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,29,121,6	script	Geffen Guard#Ch1_09_1	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,26,119,0	script	geffen9oclock_gef_fild07	WARPNPC,2,2,{
	end;
	
OnTouch:
	npctalk "Guard: Leaving Geffen is prohibited. Please follow the rules.", "Geffen Guard#Ch1_09_1", BC_SELF;
	npctalk "Guard: Leaving Geffen is prohibited. Please follow the rules.", "Geffen Guard#Ch1_09_2", BC_SELF;
end;
}

//= Event VR NPC
ch1_vrgef1,106,177,0	script	eventtrigger#EG01	HIDDEN_WARP_NPC,1,1,{
	end;
	
OnTouch:
	pcblock(true);
	emotion ET_QUESTION, getcharid(3);
	sleep2 3000;
	pcblock(false);
	npctalk "Old Man: Huff huff.. Nurse, this is bad! Something terrible has happened!", "Old Man#EG01", BC_SELF;
	emotion ET_PROFUSELY_SWAT, getnpcid(0,"Old Man#EG01");
	sleep 3000;
	npctalk "Nurse: Sir, please catch your breath. Would you like a glass of water?", "Nurse#EG01";
	sleep 3000;
	npctalk "Nurse: What made you panic like this?", "Nurse#EG01";
	sleep 4000;
	npctalk "Old Man: Geffen.. the Geffen Tower collapsed!", "Old Man#EG01";
	sleep 3000;
	npctalk "Old Man: I ran straight in the moment I saw it.", "Old Man#EG01";
	sleep 3000;
	npctalk "Nurse: What? What collapsed? You don't mean that big tower? When?", "Nurse#EG01";
	sleep 4000;
	npctalk "Old Man: Just now! The sky suddenly flashed, and the tower came crashing down.", "Old Man#EG01";
	sleep 3000;
	npctalk "Old Man: People nearby were screaming and running, it was absolute chaos.", "Old Man#EG01";
	sleep 3000;
	npctalk "Nurse: I did hear a loud noise from outside.. but the tower collapsed? What on earth happened?", "Nurse#EG01";
	sleep 3000;
	npctalk "Old Man: his won't end as just some accident. Debris is scattered everywhere. Several people are hurt too.", "Old Man#EG01";
	sleep 1000;
	npctalk "Nurse: Oh.. Uhm..", "Nurse#EG01";
	sleep 3000;
	npctalk "Nurse: I think the Doctor was attending a conference at Geffen Tower today.", "Nurse#EG01";
	sleep 2000;
	npctalk "Nurse: Sir, please stay here a moment. I need to check if you're injured.", "Nurse#EG01";
	sleep 3000;
	emotion ET_PANIC,getnpcid(0, "Nurse#EG01");
	npctalk "Nurse: Doctor~~~~~~~~~~Oh~~~~ something terrible has happened~~~", "Nurse#EG01";
end;
}

ch1_vrgef1,101,175,6	script	Nurse#EG01	VR_UNKOWN_MAN,{
	mes "[Nurse]";
	mes "This is terrible! I heard the Geffen Tower collapsed.";
	close;
}

ch1_vrgef1,105,171,2	script	Old Man#EG01	4_M_EINOLD,{
	mes "[Old Man]";
	mes "Ugh- oh- I'm going to die..";
	next;
	npctalk "Nurse: We only have morning consultations for today, you're a little late.", "Nurse#EG01", BC_SELF;
	emotion ET_HUK, getnpcid(0,"Nurse#EG01");
	mes "[Old Man]";
	mes "What on earth is going on? I've never seen anything like this in my life.";
	close;
}

ch1_vrgef2,98,141,0	script	vgeffen_weaponstore	WARPNPC,2,2,{
	end;
	
OnTouch:
	npctalk "Guard: I'm sorry. We're restricting access to the building.", "Geffen Guard#GN_01", BC_SELF;
end;
}

ch1_vrgef2,156,190,0	script	Young Man#Ch1fly	4_KID01,{
	mes "[Young Man]";
	mes "Did you know? It seems like the guards are keeping a secret..";
	next;
	mes "[Young Man]";
	mes "Rumors are circulating that something was discovered at the site where the Geffen Tower collapsed.";
	next;
	mes "[Young Man]";
	mes "Don't you think that it's suspicious that they would stop the tower's restoration?";
	close;
}

ch1_vrgef2,196,167,4	script	Geju#Ch1virtual	4W_F_01,{
	mes "[Geju]";
	mes "Hmm... This is... getting complicated...";
	next;
	mes "[Geju]";
	mes "Huh? Move along now, unless you have business with me~♡";
	close;
}

ch1_vrgef2,193,152,4	script	Taming Merchant#Ch1virtual	4_F_TELEPORTER,{
	mes "[Taming Merchant]";
	mes "Welcome! Are you looking for cupet foods?";
	next;
	mes "[Taming Merchant]";
	mes "I apologize. Due to official notice from the city, I can't sell any foods to customers right now.";
	next;
	mes "[Taming Merchant]";
	mes "If it's urgent, how about returning the cupet to an egg for a while?";
	close;
}

ch1_vrgef2,203,146,5	script	Villager#Ch1virtual	4W_M_01,{
	mes "[Villager]";
	mes "I have a conspiracy theory..";
	mes "Long ago, there was a skilled blacksmith in Aldebaran who lived with his sons.";
	next;
	mes "[Villager]";
	mes "During the development of a powerful weapon, a terrible accident occurred, killing all four of his sons.";
	next;
	mes "[Villager]";
	mes "Isn't it sad the only the father survived?";
	next;
	mes "[Villager]";
	mes "Rumors say the shock drove him into hiding, vanishing without a trace..";
	next;
	mes "[Villager]";
	mes "Now, here's the real conspiracy.. don't you think that very weapon might've been used to collapse the Geffen Tower?";
	next;
	mes "[Villager]";
	mes "A weapon capable of obliterating the magically fortified Geffen Tower in one strike... developed by a smith who lost all his sons and disappeared..";
	next;
	mes "[Villager]";
	mes "Doesn't that seem suspicious? Or maybe not. It's just a theory. A conspiracy theory!";
	close;
}

ch1_vrgef2,182,125,5	script	Geffen Resident#Ch1virtual	4_KID01,{
	mes "[Aurore Dupont]";
	mes "It's been a while. Even in this brief moment, I could sense a resolve in your face.";
	next;
	mes "[Aurore Dupont]";
	mes "I too, who study and follow the power of magic, do not consider myself an ordinary person, but..";
	next;
	mes "[Aurore Dupont]";
	mes "There truly are many strange people in this world. Having given up everything... as if they've gone mad over something...";
	next;
	mes "[Aurore Dupont]";
	mes "Could you devote everything you have to a single cause?";
	next;
	mes "[Aurore Dupont]";
	mes "... ... ... ... ...";
	next;
	mes "[Aurore Dupont]";
	mes "And yet, you won't speak of that resolve, not to me, not to anyone.";
	next;
	mes "[Aurore Dupont]";
	mes "It's unfortunate, but there's nothing I can do... Instead, let me offer you one piece of advice.";
	next;
	mes "[Aurore Dupont]";
	mes "Keep this in mind. Time that once passed never returns. Not even the power of the great magic can change that.";
	next;
	mes "[Aurore Dupont]";
	mes "Even in this very moment as we speak, time continues to flow.";
	next;
	mes "[Aurore Dupont]";
	mes "Humans are also a part of nature... and so human will... is like the great flow of nature itself.";
	close;
}

ch1_vrgef2,203,123,4	duplicate(Kafra Employee#Ch1Fountain)	Kafra Employee#Ch1virtual	4_F_KAFRA4

ch1_vrgef2,196,111,3	script	Cooking Merchant#EG04	4_M_03,{
	mes "[Cooking Merchant]";
	mes "Did you come to buy cooking ingredients by any chance? Freshness is crucial for ingredients, but we can't get any supplies in.. tsk..";
	next;
	mes "[Cooking Merchant]";
	mes "It's probably the same situation wherever you go. Try looking around the distribution center.";
	close;
}

ch1_vrgef2,193,114,5	script	Villager#EG04	4W_M_01,{
	mes "[Villager]";
	mes "It's unsettling. So unsettling.. This world is truly unsettling..";
	close;
}

ch1_vrgef2,189,111,5	script	Arjen#EG04	4_M_YOYOROGUE,{
	mes "[Arjen]";
	mes "The tower collapsed, and now there's even an plague going around.. I really need to be careful. Though I don't know if being careful will change anything..";
	close;
}

ch1_vrgef2,214,118,4	script	Geffen Guard#Ch1_03_1	VR_UNKOWN_MAN,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,214,121,4	script	Geffen Guard#Ch1_03_2	4_M_GEF_SOLDIER,{
	cutin "gef_soldier.bmp",2;
	mes "[Geffen Guard]";
	mes "Welcome to the magic city of ^8B4513Geffen^000000.";
	next;
	mes "[Geffen Guard]";
	mes "Geffen is currently under lockdown. For the control of the situation, please follow the guards instructions.";
	close3;
}

ch1_vrgef2,217,119,0	script	geffen3oclock_gef_fild00	WARPNPC,2,2,{
	end;
	
OnTouch:
	npctalk "Guard: Leaving Geffen is prohibited. Please follow the rules.", "Geffen Guard#Ch1_03_1", BC_SELF;
	npctalk "Guard: Leaving Geffen is prohibited. Please follow the rules.", "Geffen Guard#Ch1_03_2", BC_SELF;
end;
}
	
ch1_vrgef1,61,56,7	script	Complaining Woman#EG13	4_F_EINWOMAN,{
	mes "[Complaining Woman]";
	mes "We demand action! We demand action!";
	close;
}

ch1_vrgef1,63,59,3	script	Bard Eland#EG13	1_M_BARD,{
	cutin "bard_eland02.bmp",2;
	mes "[Eland]";
	mes "We should always live with a smile.";
	close3;
}

ch1_vrgef1,59,57,7	script	Complaining Man#EG13	4_M_REPAIR,{
	mes "[Complaining Man]";
	mes "I demand an explanation of how you intend to fix this situation!";
	close;
}

ch1_vrgef1,80,57,1	script	Parent#EG11_1	4_F_RUSWOMAN3,{
	mes "[Parent]";
	mes "It's a relief that it's mild cold only.";
	close;
}

ch1_vrgef1,78,57,7	script	Parent#EG11_2	4_F_BRZ_WOMAN,{
	mes "[Parent]";
	mes "Our neighbors next-door... their whole family was taken to the quarantine ward.";
	close;
}

ch1_vrgef1,77,60,5	script	Parent#EG11_3	4_M_DEWWOMAN,{
	mes "[Parent]";
	mes "Hang in there just a little longer.";
	close;
}

ch1_vrgef1,78,74,5	script	Woman#EG10_2	4_F_CHILD,{
	mes "[Woman]";
	mes "Oh.. brother.. what do we do? I'm so worried, I feel like I'm going crazy.";
	close;
}

ch1_vrgef1,80,75,3	script	Man#EG10_1	4_M_RACHMAN2,{
	mes "[Man]";
	mes "They said there have been recovery cases even with severe symptoms. Don't worry!";
	close;
}

ch1_vrgef1,82,72,1	script	Man#EG10_3	4_M_RACHMAN1,{
	mes "[Man]";
	mes "Yeah. Let's stay quiet and pray.";
	close;
}

ch1_vrgef1,60,74,7	script	Former Patient#EG12	1_F_ORIENT_03,{
	mes "[Former Patient]";
	mes "It felt like no one could hear my voice in that place. Everyone was just sobbing in pain.";
	close;
}

ch1_vrgef1,61,76,3	script	Patient's Acquaintance#EG12	1_F_ORIENT_02,{
	mes "[Patient's Acquaintance]";
	mes "You're strong for enduring that.";
	close;
}

ch1_vrgef1,67,29,0	script	vcorpseroom	WARPNPC,1,1,{
	end;

OnTouch:
	pctalk "It's locked.";
	end;
end;
}

ch1_vrgef1,140,169,7	script	Doctor#EG02_1	VR_UNKOWN_MAN,{
	mes "[Doctor]";
	mes "You've been up for several nights, haven't you? You need to rest.";
	close;
}

ch1_vrgef1,141,173,4	script	Nurse#EG02	4_EP17_BROKENBETA,{
	mes "... ... ... ... ...";
	close;
}

ch1_vrgef1,138,139,6	script(DISABLED)	???#EG03	IMMOTAL_CORPS,{
	mes "[???]";
	mes "Grrrr-";
	close;
}

ch1_vrgef1,137,139,1	script(DISABLED)	Doctor#EG03_2	4_CH1_NEUMANN,{
	mes "[Doctor]";
	mes "Yes.. I'm a doctor.";
	next;
	mes "[Doctor]";
	mes "It can't be helped. I'll go for now.";
	close;
}

ch1_vrgef1,138,144,4	script(DISABLED)	Geffen Guard#EG03	4_M_GEF_SOLDIER,{
	mes "[Geffen Guard]";
	mes "Um- hello? I was sent from the clinic...";
	close;
}

ch1_vrgef1,138,144,4	script(DISABLED)	Thief#EG03_1	17,{
	mes "[Thief]";
	mes "Hehehe- In my experience, the valuable stuff was always in the hospital.";
	close;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 17;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 12;
	setunitdata getnpcid(0), UNPC_HAIRCOLOR, 8;
end;
}

ch1_vrgef1,137,139,6	script(DISABLED)	Thief#EG03_2	17,{
	mes "[Thief]";
	mes "Ha! Got him!! Yeah f- ■◇#☆ That was exhilarating.";
	close;
	
OnInit:
	setunitdata getnpcid(0), UNPC_CLASS, 17;
	setunitdata getnpcid(0), UNPC_SEX, 1;
	setunitdata getnpcid(0), UNPC_HAIRSTYLE, 12;
	setunitdata getnpcid(0), UNPC_HAIRCOLOR, 8;
end;
}

ch1_vrgef1,138,139,6	script	Doctor#EG03_1	VR_UNKOWN_MAN,{
	mes "... ... ...";
	close;
}

ch1_vrgef1,159,100,5	script	Auntie Momo#EG09	4_F_GODEMOM,{
	mes "[Momo]";
	mes "I'm sorry. I couldn't take better care of you..";
	close;
}

ch1_vrgef1,167,103,5	script	Chauma#EG09	4_M_LGTPOOR,{
	mes "[Chauma]";
	mes "Ugh.. hungry..";
	close;
}

ch1_vrgef1,168,100,1	script	Imin#EG09	4_M_02,{
	mes "[Imin]";
	mes "No! It's still not enough! I won't share it!";
	close;
}

ch1_vrgef1,170,102,2	script	Honyang#EG09	4_M_HUMAN_01,{
	mes "[Honyang]";
	mes "Let's cut back a little on the three of our rations and give that person a bit more..";
	close;
}

ch1_vrgef1,162,98,0	script(DISABLED)	Doctor#EG09	VR_UNKOWN_MAN,{
	end;
}

ch1_vrgef1,163,100,0	script	eventtrigger#EG09	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnInit:
	.active = false;
end;
	
OnTouch:
	if ( .active ) {
		end;
	}
	.active = true;
	pcblock(true);
	sleep2 2000;
	pcblock(false);
	npctalk "Imin: They're giving out too little again today..", "Imin#EG09";
	sleep 3000;
	npctalk "Imin: Auntie Momo, when will you be able to give us more?", "Imin#EG09";
	sleep 3000;
	npctalk "Chauma: Yeah, even though we all gather and wait like this, they only ever give us a little..", "Chauma#EG09";
	sleep 3000;
	npctalk "Momo: I'm sorry, everyone..", "Auntie Momo#EG09";
	sleep 3000;
	npctalk "Momo: Right now, the supplies and food might seem sufficient..", "Auntie Momo#EG09";
	sleep 3000;
	npctalk "Momo: But we don't know when the lockdown will end, so we need to save as much as we can while we can.", "Auntie Momo#EG09";
	sleep 3000;
	npctalk "Imin: If you're saying it like that, Auntie.. alright. I won't complain. I was just hungry.", "Imin#EG09";
	sleep 1000;
	enablenpc "Doctor#EG09";
	emotion ET_SURPRISE, getnpcid(0,"Imin#EG09");
	emotion ET_SURPRISE, getnpcid(0,"Chauma#EG09");
	emotion ET_SURPRISE, getnpcid(0,"Honyang#EG09");
	emotion ET_SURPRISE, getnpcid(0,"Auntie Momo#EG09");
	sleep 3000;
	npctalk "Doctor: I've come to receive the rations.", "Doctor#EG09";
	sleep 3000;
	npctalk "Momo: Here you go. I added a bit more today. You look really pale.", "Auntie Momo#EG09";
	sleep 3000;
	npctalk "Doctor: Thank you as always.", "Doctor#EG09";
	sleep 1000;
	disablenpc "Doctor#EG09";
	sleep 3000;
	npctalk "Imin: Is that guy even sleeping? He looks so exhausted.. doesn't he look like a corpse already?", "Imin#EG09";
	sleep 3000;
	npctalk "Chauma: It's hard for us just to endure..", "Chauma#EG09";
	sleep 3000;
	npctalk "Chauma: But that person even works at the clinic.. I almost feel sorry for him.", "Chauma#EG09";
	sleep 3000;
	npctalk "Honyang: Maybe we should give him a little more?", "Honyang#EG09";
	sleep 2000;
	npctalk "Honyang: Auntie, we will cut back a little on three of our rations and give that person a bit more..", "Honyang#EG09";
	sleep 1000;
	emotion ET_OK, getnpcid(0,"Auntie Momo#EG09");
	sleep 3000;
	npctalk "Imin: What? Mine too? I really don't want to....", "Imin#EG09";
	sleep 7000;
	disablenpc "Doctor#EG09";
	.active = false;
end;
}

ch1_vrgef1,181,181,0	script	eventtrigger#EG05	HIDDEN_WARP_NPC,1,1,{
	end;
	
OnTouch:
	pcblock(true);
	pctalk "Is this the city hall?Is there a meeting going on?";
	sleep2 3000;
	pcblock(false);
	npctalk "Chief: Let's hear the precautions. Please welcome him with a round of applause.", "Chief#EG05";
	emotion ET_WRAP, getnpcid(0,"Jerome#EG05");
	emotion ET_WRAP, getnpcid(0,"Procyon#EG05");
	emotion ET_WRAP, getnpcid(0,"Wandering Bard#EG05");
	emotion ET_WRAP, getnpcid(0,"Anselia#EG05");
	emotion ET_WRAP, getnpcid(0,"Aurore Dupont#EG05");
	emotion ET_WRAP, getnpcid(0,"Aki#EG05");
	emotion ET_WRAP, getnpcid(0,"Weisritter#EG05");
	emotion ET_WRAP, getnpcid(0,"Diqui#EG05");
	sleep 4000;
	npctalk "Doctor: I am Matthias Ziegler. Though I'm a bit unexperienced, I will now explain the dangers of the plague", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: and the guidelines we must follow.", "Doctor#EG05";
	sleep 3000;
	npctalk "Doctor: Based on the case in the comodo village, the unidentified disease is presumed to be airborne.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: Since it is likely transmitted through coughing or breathing, everyone must wear a mask.", "Doctor#EG05";
	sleep 3000;
	npctalk "Doctor: At this stage where nothing is certain, the mask is the first line of defense to protect your life.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: Please try not to take off your mask even indoors.", "Doctor#EG05";
	sleep 2000;
	emotion ET_QUESTION, getnpcid(0,"Diqui#EG05");
	npctalk "Diqui: Is a mask enough to keep us safe?", "Diqui#EG05";
	sleep 4000;
	npctalk "Doctor: In the current situation, complete safety cannot be guaranteed.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: It only reduces the risk. Please cooperate as much as possible.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: The next issue is about handling the corpses.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: If a body that died from the plague is left unattended, the disease will spread even faster.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: All bodies must be buried quickly.", "Doctor#EG05";
	sleep 2000;
	npctalk "Anselia: Will just burying them solve everything? Wouldn't cremation be better?", "Anselia#EG05";
	sleep 4000;
	npctalk "Doctor: Cremation is not allowed. We already have the same case from the comodo village.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: For now, the best method is to disinfect the bodies thoroughly and bury them deep in the ground.", "Doctor#EG05";
	sleep 2000;
	npctalk "Chief: We've already expanded land outside the village to use as a cemetery", "Chief#EG05";
	sleep 4000;
	npctalk "Chief: and prepared for disposal with the help of the few remaining mages.", "Chief#EG05";
	sleep 4000;
	npctalk "Doctor: Next agenda. With the support of the innkeeper, the inn will be used as a treatment center for the plague.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: Patients will be classified according to the severity of symptoms and treated in isolation to prevent further spread.", "Doctor#EG05";
	sleep 2000;
	npctalk "Weisritter: Is it okay to gather people in the treatment center?", "Weisritter#EG05";
	sleep 4000;
	npctalk "Weisritter: You're sure the treatment center won't end up spreading the plague even more?", "Weisritter#EG05";
	sleep 4000;
	npctalk "Doctor: The treatment center will be strictly managed with disinfection procedures and hygiene protocols.", "Doctor#EG05";
	sleep 4000;
	npctalk "Doctor: It will not be a source of spread, but the final stronghold for treatment and containment.", "Doctor#EG05";
	sleep 4000;
	npctalk "Chief: If we don't cooperate in times like these, we're all at risk.", "Chief#EG05";
	sleep 4000;
	npctalk "Chief: Always remember that you are citizens of the magic city of Geffen.", "Chief#EG05";
end;
}

ch1_vrgef1,182,194,4	script	Doctor#EG05	VR_UNKOWN_MAN,{
	mes "[Doctor]";
	mes "During times when nothing is certain, the mask is the first line of defense to protect your life. Please try not to take it off even at home.";
	close;
}

ch1_vrgef1,183,189,0	script	Weisritter#EG05	VR_UNKOWN_MAN,{
	mes "[Weisritter]";
	mes "I thought I had escaped misfortune, but I hadn't.. I see.. so that's how it is..";
	close;
}

ch1_vrgef1,191,188,1	script	Anselia#EG05	4_F_KHGIRL,{
	mes "[Anselia]";
	mes "Ah- this is a curse.. it's the curse of Glast Heim reaching Geffen!";
	close;
}

ch1_vrgef1,179,194,5	script	Chief#EG05	2_M_PHARMACIST,{
	mes "[Chief]";
	mes "Ahem.. We will now begin the plague countermeasure meeting for the village.";
	close;
}

ch1_vrgef1,180,187,1	script	Procyon#EG05	4_F_EINWOMAN,{
	mes "[Procyon]";
	mes "Jerome, shut up and just listen, please.";
	close;
}

ch1_vrgef1,179,184,0	script	Aki#EG05	4_F_JOB_BLACKSMITH,{
	mes "[Aki]";
	mes "Do we even have any masks at home?";
	close;
}

ch1_vrgef1,187,186,1	script	Wandering Bard#EG05	1_M_BARD,{
	cutin "bard_eland01.bmp",2;
	mes "[Eland]";
	mes "Ah- I wanted to drift away like the wind, but I guess I'll have to stay in Geffen a bit longer.";
	close3;
}

ch1_vrgef1,176,186,7	script	Jerome#EG05	4_M_DWARF,{
	mes "[Jerome]";
	mes "Procyon, this situation doesn't look like it'll end anytime soon, huh? What do we do? Hey, are you listening?";
	close;
}

ch1_vrgef1,172,186,7	script	Aurore Dupont#EG05	4_KID01,{
	mes "[Aurore Dupont]";
	mes "This too must be the flow of the great nature...";
	close;
}

ch1_vrgef1,172,191,7	script	Diqui#EG05	4_M_JOB_KNIGHT2,{
	mes "[Diqui]";
	mes "Don't worry, everyone. We'll be able to rise again.";
	close;
}

ch1_vrgef1,77,74,0	script	eventtrigger#EG10	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	npctalk "Woman: Oh.. brother.. dad was diagnosed with a severe case.. what do we do? I'm so worried I'm going crazy.", "Woman#EG10_2";
	sleep 4000;
	npctalk "Man: Get a hold of yourself. Didn't you hear what Doctor said? He said there have been recovery cases even with severe symptoms.", "Man#EG10_1";
	sleep 5000;
	npctalk "Man: Yeah. Let's stay quiet and pray.", "Man#EG10_3";
	sleep 4000;
	npctalk "Man: If we pray while encouraging each other, dad will shake it off and get back on his feet.", "Man#EG10_3";
end;
}

ch1_vrgef1,78,58,0	script	eventtrigger#EG11	HIDDEN_WARP_NPC,3,3,{
	end;

OnTouch:
	npctalk "Parent: Fortunately, they said my son's symptoms are mild.", "Parent#EG11_1";
	sleep 3000;
	npctalk "Parent: He's in quarantine, but just waiting endlessly outside like this makes me uneasy.", "Parent#EG11_1";
	sleep 5000;
	npctalk "Parent: Still, it's mild.", "Parent#EG11_2";
	sleep 4000;
	npctalk "Parent: For now, he's being quarantined not because his life is in danger, but because of the risk of infection.", "Parent#EG11_2";
	sleep 4000;
	npctalk "Parent:  try to take comfort in that. Our neighbors... their whole family was taken to the quarantine ward.", "Parent#EG11_2";
	sleep 4000;
	npctalk "Parent: If we rely on each other and endure, I'm sure they'll recover. Hang in there just a bit more.", "Parent#EG11_3";
end;
}

ch1_vrgef1,62,73,0	script	eventtrigger#EG12	HIDDEN_WARP_NPC,3,3,{
	end;

OnTouch:
	npctalk "Former Patient: I was lucky. Do you know how lonely it feels in the quarantine room?", "Former Patient#EG12";
	sleep 4000;
	npctalk "Former Patient: Other than when the medical staff came, I could only faintly hear the sobbing of other patients.", "Former Patient#EG12";
	sleep 4000;
	npctalk "Former Patient: It felt like no one could hear my voice in that space.", "Former Patient#EG12";
	sleep 4000;
	npctalk "Former Patient: Everyone was just crying in pain.", "Former Patient#EG12";
	sleep 4000;
	npctalk "Patient's Acquaintance: You went through a lot. You're amazing for endurng that.", "Patient's Acquaintance#EG12";
	sleep 4000;
	npctalk "Patient's Acquaintance: I heard you're starting volunteer work here at the quarantine facility?", "Patient's Acquaintance#EG12";
	sleep 4000;
	npctalk "Former Patient: I realized it after being sick.", "Former Patient#EG12";
	sleep 4000;
	npctalk "Former Patient: When you're sick and there's no one by your side.. it feels lonely..", "Former Patient#EG12";
	sleep 4000;
	npctalk "Former Patient: If I can at least be someone to talk to for the sick, maybe I can help ease their loneliness.", "Former Patient#EG12";
end;
}

ch1_vrgef1,61,58,0	script	eventtrigger#EG13	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	npctalk "Complaining Man: Is it really right to build a quarantine facility inside the village?", "Complaining Man#EG13";
	sleep 3000;
	npctalk "Complaining Man: f you're going to build a quarantine facility, it should be outside the village where there are no people.", "Complaining Man#EG13";
	sleep 3000;
	npctalk "Complaining Man: Just how do you plan to control this situation? At least give us an explanation!", "Complaining Man#EG13";
	sleep 3000;
	npctalk "Complaining Woman: That's right! Even if it's a temporary quarantine hospital, it's way too busy around here..", "Complaining Woman#EG13";
	sleep 3000;
	npctalk "Complaining Woman: I get it now, how dangerously unstable Geffen is right now. Just how are you going to control this situation..", "Complaining Woman#EG13";
	sleep 3000;
	npctalk "Complaining Woman: Why won't you explain it to us?", "Complaining Woman#EG13";
	sleep 3000;
	npctalk "Eland: Oh.. oh.. dear complainants~♬", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland: You didn't attend the meeting at the village hall, did you?", "Bard Eland#EG13";
	sleep 2000;
	npctalk "Eland: Like the gentle breeze, I, Eland, shall share with you what was said at the meeting~♬", "Bard Eland#EG13";
	sleep 2000;
	npctalk "Eland: One~♬", "Bard Eland#EG13";
	sleep 2000;
	npctalk "Eland: Two~♬", "Bard Eland#EG13";
	sleep 2000;
	npctalk "Eland: Geffen Tower collapses, a city shrouded in darkness~♬", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland: Unknown plague! Standing amidst the fear", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland:  Fear can be overcome, gather your hearts firmly~♬", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland: From the moment the countermeasure committee was formed, we vowed to overcome this together as one.", "Bard Eland#EG13";
	sleep 5000;
	emotion ET_OTL,getnpcid(0, "Bard Eland#EG13");
	npctalk "Eland: Unite, oh Geffen~♬", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland: Even before an uncertain future, we shall walk on, relying on each other~♬", "Bard Eland#EG13";
	sleep 3000;
	emotion ET_DELIGHT, getnpcid(0,"Bard Eland#EG13");
	npctalk "Eland: The voice of Doctor rings out like a warning~♬", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland: In the airborne threat, the mask is the shield of life!", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland: Painful memories of the comodo village, bury the bodies swiftly in the ground~♬", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland: The inn becomes a treatment center to fight on the front lines!", "Bard Eland#EG13";
	sleep 3000;
	emotion ET_DELIGHT, getnpcid(0,"Bard Eland#EG13");
	npctalk "Eland: Unite, oh Geffen~♬", "Bard Eland#EG13";
	sleep 3000;
	npctalk "Eland: Even before an uncertain future, we shall walk on, relying on each other~♬", "Bard Eland#EG13";
	sleep 2000;
	emotion ET_THANKS, getnpcid(0,"Bard Eland#EG13");
	emotion ET_SWEAT, getnpcid(0,"Complaining Man#EG13");
	emotion ET_SWEAT, getnpcid(0,"Complaining Woman#EG13");
end;
}

ch1_vrgef1,77,68,3	script	Mild Case Room#sign03clock	4_BULLETIN_BOARD2,{
	end;
}

ch1_vrgef1,77,63,3	script	Mild Case Room#sign03clock-2	4_BULLETIN_BOARD2,{
	end;
}

ch1_vrgef1,62,68,6	script	Quarantine Room#sign09clock	4_BULLETIN_BOARD2,{
	end;
}

ch1_vrgef1,62,63,6	script	Quarantine Room#sign09clock-2	4_BULLETIN_BOARD2,{
	end;
}

ch1_vrgef1,67,73,4	script	Severe Case Room#sign12clock	4_BULLETIN_BOARD2,{
	end;
}

ch1_vrgef1,72,73,4	script	Severe Case Room#sign12clock-2	4_BULLETIN_BOARD2,{
	end;
}

ch1_vrgef2,193,111,0	script	eventtrigger#EG04	HIDDEN_WARP_NPC,3,3,{
	end;
	
OnTouch:
	emotion ET_BLABLA,getnpcid(0, "Cooking Merchant#EG04");
	npctalk "Merchant: Have you heard the rumor? About the comodo village?", "Cooking Merchant#EG04";
	sleep 3000;
	npctalk "Villager: I've heard a bit.. they say the comodo village was wiped out? How did that happened?", "Villager#EG04";
	sleep 3000;
	npctalk "Arjen: You're a bit behind on the news. That plague has been going around lately.", "Arjen#EG04";
	sleep 3000;
	npctalk "Arjen: Apparently, they cremated the bodies of those who died from it according to their funeral customs, and that's what caused the disaster.", "Arjen#EG04";
	sleep 4000;
	emotion ET_QUESTION, getnpcid(0,"Villager#EG04");
	npctalk "Villager: Cremation? They burned the bodies? Why would they do that? They could've just buried them..", "Villager#EG04";
	sleep 4000;
	npctalk "Merchant: Cremating the dead is part of the gypsy culture, or so I heard. Honestly, they didn't do anything wrong. They were just doing what they've always done.", "Cooking Merchant#EG04";
	sleep 4000;
	npctalk "Merchant: Who could've known that cremating the bodies would spread the disease faster?", "Cooking Merchant#EG04";
	sleep 4000;
	emotion ET_HUM,getnpcid(0, "Cooking Merchant#EG04");
	npctalk "Merchant: They were just unlucky.", "Cooking Merchant#EG04";
end;
}

ch1_vrgef1,144,166,2	script(DISABLED)	Doctor#EG02_2	VR_UNKOWN_MAN,{
	mes "[Doctor]";
	mes "Haa.. I think I'll need a drink tonight to sleep.";
	close;
}

ch1_vrgef1,139,169,6	script(DISABLED)	Geffen Guard#EG02_2	4_M_GEF_SOLDIER,{
	mes "[Geffen Guard]";
	mes "Excuse me. I'm here to response to a report.";
	close;
}

ch1_vrgef1,139,170,6	script(DISABLED)	Geffen Guard#EG02_1	4_M_GEF_SOLDIER,{
	mes "[Geffen Guard]";
	mes "Excuse me. I'm here to response to a report.";
	close;
}

ch1_vrgef1,139,169,0	script	eventtrigger#EG02	HIDDEN_WARP_NPC,1,1,{
	end;
	
OnInit:
	.active = false;
end;

OnTouch:
	if ( .active ) {
		end;
	}
	.active = true;
	pctalk "Hm..";
	pcblock(true);
	sleep2 4000;
	pcblock(false);
	npctalk "Doctor: It's concerning that the plague response seems to be going well, but the number of patients keeps increasing..", "Doctor#EG02_1";
	emotion ET_QUESTION, getnpcid(0,"Doctor#EG02_1");
	sleep 4000;
	npctalk "Doctor: What's that in the corner? No way..", "Doctor#EG02_1";
	unitwalk getnpcid(0, "Doctor#EG02_1"), 142, 171;
	sleep 4000;
	emotion ET_OTL,getnpcid(0, "Doctor#EG02_1");
	npctalk "Doctor: Ah.. no.. Nurse, why are you in a place like this..", "Doctor#EG02_1";
	sleep 4000;
	npctalk "Doctor: Was it a death caused by the plague? Just how long has the infection been hidden?", "Doctor#EG02_1";
	sleep 4000;
	npctalk "Doctor: Why didn't you say anything? Did you think you could handle it alone?", "Doctor#EG02_1";
	sleep 4000;
	npctalk "Doctor: If you had told me.. we might have been able to slow it down, even just a little..", "Doctor#EG02_1";
	sleep 4000;
	npctalk "Doctor: It's too late..", "Doctor#EG02_1";
	sleep 2000;
	disablenpc "Doctor#EG02_1";
	enablenpc "Doctor#EG02_2";
	enablenpc "Geffen Guard#EG02_1";
	enablenpc "Geffen Guard#EG02_2";
	sleep 2000;
	npctalk "Guard: This is the Geffen Guard. We're here in response to a report.", "Geffen Guard#EG02_1";
	sleep 4000;
	npctalk "Guard: What happened here?", "Geffen Guard#EG02_1";
	sleep 4000;
	npctalk "Doctor: It seems the Nurse was hiding her infection.", "Doctor#EG02_2";
	sleep 4000;
	npctalk "Doctor: I've been too busy with work the at the clinic.. I only discovered it now.", "Doctor#EG02_2";
	sleep 4000;
	npctalk "Guard: I see. We'll handle the Nurse's body according to the relevant regulations.", "Geffen Guard#EG02_1";
	sleep 4000;
	npctalk "Guard: This should be officially recorded.", "Geffen Guard#EG02_1";
	sleep 4000;
	npctalk "Doctor: Please handle her with care.", "Doctor#EG02_2";
	sleep 4000;
	npctalk "Doctor: She was someone who worked tirelessly for her patients until the very end.", "Doctor#EG02_2";
	sleep 4000;
	npctalk "Doctor: For this clinic and this chaotic city.", "Doctor#EG02_2";
	sleep 4000;
	npctalk "Guard: We'll keep that in mind.", "Geffen Guard#EG02_2";
	sleep 2000;
	disablenpc "Geffen Guard#EG02_1";
	disablenpc "Geffen Guard#EG02_2";
	disablenpc "Nurse#EG02";
	sleep 4000;
	npctalk "Doctor: Was she enduring it with the medicines we have that matches with her symptoms?", "Doctor#EG02_2";
	sleep 4000;
	npctalk "Doctor: She must have been lonely.. haa.. I think I'll need a drink tonight to fall asleep.", "Doctor#EG02_2";
	disablenpc "Doctor#EG02_2";
	sleep 10000;
	.active = false;
	enablenpc "Doctor#EG02_1";
	unitwarp getnpcid(0, "Doctor#EG02_1"), "ch1_vrgef1", 140, 169;
	enablenpc "Nurse#EG02";
end;
}

ch1_vrgef1,138,146,0	script	eventtrigger#EG03	HIDDEN_WARP_NPC,1,1,{
	end;
	
OnInit:
	.active = false;
end;
	
OnTouch:
	if ( .active ) {
		end;
	}
	if ( isbegin_quest(19240) == 1 ) {
		pcblock(true);
		pctalk "Let's leave the Illusion Book.";
		sleep2 2000;
		pcblock(false);
		pctalk "Is there anything else to see in this room?";
		end;
	}
	.active = true;
	pcblock(true);
	sleep2 2000;
	emotion ET_SURPRISE, getcharid(3);
	npctalk "Doctor: ... ... ...", "Doctor#EG03_1";
	emotion ET_PROFUSELY_SWAT, getnpcid(0,"Doctor#EG03_1");
	specialeffect EF_MAGICCRASHER2, AREA, "Doctor#EG03_1";
	sleep2 3000;
	specialeffect EF_SOULBREAKER2, AREA, "Doctor#EG03_1";
	disablenpc "Doctor#EG03_1";
	enablenpc "???#EG03";
	specialeffect EF_MAGICCRASHER2, AREA, "???#EG03";
	if ( isbegin_quest(19239) == 1 ) {
		erasequest 19239;
		setquest 19240;
	}
	sleep2 2000;
	npctalk "???: Grrrr-", "???#EG03";
	pctalk "A human turned into a monster?! That's terrifying..";
	pcblock(false);
	sleep 4000;
	enablenpc "Thief#EG03_1";
	sleep 1000;
	npctalk "Thief: In my experience, there's always money in hospitals. The town rumors said Doctor hadn't been seen for quite a while..", "Thief#EG03_1";
	sleep 4000;
	emotion ET_HUK, getnpcid(0,"Thief#EG03_1");
	npctalk "Thief: Ack! What the, why is there a monster?!", "Thief#EG03_1";
	sleep 4000;
	npctalk "Thief: Ugh ■◇#☆@○★ what a crappy day..", "Thief#EG03_1";
	sleep 1000;
	disablenpc "Thief#EG03_1";
	sleep 1000;
	enablenpc "Thief#EG03_2";
	specialeffect EF_TEIHIT3, AREA, "Thief#EG03_2";
	specialeffect EF_BACKSTAP, AREA, "???#EG03";
	sleep 1000;
	disablenpc "???#EG03";
	sleep 2000;
	emotion ET_BEST, getnpcid(0,"Thief#EG03_2");
	npctalk "Thief: Ha- ■◇#☆ that was wild. So this means the place is empty, right?", "Thief#EG03_2";
	sleep 4000;
	npctalk "Thief: Let's see- what should I take that'll fetch a good price~", "Thief#EG03_2";
	sleep 4000;
	emotion ET_AHA, getnpcid(0,"Thief#EG03_2");
	npctalk "Thief: So this is a doctor's coat? I'll take it~", "Thief#EG03_2";
	sleep 3000;
	emotion ET_QUESTION, getnpcid(0,"Thief#EG03_2");
	npctalk "Thief: What's this book? I'll take it too, just in case~", "Thief#EG03_2";
	sleep 2000;
	enablenpc "Geffen Guard#EG03";
	sleep 2000;
	npctalk "Guard: Ah! Nice to meet you. You must be the new Doctor, right?", "Geffen Guard#EG03";
	emotion ET_HUK, getnpcid(0,"Thief#EG03_2");
	sleep 1000;
	disablenpc "Thief#EG03_2";
	enablenpc "Doctor#EG03_2";
	sleep 3000;
	emotion ET_SCRATCH, getnpcid(0,"Doctor#EG03_2");
	npctalk "Doctor: Uh.. well.. yes.. I'm the Doctor.", "Doctor#EG03_2";
	sleep 4000;
	npctalk "Guard: The clinic is short on specialists. Could you come now?", "Geffen Guard#EG03";
	sleep 4000;
	npctalk "Doctor: Ah.. hmm... it can't be helped. Let's go.", "Doctor#EG03_2";
	sleep 2000;
	disablenpc "Doctor#EG03_2";
	disablenpc "Geffen Guard#EG03";
	sleep 7000;
	.active = false;
	enablenpc "Doctor#EG03_1";
end;
}

ch1_vrgef1,169,48,1	script	Brenda#EG08	4_F_05,{
	mes "[Brenda]";
	mes "Please, come quickly!";
	close;
}

ch1_vrgef1,173,44,4	script	Angel#EG08	4_M_03,{
	mes "[Angel]";
	mes "Damn it, I think I've done everything I can...";
	close;
}

ch1_vrgef1,173,42,3	script	Patient#EG08_1	4_M_DSTMANDEAD,{
	mes "[Patient]";
	mes "Ugh.. it hurts..";
	close;
}

ch1_vrgef1,173,38,3	script	Patient#EG08_2	4_M_PATIENT,{
	mes "[Patient]";
	mes "Agh..";
	close;
}

ch1_vrgef1,166,50,5	script(DISABLED)	Doctor#EG08_1	VR_UNKOWN_MAN,{
	mes "[Doctor]";
	mes "I'm sorry I'm late. What's the situation?";
	close;
}

ch1_vrgef1,171,41,6	script(DISABLED)	Doctor#EG08_2	VR_UNKOWN_MAN,{
	mes "[Doctor]";
	mes "While I administer the treatment, please hold the patient's body firmly. Once the medication is given, they should calm down a bit.";
	close;
}

ch1_vrgef1,169,41,0	script	eventtrigger#EG08	HIDDEN_WARP_NPC,5,5,{
	end;
	
OnInit:
	.active = false;
end;
	
OnTouch:
	if ( .active ) {
		end;
	}
	.active = true;
	npctalk "Brenda: Please, come quickly! Someone's dying.", "Brenda#EG08";
	sleep 3000;
	npctalk "Angel: Damn it, Brenda, calm down. I think I've done everything I can.. but their condition keeps getting worse.", "Angel#EG08";
	sleep 3000;
	npctalk "Patient: Ugh..", "Patient#EG08_1";
	npctalk "Patient: Haa..", "Patient#EG08_2";
	sleep 2000;
	enablenpc "Doctor#EG08_1";
	sleep 2000;
	npctalk "Doctor: I'm sorry I'm late. What's the situation? Where is the patient?", "Doctor#EG08_1";
	sleep 3000;
	npctalk "Brenda: Over there. We moved them to a bed.", "Brenda#EG08";
	sleep 3000;
	npctalk "Angel: This! This person is in danger.", "Angel#EG08";
	sleep 2000;
	disablenpc "Doctor#EG08_1";
	enablenpc "Doctor#EG08_2";
	sleep 3000;
	npctalk "Doctor: Please calm down. Let me have a look.", "Doctor#EG08_2";
	sleep 3000;
	npctalk "Doctor: This is bad.. the condition is serious. Can you assist me?", "Doctor#EG08_2";
	sleep 1000;
	emotion ET_OK, getnpcid(0,"Angel#EG08");
	sleep 2000;
	npctalk "Doctor: While I apply treatment, please hold the patient's body firmly. Once I administer it, they should calm down a bit.", "Doctor#EG08_2";
	sleep 3000;
	npctalk "Angel: Ngh..", "Angel#EG08";
	sleep 3000;
	npctalk "Doctor: Alright. I will administer the medicine now. Please hold that person for a while.", "Doctor#EG08_2";
	sleep 3000;
	disablenpc "Doctor#EG08_2";
	.active = false;
end;
}

ch1_vrgef2,119,48,0	script	Fountain#Ch1Fountain	HIDDEN_NPC,{
	mes "There is a fountain.";
	next;
	mes "Illusion Book-????";
	mes "Interactive Illusion Book";
	setdialogalign(DIALOG_ALIGN_CENTER);
	mes "<FONT SIZE = 15><B>A certain doctor's record..</B></FONT>";
	mes "<FONT SIZE = 15><B>Geffen Recreation</B></FONT>";
	mes " ";
	mes "^4d4dffUnknown record, recreated by Telcos^000000";
	next;
	select("Exit the Record");
	warp "ygg_edge", 241, 226;
	end;
}

ygg_edge,164,140,0	warp	15py_room1_in	1,1,ygg_fruit,73,117
ygg_edge,198,120,0	warp	15py_room2_in	1,1,ygg_fruit,121,115
ygg_edge,139,211,0	warp	15py_room3_in	1,1,ygg_fruit,160,118
ygg_edge,88,184,0	warp	10py_room4_in	1,1,ygg_fruit,33,71
ygg_edge,150,127,0	warp	10py_room5_in	1,1,ygg_fruit,69,70
ygg_edge,72,154,0	warp	10py_room6_in	1,1,ygg_fruit,101,71
ygg_edge,54,183,0	warp	10py_room7_in	1,1,ygg_fruit,135,72

ygg_edge,31,117,0	script	Land of Darkness#to_ygg_roots	GATE_SKYBLUE,1,1,{
	end;
	
OnTouch:
	warp "ygg_roots", 325, 147;
end;
}

ygg_edge,259,245,0	script	Hvergelmir#hvergelmirto_ygg_edge	GATE_SKYBLUE,1,1,{
	end;

OnTouch:
	warp "yggdrasil01", 145, 96;
end;
}

ygg_edge,216,91,0	script	Prontera Field#to_ygg_ro	GATE_SKYBLUE,1,1,{
	end;
	
OnTouch:
	warp "prt_fild05", 353, 248;
end;
}

ygg_edge,193,92,0	script	Payon Field#to_ygg_root	GATE_SKYBLUE,1,1,{
	end;
	
OnTouch:
	warp "pay_fild01", 308, 337;
end;
}

ygg_fruit,71,114,0	warp	15py_room1_out	1,1,ygg_edge,168,139
ygg_fruit,121,112,0	warp	15py_room2_out	1,1,ygg_edge,195,117
ygg_fruit,157,116,0	warp	15py_room3_out	1,1,ygg_edge,142,209
ygg_fruit,31,69,0	warp	10py_room4_out	1,1,ygg_edge,89,188
ygg_fruit,67,67,0	warp	10py_room5_out	1,1,ygg_edge,149,130
ygg_fruit,98,69,0	warp	10py_room6_out	1,1,ygg_edge,73,151
ygg_fruit,132,71,0	warp	10py_room7_out	1,1,ygg_edge,57,191
ygg_fruit,167,77,0	warp	10py_room8_out	1,1,ygg_edge,0,0

ygg_roots,325,153,0	script	Ashridge#to_ygg_edge	GATE_SKYBLUE,1,1,{
	end;
	
OnTouch:
	warp "ygg_edge", 36, 117;
end;
}

ygg_roots,157,242,0	script	#to_hem_fild	21813,1,1,{
	end;

OnTouch:
	warp "hem_fild", 207, 27;
end;
}

ygg_roots,34,126,0	script	#to_uknw_ruin	21813,1,1,{
	end;
	
OnTouch:
	warp "uknw_ruin", 264, 238;
end;
}

ygg_roots,343,252,0	script	#to_mu_fild01	21813,1,1,{
	end;
	
OnTouch:
	if ( isbegin_quest(18376) < 1 ) {
		pctalk "It's too hot to approach......!";
		end;
	}
	warp "mu_fild01", 89, 154;
end;
}

hem_fild,207,32,0	script	#hem_fild_to_ygg_roots	GATE_SKYBLUE,1,1,{
	end;
	
OnTouch:
	warp "ygg_roots", 161, 241;
end;
}

uknw_ruin,266,243,0	script	#ruin_to_ygg_roots	GATE_SKYBLUE,1,1,{
	end;
	
OnTouch:
	warp "ygg_roots", 37, 123;
end;
}

//Geffen
ch1_gef_in,28,156,0	warp	weaponshop_geffen	1,1,ch1_geffen,101,138
ch1_gef_in,28,106,0	warp	tavern_geffen	1,1,ch1_geffen,136,136
ch1_gef_in,70,132,0	warp	toolshop_geffen	1,1,ch1_geffen,46,88
ch1_gef_in,70,158,0	warp	toolbackroom_geffen	1,1,ch1_geffen,76,191
ch1_gef_in,163,94,0	warp	mageacademy_geffen	1,1,ch1_geffen,65,176
ch1_gef_in,70,48,0	warp	geffeninn_geffen	1,1,ch1_geffen,168,170
ch1_gef_in,41,67,0	warp	9clockroom_innlobby	1,1,ch1_gef_in,55,65
ch1_gef_in,25,38,0	warp	basement_9clockroom	1,1,ch1_gef_in,26,69
ch1_gef_in,67,39,0	warp	nextroom_basement	1,1,ch1_gef_in,30,26
ch1_gef_in,87,65,0	warp	lobby_3clockroom	1,1,ch1_gef_in,103,67
ch1_gef_in,100,67,0	warp	3clockroom_lobby	1,1,ch1_gef_in,84,65
ch1_gef_in,114,60,0	warp	3clockroom_3clockroom2	1,1,ch1_gef_in,114,34
ch1_gef_in,114,37,0	warp	3clockroom2_3clockroom	1,1,ch1_gef_in,114,63
ch1_gef_in,70,83,0	warp	lobby_12clockroom	1,1,ch1_gef_in,72,101
ch1_gef_in,72,98,0	warp	12clockroom_lobby	1,1,ch1_gef_in,70,80
ch1_gef_in,79,107,0	warp	12clockroom_12clockroom2	1,1,ch1_gef_in,107,109
ch1_gef_in,104,109,0	warp	12clockroom2_12clockroom	1,1,ch1_gef_in,76,107
ch1_gef_in,106,181,0	warp	bls1_geffen	1,1,ch1_geffen,178,63
ch1_gef_in,113,163,0	warp	bls1_bls2	1,1,ch1_gef_in,139,169
ch1_gef_in,136,169,0	warp	bls2_bls1	1,1,ch1_gef_in,110,163
ch1_gef_in,138,149,0	warp	blsroom2_bls2	1,1,ch1_gef_in,138,165
ch1_gef_in,166,54,0	warp	howard_geffen	1,1,ch1_geffen,138,101
ch1_gef_in,181,177,0	warp	misa_Exit	1,1,ch1_geffen,102,193


ch1_gef_in,52,65,0	script	lobbyinn_9clock	WARPNPC,1,1,{
	end;
	
OnTouch:
	if ( isbegin_quest(16940) > 0 ) { //TODO COMPLETEQUEST FINISH CHECK
		warp "ch1_gef_in", 38, 67;
		end;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16940) == 1";
end;
}

ch1_gef_in,25,64,0	script	9clockroom_basement	WARPNPC,1,1,{
	end;
	
OnTouch:
	if ( isbegin_quest(16941) == 1 ) {
		cloaknpc("Doorway#rch1_01", false, getcharid(0));
		cloaknpc("Henry Bauer#rch1_02", false, getcharid(0));
		cloaknpc("Doctor Neumann#rch1_04", false, getcharid(0));
		cloaknpc("Becky Bauer#rch1_01", false, getcharid(0));
	}
	if ( isbegin_quest(16942) == 1 ) {
		cloaknpc("Doorway#rch1_01", true, getcharid(0));
	}
	if ( isbegin_quest(16945) == 1 ) {
		erasequest 16945;
		setquest 16946;
	}
	if ( isbegin_quest(16941) > 0 ) { //TODO COMPLETEQUEST FINISH CHECK
		warp "ch1_gef_in", 26, 33;
		end;
	}
end;
}

ch1_gef_in,29,22,0	script	9clock_basementafter	WARPNPC,1,1,{
	end;
	
OnTouch:
	if ( isbegin_quest(16942) == 1 ) {
		cloaknpc("Einhar#rch1_10", false, getcharid(0));
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "Please wait a moment, "+strcharinfo(0)+"!";
		next;
		cutin "",255;
		select("It's urgent!", "We need to follow them right now!");
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "I don't know what exactly happened...";
		mes "But I saw Doctor Neumann being dragged inside.";
		next;
		mes "[Einhar]";
		mes "But, "+strcharinfo(0)+" you shouldn't go inside alone, it feels ominous in there.";
		mes "We better prepare.";
		next;
		cutin "",255;
		select("Then come with me.");
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "...Let's do this instead. I'll go in first and check the situation.";
		mes "In the meantime, could you return to Ashridge and bring someone else to help us?";
		next;
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "I can take care of myself.";
		mes "Please hurry. We don't have time on our hands.";
		next;
		cutin "4job_einhar_01.png",2;
		mes "[Einhar]";
		mes "I always have a <NAVI>[warp]<INFO>ch1_gef_in.gat,29,29,0,101,0</INFO></NAVI> ready to Ashridge.";
		mes "I'll open it for you. Please go and come back quickly.";
		cloaknpc("#rch1_w2", false, getcharid(0));
		specialeffect EF_PORTAL, AREA, "#rch1_w2";
		next;
		cutin "",255;
		select("Be careful.");
		cutin "4job_einhar_02.png",2;
		mes "[Einhar]";
		mes "Don't worry about my safety. I'll be more at ease if you return quickly with someone.";
		mes "I'll leave a trail behind when II track them.";
		erasequest 16942;
		setquest 16943;
		close3;
	}
	if ( isbegin_quest(16945) == 1 ) {
		erasequest 16945;
		setquest 16946;
	}
	if ( isbegin_quest(16946) > 0 ) { //TODO COMPLETEQUEST FINISH CHECK
		warp "ch1_gef_in", 67, 34;
		end;
	}
end;

OnInit:
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16942) == 1";
	questinfo QTYPE_QUEST, QMARK_YELLOW, "isbegin_quest(16946) == 1";
end;
}

ch1_gef_in,138,162,0	script	#warp_mago	WARPNPC,2,2,{
	end;

OnTouch:
	if ( isbegin_quest(23276) == 0 ) {
		pctalk "You can't go in because the door is locked.";
		end;
	}
	warp "ch1_gef_in", 138, 146;
end;
}

ch1_gef_in,67,29,0	script	corpseroom	WARPNPC,1,1,{
	end;
	
OnTouch:
	if ( isbegin_quest(16946) == 0 ) { //TODO COMPLETEQUEST FINISH CHECK
		end;
	}
	warp "ch1_que01", 135, 119;
end;
}

ch1_geffen,120,123,0	script	to_hem_dun01	WARPNPC,1,1,{
	end;
	
OnTouch:
	if ( isbegin_quest(17899) == 2 ) { //TODO ADD FINISH COMPLETEQUEST
		warp "hem_dun01",18,86;
		end;
	}
end;
}

ch1_geffen,215,119,0	warp	gef_to_hem_fild	1,1,hem_fild,36,193
ch1_geffen,98,141,0	warp	geffen_weaponshop	1,1,ch1_gef_in,28,160
ch1_geffen,138,138,0	warp	geffen_tavern	1,1,ch1_gef_in,28,110
ch1_geffen,43,85,0	warp	geffen_toolshop	1,1,ch1_gef_in,70,138
ch1_geffen,73,194,0	warp	geffen_toolshoproom	1,1,ch1_gef_in,70,161
ch1_geffen,61,180,0	warp	geffen_toolshopback	1,1,ch1_gef_in,162,97
ch1_geffen,172,174,0	warp	geffen_mageacademy	1,1,ch1_gef_in,70,52
ch1_geffen,182,59,0	warp	geffen_bls1	1,1,ch1_gef_in,106,177
ch1_geffen,140,99,0	warp	geffen_haword	1,1,ch1_gef_in,166,50
ch1_geffen,102,197,0	warp	misa_in	1,1,ch1_gef_in,181,181
ch1_gfn01,59,173,0	warp	gfn01_to_hem_fild	1,1,hem_fild,231,292
ch1_gfn03,145,27,0	warp	gfn03_to_hem_fild	1,1,hem_fild,133,329
ch1_que01,130,119,0	warp	geffenia-corpseroom	1,1,ch1_gef_in,68,33

ch1_vrgef1,28,156,0	warp	#weaponshop_geffen	1,1,ch1_vrgef2,101,138
ch1_vrgef1,70,132,0	warp	#toolshop_geffen	1,1,ch1_vrgef2,46,88
ch1_vrgef1,28,106,0	warp	#tavern_geffen	1,1,ch1_vrgef2,136,136
ch1_vrgef1,70,158,0	warp	#toolbackroom_geffen	1,1,ch1_vrgef2,76,191
ch1_vrgef1,163,94,0	warp	#mageacademy_geffen	1,1,ch1_vrgef2,65,176
ch1_vrgef1,70,48,0	warp	#geffeninn_geffen	1,1,ch1_vrgef2,168,170
ch1_vrgef1,52,65,0	warp	#lobbyinn_9clock	1,1,ch1_vrgef1,38,67
ch1_vrgef1,41,67,0	warp	#9clockroom_innlobby	1,1,ch1_vrgef1,55,65
ch1_vrgef1,25,64,0	warp	#9clockroom_basement	1,1,ch1_vrgef1,26,33
ch1_vrgef1,25,38,0	warp	#basement_9clockroom	1,1,ch1_vrgef1,26,69
ch1_vrgef1,29,22,0	warp	#9clock_basementafter	1,1,ch1_vrgef1,67,34
ch1_vrgef1,67,39,0	warp	#nextroom_basement	1,1,ch1_vrgef1,30,26
ch1_vrgef1,87,65,0	warp	#lobby_3clockroom	1,1,ch1_vrgef1,103,67
ch1_vrgef1,100,67,0	warp	#3clockroom_lobby	1,1,ch1_vrgef1,84,65
ch1_vrgef1,114,60,0	warp	#3clockroom_3clockroom2	1,1,ch1_vrgef1,114,34
ch1_vrgef1,114,37,0	warp	#3clockroom2_3clockroom	1,1,ch1_vrgef1,114,63
ch1_vrgef1,70,83,0	warp	#lobby_12clockroom	1,1,ch1_vrgef1,72,101
ch1_vrgef1,72,98,0	warp	#12clockroom_lobby	1,1,ch1_vrgef1,70,80
ch1_vrgef1,79,107,0	warp	#12clockroom_12clockroom2	1,1,ch1_vrgef1,107,109
ch1_vrgef1,104,109,0	warp	#12clockroom2_12clockroom	1,1,ch1_vrgef1,76,107
ch1_vrgef1,106,181,0	warp	#bls1_geffen	1,1,ch1_vrgef2,178,63
ch1_vrgef1,113,163,0	warp	#bls1_bls2	1,1,ch1_vrgef1,139,169
ch1_vrgef1,136,169,0	warp	#bls2_bls1	1,1,ch1_vrgef1,110,163
ch1_vrgef1,138,162,0	warp	#blsroom2_bls2	1,1,ch1_vrgef1,138,146
ch1_vrgef1,138,149,0	warp	#blsroom2_bls2-2	1,1,ch1_vrgef1,138,165
ch1_vrgef1,166,54,0	warp	#howard_geffen	1,1,ch1_vrgef2,138,101
ch1_vrgef1,181,177,0	warp	#misa_Exit	1,1,ch1_vrgef2,102,193
ch1_vrgef2,43,85,0	warp	#geffen_toolshop	1,1,ch1_vrgef1,70,138
ch1_vrgef2,138,138,0	warp	#geffen_tavern	1,1,ch1_vrgef1,28,110
ch1_vrgef2,73,194,0	warp	#geffen_toolshoproom	1,1,ch1_vrgef1,70,161
ch1_vrgef2,61,180,0	warp	#geffen_toolshopback	1,1,ch1_vrgef1,162,97
ch1_vrgef2,172,174,0	warp	#geffen_mageacademy	1,1,ch1_vrgef1,70,52
ch1_vrgef2,182,59,0	warp	#geffen_bls1	1,1,ch1_vrgef1,106,177
ch1_vrgef2,140,99,0	warp	#geffen_haword	1,1,ch1_vrgef1,166,50
ch1_vrgef2,102,197,0	warp	#misa_in	1,1,ch1_vrgef1,181,181

ch1_sf03,174,246,0	warp	#c01msw00	2,2,hem_fild,102,155
ch1_sf03,271,189,0	warp	#c01msw20	2,2,hem_dun01,85,145
ch1_sf03,103,169,0	warp	#c01msw21	2,2,hem_dun01,182,192
ch1_dw,270,294,0	warp	#c01msw30	2,2,hem_dun01,226,212
ch1_dw,259,352,0	warp	#c01msw31	1,1,ch1_dw,265,65
ch1_dw,265,60,0	warp	#c01msw32	1,1,ch1_dw,253,349
ch1_dw,256,69,0	warp	c01msw33	1,1,ch1_dw,212,64
ch1_dw,217,64,0	warp	#c01msw33	1,1,ch1_dw,260,69
ch1_dw,275,69,0	warp	c01msw34	1,1,ch1_dw,326,54
ch1_dw,321,54,0	warp	#c01msw34	1,1,ch1_dw,272,69

//Brimir
ch1_sf02,155,169,0	warp	#uknw00	1,1,uknw_ruin,155,166
ch1_sf02,103,52,0	warp	#uknw02	1,1,uknw_ruin,149,152
ch1_sf02,125,273,0	warp	#uknw03	1,1,uknw_ruin,255,62
ch1_sf02,32,53,0	warp	#uknw04	1,1,uknw_ruin,93,172

//Muspelheim
ch1_sf01,191,205,0	warp	#out_chez	2,2,mu_fild01,89,153
ch1_sf01,54,205,0	warp	#out_harban	2,2,mu_fild01,154,218
ch1_sf01,274,220,0	warp	#out_tamarin	2,2,mu_fild01,220,241
ch1_sf01,101,224,0	warp	#out_yuvilus	2,2,mu_fild01,194,183

mu_fild01,84,160,0	script	#mufild_01_to_ygg_roots	GATE_SKYBLUE,1,1,{
	end;
	
OnTouch:
	warp "ygg_roots", 342, 248;
end;
}

hem_fild,31,193,0	warp	#to_ch1_geffen	2,2,ch1_geffen,210,119
hem_dun01,13,82,0	warp	#hemd_to_ch1_geffen	1,1,ch1_geffen,120,116
uknw_ruin,275,235,0	warp	#sf0201	1,1,ch1_sf02,190,39
ch1_sf02,194,34,0	warp	#uknw01	1,1,uknw_ruin,270,235
uknw_ruin,37,246,0	warp	#to_uknw_ruin2	1,1,uknw_ruin2,33,246
uknw_ruin2,37,246,0	warp	#ruin2_to_uknw_ruin	1,1,uknw_ruin,33,246
hem_dun02,59,171,0	warp	#to_hem_fild_1	1,1,hem_fild,329,188
ch1_gfn01,59,173,0	warp	#to_hem_fild_2	1,1,hem_fild,231,292
ch1_gfn03,145,27,0	warp	#to_hem_fild_3	1,1,hem_fild,133,329

ygg_roots,0,0	monster	--en--	22443,30,5000
ygg_roots,0,0	monster	--en--	22444,30,5000
hem_dun01,0,0	monster	--en--	22454,30,5000
hem_dun01,0,0	monster	--en--	22455,30,5000
hem_dun01,0,0	monster	--en--	22456,15,5000
hem_dun01,0,0	monster	--en--	22457,15,5000
hem_dun01,0,0	monster	--en--	22458,15,5000
hem_dun01,0,0	monster	--en--	22459,15,5000
hem_dun01,0,0	monster	--en--	22460,20,5000
hem_dun01,0,0	monster	--en--	22461,20,5000
hem_dun01,0,0	monster	--en--	22462,20,5000
hem_dun02,0,0	monster	--en--	22471,50,5000
hem_dun02,0,0	monster	--en--	22472,50,5000
hem_dun02,0,0	monster	--en--	22473,50,5000
hem_dun02,0,0	monster	--en--	22474,50,5000
hem_dun02,0,0	monster	--en--	22475,50,5000
hem_fild,0,0	monster	--en--	22451,35,5000
hem_fild,0,0	monster	--en--	22452,35,5000
hem_fild,0,0	monster	--en--	22453,35,5000
hem_fild,0,0	monster	--en--	22460,13,5000
hem_fild,0,0	monster	--en--	22461,13,5000
hem_fild,0,0	monster	--en--	22462,13,5000
mu_fild01,0,0	monster	--en--	22445,20,5000
mu_fild01,0,0	monster	--en--	22446,20,5000
mu_fild01,0,0	monster	--en--	22447,40,5000
mu_fild01,0,0	monster	--en--	22448,40,5000
mu_fild01,0,0	monster	--en--	22449,40,5000
mu_fild01,0,0	monster	--en--	22450,40,5000
uknw_ruin,0,0	monster	--en--	22463,20,5000
uknw_ruin,0,0	monster	--en--	22464,20,5000
uknw_ruin,0,0	monster	--en--	22465,45,5000
uknw_ruin,0,0	monster	--en--	22466,45,5000
uknw_ruin,0,0	monster	--en--	22467,45,5000
uknw_ruin2,0,0	monster	--en--	22463,20,5000
uknw_ruin2,0,0	monster	--en--	22464,20,5000
uknw_ruin2,0,0	monster	--en--	22468,45,5000
uknw_ruin2,0,0	monster	--en--	22469,45,5000
uknw_ruin2,0,0	monster	--en--	22470,45,5000
ch1_gfn01,0,0	monster	--en--	22476,40,5000
ch1_gfn01,0,0	monster	--en--	22478,40,5000
ch1_gfn01,0,0	monster	--en--	22480,40,5000
ch1_gfn01,0,0	monster	--en--	22482,40,5000
ch1_gfn01,0,0	monster	--en--	22484,40,5000
ch1_gfn01,0,0	monster	--en--	22486,40,5000
ch1_gfn01,0,0	monster	--en--	22488,40,5000
ch1_gfn03,0,0	monster	--en--	22476,40,5000
ch1_gfn03,0,0	monster	--en--	22478,40,5000
ch1_gfn03,0,0	monster	--en--	22480,40,5000
ch1_gfn03,0,0	monster	--en--	22482,40,5000
ch1_gfn03,0,0	monster	--en--	22484,40,5000
ch1_gfn03,0,0	monster	--en--	22486,40,5000
ch1_gfn03,0,0	monster	--en--	22488,40,5000

