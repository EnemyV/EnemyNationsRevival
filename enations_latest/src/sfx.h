#ifndef __SFX_H__
#define __SFX_H__

#include <music.h>
#include "lastplnt.h"

class SFXGROUP {
public:
		enum {
			empty,				// 0 is null
			global,				// used everywhere
			music,				// all buffered music -- non game play
			play,					// playing the game
			music_play,		// digital audio for game play
			cut_scenes,		// cut scenes
			num_groups
			};
};

class SOUNDS {
public:

		enum {
			silence,			// silence
			button,				// button click
			trees,				// terrain sounds
			river,
			ocean,
			swamp,
			const1,				// building foundation
			const2,				// skeleton
			const3,				// shell
			damage3,			// damage level 3
			damage4,			// damage level 4

			// building sounds
			bld_base,
			rocket = bld_base,
			apartment,
			office,
			barracks,
			command,
			embassy,
			farm,
			factory,
			lumber,
			oil_well,
			refinery,
			iron_mine,
			mine,
			power,
			research,
			repair,
			seaport,
			smelter,
			warehouse,
			factory_quiet,
			refinery_quiet,
			shipyard_quiet,

			// vehicle sounds
			veh_idle_base,
			crane_idle = veh_idle_base,
			truck_idle,
			motorcycle_idle,
			tank_idle,
			ship_idle,
			troops_idle,

			veh_go_base,
			crane_go = veh_go_base,
			truck_go,
			motorcycle_go,
			tank_go,
			ship_go,
			troops_go,

			veh_running_base,
			crane_running = veh_running_base,
			truck_running,
			motorcycle_running,
			tank_running,
			ship_running,
			troops_running,

			shoot_base,			// shoot_0
			shoot_1,
			shoot_2,
			shoot_3,
			shoot_4,
			shoot_5,
			shoot_6,
			shoot_7,
			shoot_8,
			shoot_9,
			shoot_10,
			shoot_11,
			shoot_12,
			shoot_13,
			shoot_14,
			shoot_15,
			shoot_16,
			shoot_17,
			shoot_18,
			shoot_19,
			shoot_20,
			shoot_21,
			shoot_22,
			shoot_23,

			explosion_base,	// explosion_0
			explosion_1,
			explosion_2,
			explosion_3,
			explosion_4,

			rocket_landing,
			player_join,

			sounds_end
			};

		static int		GetID ( int id ) { return id; }
};

class VOICES {
public:

		enum {
			first_voice = SOUNDS::sounds_end,

			sys_out_mat = first_voice,
			sys_under_atk,
			sys_low_pop,
			sys_low_gas,
			sys_low_food,
			sys_low_power,
			sys_out_gas,
			sys_out_food,
			sys_low_apt,
			sys_low_ofc,
			sys_declare_war,
			sys_cant_bridge,
			sys_have_radar,
			sys_mine_out,

			rocket_cant_land,

			first_first_voice,

			com_sel_first = first_first_voice,
			com_sel_2,
			com_sel_last,
			com_ok_first,
			com_ok_2,
			com_ok_3,
			com_ok_last,
			com_stuck,
			com_obj_atk,
			com_obj_des,

			bld_enemy_close,
			bld_heavy_damage,
			bld_almost_dead,

			sci_first,
			sci_last,

			rec_mail,
			rec_call,

			tem_sel_first,
			tem_sel_last,
			tem_const_start,
			tem_bldg_comp,
			tem_bldg_rep,
			tem_road_comp,
			tem_cant_build,
			tem_under_atk,
			tem_where_build,

			fac_sel_first,
			fac_sel_last,
			fac_veh_comp,

			last_first_voice = fac_veh_comp,

			cut_first,
			cut_2,
			cut_3,
			cut_4,
			cut_5,
			cut_6,
			cut_7,
			cut_8,
			cut_9,
			cut_10,
			cut_11,
			cut_12,
			cut_last,

			// second voice
			first_second_voice,
			num_second_voice = last_first_voice - first_first_voice + 1,
			last_second_voice = first_second_voice + num_second_voice - 1,

			voices_end,

			num_com_sel = com_sel_last - com_sel_first + 1,
			num_com_ok = com_ok_last - com_ok_first + 1,
			num_sci = sci_last - sci_first + 1,
			num_tem_sel = tem_sel_last - tem_sel_first + 1,
			num_fac_sel = fac_sel_last - fac_sel_first + 1,
			};

		static int		GetID ( int id, int iVoice )
											{ if ( ( id < first_first_voice ) || ( last_first_voice < id ) )
													return id;
												if ( iVoice != 0 )
													return id + VOICES::first_second_voice - VOICES::first_first_voice;
												return id;
											}
};

class MUSIC {
public:

		enum {
			first_music = VOICES::voices_end,

			main_screen = first_music,
			create_game,
			win_game,
			loose_game,
			credits,

			play_game,
			num_play_game = 5
			};

		static int		GetID ( int id ) 
											{ if ( theApp.HaveMultVoices () ) 
													return id; 
												return id - VOICES::num_second_voice; 
											}
};


class SFXPRIORITY
{
public:
		enum { 	
			terrain_pri = 600, 
			const_pri = 500, 
			inc_pri = BACKGROUND_PRI,
			goto_pri = 300, 
			damage_pri = 200,
			selected_pri = 100, 
			voice_pri = 50 };
};


#endif
