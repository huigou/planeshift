
# You can use this to check if there are any dependencies on other tables before deleting a monster

set @npcid=100024;
select count(*) from character_traits where character_id=@npcid;
select count(*) from item_instances where char_id_owner=@npcid;
select count(*) from character_skills where character_id=@npcid;
select count(*) from npc_knowledge_areas where player_id=@npc_id;
select count(*) from merchant_item_categories where player_id=@npc_id;

# Others tables you may be interested in checking for sentient NPCs are:

# npc_triggers where area=@npc_name;
# npc_bad_text where npc=@npc_name;
# npc_knowledge_areas where player_id=@npc_id;
# npc_responses where id>=@min_resp and id<=@max_resp;
