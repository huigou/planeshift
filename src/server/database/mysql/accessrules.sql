#
# Table structure for table 'accessrules'
#
# objecttype: 'npc', 'quest', 'adminserv'
# fieldname : Empty or one field present in the object table
# fieldvalue : Empty or the value of the field
# read, edit, delete: 'admin', 'leader_any', 'leader_dept', 'member_any', 'member_dept', 'all', 'owner'
# where dept can be : eng, set, rul, gra, mus, pr
#
# on ps database accounts.security_level determines type of access:
#   admin          = 50
#   leader_pr      = 46
#   leader_music   = 45
#   leader_graphic = 44
#   leader_rules   = 43
#   leader_setting = 42
#   leader_engine  = 41
#   leader_any     = 40  (not assigned to a user, used only by access rules)
#   member_pr      = 36
#   member_music   = 35
#   member_graphic = 34
#   member_rules   = 33
#   member_setting = 32
#   member_engine  = 31
#   member_any     = 30  (not assigned to a user, used only by access rules)
#   gm             = 20
#   player         = 0
#
# Current list of gm powers based on level
# Level 29: /death , /npc
# Level 28: /killnpc
# Level 27: /ban , /unban , /spawn_item
# Level 24: /invisible , /visible , /crystal , /item
# Level 22: /kick , /changename
# Level 21: /teleport_to , /slide , /slide_me , /mute , /unmute
# 
#
# Access is denied by default, unless there is a rule that allows access.
# Example: 31 is not higher access than 30. The most significant digit changes access class, the second is department.
# An access level higher than the one specified by the rule allows the action.
# The most specific access wins.
#      Example: if you have a rule "quest" "all" "all" "20" and another "quest" "name" "Leila Rescue" "30"
#      The second rule is more specific because the scope is the name of the quest.
#      If someone with level 30 connects he will be able to see all quests,
#      If someone with level 20 connects he will be able to see all quests except "Leila Rescue"
#

DROP TABLE IF EXISTS `accessrules`;
CREATE TABLE accessrules (
  objecttype varchar(50),
  fieldname varchar(50),
  fieldvalue varchar(50),
  a_read varchar(20),
  a_create varchar(20),
  a_edit varchar(20),
  a_delete varchar(20)
);


#
# Dumping data for table 'accessrules'
#
INSERT INTO accessrules VALUES ('quest','','','30','31','31','50');
INSERT INTO accessrules VALUES ('quest','name','Leila Rescue','41','41','','50');
INSERT INTO accessrules VALUES ('npc','','','30','30','30','50');
INSERT INTO accessrules VALUES ('main','','','30','30','30','50');
INSERT INTO accessrules VALUES ('listnpc','','','30','30','30','50');

