#include "rebuilt_animations.hpp"
#include "vfunc.hpp"
#include <cmath>

/* rebuilded from skeet*/

void anims::copy_client_layers( Player* ent, std::array<C_AnimationLayer, 13>& to, std::array<C_AnimationLayer, 13>& from ) { 
	const auto layers = ent->layers( );

	for ( auto i = 0; i < to.size( ); i++ ) {
		if ( i == 3 || i == 4 || i == 5 || i == 6 || i == 9 ) {
			to[ i ].m_sequence = from[ i ].m_sequence;
			to[ i ].m_prev_cycle = from[ i ].m_prev_cycle;
			to[ i ].m_weight = from[ i ].m_weight;
			to[ i ].m_weight_delta_rate = from[ i ].m_weight_delta_rate;
			to[ i ].m_playback_rate = from[ i ].m_playback_rate;
			to[ i ].m_cycle = from[ i ].m_cycle;
		}
	}
}

void modify_eye_position( vec3_t& eye_position, float head_position_z ) {
	if ( !g_cl.m_local->m_PlayerAnimState()->m_land
		 && g_cl.m_local->m_flDuckAmount() == 0.0f
		 && g_cl.m_local->m_PlayerAnimState()->m_player->GetGroundEntity( ) )
		return;

	head_position_z += 1.7f;

	if ( eye_position.z <= head_position_z )
		return;

	const float delta = eye_position.z - head_position_z;

	float some_factor = 0.0f;
	float some_offset = ( delta - 4.0f ) / 6.0f;

	if ( some_offset >= 0.0f )
		some_factor = std::min( some_offset, 1.0f );

	eye_position.z += ( ( head_position_z - eye_position.z ) * ( ( ( some_factor * some_factor ) * 3.0f ) - ( ( ( some_factor * some_factor ) * 2.0f ) * some_factor ) ) );
}

constexpr inline float lerp( const float _ArgA, const float _ArgB, const float _ArgT ) noexcept {
	return _ArgA + ( _ArgB - _ArgA ) * _ArgT;
}

float head_height_from_origin_down_pitch = 0.0f;
float get_predicted_head_z( vec3_t view_angle ) {
	// Get estimated head position when facing downwards
	const auto predicted_head_position_down = g_cl.m_local->GetAbsOrigin( ).z + head_height_from_origin_down_pitch;

	// Compensate pitch change for recoil
	view_angle -= (vec3_t&)g_cl.m_local->m_aimPunchAngle( ) * 2.0f; // should work 

	math::clamp_skeet( view_angle );

	// from Desmos quadratic regression
	// pitch to head offset from down relation
	// 
	// Estimate head offset from pitch
	// Use doubles for more accuracy calculation,
	//	because we are dealing with multiplication of very small numbers here
	double pitch = static_cast< double >( view_angle.x );

	double head_offset_from_down_stand_still = ( -0.000454846 * ( pitch * pitch ) ) + ( -0.0451027 * pitch ) + 7.50845;
	double head_offset_from_down_moving = ( -0.000208786 * ( pitch * pitch ) ) + ( -0.0183211 * pitch ) + 3.26285;
	double head_offset_from_down_crouching = ( -0.000257951 * ( pitch * pitch ) ) + ( -0.0394547 * pitch ) + 5.10114;
	double head_offset_from_down_crouching_moving = ( 0.0000570523 * ( pitch * pitch ) ) + ( -0.0527598 * pitch ) + 3.34832;

	const float max_speed = g_cl.m_weapon->max_speed( );
	const float max_head_height_speed = lerp( max_speed * 0.34f, max_speed * 0.52f, g_cl.m_local->m_flDuckAmount( ) );
	const float run_speed_fraction = std::clamp( g_cl.m_local->m_vecVelocity().length_2d( ), 0.0f, max_head_height_speed ) / max_head_height_speed;

	//auto& records = game::lagcomp::get_track ( local_player );
	//
	//if ( !records.empty ( ) )
	//	run_speed_fraction = records.back ( ).animstates [ game::lagcomp::DesyncSide::Middle ].speed_to_walk_fraction;

	const float head_offset_from_down_still_lerped = lerp( static_cast< float >( head_offset_from_down_stand_still ), static_cast< float >( head_offset_from_down_crouching ), g_cl.m_local->m_flDuckAmount( ) );
	const float head_offset_from_down_moving_lerped = lerp( static_cast< float >( head_offset_from_down_moving ), static_cast< float >( head_offset_from_down_crouching_moving ), g_cl.m_local->m_flDuckAmount( ) );

	return predicted_head_position_down + std::clamp( static_cast< float >( lerp( head_offset_from_down_still_lerped, head_offset_from_down_moving_lerped, run_speed_fraction ) ), 0.0f, 9.0f );
}

