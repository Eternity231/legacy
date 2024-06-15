#pragma once
#include "includes.h"

namespace anims {
	void copy_client_layers( Player* ent, std::array<C_AnimationLayer, 13>& to, std::array<C_AnimationLayer, 13>& from );
	vec3_t get_server_shoot_position( const vec3_t& wanted_view_angles );

	/* anim update funcs */
	void do_animation_event( CCSGOPlayerAnimState* anim_state, int id, int data );
	void trigger_animation_events( CCSGOPlayerAnimState* anim_state );

	void set_sequence( CCSGOPlayerAnimState* anim_state, int layer, int sequence );

}