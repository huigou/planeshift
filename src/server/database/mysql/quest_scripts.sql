# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'quest_scripts'
#

CREATE TABLE quest_scripts (
  id int(10) unsigned NOT NULL auto_increment,
  `quest_id` int(10) NOT NULL default '0' COMMENT 'FK to quests table, or -1 for KA scripts.',
  `script` blob COMMENT 'The script for the quest, parsed by questmanager.cpp',
  PRIMARY KEY (id)
);


#
# Dumping data for table 'quest_scripts'
#

#
# Quest: "Rescue the Princess"
#
INSERT INTO quest_scripts VALUES("1","1","P:give me quest
Menu: Is there anyone you need saving?

Merchant: Congratulations! You get to save the princess now.[Merchant bows to $playername.]I wish you luck.[As he finishes talking, you notice clouds darkening on the horizon...]

Assign Quest

...

P: done
Menu: Ok she is safe on Yavin 4 now.

M: OMG you did it!  I can\'t believe it.

Give Mug or Steel Falchion or Claymore

");

#
# Quest: "Sandwich Quest"
#
INSERT INTO quest_scripts VALUES("2","2","P: can bring you sandwich
Menu: Is there anything I can bring you?

Merchant: I'm kinda hungry.. bring me a sandwich and hurry if you want a hexa and a mug for your trouble.(/planeshift/data/voice/merchant/clawatit.wav)

Assign Quest

...

P: cannot find. P: here you
Menu: Sorry but I can't find any sandwiches.  Menu: Here is a nice sandwich for you, good sir.

M: Well sandwiches are rare in these parts.  Thanks anyway.

M: Many thanks, this help for my hunger.(/planeshift/data/voice/merchant/thank_you.wav)

Give 1 hexa. Give Mug. Give 3 Potion of Healing. Give 5 faction merchants.
");

#
# Quest: "Falchion Quest"
#
INSERT INTO quest_scripts VALUES("3","3","P: can bring you falchion
Menu: Is there anything you need?

Merchant: Kill the enkidukai who wanders in the forest and bring me his steel falchion.

Assign Quest

...

Player gives Merchant Steel Falchion
Menu: Give the merchant a Steel Falchion.

M: Many thanks, brave stranger.  This will come in handy if the fans misbehave.

Give 1 octa

");


#
# Quest: "Male Enki Alina Quest"
#
INSERT INTO quest_scripts VALUES("4","10","P: greetings

maleEnki: Hail!  Would you like to earn a little money?

P: No   P: Yes
Menu: I am not interested in your chores old man!  Menu: Cool yeah, I am totally broke.

M: Well ok then.  Have a good day.

M: My daughter Alina ran off with the smith and I fear they are up to no good. If you can find out if he really loves her, I'll pay you.  Can you do this for me?

P: No P: Yes
Menu: You will never find her.  Menu: $name shrugs $his shoulders and nods.

M: Fair enough.  I guess I'll just try to find someone else.

M: Last time I caught them upstairs above the blacksmith shop.  Please hurry up and ask her!

Assign Quest.


P: Greetings

Smith: Go away!  We're busy.{her:Alina}

P: *  P: Do you love Alina

S: You are so rude!  Go away!
Run script(3) kick_you

S: Of course I love her!  We are going to marry in the morning.



P: Greetings

MaleEnki: Oh I'm so glad you are back.  Did you find him?  Does he love her?{him:Smith,her:Alina}

P: *   P: Smith says Smith does. Smith love Alina.

M: That isn't what Thalia told me 5 minutes ago.  You're just faking to get
   the money!  Go away!

M: Ah! Thank heavens.  Perhaps now they will get married!  Thanks for your
   help!

Give 25 tria.
");

#
# Quest: "Male Enki Gold"
#
INSERT INTO quest_scripts VALUES("5","11","P: greetings

MaleEnki: Hello there.  I'm looking for gold ore... But I need someone I can trust...

P: No. P: Yes.
Menu: Well then I am not your guy because I am chaotic neutral.  Menu: You can trust your instincts with me.

M: Chaotic neutral characters are a pain but ok, thanks for telling me.  Bye.

M: Yes, I think I will trust my instincts.  Can you find gold for me?  I'll gladly pay you a pittance.

P: No.  P: Yes.
Menu: I am a war hero, not a grubby miner.  Menu: I am a $race!  I love pittances!  $name will start right away!

M: Ok I guess I'll just try to find someone else to help me.

M: Excellent.  If you will bring me 5 gold ores, I will reward you.  Now go!

Assign Quest.

...

Player gives MaleEnki 5 Gold Ore.

M: Ah these are just what I needed!  Thanks so much! (How generous of you to just give me gold.  Now go away!)

Give Small Battle Axe.
");

#
# Quest: "Male Enki Trusted Transport"
#
INSERT INTO quest_scripts VALUES("6","12","P: Greetings

MaleEnki: Hello $sir my good friend! Can I trust you to carry out one more thing for me?(I have nothing to say to you!)

P: No P: Yes

M: I will not bother you anymore

M: Greate! I have a glyph that I have agreed to exchange with a special sword that the Smith have made for me.
   Can you take this to him and bring back the sword?

P: No P: Yes

M: Come by some other time, when you have time

M: Excellent. I will reward you when you return

Assign Quest

Give Faith

...

P: Bring glyph from MaleEnki

Smith: I have been waiting for this. Give me the glyph and I give you the sword.

Player gives Smith Faith

Smith: Here you go

Give Gold Falchion. Complete Male Enki Trusted Transport Step 2

...

Player gives Smith Faith

Smith: A the glyph from the MaleEnki

Give Gold Falchion. Complete Male Enki Trusted Transport Step 2

...

Require completion of Male Enki Trusted Transport Step 2

Player gives MaleEnki Gold Falchion

MaleEnki: Thank you my friend

Give 2 octa
");

#
# Action Location Entrance test.
#
INSERT INTO `quest_scripts` VALUES ("8","13","P: Lost.

MaleEnki:  Do you want a key?

P: No   P: Yes
M: Well ok then.
M: Come back here to get key later!

Assign Quest.

P: Found

MaleEnki: Back again?

P: No   P: Yes
M: Bye.
M: Here is your key!

Run script get_next_entrance

Give 25 tria.");

#
# Active magic tests
#
INSERT INTO `quest_scripts` VALUES ("14","14","P: Windy.

MaleEnki: Windy it is indeed!  Ask me for a kite.

Assign Quest.

P: Kite.
M: Oh no, looks like rain!  Too bad.  Here, have a rain check for the kite.

Run script rain

Give 25 tria.");

INSERT INTO quest_scripts VALUES ("15","15","P: give me quest.
Smith: Would you like a quest?
P: No. P: Yes.
S: Oh. That's not helpful.
S: Great. What you'll need to do is tell me 'I want to test' and then I'll ask you for a tria. Try it by saying 'I want to test' first, then 'test' the next time. It should work both ways. Let's see if it does.
Assign Quest.
... NoRepeat
P: test.
Smith: Great. Give me 1 tria, please.
Player gives Smith 1 Tria.
Smith: Thanks for testing. Here's your tria back.
Give 1 Tria.");

#
# Multiple items test.
#
INSERT INTO `quest_scripts` VALUES ("16","16","P: Hungry.

MaleEnki: Starving, aye?  Bring me 5 eggs, 1 thing of milk and I can fix up something...for 1 hexa.

Assign Quest.

Player gives MaleEnki 1 Milk, 1 Hexa, 5 Egg.
MaleEnki: Here's some freshly baked Waybread!  Enjoy...

Give 1 Waybread.");

#
# Quest Test Case Quests
#
INSERT INTO quest_scripts VALUES("101","101","P: step1

QuestMaster1: Yes, this is step1. [Type 'step3', verify error response. Type 'step4', verify error response. Type 'step2']

Assign Quest.
...
...
...
P: step two. step2.

Q: Yes, this is step2. [You have now completed step2. Type 'step2',verify quest substep can be restarted. Type 'step3' or 'step3b' if on second run.]

Complete QuestMaster1 Quest 1 Step 2.

...

Require completion of QuestMaster1 Quest 1 Step 2.

P: step3. step three

Q: Yes this is step3. Answer Yes or No. [First run type 'yes', Second run type 'no']

P: No. P: Yes

Q: This is the answer to No.[Type 'step2']

Complete QuestMaster1 Quest 1 Step 3. Complete QuestMaster1 Quest 1 Step 4. Give 3 Tria.
Give 300 Exp.

Q: This is the answer to Yes.[Type 'step3b' verify no execution of other path. Type 'step5']

Give 4 Tria. Complete QuestMaster1 Quest 1 Step 3. Complete QuestMaster1 Quest 1 Step 5.

...

Require completion of QuestMaster1 Quest 1 Step 4.

P: step3b

Q: Yes this is step3b. You can execute this only if you previously answered No. [Type 'step4']

Complete QuestMaster1 Quest 1 Step 6

...

Require completion of QuestMaster1 Quest 1 Step 6

P: step4.

Q: Yes, this is step4. This is the ending if you said No. [Type 'step4', verify error response. Type 'step2', verify error response. To restart the quest type 'step1']
Complete QuestMaster1 Quest 1.
...

Require completion of QuestMaster1 Quest 1 Step 5

P: step5.

Q: Yes, this is step5. This is the ending if you said Yes. [Type 'step4', verify error response. Type 'step2', verify error response. To restart the quest type 'step1']
");

INSERT INTO quest_scripts VALUES("102","102","P: quest2

QuestMaster1: Yes, this is QuestMaster1 Quest2. [Type 'done']

Assign Quest.

...

P: done.

Q: Your done. [First run: Type 'quest2', within 30 sec to verify no quest assigned. Type 'quest2' after 30 sec verify 'QuestMaster1 Quest 2' assigned again. Second run: Type 'quest3']
");


INSERT INTO quest_scripts VALUES("104","104","P: quest4

QuestMaster1: Yes, this is Quest4. Please give me 3 Batter.

Assign Quest.

...

Player gives QuestMaster1 3 Batter.

Q: Good thanks! Now give me 5 Trias.

Player gives QuestMaster1 5 Tria.

Q: Thanks for the trias! You are done.

");


INSERT INTO quest_scripts VALUES("103","103","P: quest3

QuestMaster1: Yes, this is QuestMaster1 Quest3. [Type 'done']

Assign Quest.

...

P: done.

Q: Your done. [First run: Type 'quest3', within 30 sec of assignment to verify no quest assigned. Type 'quest2' after 30 sec avter assignemnt, verify 'QuestMaster1 Quest 2' assigned again. Second run: Do this from another client.]
");


INSERT INTO quest_scripts VALUES("201","201","P: quest1

QuestMaster2: Yes, this is QuestMaster2 Quest1. [Type 'done']

Assign Quest.

...

P: done.

Q: Your done. [Type 'quest4',verify no quest assigned. Type 'quest2' verify 'QuestMaster1 Quest 2' assigned.]
");

INSERT INTO quest_scripts VALUES("202","202","P: quest2

QuestMaster2: Yes, this is QuestMaster2 Quest2. [Type 'done']

Assign Quest.

...

P: done.

Q: Your done. [Type 'quest5' verify no quest assigned. Type 'quest3',verify 'QuestMaster1 Quest 3' assigned.]
");

INSERT INTO quest_scripts VALUES("203","203","P: quest3

QuestMaster2: Yes, this is QuestMaster2 Quest3. [Type 'done']

Assign Quest.

...

P: done.

Q: Your done. [Type 'quest4',verify 'QuestMaster1 Quest 4' assigned.]
");

INSERT INTO quest_scripts VALUES (204,204,"P: quest4

QuestMaster2: Yes, this is QuestMaster2 Quest4.

Assign Quest.

... NoRepeat

P: step2

QuestMaster2: Yes, this is step2, it's not repeatable.

P: step2b

QuestMaster2: Yes, this is step2b, it's not repeatable.

Complete QuestMaster2 Quest 4 Step 2.

... NoRepeat
Require completion of QuestMaster2 Quest 4 Step 2.
P: step3
QuestMaster2: Yes, this is step3,  it's not repeatable.
Complete QuestMaster2 Quest 4 Step 3.
...

P: done.

Q: Your done. [Type 'quest5',verify 'QuestMaster1 Quest 5' assigned.]
");

INSERT INTO quest_scripts VALUES("205","205","P: quest5

QuestMaster2: Yes, this is QuestMaster2 Quest5. [Type 'done']

Assign Quest.

...

P: done.

Q: Your done.
");

INSERT INTO quest_scripts VALUES("206","206","P: quest6

QuestMaster2: Yes, this is QuestMaster2 Quest6. [Type 'done']

Assign Quest.

...

P: done.

Q: Your done[You should now never be able to start quest6 again].
");

#
# KA test. Use -1 for quest_id for KAs.
#
INSERT INTO `quest_scripts` VALUES (207,-1,"P: ring
general: Rings are interesting because they often have magical powers.  You should definitely keep them if you find any.
...
P: talad
g: He is best described as drunk in love.
...
P: laanx
g: Not sure if she is a he or a she, but s/he likes very spiky temples apparently.");




INSERT INTO quest_scripts VALUES("208","207","# Quest Acquire Lapar
# ID 1214
# Quest Description: Smith needs some juiceberry fruit before he will sell you any lapar.
# Player lockout: 43200
# 12 hours 
# Quest Lockout: 1800
# 30 minutes 
# Syntax update 5.11.2008
# Updated with menu system 11/2008.

P: Give me Lapar.
Menu: I\'m looking for some Lapar.

Smith: Very well $sir, I have some lapar you can have, but it will cost you. [Levrus chuckles.] Shall we say two hundred tria, and three juiceberry fruit? (/planeshift/data/voice/Levrusacquirelapar/1verywellsir.spx)
P: No. P: Yes.
Menu: No, that is too much!  Are you kidding me old man?  Menu: You hold your lapar dear old man, but I\'ll pay your price.

Smith: That is my price $playerrace, come back if you change your mind. (/planeshift/data/voice/Levrusacquirelapar/2thatismyprice.spx)
Smith: Excellent! Bring me the fruit, and I\'ll have some lapar ready for you. (/planeshift/data/voice/Levrusacquirelapar/3excellentbringmethefruit.spx)
Assign Quest.

... NoRepeat
# Step 2
Player Gives Smith 3 Juiceberry fruit.
Smith: Very nice, now for the tria, all two hundred of them please. [Levrus holds out his hand with a smile.] (/planeshift/data/voice/Levrusacquirelapar/4verynicenowforthetria.spx)
Complete Acquire Lapar Step 2.

...
# Step 3
Require Completion of Acquire Lapar Step 2.
Player Gives Smith 200 tria.
Smith: [The Smith takes the coins and stuffs them into his coin-pouch.] Very well, one bottle of Lapar for the $playerrace. [Smith hands you a bottle.] Use it wisely, it is not as abundant as water. [Smith chuckles as he turns his attention elsewhere.] (/planeshift/data/voice/Levrusacquirelapar/5verywellonebottle.spx)
Give 1 Lapar. Give 600 Exp.
Complete Acquire Lapar Step 3.");
