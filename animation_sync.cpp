#include "includes.h"

void IWS2000Targer::CorrectAnimlayers(IWSRecord* record) {

	if (!record)
		return;

	CCSGOPlayerAnimState* state = m_player->m_PlayerAnimState();
	IWSRecord* previous{ nullptr };

	if (this->m_records.size() > 2) {
		previous = this->m_records[1].get();
	}

	if (previous && !previous->dormant() && previous->m_setup)
	{
		state->m_primary_cycle = previous->m_server_layers[6].m_cycle;
		state->m_move_weight = previous->m_server_layers[6].m_weight;
		state->m_strafe_sequence = previous->m_server_layers[7].m_sequence;
		state->m_strafe_change_weight = previous->m_server_layers[7].m_weight;
		state->m_strafe_change_cycle = previous->m_server_layers[7].m_cycle;
		state->m_acceleration_weight = previous->m_server_layers[12].m_weight;
		this->m_player->SetAnimLayers(previous->m_server_layers);
	}
	else
	{

		if (record->m_flags & FL_ONGROUND) {
			state->m_on_ground = true;
			state->m_landing = false;
		}

		state->m_primary_cycle = record->m_server_layers[6].m_cycle;
		state->m_move_weight = record->m_server_layers[6].m_weight;
		state->m_strafe_sequence = record->m_server_layers[7].m_sequence;
		state->m_strafe_change_weight = record->m_server_layers[7].m_weight;
		state->m_strafe_change_cycle = record->m_server_layers[7].m_cycle;
		state->m_acceleration_weight = record->m_server_layers[12].m_weight;
		state->m_duration_in_air = 0.f;
		this->m_player->m_flPoseParameter()[6] = 0.f;
		this->m_player->SetAnimLayers(record->m_server_layers);

		state->m_last_update_time = (record->m_sim_time - g_csgo.m_globals->m_interval);
	}

}

