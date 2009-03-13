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


INSERT INTO quest_scripts VALUES("209","208","# Quest One Thousand Year Sammich 

# Quest Description: You are hungry. Feed yourself.

# Quest Category: 

# Description Red Paperclip - an ordinary red paperclip. It might possibly be traded for amazing and unheard-of wonders.

# Description Sandwich Plate - An ancient chipped plate divided into gaudily coloured sections.

# Description Artisan Waybread - A fluffy warm loaf of freshly-baked bread.

# Description Corned Beef - A packet of tender, juicy, wafer thin meat.

# Description Smoked Provolone - A bundle of creamy white cheese slices that carry a faint smoky aroma.

# Description Sublime Sandwich - The most perfect sandwich you have ever seen, bar none.

# Description Condiment Recipe - An old parchment, badly creased and stained but still legible.

# Description Silver Penny - A unit of currency.

#--------------------------------------------------------------

# Version 1.0

# Author(s): Oomi, UTM

#--------------------------------------------------------------

# Parameters (Remarked out for now)

# player lockout -1

# quest lockout 500

# Prerequisites: none

#--------------------------------------------------------------

# Notes: 

#-------------------------------------------------------------- 
 

P: Give me quest.
Menu: I am hungry for something different. Got any food?

Gertie Hollaback: Yup.

P: rude question. P: Nice question.
Menu: Can I have some? Menu: Well... what kind?

G: No, ya ain\'t gunna get yer grubby mitts on my food. What kinda person comes up to a complete stranger an esks for a bite\'a her well earned grub? Now you git yerself off my premises a\'for I have ta blast you off.
G: [Gertie looks down at her bag, and rummages around a bit.] You like crawdads?

Assign Quest. 

... NoRepeat 

# Step 2

P: No. P: Yes. P: Don't know.
Menu: They are horrid little creatures. Menu: They are delicious. Menu: Do I like what?

G: What? You just ain\'t got no taste. Get on outta here!
Complete One Thousand Year Sammich Step 2. Complete One Thousand Year Sammich.

G: [Gertie rummages around in her bag.] That\'s too bad. I ain\'t got none anyhow.
Complete One Thousand Year Sammich Step 2.

G: How am I suppose to know what you like? Don\' ask me no fool questions.
Complete One Thousand Year Sammich Step 2. 
 

... NoRepeat 

# Step 3

Require completion of One Thousand Year Sammich Step 2.

P: give me food.
Menu: So, where CAN a body get a snack around here?

G: Well, I just had a right tasty sammich. Yup, I did. Thick an\' juicy, one-o\'-a-kind like ya done ne\'er had bee-for. [Gertie stares off into the distance and pulls a folded red wire out of her pocket.]
Complete One Thousand Year Sammich Step 3. 

... NoRepeat 

# Step 4

Require completion of One Thousand Year Sammich Step 3.

P: about sammich.
Menu: Well, that sounds nice. Where do I learn more about this \"sammich\"?

G: [Gertie unfolds the wire and uses it to pick her teeth with great gusto.] Hmmm. Dwarves is real fond o\' they food. Try askin\' one. Oh, here, have a toothpicker. I'm done with it. [Gertie wipes off the red wire, folds it back up, and hands it to you.]
Give 1 Red Paperclip.
Complete One Thousand Year Sammich Step 4. 

... NoRepeat 

# Step 5.

Require completion of One Thousand Year Sammich Step 4.

P: Gertie sent me.
Menu: I\'m starving! Gertie told me you made the best sandwiches around.

Reginald Hartlepool: [Reginald arches an eyebrow.] So, the lowly come to me once again with the desire to sample the fabled festival of sumptuousness that is the pride and heritage of my ancestors? Of course you are. My family's wondrous delectables are legendary in these lands. This astounding recipe has been handed down through generations of Hartlepools, each keeping the secrets of those who came before. And so it has come to me, Reginald HH Hartlepool the Third. As you can see, I have used it wisely. [Reginald grins and pats his protruding belly. As he does so, the button on his pants makes a loud pop and goes flying into the air.]

Complete One Thousand Year Sammich Step 5. 
 

...

...

... NoRepeat 

# Step 6, 7, [8 for lock]. 6 and 7 will make small branches that will merge a little later. 6 will be the harder branch, though it seems to start off nicer.

P: you fat. P: lost something.
Menu: You like your own food a little too much, chubby. Menu: You seem to have lost something.

Require completion of One Thousand Year Sammich Step 5.

Reginald Hartlepool: [Reginald looks down and gasps, grabbing his pants before they fall down. He pats his pockets awkwardly with his free hand as if looking for something, but stops and looks at you in dismay.] Alas... you may be right on that count. It will not be the first time our great Sandwich of sandwiches has brought a Hartlepool to his knees in shame. Woe is upon my family once again as I stand before you, shed of my pride and nearly my pants for the love of the finest sandwich ever constructed. Anguish and pity is upon me as my worthy ancestors look upon this catastrophe that has befallen my unworthy garment fastener. Misery shall become my name, for I have not a single bit of string, nor a bent bit of metal to repair this onerous calamity.

Complete One Thousand Year Sammich Step 6. Complete One Thousand Year Sammich Step 8.

Require completion of One Thousand Year Sammich Step 5.

Reginald Hartlepool: [Reginald looks down and gasps, grabbing his pants before they fall down. He pats his pockets awkwardly with his free hand as if looking for something, but stops and looks at you forlornly.] Oh, good gracious! What an embarrassing predicament. It seems my lust for flavourful delicacies has overcome the fortitude of my slacks, and here I sit without the means to repair the travesty that has become of said garment. If you have something... anything to aid me in this most momentous time of need, I will reward you most graciously.

Complete One Thousand Year Sammich Step 7. Complete One Thousand Year Sammich Step 8. 

... NoRepeat 

# Step 9 - branch from 6. \"You are fat\"

Require completion of One Thousand Year Sammich Step 6.

P: Have paperclip.
Menu: I do have a red paperclip, but it will cost you.

Reginald Hartlepool: I will pay any price you name. I will do anything you ask to save my fallen pride. [Reginald closes his eyes, seeming to brace for the worst.]
Complete One Thousand Year Sammich Step 9. 

... NoRepeat 

# Step 10

Require completion of One Thousand Year Sammich Step 9.

P: sammich.
Menu: It will cost you... a sammich!

Reginald Hartlepool: [Reginald looks aghast for a moment and almost drops his pants before he seems to realise what you asked for.] Wait... just a sandwich? Very well. I do not have one, but on my honour as a Hartlepool, I will tell you where to procure one in exchange for the paperclip.
Complete One Thousand Year Sammich Step 10. 

... NoRepeat 

# Step 11

Require completion of One Thousand Year Sammich Step 10.

Player gives Reginald Hartlepool 1 Red Paperclip.
Menu: Give Reginald your Red Paperclip.

Reginald Hartlepool: [Reginald takes the red paperclip and affixes it to his wayward trousers. He grins as the thing holds.] Wondrous And even a little stylish. Why, red paperclip trouser afixers might just become the new style! Thank you oh so much. Ah, yes, your \'sammich\'. Gertie that you conversed with before coming to me should have more. I sold her quite a few of my best creations. I don\'t know why she didn't just give you one in the first place. Here, take a few coins, as I am sure she will not part with such wondrous and savoury feasts cheaply.
Give 10 Silver Penny.
Complete One Thousand Year Sammich Step 11. 

... NoRepeat 

# Step 12 - talk to the Gertie again.

Require completion of One Thousand Year Sammich Step 11.

P: About sammich.
Menu: Reginald told me you have the sandwiches.

Gertie Hollaback: Ya, so?
Complete One Thousand Year Sammich Step 12. 

... NoRepeat 

# Step 13

Require completion of One Thousand Year Sammich Step 12.

P: more about sammich.
Menu: Why didn\'t you tell me you had one?

G: You asked whar you could larn about one. Ya din\'it ask if I had one. If you asked that, I would\'a said, \'Sure I \'as got one, but it\'ll cost ya five, no, ten silver pennies.\' You got ten silver pennies on you? Hand em over and you got yourself a sammich.
Complete One Thousand Year Sammich Step 13. 

... NoRepeat 

# Step 14 - End branch.

Require completion of One Thousand Year Sammich Step 13.

Player gives Gertie Hollaback 10 Silver Penny.
Menu: Give Gertie the 10 Silver Pennies.

G: Lemme count these har coins first. One...two...tree...five... [Gertie glances up at you and closes her hand.] Ten. All here. Here is yar sammich.
Give 1 Sublime Sandwich. Give 10000 Exp.
Complete One Thousand Year Sammich Step 14. Complete One Thousand Year Sammich. 

... NoRepeat 

# Step 15 - branch from 7 \"lost something\"

Require completion of One Thousand Year Sammich Step 7.

P: Have paperclip.
Menu: I happen to have this odd little bit of metal; would it help?

Reginald Hartlepool: [Reginald claps his hands together in joy, then quickly grabs at his falling trousers again.] Oh please, if you would be so kind as to give me this wonderful object, I shall, I shall... [Reginald lowers his voice to a whisper.] I shall help you to learn the secret of the Sandwich!
Complete One Thousand Year Sammich Step 15. 

... NoRepeat 

# Step 16

Require completion of One Thousand Year Sammich Step 15.

Player gives Reginald Hartlepool 1 Red Paperclip.
Menu: Give Reginald your Red Paperclip.

Reginald Hartlepool: Alas, I have no children to whom I may pass on the marvellous lore of the Sandwich. Since you have proven yourself a remarkably considerate and compassionate being, I have chosen to pass my knowledge on to you. However, you must learn the lore just as a child of my own getting would be required to do. I hereby present to you that which has been passed down in my family for generations unnumbered. I give you... the Sandwich Plate! [Reginald gingerly places an elderly serving plate in your hands. It is quite chipped and contains four sections, each in a different, and somewhat gaudy, colour.] When you have given this magnificent object the reverence due to it, tell me you are ready and I will impart its sublime knowledge to you.
Give 1 Sandwich Plate.
Complete One Thousand Year Sammich Step 16. 

... 

# Step 17

Require completion of One Thousand Year Sammich Step 16.

P: Ready.
Menu: I am ready to learn all that you have to teach me. Please begin.

Reginald Hartlepool: [Reginald clears his throat.] Ahem! This was first done by my great great great grandsire, nearly five hundred years ago. Every part of the Sandwich has its proper place during assembly. The bread is placed in the fuchsia section in the centre. The meat must always reside in the cerulean section, just there. Cheese finds its proper place in the ochre section. [Reginald suddenly glares at you.] Remove your filthy thumb from the ochre section AT ONCE!

Complete One Thousand Year Sammich Step 17. 

... 

# Step 18

Require completion of One Thousand Year Sammich Step 17.

P: removes thumb.
Menu: $name removes $his thumb quickly under Reginald's watchful eye.

Reginald Hartlepool: That\'s better. Now to continue, the last and most important section is the chartreuse section. This section contains the condiment. It is the condiment which binds the Sandwich together into a cohesive whole that is better than the sum of its respective parts. Your next task will be to collect the finest representatives of each of these four ingredients and return to me once you have them. Then and only then will the next mystery be revealed unto you.

Complete One Thousand Year Sammich Step 18. 

... 

# Step 19

Require completion of One Thousand Year Sammich Step 18.

P: next step.
Menu: What must I do first, oh grand Sandwich Master?

Reginald Hartlepool: I am so glad you asked. Find Butch, the Baker, and get his finest bread. Artisan Waybread only, if you please. Then go to Loudon, the butcher, to procure some of his finest cuts of meat. Corned Beef, nothing else will do. Finally, you will travel to Cabot, purveyor of the finest cheeses. What will you get there? Why, only the finest Smoked Provolone! When you have all these things, you will return to me and I will give you the Final Secret!

Complete One Thousand Year Sammich Step 19. 

...  
 

# Step 20

Require completion of One Thousand Year Sammich Step 19.

P: Got bread.
Menu: Reginald sent me for some of your finest bread.

Butch: Ah yes, we have ze fineszt breadz in all ze landz! Come, come, look upon my cre-a-see-onz and name ze bread you need.

Complete One Thousand Year Sammich Step 20. 

... NoRepeat 

# Step 21

P: *. P: Artisan Waybread.
Menu: Ummm... cinnamon rolls? Menu: ?=Name your bread

Require completion of One Thousand Year Sammich Step 20.

Butch: No no, Rezhinald deed not send you here to buy ZAT!

Require completion of One Thousand Year Sammich Step 20.

Butch: Ah Yez! Ze fineszt Arteesan Waybread, she is yours to take. Go quickly, now, so she does not go stale on you.

Give 1 Artisan Waybread.

Complete One Thousand Year Sammich Step 21. 

... 

# Step 22

Require completion of One Thousand Year Sammich Step 19.

P: Got meat.
Menu: Got any meat?

Loudon: That I do! Only the finest meat. What cut would you be looking for today?

Complete One Thousand Year Sammich Step 22. 

... NoRepeat 

# Step 23

P: *. P: corned beef.
Menu: Would pickled consumer be good on a sandwich? Menu: ?=Name your meat

Require completion of One Thousand Year Sammich Step 22.

Loudon: What? No no no. that will not do at all.

Require completion of One Thousand Year Sammich Step 22.

Loudon: Ah ha! You must be making one of Reginald\'s famed Sublime Sandwiches. Here are your cuts.

Give 1 Corned Beef.

Complete One Thousand Year Sammich Step 23. 

... 

# Step 24 - Last option, breaking the chain up to test the bother of reclicking the npc.

Require completion of One Thousand Year Sammich Step 19.

P: Got cheese.

Menu: Hello, Cabot. Would you have some fresh cheese for Reginald\'s sandwich?

Cabot: So, it is the famed $playername, come to sample my wares. Well then, I say, No cheese for you! That is, unless you can perfectly name the cheese you wish to have. No.... I want you to spell it.... backwards!

Complete One Thousand Year Sammich Step 24. 

... NoRepeat 

# Step 25

P: *. P: Clue. P: enolovorp dekoms.
Menu: Backwards? Are you insane? Menu: Can I have a clue? Menu: ?=Name your cheese

Require completion of One Thousand Year Sammich Step 24.

Cabot: You obviously do not understand the importance of the cheese. Come back when you know what you are really searching for.

Require completion of One Thousand Year Sammich Step 24.

Cabot: Ha! Too hard for you, is it? Well, I can tell you that the last word is... Dekoms!

Require completion of One Thousand Year Sammich Step 24.

Cabot: The Smoked Provolone! Wonderful choice. Let me package that right up for you. [Newman wraps the cheese quickly and hands it to you.]

Give 1 Smoked Provolone.

Complete One Thousand Year Sammich Step 25. 

... 

# Step 26.

Require completion of One Thousand Year Sammich Step 21.

Require completion of One Thousand Year Sammich Step 23.

Require completion of One Thousand Year Sammich Step 25.

P: Have things.
Menu: I have procured all the ingredients.

Reginald Hartlepool: Very good! You are now ready for the final lesson. Give me the ingredients and the Sandwich plate, if you please.

Complete One Thousand Year Sammich Step 26. 

... NoRepeat 

#Step 27

Require completion of One Thousand Year Sammich Step 26.

Player gives Reginald Hartlepool 1 Sandwich Plate, 1 Artisan Waybread, 1 Corned Beef, 1 Smoked Provolone.
Menu: Give Reginald the 3 ingredients you have gathered and the plate.

Reginald Hartlepool: Observe carefully as I put the ingredients in their proper sections on the Sandwich Plate. Now tell me, my good apprentice, which section is empty?

Complete One Thousand Year Sammich Step 27. 

... NoRepeat 

# Step 28

P: *. P: chartreuse section. chartreuse.
Menu: The one without anything in it? Menu: The chartreuse section appears to be empty.

Require completion of One Thousand Year Sammich Step 26.

Reginald Hartlepool: I have no patience for such sarcastic drivel.

Require completion of One Thousand Year Sammich Step 26.

Reginald Hartlepool: Absolutely correct! You have learned well. The Sandwich is nothing without its condiment. Now, pay attention whilst I deliver the tasty denouement to its proper location. [Reginald pulls a small glass jar from his pocket, gingerly removes the cork from it, then dribbles a few drops of dark golden liquid onto the proper section.] The Sandwich is now ready for assembly. Put the condiment on both sides of the bread, then lightly drape the meat across both halves of the bread. Next, delicately place the cheese in a similar fashion. At last, we may unite the two halves and admire the completed Sandwich. [Reginald holds the sandwich aloft for a moment and stares at it with moist eyes.] Words cannot adequately express my pride in your achievements but perhaps these gifts will. They hold your future, young one.

Give 1 Sandwich Plate. Give 1 Condiment Recipe.

Complete One Thousand Year Sammich Step 28. 

... 

# Step 29 Reginald goes Scottish

Require completion of One Thousand Year Sammich Step 28.

P: hungry.
Menu: But I'm STILL hungry!

Reginald Hartlepool: [Reginald turns beet red and starts sputtering.] Ye ingrate! Ye... ye... pasty-faced git! I gi\' ye all I knoo and ye ken do nowt but whinge! Och, tae it then and away wi\' ye! [Reginald throws the sammich in the general direction of your head.]

Give 1 Sublime Sandwich. Give 40000 Exp.

Complete One Thousand Year Sammich Step 29.");


#
# Quest to test riddle prompts
#
INSERT INTO quest_scripts VALUES("210","209","P:can I help
Menu: I\'ll solve a riddle for a tria.

Merchant: Ok that is fun.  I like riddles.  What is my favorite color?

Assign Quest

...

P: *. P: blue
Menu: That isn\'t a riddle, little man.  Menu: ?=Favorite color

M: I don\'t care what you think of my riddle!  What is my favorite color?
M: Yes blue is my favorite color!  Isn\'t it a wonderful color?  Here is your reward.
Give 1 tria.
");
