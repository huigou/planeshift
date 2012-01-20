# HeidiSQL Dump 
#
# --------------------------------------------------------
# Host:                 127.0.0.1
# Database:             planeshift
# Server version:       5.0.67-community-nt
# Server OS:            Win32
# Target-Compatibility: MySQL 5.0
# max_allowed_packet:   1048576
# HeidiSQL version:     3.2 Revision: 1129
# --------------------------------------------------------

/*!40100 SET CHARACTER SET latin1*/;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0*/;


DROP TABLE IF EXISTS `quest_scripts`;

#
# Table structure for table 'quest_scripts'
#

CREATE TABLE `quest_scripts` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `quest_id` int(10) NOT NULL default '0' COMMENT 'FK to quests table, or -1 for KA scripts.',
  `script` TEXT COMMENT 'The script for the quest, parsed by questmanager.cpp',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=212 /*!40100 DEFAULT CHARSET=utf8*/;



#
# Dumping data for table 'quest_scripts'
#

#LOCK TABLES `quest_scripts` WRITE;
/*!40000 ALTER TABLE `quest_scripts` DISABLE KEYS*/;
INSERT INTO `quest_scripts` (`id`, `quest_id`, `script`) VALUES
	('1',1,'P:give me quest\nMenu: Is there anyone you need saving?\n\nMerchant: Congratulations! You get to save the princess now.[Merchant bows to $playername.]I wish you luck.[As he finishes talking, you notice clouds darkening on the horizon...]\n\nAssign Quest\n\n...\n\nP: done\nMenu: Ok she is safe on Yavin 4 now.\n\nM: OMG you did it!  I can\'t believe it.\n\nGive Mug or Steel Falchion or Claymore\n\n'),
	('2',2,'P: can bring you sandwich\nMenu: Is there anything I can bring you?\n\nMerchant: I\'m kinda hungry.. bring me a sandwich and hurry if you want a hexa and a mug for your trouble.(/planeshift/data/voice/merchant/clawatit.wav)\n\nAssign Quest\n\n...\n\nP: cannot find. P: here you\nMenu: Sorry but I can\'t find any sandwiches.  Menu: Here is a nice sandwich for you, good sir.\n\nM: Well sandwiches are rare in these parts.  Thanks anyway.\n\nM: Many thanks, this help for my hunger.(/planeshift/data/voice/merchant/thank_you.wav)\n\nGive 1 hexa. Give Mug. Give 3 Potion of Healing. Give 5 faction merchants.\n'),
	('3',3,'P: can bring you falchion\nMenu: Is there anything you need?\n\nMerchant: Kill the enkidukai who wanders in the forest and bring me his steel falchion.\n\nAssign Quest\n\n...\n\nPlayer gives Merchant Steel Falchion\nMenu: Give the merchant a Steel Falchion.\n\nM: Many thanks, brave stranger.  This will come in handy if the fans misbehave.\n\nGive 1 octa\n\n'),
	('4',10,'P: greetings\n\nmaleEnki: Hail!  Would you like to earn a little money?\n\nP: No   P: Yes\nMenu: I am not interested in your chores old man!  Menu: Cool yeah, I am totally broke.\n\nM: Well ok then.  Have a good day.\n\nM: My daughter Alina ran off with the smith and I fear they are up to no good. If you can find out if he really loves her, I\'ll pay you.  Can you do this for me?\n\nP: No P: Yes\nMenu: You will never find her.  Menu: $playername shrugs $his shoulders and nods.\n\nM: Fair enough.  I guess I\'ll just try to find someone else.\n\nM: Last time I caught them upstairs above the blacksmith shop.  Please hurry up and ask her!\n\nAssign Quest.\n\n\nP: Greetings\n\nSmith: Go away!  We\'re busy.{her:Alina}\n\nP: *  P: Do you love Alina\n\nS: You are so rude!  Go away!\nRun script(3) kick_you\n\nS: Of course I love her!  We are going to marry in the morning.\n\n\n\nP: Greetings\n\nMaleEnki: Oh I\'m so glad you are back.  Did you find him?  Does he love her?{him:Smith,her:Alina}\n\nP: *   P: Smith says Smith does. Smith love Alina.\n\nM: That isn\'t what Thalia told me 5 minutes ago.  You\'re just faking to get\n   the money!  Go away!\n\nM: Ah! Thank heavens.  Perhaps now they will get married!  Thanks for your\n   help!\n\nGive 25 tria.\n'),
	('5',11,'P: greetings\n\nMaleEnki: Hello there.  I\'m looking for gold ore... But I need someone I can trust...\n\nP: No. P: Yes.\nMenu: Well then I am not your guy because I am chaotic neutral.  Menu: You can trust your instincts with me.\n\nM: Chaotic neutral characters are a pain but ok, thanks for telling me.  Bye.\n\nM: Yes, I think I will trust my instincts.  Can you find gold for me?  I\'ll gladly pay you a pittance.\n\nP: No.  P: Yes.\nMenu: I am a war hero, not a grubby miner.  Menu: I am a $playerrace!  I love pittances!  $playername will start right away!\n\nM: Ok I guess I\'ll just try to find someone else to help me.\n\nM: Excellent.  If you will bring me 5 gold ores, I will reward you.  Now go!\n\nAssign Quest.\n\n...\n\nPlayer gives MaleEnki 5 Gold Ore.\n\nM: Ah these are just what I needed!  Thanks so much! (How generous of you to just give me gold.  Now go away!)\n\nGive Small Battle Axe.\n'),
	('6',12,'P: Greetings\n\nMaleEnki: Hello $sir my good friend! Can I trust you to carry out one more thing for me?(I have nothing to say to you!)\n\nP: No P: Yes\n\nM: I will not bother you anymore\n\nM: Greate! I have a glyph that I have agreed to exchange with a special sword that the Smith have made for me.\n   Can you take this to him and bring back the sword?\n\nP: No P: Yes\n\nM: Come by some other time, when you have time\n\nM: Excellent. I will reward you when you return\n\nAssign Quest\n\nGive Faith\n\n...\n\nP: Bring glyph from MaleEnki\n\nSmith: I have been waiting for this. Give me the glyph and I give you the sword.\n\nPlayer gives Smith Faith\n\nSmith: Here you go\n\nGive Gold Falchion. Complete Male Enki Trusted Transport Step 2\n\n...\n\nPlayer gives Smith Faith\n\nSmith: A the glyph from the MaleEnki\n\nGive Gold Falchion. Complete Male Enki Trusted Transport Step 2\n\n...\n\nRequire completion of Male Enki Trusted Transport Step 2\n\nPlayer gives MaleEnki Gold Falchion\n\nMaleEnki: Thank you my friend\n\nGive 2 octa\n'),
	('8',13,'P: Lost.\n\nMaleEnki:  Do you want a key?\n\nP: No   P: Yes\nM: Well ok then.\nM: Come back here to get key later!\n\nAssign Quest.\n\nP: Found\n\nMaleEnki: Back again?\n\nP: No   P: Yes\nM: Bye.\nM: Here is your key!\n\nRun script get_next_entrance\n\nGive 25 tria.'),
	('14',14,'P: Windy.\n\nMaleEnki: Windy it is indeed!  Ask me for a kite.\n\nAssign Quest.\n\nP: Kite.\nM: Oh no, looks like rain!  Too bad.  Here, have a rain check for the kite.\n\nRun script rain\n\nGive 25 tria.'),
	('15',15,'P: give me quest.\nSmith: Would you like a quest?\nP: No. P: Yes.\nS: Oh. That\'s not helpful.\nS: Great. What you\'ll need to do is tell me \'I want to test\' and then I\'ll ask you for a tria. Try it by saying \'I want to test\' first, then \'test\' the next time. It should work both ways. Let\'s see if it does.\nAssign Quest.\n... NoRepeat\nP: test.\nSmith: Great. Give me 1 tria, please.\nPlayer gives Smith 1 Tria.\nSmith: Thanks for testing. Here\'s your tria back.\nGive 1 Tria.'),
	('16',16,'P: Hungry.\n\nMaleEnki: Starving, aye?  Bring me 5 eggs, 1 thing of milk and I can fix up something...for 1 hexa.\n\nAssign Quest.\n\nPlayer gives MaleEnki 1 Milk, 1 Hexa, 5 Egg.\nMaleEnki: Here\'s some freshly baked Waybread!  Enjoy...\n\nGive 1 Waybread.'),
	('101',101,'P: step1\n\nQuestMaster1: Yes, this is step1. [Type \'step3\', verify error response. Type \'step4\', verify error response. Type \'step2\']\n\nAssign Quest.\n...\n...\n...\nP: step two. step2.\n\nQ: Yes, this is step2. [You have now completed step2. Type \'step2\',verify quest substep can be restarted. Type \'step3\' or \'step3b\' if on second run.]\n\nComplete QuestMaster1 Quest 1 Step 2.\n\n...\n\nRequire completion of QuestMaster1 Quest 1 Step 2.\n\nP: step3. step three\n\nQ: Yes this is step3. Answer Yes or No. [First run type \'yes\', Second run type \'no\']\n\nP: No. P: Yes\n\nQ: This is the answer to No.[Type \'step2\']\n\nComplete QuestMaster1 Quest 1 Step 3. Complete QuestMaster1 Quest 1 Step 4. Give 3 Tria.\nGive 300 Exp.\n\nQ: This is the answer to Yes.[Type \'step3b\' verify no execution of other path. Type \'step5\']\n\nGive 4 Tria. Complete QuestMaster1 Quest 1 Step 3. Complete QuestMaster1 Quest 1 Step 5.\n\n...\n\nRequire completion of QuestMaster1 Quest 1 Step 4.\n\nP: step3b\n\nQ: Yes this is step3b. You can execute this only if you previously answered No. [Type \'step4\']\n\nComplete QuestMaster1 Quest 1 Step 6\n\n...\n\nRequire completion of QuestMaster1 Quest 1 Step 6\n\nP: step4.\n\nQ: Yes, this is step4. This is the ending if you said No. [Type \'step4\', verify error response. Type \'step2\', verify error response. To restart the quest type \'step1\']\nComplete QuestMaster1 Quest 1.\n...\n\nRequire completion of QuestMaster1 Quest 1 Step 5\n\nP: step5.\n\nQ: Yes, this is step5. This is the ending if you said Yes. [Type \'step4\', verify error response. Type \'step2\', verify error response. To restart the quest type \'step1\']\n'),
	('102',102,'P: quest2\n\nQuestMaster1: Yes, this is QuestMaster1 Quest2. [Type \'done\']\n\nAssign Quest.\n\n...\n\nP: done.\n\nQ: Your done. [First run: Type \'quest2\', within 30 sec to verify no quest assigned. Type \'quest2\' after 30 sec verify \'QuestMaster1 Quest 2\' assigned again. Second run: Type \'quest3\']\n'),
	('103',103,'P: quest3\n\nQuestMaster1: Yes, this is QuestMaster1 Quest3. [Type \'done\']\n\nAssign Quest.\n\n...\n\nP: done.\n\nQ: Your done. [First run: Type \'quest3\', within 30 sec of assignment to verify no quest assigned. Type \'quest2\' after 30 sec avter assignemnt, verify \'QuestMaster1 Quest 2\' assigned again. Second run: Do this from another client.]\n'),
	('104',104,'P: quest4\n\nQuestMaster1: Yes, this is Quest4. Please give me 3 Batter.\n\nAssign Quest.\n\n...\n\nPlayer gives QuestMaster1 3 Batter.\n\nQ: Good thanks! Now give me 5 Trias.\n\nPlayer gives QuestMaster1 5 Tria.\n\nQ: Thanks for the trias! You are done.\n\n'),
	('201',201,'P: quest1\n\nQuestMaster2: Yes, this is QuestMaster2 Quest1. [Type \'done\']\n\nAssign Quest.\n\n...\n\nP: done.\n\nQ: Your done. [Type \'quest4\',verify no quest assigned. Type \'quest2\' verify \'QuestMaster1 Quest 2\' assigned.]\n'),
	('202',202,'P: quest2\n\nQuestMaster2: Yes, this is QuestMaster2 Quest2. [Type \'done\']\n\nAssign Quest.\n\n...\n\nP: done.\n\nQ: Your done. [Type \'quest5\' verify no quest assigned. Type \'quest3\',verify \'QuestMaster1 Quest 3\' assigned.]\n'),
	('203',203,'P: quest3\n\nQuestMaster2: Yes, this is QuestMaster2 Quest3. [Type \'done\']\n\nAssign Quest.\n\n...\n\nP: done.\n\nQ: Your done. [Type \'quest4\',verify \'QuestMaster1 Quest 4\' assigned.]\n'),
	('204',204,'P: quest4\n\nQuestMaster2: Yes, this is QuestMaster2 Quest4.\n\nAssign Quest.\n\n... NoRepeat\n\nP: step2\n\nQuestMaster2: Yes, this is step2, it\'s not repeatable.\n\nP: step2b\n\nQuestMaster2: Yes, this is step2b, it\'s not repeatable.\n\nComplete QuestMaster2 Quest 4 Step 2.\n\n... NoRepeat\nRequire completion of QuestMaster2 Quest 4 Step 2.\nP: step3\nQuestMaster2: Yes, this is step3,  it\'s not repeatable.\nComplete QuestMaster2 Quest 4 Step 3.\n...\n\nP: done.\n\nQ: Your done. [Type \'quest5\',verify \'QuestMaster1 Quest 5\' assigned.]\n'),
	('205',205,'P: quest5\n\nQuestMaster2: Yes, this is QuestMaster2 Quest5. [Type \'done\']\n\nAssign Quest.\n\n...\n\nP: done.\n\nQ: Your done.\n'),
	('206',206,'P: quest6\n\nQuestMaster2: Yes, this is QuestMaster2 Quest6. [Type \'done\']\n\nAssign Quest.\n\n...\n\nP: done.\n\nQ: Your done[You should now never be able to start quest6 again].\n'),
	('207',-1,'P: ring\r\nMenu: Can you tell me about rings?\r\n\r\ngeneral: Rings are interesting because they often have magical powers.  You should definitely keep them if you find any.\r\n...\r\nP: talad\r\nMenu: Please describe the god Talad for me.\r\n\r\ng: He is best described as drunk in love.\r\n...\r\nP: laanx\r\nMenu: Please describe the god Laanx for me.\r\n\r\ng: Not sure if she is a he or a she, but s/he likes very spiky temples apparently.'),
	('208',207,'# Quest Acquire Lapar\n# ID 1214\n# Quest Description: Smith needs some juiceberry fruit before he will sell you any lapar.\n# Player lockout: 43200\n# 12 hours \n# Quest Lockout: 1800\n# 30 minutes \n# Syntax update 5.11.2008\n# Updated with menu system 11/2008.\n\nP: Give me Lapar.\nMenu: I\'m looking for some Lapar.\n\nSmith: Very well $sir, I have some lapar you can have, but it will cost you. [Levrus chuckles.] Shall we say two hundred tria, and three juiceberry fruit? (/planeshift/data/voice/Levrusacquirelapar/1verywellsir.spx)\nP: No. P: Yes.\nMenu: No, that is too much!  Are you kidding me old man?  Menu: You hold your lapar dear old man, but I\'ll pay your price.\n\nSmith: That is my price $playerrace, come back if you change your mind. (/planeshift/data/voice/Levrusacquirelapar/2thatismyprice.spx)\nSmith: Excellent! Bring me the fruit, and I\'ll have some lapar ready for you. (/planeshift/data/voice/Levrusacquirelapar/3excellentbringmethefruit.spx)\nAssign Quest.\n\n... NoRepeat\n# Step 2\nPlayer Gives Smith 3 Juiceberry fruit.\nSmith: Very nice, now for the tria, all two hundred of them please. [Levrus holds out his hand with a smile.] (/planeshift/data/voice/Levrusacquirelapar/4verynicenowforthetria.spx)\nComplete Acquire Lapar Step 2.\n\n...\n# Step 3\nRequire Completion of Acquire Lapar Step 2.\nPlayer Gives Smith 200 tria.\nSmith: [The Smith takes the coins and stuffs them into his coin-pouch.] Very well, one bottle of Lapar for the $playerrace. [Smith hands you a bottle.] Use it wisely, it is not as abundant as water. [Smith chuckles as he turns his attention elsewhere.] (/planeshift/data/voice/Levrusacquirelapar/5verywellonebottle.spx)\nGive 1 Lapar. Give 600 Exp.\nComplete Acquire Lapar Step 3.'),
	('209',208,'# Quest One Thousand Year Sammich \r\n\r\n# Quest Description: You are hungry. Feed yourself.\r\n\r\n# Quest Category: \r\n\r\n# Description Red Paperclip - an ordinary red paperclip. It might possibly be traded for amazing and unheard-of wonders.\r\n\r\n# Description Sandwich Plate - An ancient chipped plate divided into gaudily coloured sections.\r\n\r\n# Description Artisan Waybread - A fluffy warm loaf of freshly-baked bread.\r\n\r\n# Description Corned Beef - A packet of tender, juicy, wafer thin meat.\r\n\r\n# Description Smoked Provolone - A bundle of creamy white cheese slices that carry a faint smoky aroma.\r\n\r\n# Description Sublime Sandwich - The most perfect sandwich you have ever seen, bar none.\r\n\r\n# Description Condiment Recipe - An old parchment, badly creased and stained but still legible.\r\n\r\n# Description Silver Penny - A unit of currency.\r\n\r\n#--------------------------------------------------------------\r\n\r\n# Version 1.0\r\n\r\n# Author(s): Oomi, UTM\r\n\r\n#--------------------------------------------------------------\r\n\r\n# Parameters (Remarked out for now)\r\n\r\n# player lockout -1\r\n\r\n# quest lockout 500\r\n\r\n# Prerequisites: none\r\n\r\n#--------------------------------------------------------------\r\n\r\n# Notes: \r\n\r\n#-------------------------------------------------------------- \r\n \r\n\r\nP: Give me quest.\r\nMenu: I am hungry for something different. Got any food?\r\n\r\nGertie Hollaback: Yup. (/this/data/voice/sammich/1-yup.spx)\r\n\r\nP: rude question. P: Nice question.\r\nMenu: Can I have some? Menu: Well... what kind?\r\n\r\nG: No, ya ain\'t gunna get yer grubby mitts on my food. What kinda person comes up to a complete stranger an esks for a bite\'a her well earned grub? Now you git yerself off my premises a\'for I have ta blast you off. (/this/data/voice/sammich/2-grubbymitts.spx)\r\nG: [Gertie looks down at her bag, and rummages around a bit.] You like crawdads? (/this/data/voice/sammich/3-crawdads.spx)\r\n\r\nAssign Quest. \r\n\r\n... NoRepeat \r\n\r\n# Step 2\r\n\r\nP: No. P: Yes. P: Don\'t know.\r\nMenu: They are horrid little creatures. Menu: They are delicious. Menu: Do I like what?\r\n\r\nG: What? You just ain\'t got no taste. Get on outta here! (/this/data/voice/sammich/4-notaste.spx)\r\nComplete One Thousand Year Sammich Step 2. Complete One Thousand Year Sammich.\r\n\r\nG: [Gertie rummages around in her bag.] That\'s too bad. I ain\'t got none anyhow. (/this/data/voice/sammich/5-toobad.spx)\r\nComplete One Thousand Year Sammich Step 2.\r\n\r\nG: How am I suppose to know what you like? Don\' ask me no fool questions. (/this/data/voice/sammich/6-whatyoulike.spx)\r\nComplete One Thousand Year Sammich Step 2. \r\n \r\n\r\n... NoRepeat \r\n\r\n# Step 3\r\n\r\nRequire completion of One Thousand Year Sammich Step 2.\r\n\r\nP: give me food.\r\nMenu: So, where CAN a body get a snack around here?\r\n\r\nG: Well, I just had a right tasty sammich. Yup, I did. Thick an\' juicy, one-o\'-a-kind like ya done ne\'er had bee-for. [Gertie stares off into the distance and pulls a folded red wire out of her pocket.] (/this/data/voice/sammich/7-tastysammich.spx)\r\nComplete One Thousand Year Sammich Step 3. \r\n\r\n... NoRepeat \r\n\r\n# Step 4\r\n\r\nRequire completion of One Thousand Year Sammich Step 3.\r\n\r\nP: about sammich.\r\nMenu: Well, that sounds nice. Where do I learn more about this \"sammich\"?\r\n\r\nG: [Gertie unfolds the wire and uses it to pick her teeth with great gusto.] Hmmm. Dwarves is real fond o\' they food. Try askin\' one. Oh, here, have a toothpicker. I\'m done with it. [Gertie wipes off the red wire, folds it back up, and hands it to you.] (/this/data/voice/sammich/8-dwarvesfond.spx)\r\nGive 1 Red Paperclip.\r\nComplete One Thousand Year Sammich Step 4. \r\n\r\n... NoRepeat \r\n\r\n# Step 5.\r\n\r\nRequire completion of One Thousand Year Sammich Step 4.\r\n\r\nP: Gertie sent me.\r\nMenu: I\'m starving! Gertie told me you made the best sandwiches around.\r\n\r\nReginald Hartlepool: [Reginald arches an eyebrow.] So, the lowly come to me once again with the desire to sample the fabled festival of sumptuousness that is the pride and heritage of my ancestors? Of course you are. My family\'s wondrous delectables are legendary in these lands. This astounding recipe has been handed down through generations of Hartlepools, each keeping the secrets of those who came before. And so it has come to me, Reginald HH Hartlepool the Third. As you can see, I have used it wisely. [Reginald grins and pats his protruding belly. As he does so, the button on his pants makes a loud pop and goes flying into the air.] (/this/data/voice/sammich/1Sothelowly-1.spx|/this/data/voice/sammich/1Sothelowly-2.spx)\r\n\r\nComplete One Thousand Year Sammich Step 5. \r\n \r\n\r\n...\r\n\r\n...\r\n\r\n... NoRepeat \r\n\r\n# Step 6, 7, [8 for lock]. 6 and 7 will make small branches that will merge a little later. 6 will be the harder branch, though it seems to start off nicer.\r\n\r\nP: you fat. P: lost something.\r\nMenu: You like your own food a little too much, chubby. Menu: You seem to have lost something.\r\n\r\nRequire completion of One Thousand Year Sammich Step 5.\r\n\r\nReginald Hartlepool: [Reginald looks down and gasps, grabbing his pants before they fall down. He pats his pockets awkwardly with his free hand as if looking for something, but stops and looks at you in dismay.] Alas... you may be right on that count. It will not be the first time our great Sandwich of sandwiches has brought a Hartlepool to his knees in shame. Woe is upon my family once again as I stand before you, shed of my pride and nearly my pants for the love of the finest sandwich ever constructed. Anguish and pity is upon me as my worthy ancestors look upon this catastrophe that has befallen my unworthy garment fastener. Misery shall become my name, for I have not a single bit of string, nor a bent bit of metal to repair this onerous calamity.(/this/data/voice/sammich/2Reginaldlooksdown-1.spx|/this/data/voice/sammich/2Reginaldlooksdown-2.spx)\r\n\r\nComplete One Thousand Year Sammich Step 6. Complete One Thousand Year Sammich Step 8.\r\n\r\nRequire completion of One Thousand Year Sammich Step 5.\r\n\r\nReginald Hartlepool: [Reginald looks down and gasps, grabbing his pants before they fall down. He pats his pockets awkwardly with his free hand as if looking for something, but stops and looks at you forlornly.] Oh, good gracious! What an embarrassing predicament. It seems my lust for flavourful delicacies has overcome the fortitude of my slacks, and here I sit without the means to repair the travesty that has become of said garment. If you have something... anything to aid me in this most momentous time of need, I will reward you most graciously.(/this/data/voice/sammich/3ohgoodgracious.spx)\r\n\r\nComplete One Thousand Year Sammich Step 7. Complete One Thousand Year Sammich Step 8. \r\n\r\n... NoRepeat \r\n\r\n# Step 9 - branch from 6. \"You are fat\"\r\n\r\nRequire completion of One Thousand Year Sammich Step 6.\r\n\r\nP: Have paperclip.\r\nMenu: I do have a red paperclip, but it will cost you.\r\n\r\nReginald Hartlepool: I will pay any price you name. I will do anything you ask to save my fallen pride. [Reginald closes his eyes, seeming to brace for the worst.] (/this/data/voice/sammich/4Iwillpayanyprice.spx)\r\nComplete One Thousand Year Sammich Step 9. \r\n\r\n... NoRepeat \r\n\r\n# Step 10\r\n\r\nRequire completion of One Thousand Year Sammich Step 9.\r\n\r\nP: sammich.\r\nMenu: It will cost you... a sammich!\r\n\r\nReginald Hartlepool: [Reginald looks aghast for a moment and almost drops his pants before he seems to realise what you asked for.] Wait... just a sandwich? Very well. I do not have one, but on my honour as a Hartlepool, I will tell you where to procure one in exchange for the paperclip. (/this/data/voice/sammich/5Waitjustasandwich.spx)\r\nComplete One Thousand Year Sammich Step 10. \r\n\r\n... NoRepeat \r\n\r\n# Step 11\r\n\r\nRequire completion of One Thousand Year Sammich Step 10.\r\n\r\nPlayer gives Reginald Hartlepool 1 Red Paperclip.\r\nMenu: Give Reginald your Red Paperclip.\r\n\r\nReginald Hartlepool: [Reginald takes the red paperclip and affixes it to his wayward trousers. He grins as the thing holds.] Wondrous And even a little stylish. Why, red paperclip trouser afixers might just become the new style! Thank you oh so much. Ah, yes, your \'sammich\'. Gertie that you conversed with before coming to me should have more. I sold her quite a few of my best creations. I don\'t know why she didn\'t just give you one in the first place. Here, take a few coins, as I am sure she will not part with such wondrous and savoury feasts cheaply. (/this/data/voice/sammich/6wondrousandeven.spx)\r\nGive 10 Silver Penny.\r\nComplete One Thousand Year Sammich Step 11. \r\n\r\n... NoRepeat \r\n\r\n# Step 12 - talk to the Gertie again.\r\n\r\nRequire completion of One Thousand Year Sammich Step 11.\r\n\r\nP: About sammich.\r\nMenu: Reginald told me you have the sandwiches.\r\n\r\nGertie Hollaback: Ya, so? (/this/data/voice/sammich/9-yeahso.spx)\r\nComplete One Thousand Year Sammich Step 12. \r\n\r\n... NoRepeat \r\n\r\n# Step 13\r\n\r\nRequire completion of One Thousand Year Sammich Step 12.\r\n\r\nP: more about sammich.\r\nMenu: Why didn\'t you tell me you had one?\r\n\r\nG: You asked whar you could larn about one. Ya din\'it ask if I had one. If you asked that, I would\'a said, \'Sure I \'as got one, but it\'ll cost ya five, no, ten silver pennies.\' You got ten silver pennies on you? Hand em over and you got yourself a sammich. (/this/data/voice/sammich/10-youasked.spx)\r\nComplete One Thousand Year Sammich Step 13. \r\n\r\n... NoRepeat \r\n\r\n# Step 14 - End branch.\r\n\r\nRequire completion of One Thousand Year Sammich Step 13.\r\n\r\nPlayer gives Gertie Hollaback 10 Silver Penny.\r\nMenu: Give Gertie the 10 Silver Pennies.\r\n\r\nG: Lemme count these har coins first. One...two...tree...five... [Gertie glances up at you and closes her hand.] Ten. All here. Here is yar sammich. (/this/data/voice/sammich/11-lemmecount.spx)\r\nGive 1 Sublime Sandwich. Give 10000 Exp.\r\nComplete One Thousand Year Sammich Step 14. Complete One Thousand Year Sammich. \r\n\r\n... NoRepeat \r\n\r\n# Step 15 - branch from 7 \"lost something\"\r\n\r\nRequire completion of One Thousand Year Sammich Step 7.\r\n\r\nP: Have paperclip.\r\nMenu: I happen to have this odd little bit of metal; would it help?\r\n\r\nReginald Hartlepool: [Reginald claps his hands together in joy, then quickly grabs at his falling trousers again.] Oh please, if you would be so kind as to give me this wonderful object, I shall, I shall... [Reginald lowers his voice to a whisper.] I shall help you to learn the secret of the Sandwich! (/this/data/voice/sammich/7ohplease.spx)\r\nComplete One Thousand Year Sammich Step 15. \r\n\r\n... NoRepeat \r\n\r\n# Step 16\r\n\r\nRequire completion of One Thousand Year Sammich Step 15.\r\n\r\nPlayer gives Reginald Hartlepool 1 Red Paperclip.\r\nMenu: Give Reginald your Red Paperclip.\r\n\r\nReginald Hartlepool: Alas, I have no children to whom I may pass on the marvellous lore of the Sandwich. Since you have proven yourself a remarkably considerate and compassionate being, I have chosen to pass my knowledge on to you. However, you must learn the lore just as a child of my own getting would be required to do. I hereby present to you that which has been passed down in my family for generations unnumbered. I give you... the Sandwich Plate! [Reginald gingerly places an elderly serving plate in your hands. It is quite chipped and contains four sections, each in a different, and somewhat gaudy, colour.] When you have given this magnificent object the reverence due to it, tell me you are ready and I will impart its sublime knowledge to you. (/this/data/voice/sammich/8alasihavenochildren-1.spx|/this/data/voice/sammich/8alasihavenochildren-2.spx)\r\nGive 1 Sandwich Plate.\r\nComplete One Thousand Year Sammich Step 16. \r\n\r\n... \r\n\r\n# Step 17\r\n\r\nRequire completion of One Thousand Year Sammich Step 16.\r\n\r\nP: Ready.\r\nMenu: I am ready to learn all that you have to teach me. Please begin.\r\n\r\nReginald Hartlepool: [Reginald clears his throat.] Ahem! This was first done by my great great great grandsire, nearly five hundred years ago. Every part of the Sandwich has its proper place during assembly. The bread is placed in the fuchsia section in the centre. The meat must always reside in the cerulean section, just there. Cheese finds its proper place in the ochre section. [Reginald suddenly glares at you.] Remove your filthy thumb from the ochre section AT ONCE! (/this/data/voice/sammich/9ahem.spx)\r\n\r\nComplete One Thousand Year Sammich Step 17. \r\n\r\n... \r\n\r\n# Step 18\r\n\r\nRequire completion of One Thousand Year Sammich Step 17.\r\n\r\nP: removes thumb.\r\nMenu: $playername removes $his thumb quickly under Reginald\'s watchful eye.\r\n\r\nReginald Hartlepool: That\'s better. Now to continue, the last and most important section is the chartreuse section. This section contains the condiment. It is the condiment which binds the Sandwich together into a cohesive whole that is better than the sum of its respective parts. Your next task will be to collect the finest representatives of each of these four ingredients and return to me once you have them. Then and only then will the next mystery be revealed unto you.(/this/data/voice/sammich/10thatsbetter.spx)\r\n\r\nComplete One Thousand Year Sammich Step 18. \r\n\r\n... \r\n\r\n# Step 19\r\n\r\nRequire completion of One Thousand Year Sammich Step 18.\r\n\r\nP: next step.\r\nMenu: What must I do first, oh grand Sandwich Master?\r\n\r\nReginald Hartlepool: I am so glad you asked. Find Butch, the Baker, and get his finest bread. Artisan Waybread only, if you please. Then go to Loudon, the butcher, to procure some of his finest cuts of meat. Corned Beef, nothing else will do. Finally, you will travel to Cabot, purveyor of the finest cheeses. What will you get there? Why, only the finest Smoked Provolone! When you have all these things, you will return to me and I will give you the Final Secret! (/this/data/voice/sammich/11iamsogladyou.spx)\r\n\r\nComplete One Thousand Year Sammich Step 19. \r\n\r\n...  \r\n \r\n\r\n# Step 20\r\n\r\nRequire completion of One Thousand Year Sammich Step 19.\r\n\r\nP: Got bread.\r\nMenu: Reginald sent me for some of your finest bread.\r\n\r\nButch: Ah yes, we have ze fineszt breadz in all ze landz! Come, come, look upon my cre-a-see-onz and name ze bread you need.\r\n\r\nComplete One Thousand Year Sammich Step 20. \r\n\r\n... NoRepeat \r\n\r\n# Step 21\r\n\r\nP: *. P: Artisan Waybread.\r\nMenu: Ummm... cinnamon rolls? Menu: ?=Name your bread\r\n\r\nRequire completion of One Thousand Year Sammich Step 20.\r\n\r\nButch: No no, Rezhinald deed not send you here to buy ZAT!\r\n\r\nRequire completion of One Thousand Year Sammich Step 20.\r\n\r\nButch: Ah Yez! Ze fineszt Arteesan Waybread, she is yours to take. Go quickly, now, so she does not go stale on you.\r\n\r\nGive 1 Artisan Waybread.\r\n\r\nComplete One Thousand Year Sammich Step 21. \r\n\r\n... \r\n\r\n# Step 22\r\n\r\nRequire completion of One Thousand Year Sammich Step 19.\r\n\r\nP: Got meat.\r\nMenu: Got any meat?\r\n\r\nLoudon: That I do! Only the finest meat. What cut would you be looking for today?\r\n\r\nComplete One Thousand Year Sammich Step 22. \r\n\r\n... NoRepeat \r\n\r\n# Step 23\r\n\r\nP: *. P: corned beef.\r\nMenu: Would pickled consumer be good on a sandwich? Menu: ?=Name your meat\r\n\r\nRequire completion of One Thousand Year Sammich Step 22.\r\n\r\nLoudon: What? No no no. that will not do at all.\r\n\r\nRequire completion of One Thousand Year Sammich Step 22.\r\n\r\nLoudon: Ah ha! You must be making one of Reginald\'s famed Sublime Sandwiches. Here are your cuts.\r\n\r\nGive 1 Corned Beef.\r\n\r\nComplete One Thousand Year Sammich Step 23. \r\n\r\n... \r\n\r\n# Step 24 - Last option, breaking the chain up to test the bother of reclicking the npc.\r\n\r\nRequire completion of One Thousand Year Sammich Step 19.\r\n\r\nP: Got cheese.\r\n\r\nMenu: Hello, Cabot. Would you have some fresh cheese for Reginald\'s sandwich?\r\n\r\nCabot: So, it is the famed $playername, come to sample my wares. Well then, I say, No cheese for you! That is, unless you can perfectly name the cheese you wish to have. No.... I want you to spell it.... backwards!\r\n\r\nComplete One Thousand Year Sammich Step 24. \r\n\r\n... NoRepeat \r\n\r\n# Step 25\r\n\r\nP: *. P: Clue. P: enolovorp dekoms.\r\nMenu: Backwards? Are you insane? Menu: Can I have a clue? Menu: ?=Name your cheese\r\n\r\nRequire completion of One Thousand Year Sammich Step 24.\r\n\r\nCabot: You obviously do not understand the importance of the cheese. Come back when you know what you are really searching for.\r\n\r\nRequire completion of One Thousand Year Sammich Step 24.\r\n\r\nCabot: Ha! Too hard for you, is it? Well, I can tell you that the last word is... Dekoms!\r\n\r\nRequire completion of One Thousand Year Sammich Step 24.\r\n\r\nCabot: The Smoked Provolone! Wonderful choice. Let me package that right up for you. [Newman wraps the cheese quickly and hands it to you.]\r\n\r\nGive 1 Smoked Provolone.\r\n\r\nComplete One Thousand Year Sammich Step 25. \r\n\r\n... \r\n\r\n# Step 26.\r\n\r\nRequire completion of One Thousand Year Sammich Step 21.\r\n\r\nRequire completion of One Thousand Year Sammich Step 23.\r\n\r\nRequire completion of One Thousand Year Sammich Step 25.\r\n\r\nP: Have things.\r\nMenu: I have procured all the ingredients.\r\n\r\nReginald Hartlepool: Very good! You are now ready for the final lesson. Give me the ingredients and the Sandwich plate, if you please. (/this/data/voice/sammich/12verygood.spx)\r\n\r\nComplete One Thousand Year Sammich Step 26. \r\n\r\n... NoRepeat \r\n\r\n#Step 27\r\n\r\nRequire completion of One Thousand Year Sammich Step 26.\r\n\r\nPlayer gives Reginald Hartlepool 1 Sandwich Plate, 1 Artisan Waybread, 1 Corned Beef, 1 Smoked Provolone.\r\nMenu: Give Reginald the 3 ingredients you have gathered and the plate.\r\n\r\nReginald Hartlepool: Observe carefully as I put the ingredients in their proper sections on the Sandwich Plate. Now tell me, my good apprentice, which section is empty? (/this/data/voice/sammich/13observecarefully.spx)\r\n\r\nComplete One Thousand Year Sammich Step 27. \r\n\r\n... NoRepeat \r\n\r\n# Step 28\r\n\r\nP: *. P: chartreuse section. chartreuse.\r\nMenu: The one without anything in it? Menu: The chartreuse section appears to be empty.\r\n\r\nRequire completion of One Thousand Year Sammich Step 26.\r\n\r\nReginald Hartlepool: I have no patience for such sarcastic drivel. (/this/data/voice/sammich/14ihavenopatience.spx)\r\n\r\nRequire completion of One Thousand Year Sammich Step 26.\r\n\r\nReginald Hartlepool: Absolutely correct! You have learned well. The Sandwich is nothing without its condiment. Now, pay attention whilst I deliver the tasty denouement to its proper location. [Reginald pulls a small glass jar from his pocket, gingerly removes the cork from it, then dribbles a few drops of dark golden liquid onto the proper section.] The Sandwich is now ready for assembly. Put the condiment on both sides of the bread, then lightly drape the meat across both halves of the bread. Next, delicately place the cheese in a similar fashion. At last, we may unite the two halves and admire the completed Sandwich. [Reginald holds the sandwich aloft for a moment and stares at it with moist eyes.] Words cannot adequately express my pride in your achievements but perhaps these gifts will. They hold your future, young one. (/this/data/voice/sammich/15absolutelycorrect-1.spx|/this/data/voice/sammich/15absolutelycorrect-2.spx)\r\n\r\nGive 1 Sandwich Plate. Give 1 Condiment Recipe.\r\n\r\nComplete One Thousand Year Sammich Step 28. \r\n\r\n... \r\n\r\n# Step 29 Reginald goes Scottish\r\n\r\nRequire completion of One Thousand Year Sammich Step 28.\r\n\r\nP: hungry.\r\nMenu: But I\'m STILL hungry!\r\n\r\nReginald Hartlepool: [Reginald turns beet red and starts sputtering.] Ye ingrate! Ye... ye... pasty-faced git! I gi\' ye all I knoo and ye ken do nowt but whinge! Och, tae it then and away wi\' ye! [Reginald throws the sammich in the general direction of your head.] (/this/data/voice/sammich/16yeingrate.spx)\r\n\r\nGive 1 Sublime Sandwich. Give 40000 Exp.\r\n\r\nComplete One Thousand Year Sammich Step 29.\r\n'),
	('210',209,'P:can I help\r\nMenu: I\'ll solve a riddle for a tria.\r\n\r\nMerchant: Ok that is fun.  I like riddles.  What is my favorite color?\r\n\r\nAssign Quest\r\n\r\n...\r\n\r\nP: *. P: blue\r\nMenu: That isn\'t a riddle, little man.  Menu: ?=Favorite color\r\n\r\nM: I don\'t care what you think of my riddle!  What is my favorite color?\r\nM: Yes, blue is my favorite color.  Isn\'t it a wonderful color?  Here is your reward.\r\nGive 1 tria.\r\n'),
	('211',210,'P:can tell fortune\r\nMenu: I\'ve heard you can read the future.\r\n\r\nSmith: Pay me 5 tria and I\'ll tell you about your love life.\r\n\r\nPlayer gives Smith 5 tria.\r\nMenu: Give him 5 tria for your fortune.\r\n\r\nSmith: Huzzah!  Payment for services has been received... Now let me read your future...\r\n\r\nAssign Quest\r\n\r\n...\r\n\r\nP: tell fortune\r\nMenu: So??? What\'s going to happen in my love life?\r\n\r\nSmith: You are coding a game for goodness sakes! One doesn\'t have to be a psychic to know your love life is going to suffer.  Now go away!\r\n');
/*!40000 ALTER TABLE `quest_scripts` ENABLE KEYS*/;
UNLOCK TABLES;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS*/;