void IWS2000Targer::CorrectVelocity(Player* m_player, IWSRecord* record, IWSRecord* previous, IWSRecord* pre_previous, float max_speed)
{
	record->m_velocity.Init();
	record->m_anim_velocity.Init();
	vec3_t vec_anim_velocity = m_player->m_vecVelocity();

	if (!previous)
	{
		if (record->m_server_layers[6].m_playback_rate > 0.0f && vec_anim_velocity.length() > 0.f)
		{
			if (record->m_server_layers[6].m_weight > 0.0f) {
				auto v30 = max_speed;

				if (record->m_flags & 6)
					v30 = v30 * 0.34f;
				else if (m_player->m_bIsWalking())
					v30 = v30 * 0.52f;

				auto v35 = record->m_server_layers[6].m_weight * v30;
				vec_anim_velocity *= v35 / vec_anim_velocity.length();
			}

			if (record->m_flags & 1)
				vec_anim_velocity.z = 0.f;
		}
		else
			vec_anim_velocity.Init();

		record->m_velocity = vec_anim_velocity;
		return;
	}

	// check if player has been on ground for two consecutive ticks
	bool bIsOnground = record->m_flags & FL_ONGROUND && previous->m_flags & FL_ONGROUND;

	const vec3_t vecOriginDelta = record->m_origin - previous->m_origin;

	if (bIsOnground && record->m_server_layers[6].m_playback_rate <= 0.0f)
		vec_anim_velocity.Init();

	//
	// entity teleported, reset his velocity (thats what server does)
	// - L3D451R7
	//
	if ((m_player->m_fEffects() & 8) != 0
		|| m_player->m_ubEFNoInterpParity() != m_player->m_ubEFNoInterpParityOld())
	{
		record->m_velocity.Init();
		record->m_anim_velocity.Init();
		return;
	}

	if (record->m_lag <= 1) {
		record->m_velocity = vec_anim_velocity;
		record->m_anim_velocity = vec_anim_velocity;
		return;
	}

	// fix up velocity 
	auto origin_delta_lenght = vecOriginDelta.length();

	if (record->m_lag_time > 0.0f && record->m_lag_time < 1.0f && origin_delta_lenght >= 1.f && origin_delta_lenght <= 1000000.0f) {

		vec_anim_velocity = vecOriginDelta / game::TICKS_TO_TIME(record->m_lag);
		vec_anim_velocity.validate_vec();

		record->m_velocity = vec_anim_velocity;
		// store estimated velocity as main velocity.

		if (!bIsOnground)
		{
			//
			// s/o estk for this correction :^)
			// -L3D451R7
			if ((previous->m_flags & 2) != (record->m_flags & 2))
			{
				float duck_modifier = -9.f;
				if (record->m_flags & 2)
					duck_modifier = 9.f;

				vec_anim_velocity.z += duck_modifier;
			}
		}
	}

	float anim_speed = 0.f;

	// determine if we can calculate animation speed modifier using server anim overlays
	if (bIsOnground
		&& record->m_server_layers[ANIMATION_LAYER_ALIVELOOP].m_weight > 0.0f
		&& record->m_server_layers[ANIMATION_LAYER_ALIVELOOP].m_weight < 1.0f
		&& record->m_server_layers[ANIMATION_LAYER_ALIVELOOP].m_playback_rate == record->m_server_layers[ANIMATION_LAYER_ALIVELOOP].m_playback_rate)
	{
		// calculate animation speed yielded by anim overlays
		auto flAnimModifier = 0.35f * (1.0f - record->m_server_layers[ANIMATION_LAYER_ALIVELOOP].m_weight);
		if (flAnimModifier > 0.0f && flAnimModifier < 1.0f)
			anim_speed = max_speed * (flAnimModifier + 0.55f);
	}

	// this velocity is valid ONLY IN ANIMFIX UPDATE TICK!!!
	// so don't store it to record as m_vecVelocity. i added vecAbsVelocity for that, it acts as a animationVelocity.
	// -L3D451R7
	if (anim_speed > 0.0f) {
		anim_speed /= vec_anim_velocity.length_2d();
		vec_anim_velocity.x *= anim_speed;
		vec_anim_velocity.y *= anim_speed;
	}


	bool jumping = !(record->m_flags & FL_ONGROUND)
		|| !(previous->m_flags & FL_ONGROUND);

	// calculate average player direction when bunny hopping
	if (pre_previous && vec_anim_velocity.length() >= 400.f)
	{
		auto vecPreviousVelocity = (previous->m_origin - pre_previous->m_origin) / game::TICKS_TO_TIME(record->m_lag);

		// make sure to only calculate average player direction whenever they're bhopping
		if (!vecPreviousVelocity.is_zero() && !bIsOnground) {
			auto vecCurrentDirection = math::NormalizedAngle(math::rad_to_deg(atan2(vec_anim_velocity.y, vec_anim_velocity.x)));
			auto vecPreviousDirection = math::NormalizedAngle(math::rad_to_deg(atan2(vecPreviousVelocity.y, vecPreviousVelocity.x)));

			auto vecAvgDirection = vecCurrentDirection - vecPreviousDirection;
			vecAvgDirection = math::deg_to_rad(math::NormalizedAngle(vecCurrentDirection + vecAvgDirection * 0.5f));

			auto vecDirectionCos = cos(vecAvgDirection);
			auto vecDirectionSin = sin(vecAvgDirection);

			// modify velocity accordingly
			vec_anim_velocity.x = vecDirectionCos * vec_anim_velocity.length_2d();
			vec_anim_velocity.y = vecDirectionSin * vec_anim_velocity.length_2d();
		}
	}

	if (!bIsOnground)
	{
		// correct z velocity.
		// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/shared/gamemovement.cpp#L1950
		if (!(record->m_flags & FL_ONGROUND))
			vec_anim_velocity.z -= g_csgo.sv_gravity->GetFloat() * record->m_lag_time * 0.5f;
		else
			vec_anim_velocity.z = 0.0f;
	}
	else
		vec_anim_velocity.z = 0.0f;

	// detect fakewalking players
	if (record->m_flags & FL_ONGROUND
		&& record->m_server_layers[6].m_weight == 0.f
		&& record->m_lag > 2
		&& record->m_anim_velocity.length_2d() > 0.f
		&& record->m_anim_velocity.length_2d() < record->m_max_current_speed * 0.34f) {
		record->m_fake_walk = true;
		record->m_velocity.clear();
		record->m_anim_velocity.clear();
	}

	vec_anim_velocity.validate_vec();

	// assign fixed velocity to record velocity
	record->m_anim_velocity = vec_anim_velocity;
}