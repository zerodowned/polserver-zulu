/*
History
=======
2005/02/23 Shinigami: ServSpecOpt DecayItems to enable/disable item decay
2005/03/05 Shinigami: ServSpecOpt UseAAnTileFlags to enable/disable "a"/"an" via Tiles.cfg in formatted Item Names
2005/06/15 Shinigami: ServSpecOpt UseWinLFH to enable/disable Windows XP/2003 low-fragmentation Heap
2005/08/29 Shinigami: ServSpecOpt UseAAnTileFlags renamed to UseTileFlagPrefix
2005/12/05 MuadDib:   ServSpecOpt InvulTage using 0, 1, 2 for method of invul showing.
2009/07/31 Turley:    ServSpecOpt ResetSwingOnTurn=true/false Should SwingTimer be reset with projectile weapon on facing change
                      ServSpecOpt SendSwingPacket=true/false Should packet 0x2F be send on swing.
2009/09/03 MuadDib:   Moved combat related settings to Combat Config.
2009/09/09 Turley:    ServSpecOpt CarryingCapacityMod as * modifier for Character::carrying_capacity()
2009/10/12 Turley:    whisper/yell/say-range ssopt definition
2009/11/19 Turley:    ssopt.core_sends_season & .core_handled_tags - Tomi
2009/12/02 Turley:    added ssopt.support_faces

Notes
=======

*/

#ifndef SSOPT_H
#define SSOPT_H

#include <string>

struct ServSpecOpt {
    bool allow_secure_trading_in_warmode;
	unsigned long dblclick_wait;
    bool decay_items;
	unsigned long default_decay_time;
    unsigned short default_doubleclick_range;
    unsigned short default_light_level;
	bool event_visibility_core_checks;
	unsigned long max_pathfind_range;
    bool movement_uses_stamina;
    bool use_tile_flag_prefix;
	unsigned short default_container_max_items;
	unsigned short default_container_max_weight;
	bool hidden_turns_count;
    unsigned short invul_tag;
	unsigned short uo_feature_enable;
	unsigned short starting_gold;
	unsigned short item_color_mask;
	unsigned char default_max_slots;
    bool use_win_lfh;
	bool privacy_paperdoll;
	bool force_new_objcache_packets;
	bool allow_moving_trade;
	bool core_handled_locks;
	bool use_slot_index;
	double carrying_capacity_mod;
	bool use_edit_server;

	bool mobiles_block_npc_movement;

	unsigned short default_attribute_cap;
	bool core_sends_caps;
	bool send_stat_locks;

    unsigned short speech_range;
	unsigned short whisper_range;
	unsigned short yell_range;

	bool core_sends_season;
	unsigned short core_handled_tags;
    unsigned char support_faces;

	std::vector<std::string> total_stats_at_creation;
};

extern ServSpecOpt ssopt;

void read_servspecopt();
void ssopt_parse_totalstats(ConfigElem& elem);

#endif