vec3_t anims::get_server_shoot_position( const vec3_t& wanted_view_angles ) {
	vec3_t eye_position = g_cl.m_local->GetAbsOrigin( ) + g_cl.m_local->m_vecViewOffset( );

	vfunc< void( __thiscall* )( Player*, vec3_t& ) >( g_cl.m_local, 163 ) ( g_cl.m_local, eye_position );

	modify_eye_position( eye_position, get_predicted_head_z( wanted_view_angles ) );

	return eye_position;
}

inline int select_weighted_seq( CCSGOPlayerAnimState* animstate, int act ) {
	int seq = -1;

	const bool crouching = animstate->m_duck_amount > 0.55f;
	const bool moving = animstate->m_speed_as_portion_of_walk_speed > 0.25f;

	switch ( act ) {
		case ACT_CSGO_LAND_HEAVY:
		seq = 23;
		if ( crouching )
			seq = 24;
		break;
		case ACT_CSGO_FALL:
		seq = 14;
		break;
		case ACT_CSGO_JUMP:
		seq = moving + 17;
		if ( !crouching )
			seq = moving + 15;
		break;
		case ACT_CSGO_CLIMB_LADDER:
		seq = 13;
		break;
		case ACT_CSGO_LAND_LIGHT:
		seq = 2 * moving + 20;
		if ( crouching ) {
			seq = 21;
			if ( moving )
				seq = 19;
		}
		break;
		case ACT_CSGO_IDLE_TURN_BALANCEADJUST:
		seq = 4;
		break;
		case ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING:
		seq = 5;
		break;
		case ACT_CSGO_FLASHBANG_REACTION:
		seq = 224;
		if ( crouching )
			seq = 225;
		break;
		case ACT_CSGO_ALIVE_LOOP:
		seq = 8;
		if ( animstate->m_weapon
			 && animstate->m_weapon->GetWpnData( )
			 && animstate->m_weapon->GetWpnData( )->m_weapon_type == WEAPONTYPE_KNIFE )
			seq = 9;
		break;
		default:
		return -1;
	}

	if ( seq < 2 )
		return -1;

	return seq;
}

void anims::do_animation_event( CCSGOPlayerAnimState* anim_state, int id, int data ) {
	const auto active_weapon = anim_state->m_player ? anim_state->m_player->GetActiveWeapon( ) : nullptr;

	switch ( id ) {
		/* this is the only one we would probably care about rn */
		case 8 /* PLAYERANIMEVENT_JUMP */:
		/*
		*	use 0x10A as m_bJumping instead of 0x109 since m_bJumping does not seem to exist on the client
		*	(i'm too lazy to reverse server animstate class, so lets use this workaround and use the "free" space we have in the struct)
		*	animstate + 0x10A seems to be unused, and is in a great position (m_bJumping is supposed to be after m_bOnGround on the server?)
		*	good enough.
		*/
		*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x10A ) = true;
		//set_sequence( anim_state, 4, select_weighted_seq( anim_state, act_csgo_jump ) );
		break;
	}
}

void anims::trigger_animation_events( CCSGOPlayerAnimState* anim_state ) {
	const auto player = anim_state->m_player;

	/* work on running more animation events on the client later */
	/* handle jump events in ghetto way (what is a ghetto jump?) */
	if ( !( player->m_fFlags( ) & FL_ONGROUND )
		 && anim_state->m_ground
		 && ( ( player == g_cl.m_local && g_cl.m_local ) ? reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( player ) + 0x94 )->z : player->m_vecVelocity( ).z ) > 0.0f ) {
		do_animation_event( anim_state, 8, 0 /* is this event used?*/ );
	}
}