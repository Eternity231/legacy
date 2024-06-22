#include "includes.h"
#include <execution>

IWS2000 g_iws{ };;

void IWS2000Target::UpdateAnimations(IWSRecord* record) {
	CCSGOPlayerAnimState* state = this->m_player->m_PlayerAnimState();
	if (!state)
		return;

	// player respawned.
	if (m_player->m_flSpawnTime() != m_spawn) {
		// reset animation state.
		game::ResetAnimationState(state);

		// note new spawn time.
		m_spawn = m_player->m_flSpawnTime();
	}

	// s/o onetap
	float m_flRealtime = g_csgo.m_globals->m_realtime;
	float m_flCurtime = g_csgo.m_globals->m_curtime;
	float m_flFrametime = g_csgo.m_globals->m_frametime;
	float m_flAbsFrametime = g_csgo.m_globals->m_abs_frametime;
	int m_iFramecount = g_csgo.m_globals->m_frame;
	int m_iTickcount = g_csgo.m_globals->m_tick_count;

	g_csgo.m_globals->m_realtime = this->m_player->m_flSimulationTime();
	g_csgo.m_globals->m_curtime = this->m_player->m_flSimulationTime();
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_abs_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_frame = game::TIME_TO_TICKS(this->m_player->m_flSimulationTime());
	g_csgo.m_globals->m_tick_count = game::TIME_TO_TICKS(this->m_player->m_flSimulationTime());

	// backup stuff that we do not want to fuck with.
	AnimationBackup_t backup;

	backup.m_origin = this->m_player->m_vecOrigin();
	backup.m_abs_origin = this->m_player->GetAbsOrigin();
	backup.m_velocity = this->m_player->m_vecVelocity();
	backup.m_abs_velocity = this->m_player->m_vecAbsVelocity();
	backup.m_flags = this->m_player->m_fFlags();
	backup.m_eflags = this->m_player->m_iEFlags();
	backup.m_duck = this->m_player->m_flDuckAmount();
	backup.m_body = this->m_player->m_flLowerBodyYawTarget();
	this->m_player->GetAnimLayers(backup.m_layers);

	auto weapon = (Weapon*)(this->m_player->m_hActiveWeapon().Get());
	IWSRecord* previous = m_records.size() > 1 ? m_records[1].get() : nullptr;
	// get pointer to previous record.
	IWSRecord* previous_record = nullptr;
	IWSRecord* pre_previous_record = nullptr;

	// check
	if (this->m_records.size() >= 2)
		previous_record = this->m_records[1].get();

	if (this->m_records.size() >= 3)
		pre_previous_record = this->m_records[2].get();

	// is player a bot?
	bool bot = game::IsFakePlayer(this->m_player->index());

	// reset fakewalk state.
	record->m_fake_walk = false;
	record->m_fake_flick = false;
	record->m_mode = M200::ResolveMode::None;
	record->m_resolved = false;
	record->m_max_current_speed = 260.f;
	record->m_broke_lc = false;

	while (this->m_records.size() > (int)std::round(1.f / g_csgo.m_globals->m_interval))
		this->m_records.pop_back();

	if (m_records.size() > 0 && m_records.front()->m_broke_lc)
		m_records.pop_back();

	float max_speed = 260.f;
	const auto simulation_ticks = game::TIME_TO_TICKS(record->m_sim_time);
	auto old_simulation_ticks = game::TIME_TO_TICKS(record->m_old_sim_time);

	// fix various issues with the game
	// these issues can only occur when a player is choking data.
	if (record->m_lag >= 2 && !bot) {
		// detect fakewalk.
		float speed = record->m_velocity.length();

		if (record->broke_lc())
			record->m_broke_lc = true;

		// should be correct?
		// NOTE: vel fix should do that already????
		// so comment this if it still bugs out
		if (record->m_flags & FL_ONGROUND
			&& record->m_layers[6].m_weight == 0.f
			&& record->m_lag > 2
			&& record->m_anim_velocity.length_2d() > 0.f
			&& record->m_anim_velocity.length_2d() < record->m_max_current_speed * 0.34f)
			record->m_fake_walk = true;

		if (m_records.size() >= 2) {
			if (previous_record && !previous_record->dormant()) {
				if (record->m_velocity.length() < 20.f
					&& record->m_velocity.length() > 0.f
					&& record->m_layers[6].m_weight != 1.0f && record->m_layers[6].m_weight != 0.0f
					&& record->m_layers[6].m_weight != previous_record->m_layers[6].m_weight
					&& (record->m_flags & FL_ONGROUND))
					record->m_fake_flick = true;
			}
		}

		// we need atleast 2 updates/records
		// to fix these issues.
		if (this->m_records.size() >= 2) {
			if (previous_record && !previous_record->dormant()) {

				// set previous flags.
				this->m_player->m_fFlags() = previous_record->m_flags;

				// strip the on ground flag.
				this->m_player->m_fFlags() &= ~FL_ONGROUND;

				// been onground for 2 consecutive ticks? fuck yeah.
				if (record->m_flags & FL_ONGROUND && previous_record->m_flags & FL_ONGROUND)
					this->m_player->m_fFlags() |= FL_ONGROUND;

				// fix jump_fall. (c) esoterik
				auto sequence = this->m_player->GetSequenceActivity(record->m_layers[5].m_sequence);

				// player landed this animation tick.
				if (record->m_layers[5].m_sequence == previous_record->m_layers[5].m_sequence && (previous_record->m_layers[5].m_weight != 0.0f || record->m_layers[5].m_weight == 0.0f)
					|| !(sequence == ACT_CSGO_LAND_LIGHT || sequence == ACT_CSGO_LAND_HEAVY))
				{
					if ((record->m_flags & 1) != 0 && (previous_record->m_flags & FL_ONGROUND) == 0) {
						this->m_player->m_fFlags() &= ~FL_ONGROUND;
					}
				}
				else {
					this->m_player->m_fFlags() |= FL_ONGROUND;
				}

				// fix crouching players.
				// the duck amount we receive when people choke is of the last simulation.
				// if a player chokes packets the issue here is that we will always receive the last duckamount.
				// but we need the one that was animated.
				// therefore we need to compute what the duckamount was at animtime.

				// delta in duckamt and delta in time..
				float duck = record->m_duck - previous_record->m_duck;
				float time = record->m_sim_time - previous_record->m_sim_time;

				// get the duckamt change per tick.
				float change = (duck / time) * g_csgo.m_globals->m_interval;

				// fix crouching players.
				this->m_player->m_flDuckAmount() = previous_record->m_duck + change;
			}
		}
	}

	if (weapon && !weapon->IsKnife()) {
		if (record->m_lag > 0) {
			const auto& shot_tick = game::TIME_TO_TICKS(weapon->m_fLastShotTime());

			if (shot_tick > old_simulation_ticks && simulation_ticks >= shot_tick) {
				record->m_shot = 1;

				if (shot_tick == simulation_ticks)
					record->m_shot = 2;
			}
			else {
				if (abs(simulation_ticks - shot_tick) > record->m_lag + 2)
					record->m_last_nonshot_pitch = record->m_eye_angles.x;
			}
		}

		auto data = weapon->GetWpnData();

		if (data)
			max_speed = m_player->m_bIsScoped() ? data->m_max_player_speed_alt : data->m_max_player_speed;
	}

	if (record->m_shot == 1) {
		if (record->m_lag > 1) {
			if (record->m_eye_angles.x < 70.0f) {
				if (record->m_last_nonshot_pitch > 80.0f)
					record->m_eye_angles.x = record->m_last_nonshot_pitch;
			}
		}
	}

	CorrectVelocity(this->m_player, record, previous_record, pre_previous_record, record->m_max_current_speed); // call velocity fix b4 resolving because otherwise dump montage

	bool fake = !bot && g_config.b["iws_enable"];

	// if using fake angles, correct angles.
	if (fake) {
		bool lby_update = false;
		M200::resolve_player(this->m_player, record, lby_update);
	}

	if (fake) {
		// detect fakewalk.
		float speed = backup.m_velocity.length();

		// we need atleast 2 updates/records
		// to fix these issues.
		if (previous) {

			// correct lag amount
			record->m_lag = game::TIME_TO_TICKS(record->m_sim_time - previous->m_sim_time);

			// fix for tickbase shifting retards going boing boing
			if (record->m_lag > 0 && record->m_lag <= 17) {
				int v490 = m_player->GetSequenceActivity(record->m_layers[5].m_sequence);

				if (record->m_layers[5].m_sequence == previous->m_layers[5].m_sequence && (previous->m_layers[5].m_weight != 0.0f || record->m_layers[5].m_weight == 0.0f)
					|| !(v490 == ACT_CSGO_LAND_LIGHT || v490 == ACT_CSGO_LAND_HEAVY)) {
					if ((record->m_flags & FL_ONGROUND) && !(previous->m_flags & FL_ONGROUND))
						m_player->m_fFlags() &= ~FL_ONGROUND;
				}
				else
					m_player->m_fFlags() |= FL_ONGROUND;
			}

		}

	}

	// set stuff before animating.
	this->m_player->m_vecOrigin() = record->m_origin;
	this->m_player->m_vecVelocity() = this->m_player->m_vecAbsVelocity() = record->m_anim_velocity;
	this->m_player->m_flLowerBodyYawTarget() = record->m_body;

	// force to use correct abs origin and velocity ( no CalcAbsolutePosition and CalcAbsoluteVelocity calls )
	this->m_player->m_iEFlags() &= ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY);

	// write potentially resolved angles.
	this->m_player->m_angEyeAngles() = record->m_eye_angles;

	// fix animating in same frame.
	if (state->m_last_update_frame >= g_csgo.m_globals->m_frame)
		state->m_last_update_frame = g_csgo.m_globals->m_frame - 1;

	// 'm_animating' returns true if being called from SetupVelocity, passes raw velocity to animstate.
	this->m_player->m_bClientSideAnimation() = true;
	this->m_player->UpdateClientSideAnimation();
	this->m_player->m_bClientSideAnimation() = false;

	// restore server layers
	this->m_player->SetAnimLayers(record->m_layers);

	// store updated/animated poses and rotation in lagrecord.
	this->m_player->GetPoseParameters(record->m_poses);
	record->m_abs_ang = ang_t(0.f, state->m_foot_yaw, 0.f);

	// kaaba
	this->m_player->InvalidatePhysicsRecursive(InvalidatePhysicsBits_t::ANIMATION_CHANGED);

	//if (g_menu.main.debug.bonesetup.get() == 0) {
		record->m_setup = g_bones.SetupBonesRebuild(this->m_player, record->m_bones, 128, BONE_USED_BY_ANYTHING & ~BONE_USED_BY_BONE_MERGE, this->m_player->m_flSimulationTime(), BoneSetupFlags::UseCustomOutput);
	//}

	// restore backup data.
	this->m_player->m_vecOrigin() = backup.m_origin;
	this->m_player->m_vecVelocity() = backup.m_velocity;
	this->m_player->m_vecAbsVelocity() = backup.m_abs_velocity;
	this->m_player->m_fFlags() = backup.m_flags;
	this->m_player->m_iEFlags() = backup.m_eflags;
	this->m_player->m_flDuckAmount() = backup.m_duck;
	this->m_player->m_flLowerBodyYawTarget() = backup.m_body;
	this->m_player->SetAbsOrigin(backup.m_abs_origin);

	// restore globals.
	g_csgo.m_globals->m_realtime = m_flRealtime;
	g_csgo.m_globals->m_curtime = m_flCurtime;
	g_csgo.m_globals->m_frametime = m_flFrametime;
	g_csgo.m_globals->m_abs_frametime = m_flAbsFrametime;
	g_csgo.m_globals->m_frame = m_iFramecount;
	g_csgo.m_globals->m_tick_count = m_iTickcount;
}

void IWS2000Target::OnNetUpdate(Player* player) {
	bool reset = (!g_menu.main.aimbot.enable.get() || player->m_lifeState() == LIFE_DEAD || !player->enemy(g_cl.m_local));

	// if this happens, delete all the lagrecords.
	if (reset) {
		player->m_bClientSideAnimation() = true;
		m_records.clear();
		this->m_player = player;
		this->m_moving_time = -1;
		this->m_shots = 0;
		this->m_missed_shots = 0;
		this->m_moved = false;
		this->m_body_update = FLT_MAX;
		this->m_moving_time = -1;
		this->m_moving_origin = vec3_t();

		// reset stand and body index.
		this->m_lastmove_index = 0;
		this->m_lby_index = 0;
		this->m_ticks_since_dormant = 0;
		this->m_moving_index = 0;
		this->m_body_proxy_updated = false;
		this->m_body_index = false;
		return;
	}

	// update player ptr if required.
	// reset player if changed.
	if (this->m_player != player)

	// update player ptr.
	this->m_player = player;

	// indicate that this player has been out of pvs.
	// insert dummy record to separate records
	// to fix stuff like animation and prediction.
	if (player->dormant()) {
		bool insert = true;

		// we have any records already?
		if (!m_records.empty()) {

			IWSRecord* front = m_records.front().get();

			// we already have a dormancy separator.
			if (front->dormant())
				insert = false;
		}

		if (insert) {
			// add new record.
			m_records.emplace_front(std::make_shared< IWSRecord >(player));

			// get reference to newly added record.
			IWSRecord* current = m_records.front().get();

			// mark as dormant.
			current->m_dormant = true;
		}

		this->m_player = player;
		this->m_moving_time = -1;
		this->m_shots = 0;
		this->m_missed_shots = 0;
		this->m_moved = false;
		this->m_body_update = FLT_MAX;
		this->m_moving_time = -1;
		this->m_moving_origin = vec3_t();
		this->m_experimental_index = 0;
		this->m_test_index = 0;
		this->m_experimental_index = 0;
		this->m_airback_index = 0;
		this->m_testfs_index = 0;
		this->m_lowlby_index = 0;
		this->m_logic_index = 0;
		this->m_stand_index4 = 0;
		this->m_air_index = 0;
		this->m_airlby_index = 0;
		this->m_spin_index = 0;
		this->m_stand_index1 = 0;
		this->m_stand_index2 = 0;
		this->m_stand_index3 = 0;
		this->m_back_index = 0;
		this->m_reversefs_index = 0;
		this->m_back_at_index = 0;
		this->m_reversefs_at_index = 0;
		this->m_lastmove_index = 0;
		this->m_lby_index = 0;
		this->m_airlast_index = 0;
		this->m_body_index = 0;
		this->m_sidelast_index = 0;
		this->m_ticks_since_dormant = 0;
		this->m_moving_index = 0;
		this->m_alive_loop_cycle = -1;
		this->m_body_proxy_updated = false;
		return;
	}

	// not needed but i dont rly care, will get skipped by lagcomp if invalid
	bool update = this->m_records.empty()
		|| this->m_records.front()->m_layers[11].m_cycle != player->m_AnimOverlay()[11].m_cycle
		|| this->m_records.front()->m_layers[11].m_playback_rate != player->m_AnimOverlay()[11].m_playback_rate
		|| this->m_records.front()->m_sim_time != player->m_flSimulationTime()
		|| player->m_flSimulationTime() != player->m_flOldSimulationTime()
		|| player->m_vecOrigin() != this->m_records.front()->m_origin; // holy fuck thats a shit ton of check

	// this is the first data update we are receving
	// OR we received data with a newer simulation context.
	if (update) {
		// add new record.
		this->m_records.emplace_front(std::make_shared< IWSRecord >(player));

		// get reference to newly added record.
		IWSRecord* current = this->m_records.front().get();

		// mark as non dormant.
		current->m_dormant = false;

		// update animations on current record.
		// call resolver.
		UpdateAnimations(current);

		// set shifting tickbase record.
		current->m_shift = game::TIME_TO_TICKS(current->m_sim_time) - g_csgo.m_globals->m_tick_count;

		//if (g_menu.main.debug.bonesetup.get() == 1) {
		//	g_bones.setup(m_player, nullptr, current);
		//}

	}
}

void IWS2000Target::OnRoundStart(Player* player) {
	this->m_player = player;
	this->m_moving_time = -1;
	this->m_shots = 0;
	this->m_missed_shots = 0;
	this->m_moved = false;
	this->m_body_update = FLT_MAX;
	this->m_moving_time = -1;
	this->m_moving_origin = vec3_t();

	// reset stand and body index.
	this->m_lastmove_index = 0;
	this->m_lby_index = 0;
	this->m_body_index = 0;
	this->m_ticks_since_dormant = 0;
	this->m_moving_index = 0;
	this->m_body_proxy_updated = false;

	this->m_records.clear();
	this->m_hitboxes.clear();

}

void IWS2000Target::SetupHitboxes(IWSRecord* record, bool history) {
	// reset hitboxes & prefer body state.
	m_hitboxes.clear();
	m_prefer_head = false;
	m_prefer_body = false;
	float speed = record->m_anim_velocity.length();

	if (g_cl.m_weapon_id == ZEUS) {
		// hitboxes for the zeus.
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		this->hitscan_mode = "PREFER";
		m_prefer_body = true;
	}

	// prefer, always.
	if (g_menu.main.aimbot.baim1.get(0)) {
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		this->hitscan_mode = "PREFER";
		m_prefer_body = true;
	}

	// prefer, moving.
	if (g_menu.main.aimbot.head1.get() && (record->m_flags & FL_ONGROUND) && speed > 0.1f && !record->m_fake_walk)
		this->hitscan_mode = "PREFER";
		m_prefer_head = true;

	// prefer, resolved
	if (g_menu.main.aimbot.head1.get() && record->m_resolved)
		this->hitscan_mode = "PREFER";
		m_prefer_head = true;

	// prefer, fake.
	if (g_menu.main.aimbot.baim1.get(1) && !record->m_resolved) {
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		this->hitscan_mode = "PREFER";
		m_prefer_body = true;
	}

	// prefer, in air.
	if (g_menu.main.aimbot.baim1.get(2) && !(record->m_flags & FL_ONGROUND)) {
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		this->hitscan_mode = "PREFER";
		m_prefer_body = true;
	}

	// prefer, lethal.
	if (g_menu.main.aimbot.baim1.get(3)) {
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::LETHAL });
		this->hitscan_mode = "LETHAL";
		m_prefer_body = true;
	}

	// prefer, lethal x2.
	if (g_menu.main.aimbot.baim1.get(4)) {
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::LETHAL2 });
		this->hitscan_mode = "LETHAL-2";
		m_prefer_body = true;
	}

	bool force_body{ false };

	// only, always.
	if (g_menu.main.aimbot.baim2.get(0))
		force_body = true;

	// only, health.
	if (g_menu.main.aimbot.baim2.get(1) && m_player->m_iHealth() <= (int)g_menu.main.aimbot.baim_hp.get())
		force_body = true;

	// only, fake.
	if (g_menu.main.aimbot.baim2.get(2) && !record->m_resolved)
		force_body = true;

	// only, in air.
	if (g_menu.main.aimbot.baim2.get(3) && !(record->m_flags & FL_ONGROUND))
		force_body = true;

	// only, missed x shots to flick.
	if (g_menu.main.aimbot.baim2.get(4) && m_body_index >= g_menu.main.aimbot.body_misses.get())
		force_body = true;

	// only, on key.
	if (g_iws.m_force_body)
		force_body = true;

	// NOTE: i will push hitbox in damage % order
	bool ignore_limbs = record->m_velocity.length_2d() > 71.f && g_menu.main.aimbot.ignor_limbs.get();

	// head
	if (g_menu.main.aimbot.hitbox.get(0) && !g_iws.m_force_body) {
		m_hitboxes.push_back({ HITBOX_HEAD });
	}

	// neck
	if (g_menu.main.aimbot.hitbox.get(1) and !g_iws.m_force_body) {
		m_hitboxes.push_back({ HITBOX_NECK });
		//m_hitboxes.push_back({ HITBOX_LOWER_NECK });
	}

	// chest
	if (g_menu.main.aimbot.hitbox.get(2)) {
		m_hitboxes.push_back({ HITBOX_UPPER_CHEST });
		m_hitboxes.push_back({ HITBOX_CHEST });
		m_hitboxes.push_back({ HITBOX_THORAX });
	}

	// arms
	if (g_menu.main.aimbot.hitbox.get(3) and !g_iws.m_force_body) {
		m_hitboxes.push_back({ HITBOX_L_UPPER_ARM });
		m_hitboxes.push_back({ HITBOX_R_UPPER_ARM });
		m_hitboxes.push_back({ HITBOX_L_FOREARM });
		m_hitboxes.push_back({ HITBOX_R_FOREARM });
		m_hitboxes.push_back({ HITBOX_L_HAND });
		m_hitboxes.push_back({ HITBOX_R_HAND });
	}

	// stomach
	if (g_menu.main.aimbot.hitbox.get(4)) {
		m_hitboxes.push_back({ HITBOX_BODY });
		m_hitboxes.push_back({ HITBOX_PELVIS });
	}

	// legs.
	if (g_menu.main.aimbot.hitbox.get(5) and !g_iws.m_force_body) {
		m_hitboxes.push_back({ HITBOX_L_CALF });
		m_hitboxes.push_back({ HITBOX_R_CALF });
		m_hitboxes.push_back({ HITBOX_L_THIGH });
		m_hitboxes.push_back({ HITBOX_R_THIGH });
	}

	// feet.
	if (g_menu.main.aimbot.hitbox.get(6) && !ignore_limbs and !g_iws.m_force_body) {
		m_hitboxes.push_back({ HITBOX_L_FOOT });
		m_hitboxes.push_back({ HITBOX_R_FOOT });
	}
}

void IWS2000::init() {
	// clear old targets.
	m_targets.clear();

	m_target = nullptr;
	m_aim = vec3_t{ };
	m_angle = ang_t{ };
	m_damage = 0.f;
	m_record = nullptr;
	m_stop = false;

	m_best_dist = std::numeric_limits< float >::max();
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = std::numeric_limits< float >::max();
	m_best_height = std::numeric_limits< float >::max();
}

void IWS2000::start_target_selection() {
	IWSRecord* front{ };
	IWS2000Target* data{ };

	// setup bones for all valid targets.
	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		if (!IsValidTarget(player))
			continue;

		data = &m_players[i - 1];
		Entity* ent = g_csgo.m_entlist->GetClientEntity(i);
		player_info_t pinfo;

		g_csgo.m_engine->GetPlayerInfo(ent->index(), &pinfo);

		// we have no data, or the player ptr in data is invalid
		if (!data || !data->m_player || data->m_player->index() != player->index())
			continue;

		// mostly means he just went out of dormancy
		if (data->m_records.empty())
			continue;

		// get our front record
		front = data->m_records.front().get();

		// front record is invalid, skip this player
		if (!front || front->dormant() || front->immune() || !front->m_setup)
			continue;

		// store player as potential target this tick.
		m_targets.emplace_back(data);
	}
}

bool IWS2000::SelectTarget(IWSRecord* record, const vec3_t& aim, float damage) {
	//float dist, fov, height;
	//int   hp;

	float fov;

	//switch ( g_menu.main.aimbot.selection.get( ) ) {

	//	// distance.
	//case 0:
	//	dist = ( record->m_pred_origin - g_cl.m_shoot_pos ).length( );

	//	if ( dist < m_best_dist ) {
	//		m_best_dist = dist;
	//		return true;
	//	}

	//	break;

	//	// crosshair.
	//case 1:
	fov = math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, aim);

	if (fov < m_best_fov) {
		m_best_fov = fov;
		return true;
	}

	//	break;

	//	// damage.
	//case 2:
	//	if ( damage > m_best_damage ) {
	//		m_best_damage = damage;
	//		return true;
	//	}

	//	break;

	//	// lowest hp.
	//case 3:
	//	// fix for retarded servers?
	//	hp = std::min( 100, record->m_player->m_iHealth( ) );

	//	if ( hp < m_best_hp ) {
	//		m_best_hp = hp;
	//		return true;
	//	}

	//	break;

	//	// least lag.
	//case 4:
	//	if ( record->m_lag < m_best_lag ) {
	//		m_best_lag = record->m_lag;
	//		return true;
	//	}

	//	break;

	//	// height.
	//case 5:
	//	height = record->m_pred_origin.z - g_cl.m_local->m_vecOrigin( ).z;

	//	if ( height < m_best_height ) {
	//		m_best_height = height;
	//		return true;
	//	}

	//	break;

	//default:
	//	return false;
	//}

	//return false;
}

void IWS2000::finish_target_selection() {

	// NOTE: what you can do here is make multiples modes
	//       but most of them will be shit when using target limit
	//       ^ pandora did this mistake on their target limit
	auto sort_targets = [&](const IWS2000Target* a, const IWS2000Target* b) {


		// player b and player a are the same
		// do nothing
		if (a == b)
			return false;

		// player a is invalid, if player b is valid, prioritize him
		// else do nothing
		if (!a)
			return b ? true : false;

		// player b is invalid, if player a is valid, prioritize him
		// else do nothing
		if (!b)
			return a ? true : false;

		// this is the same player
		// in that case, do nothing
		if (a->m_player == b->m_player || a->m_player->index() == b->m_player->index())
			return false;

		// get fov of player a
		float fov_a = math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, a->m_player->WorldSpaceCenter());

		// get fov of player b
		float fov_b = math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, b->m_player->WorldSpaceCenter());

		// if player a fov lower than player b fov prioritize him
		return fov_a < fov_b;
	};

	// if we have only 2 targets or less, no need to sort
	if (m_targets.size() <= 2)
		return;

	// std::execution::par -> parallel sorting (multithreaded)
	// NOTE: not obligated, std::sort doesnt take alot of cpu power but its still better
	std::sort(std::execution::par, m_targets.begin(), m_targets.end(), sort_targets);

	// target limit based on our prioritized targets
	while (m_targets.size() > g_menu.main.aimbot.target_limit.get())
		m_targets.pop_back();
}

void IWS2000::StripAttack() {
	if (g_cl.m_weapon_id == REVOLVER)
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK2;

	else
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK;
}

void IWS2000::think() {
	// do all startup routines.
	init();

	// sanity.
	if (!g_cl.m_weapon)
		return;

	// no grenades or bomb.
	if (g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_C4)
		return;

	if (!g_cl.m_weapon_fire)
		StripAttack();

	// we have no aimbot enabled.
	if (!g_menu.main.aimbot.enable.get())
		return;

	if (!g_menu.main.aimbot.autofire.get())
		return;

	// animation silent aim, prevent the ticks with the shot in it to become the tick that gets processed.
	// we can do this by always choking the tick before we are able to shoot.
	bool revolver = g_cl.m_weapon_id == REVOLVER && g_cl.m_revolver_cock != 0;

	// one tick before being able to shoot.
	if (revolver && g_cl.m_revolver_cock > 0 && g_cl.m_revolver_cock == g_cl.m_revolver_query) {
		*g_cl.m_packet = false;
		return;
	}

	// add players in our target selection.
	start_target_selection();

	// dont run ragebot if we have no targets.
	if (m_targets.empty() || m_targets.size() <= 0)
		return;

	// NOTE: this is silent aim check (makes sure you dont shoot while lag <= 0)
	// moved it down here so autostop still runs
	// we have a normal weapon or a non cocking revolver
	// choke if its the processing tick.
	if (g_cl.m_weapon_fire && !g_cl.m_lag && !revolver) {
		*g_cl.m_packet = false;
		StripAttack();
		return;
	}

	// sort our potential targets
	finish_target_selection();

	// no point in aimbotting if we cannot fire this tick.
	if (!g_cl.m_weapon_fire)
		return;

	// run knifebot.
	if (g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS) {

		//if ( g_menu.main.aimbot.knifebot.get( ) )
		knife();

		return;
	}

	// NOTE: this is silent aim check (makes sure you dont shoot while lag <= 0)
	// moved it down here so autostop still runs
	// we have a normal weapon or a non cocking revolver
	// choke if its the processing tick.
	if (g_cl.m_weapon_fire && !g_cl.m_lag && !revolver) {
		*g_cl.m_packet = false;
		StripAttack();
		return;
	}

	// no point in aimbotting if we cannot fire this tick.
	if (!g_cl.m_weapon_fire)
		return StripAttack();

	// scan available targets... if we even have any.
	find();

	// finally set data when shooting.
	apply();
}

void IWS2000::find() {
	struct BestTarget_t { Player* player; vec3_t pos; float damage; int hitbox; int hitgroup; IWSRecord* record; BoneArray* bones; };

	vec3_t       tmp_pos;
	float        tmp_damage;
	int          tmp_hitbox, tmp_hitgroup;
	BestTarget_t best;
	best.player = nullptr;
	best.damage = -1.f;
	best.pos = vec3_t{ };
	best.record = nullptr;

	if (m_targets.empty())
		return;

	//if ( g_cl.m_weapon_id == ZEUS /*&& !g_menu.main.aimbot.zeusbot.get( )*/ )
	//	continue;

	// set this to 0
	int iterated_players{ 0 };

	// iterate all targets.
	for (const auto& t : m_targets) {
		if (t->m_records.empty())
			continue;

		// this player broke lagcomp.
		// his bones have been resetup by our lagcomp.
		// therfore now only the front record is valid.
		if (g_menu.main.aimbot.lagfix.get() && g_lagcomp.StartPrediction(t)) {
			IWSRecord* front = t->m_records.front().get();

			t->SetupHitboxes(front, false);
			if (t->m_hitboxes.empty())
				continue;

			// rip something went wrong..
			if (t->GetBestAimPosition(tmp_pos, tmp_damage, tmp_hitbox, tmp_hitgroup, front) && SelectTarget(front, tmp_pos, tmp_damage)) {

				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = front;
				best.hitbox = tmp_hitbox;
				best.hitgroup = tmp_hitgroup;

				if (best.damage >= best.player->m_iHealth())
					break;
			}
		}

		// player did not break lagcomp.
		// history aim is possible at this point.
		else {

			IWSRecord* ideal = g_resolver.FindIdealRecord(t);
			if (!ideal)
				continue;

			t->SetupHitboxes(ideal, false);
			if (t->m_hitboxes.empty())
				continue;

			bool first_hit = t->GetBestAimPosition(tmp_pos, tmp_damage, tmp_hitbox, tmp_hitgroup, ideal) && SelectTarget(ideal, tmp_pos, tmp_damage);

			// try to select best record as target.
			if (first_hit) {
				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = ideal;
				best.hitbox = tmp_hitbox;
				best.hitgroup = tmp_hitgroup;

				if (best.damage >= best.player->m_iHealth())
					break;
			}

			// we scanned half of our prioritized targets
			// ignore last record on all remaining targets
			if (g_menu.main.aimbot.aimbottweaks.get(0) && iterated_players >= (int)std::round(m_targets.size() / 2.f))
				continue;

			IWSRecord* last = g_resolver.FindLastRecord(t);
			if (!last || last == ideal || last->m_origin.dist_to(ideal->m_origin) <= 8.f) // ghetto fix but owell
				continue;

			t->SetupHitboxes(last, true);
			if (t->m_hitboxes.empty())
				continue;

			// we hit last record
			if (t->GetBestAimPosition(tmp_pos, tmp_damage, tmp_hitbox, tmp_hitgroup, last) && SelectTarget(last, tmp_pos, tmp_damage)) {

				// TODO: make this more clean i guess
				bool ideal_resolved = ideal->m_resolved;

				bool last_resolved = last->m_resolved;

				// check if last has more damage
				bool prefer_last = first_hit && tmp_damage >= best.damage && (last_resolved || !ideal_resolved);

				// is last record better than ideal?
				if (!first_hit || prefer_last) {
					// if we made it so far, set shit.
					best.player = t->m_player;
					best.pos = tmp_pos;
					best.damage = tmp_damage;
					best.record = last;
					best.hitbox = tmp_hitbox;
					best.hitgroup = tmp_hitgroup;

					if (best.damage >= best.player->m_iHealth())
						break;
				}
			}

			++iterated_players;
		}
	}

	// verify our target and set needed data.
	if (best.player && best.record) {

		// calculate aim angle.
		math::VectorAngles(best.pos - g_cl.m_shoot_pos, m_angle);

		// set member vars.
		this->m_target = best.player;
		this->m_aim = best.pos;
		this->m_damage = best.damage;
		this->m_record = best.record;
		this->m_hitbox = best.hitbox;
		this->m_hitgroup = best.hitgroup;
		if (m_target) {
			IWS2000Target* data = &g_iws.m_players[this->m_target->index() - 1];

			if (data)
				data->m_pos = best.pos;
		}

		// write data, needed for traces / etc.
		this->m_record->cache();

		// set autostop shit.
		this->m_stop = !(g_cl.m_buttons & IN_JUMP);

		// set autostop shit.
		if (g_menu.main.movement.autostop_always_on.get())
			g_movement.AutoStop();

		bool on = g_menu.main.aimbot.hitchance.get() && g_menu.main.config.mode.get() == 0;
		bool hit = CheckHitchance(this->m_target, this->m_angle, 255); // 255 is the seed amount, putting it lower is quite literally useless lol

		// if we can scope.
		bool can_scope = !g_cl.m_local->m_bIsScoped() && (g_cl.m_weapon_id == AUG || g_cl.m_weapon_id == SG553 || g_cl.m_weapon_type == WEAPONTYPE_SNIPER_RIFLE);

		// do not scope while in air ^_^
		if (can_scope && g_menu.main.aimbot.zoom.get() && g_cl.m_local->m_fFlags() & (1 << 0) && g_cl.m_flags & (1 << 0)) {
			g_cl.m_cmd->m_buttons |= IN_ATTACK2;
			return;
		}

		if (can_scope) {
			// always.
			if (g_menu.main.aimbot.zoom.get()) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}
		}

		if (hit || !on) {
			// right click attack.
			if (g_menu.main.config.mode.get() == 1 && g_cl.m_weapon_id == REVOLVER)
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;

			// left click attack.
			else
				g_cl.m_cmd->m_buttons |= IN_ATTACK;
		}
	}
}

std::vector< int > precomputed_seeds;

bool IWS2000::CheckHitchance(Player* player, const ang_t& angle, int seeds) {
	constexpr float hc_max = 100.f;
	float chance = g_menu.main.aimbot.hitchance_amount.get();

	// if didn't add already
	// precompute seeds
	if (precomputed_seeds.empty()) {
		for (int i = 0; i < seeds; i++) {
			g_csgo.RandomSeed(i + 1);
			precomputed_seeds.emplace_back(i);
		}
	}

	int hitchance_amount;

	hitchance_amount = g_menu.main.aimbot.hitchance_in_air.get() && !(g_cl.m_local->m_fFlags() & FL_ONGROUND) ? g_menu.main.aimbot.hitchance_amount_air.get() : g_menu.main.aimbot.hitchance_amount.get();

	if (g_menu.main.aimbot.aimbottweaks.get(2) && g_cl.m_local->m_flDuckAmount() >= 0.125f) {
		if (g_cl.m_flPreviousDuckAmount > g_cl.m_local->m_flDuckAmount()) {
			return false;
		}
	}

	vec3_t     start{ g_cl.m_shoot_pos }, end, fwd, right, up, dir, wep_spread, spr_angl;
	float      inaccuracy, spread;
	CGameTrace tr;
	size_t     total_hits{ }, needed_hits{ (size_t)std::ceil((hitchance_amount * seeds) / hc_max) };
	int awalls_hit = 0;

	// call game::update_accuracy_pen
	g_cl.m_weapon->UpdateAccuracyPenalty();

	// get needed directional vectors.
	math::AngleVectors(angle, &fwd, &right, &up);

	// store off inaccuracy / spread ( these functions are quite intensive and we only need them once ).
	inaccuracy = g_cl.m_weapon->GetInaccuracy();
	spread = g_cl.m_weapon->GetSpread();

	// do seeds
	for (auto& i : precomputed_seeds) {
		// call game::random_seed
		g_csgo.RandomSeed(i);

		// store rand vars
		float rand1 = g_csgo.RandomFloat(0.0f, 1.0f);
		float rand2 = g_csgo.RandomFloat(0.0f, 2.0f * math::pi);
		float rand3 = g_csgo.RandomFloat(0.0f, 1.0f);
		float rand4 = g_csgo.RandomFloat(0.0f, 2.0f * math::pi);

		// fix accuracy for a revolver
		if (g_cl.m_weapon_id == REVOLVER) {
			rand1 = 1.0f - rand1 * rand1;
			rand3 = 1.0f - rand3 * rand3;
		}

		// call spread and store it
		wep_spread = vec3_t(
			cos(rand2) * (rand1 * inaccuracy) + cos(rand4) * (rand3 * spread),
			sin(rand2) * (rand1 * inaccuracy) + sin(rand4) * (rand3 * spread),
			0.0f);

		// normalize spread direction
		dir = (fwd + right * -wep_spread.x + up * -wep_spread.y).normalized();

		// setup end of trace
		end = start + (dir * g_cl.m_weapon_info->m_range);

		// setup ray and trace.
		g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT, player, &tr);

		// we hit
		if (tr.m_entity == player) {
			++total_hits;

			if (g_menu.main.aimbot.accuracy_boost.get() > 1.f)
				++awalls_hit;

			// better run it there
			if (total_hits >= needed_hits)
				return true;
		}

		if (((static_cast<float>(total_hits) / static_cast<float>(seeds)) >= (chance / 100.f)))
		{
			if (((static_cast<float>(awalls_hit) / static_cast<float>(seeds)) >= (g_menu.main.aimbot.accuracy_boost.get() / 100.f)) || g_menu.main.aimbot.accuracy_boost.get() <= 1.f)
				return true;
		}

	}

	return false;
}

// enrage raytrace 
bool IWS2000Target::SetupHitboxPoints(IWSRecord* record, BoneArray* bones, int index, std::vector< vec3_t >& points) {
	// reset points.
	points.clear();

	const model_t* model = m_player->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(m_player->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t* bbox = set->GetHitbox(index);
	if (!bbox)
		return false;

	// get hitbox scales.
	float scale = g_menu.main.aimbot.scale.get() / 100.f;
	float bscale = g_menu.main.aimbot.body_scale.get() / 100.f;
	float lscale = 0.875f;

	// compute raw center point.
	vec3_t center = (bbox->m_mins + bbox->m_maxs) / 2.f;

	vec3_t transformed_center = center;
	math::VectorTransform(transformed_center, bones[bbox->m_bone], transformed_center);

	// calculate dynamic scale.
	if (!g_menu.main.aimbot.static_scale.get()) {
		float spread = g_cl.m_weapon->GetInaccuracy() + g_cl.m_weapon->GetSpread();
		float distance = transformed_center.dist_to(g_cl.m_shoot_pos);

		distance /= sin(math::deg_to_rad(90.f - math::rad_to_deg(spread)));
		spread = sin(spread);

		// get radius and set spread.
		float radius = std::max(bbox->m_radius - distance * spread, 0.f);
		scale = bscale = lscale = std::clamp(radius / bbox->m_radius, 0.f, 1.f);
	}

	// these indexes represent boxes.
	if (bbox->m_radius <= 0.f) {
		// references: 
		//      https://developer.valvesoftware.com/wiki/Rotation_Tutorial
		//      CBaseAnimating::GetHitboxBonePosition
		//      CBaseAnimating::DrawServerHitboxes

		// convert rotation angle to a matrix.
		matrix3x4_t rot_matrix;
		g_csgo.AngleMatrix(bbox->m_angle, rot_matrix);

		// apply the rotation to the entity input space (local).
		matrix3x4_t matrix;
		math::ConcatTransforms(bones[bbox->m_bone], rot_matrix, matrix);

		// extract origin from matrix.
		vec3_t origin = matrix.GetOrigin();

		// the feet hiboxes have a side, heel and the toe.
		if (index == HITBOX_R_FOOT || index == HITBOX_L_FOOT) {
			float d1 = (bbox->m_mins.z - center.z) * lscale;

			// invert.
			if (index == HITBOX_L_FOOT)
				d1 *= -1.f;

			// side is more optimal then center.
			points.push_back({ center.x, center.y, center.z + d1 });

			if (g_menu.main.aimbot.multipoint.get(4))
			{
				// get point offset relative to center point
				// and factor in hitbox scale.
				float d2 = (bbox->m_mins.x - center.x) * lscale;
				float d3 = (bbox->m_maxs.x - center.x) * lscale;

				// heel.
				points.push_back({ center.x + d2, center.y, center.z });

				// toe.
				points.push_back({ center.x + d3, center.y, center.z });
			}
		}

		// nothing to do here we are done.
		if (points.empty())
			return false;

		// rotate our bbox points by their correct angle
		// and convert our points to world space.
		for (auto& p : points) {
			// VectorRotate.
			// rotate point by angle stored in matrix.
			p = { p.dot(matrix[0]), p.dot(matrix[1]), p.dot(matrix[2]) };

			// transform point to world space.
			p += origin;
		}
	}

	// these hitboxes are capsules.
	else {
		points.push_back(transformed_center);

		if (index == HITBOX_R_THIGH)
			return true;

		float point_scale = 0.2f;

		if (index == HITBOX_HEAD) {
			if (!g_menu.main.aimbot.multipoint.get(0))
				return true;

			point_scale = scale;
		}

		if (index == HITBOX_THORAX || index == HITBOX_CHEST) {
			if (!g_menu.main.aimbot.multipoint.get(1))
				return true;

			point_scale = bscale;
		}

		if (index == HITBOX_PELVIS || index == HITBOX_BODY) {
			if (!g_menu.main.aimbot.multipoint.get(2))
				return true;

			point_scale = bscale;
		}

		vec3_t min, max;
		math::VectorTransform(bbox->m_mins, bones[bbox->m_bone], min);
		math::VectorTransform(bbox->m_maxs, bones[bbox->m_bone], max);

		RayTracer::Hitbox box(min, max, bbox->m_radius);
		RayTracer::Trace trace;

		vec3_t delta = (transformed_center - g_cl.m_shoot_pos).normalized();
		vec3_t max_min = (max - min).normalized();
		vec3_t cr = max_min.cross(delta);

		ang_t delta_angle;
		math::VectorAngles(delta, delta_angle);

		vec3_t right, up;
		if (index == HITBOX_HEAD) {
			ang_t cr_angle;
			math::VectorAngles(cr, cr_angle);
			cr_angle.to_vectors(right, up);
			cr_angle.z = delta_angle.x;

			vec3_t _up = up, _right = right, _cr = cr;
			cr = _right;
			right = _cr;
		}
		else
			math::VectorVectors(delta, up, right);

		if (index == HITBOX_HEAD) {
			vec3_t middle = (right.normalized() + up.normalized()) * 0.5f;
			vec3_t middle2 = (right.normalized() - up.normalized()) * 0.5f;

			RayTracer::TraceFromCenter(RayTracer::Ray(g_cl.m_shoot_pos, transformed_center + (middle * 1000.f)), box, trace, RayTracer::Flags_RETURNEND);
			points.push_back(trace.m_traceEnd);

			RayTracer::TraceFromCenter(RayTracer::Ray(g_cl.m_shoot_pos, transformed_center - (middle2 * 1000.f)), box, trace, RayTracer::Flags_RETURNEND);
			points.push_back(trace.m_traceEnd);

			RayTracer::TraceFromCenter(RayTracer::Ray(g_cl.m_shoot_pos, transformed_center + (up * 1000.f)), box, trace, RayTracer::Flags_RETURNEND);
			points.push_back(trace.m_traceEnd);

			RayTracer::TraceFromCenter(RayTracer::Ray(g_cl.m_shoot_pos, transformed_center - (up * 1000.f)), box, trace, RayTracer::Flags_RETURNEND);
			points.push_back(trace.m_traceEnd);
		}
		else {
			RayTracer::TraceFromCenter(RayTracer::Ray(g_cl.m_shoot_pos, transformed_center + (up * 1000.f)), box, trace, RayTracer::Flags_RETURNEND);
			points.push_back(trace.m_traceEnd);

			RayTracer::TraceFromCenter(RayTracer::Ray(g_cl.m_shoot_pos, transformed_center - (up * 1000.f)), box, trace, RayTracer::Flags_RETURNEND);
			points.push_back(trace.m_traceEnd);
		}

		// nothing left to do here.
		if (points.empty())
			return false;

		// transform capsule points.
		for (auto& p : points) {
			vec3_t delta_center = p - transformed_center;
			p = transformed_center + delta_center * point_scale;
		}
	}

	return true;
}

// thanks alpha
bool IWS2000::CanHit(const vec3_t start, const vec3_t end, IWSRecord* record, int box, bool in_shot, BoneArray* bones)
{
	if (!record || !record->m_player)
		return false;

	// always try to use our aimbot matrix first.
	auto matrix = record->m_bones;

	// this is basically for using a custom matrix.
	if (in_shot)
		matrix = bones;

	if (!matrix)
		return false;

	const model_t* model = record->m_player->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(record->m_player->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t* bbox = set->GetHitbox(box);
	if (!bbox)
		return false;

	vec3_t min, max;
	const auto IsCapsule = bbox->m_radius != -1.f;

	if (IsCapsule) {
		math::VectorTransform(bbox->m_mins, matrix[bbox->m_bone], min);
		math::VectorTransform(bbox->m_maxs, matrix[bbox->m_bone], max);
		const auto dist = math::SegmentToSegment(start, end, min, max);

		if (dist < bbox->m_radius) {
			return true;
		}
	}
	else {
		CGameTrace tr;

		// setup ray and trace.
		g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT, record->m_player, &tr);

		// check if we hit a valid player / hitgroup on the player and increment total hits.
		if (tr.m_entity == record->m_player && game::IsValidHitgroup(tr.m_hitgroup))
			return true;
	}

	return false;
}

bool IWS2000::CanHitPlayer(IWSRecord* pRecord, int iSide, const vec3_t& vecEyePos, const vec3_t& vecEnd, int iHitboxIndex) {
	auto hdr = *(studiohdr_t**)pRecord->m_player->GetModelPtr();

	if (!hdr)
		return false;

	auto pHitboxSet = hdr->GetHitboxSet(pRecord->m_player->m_nHitboxSet());

	if (!pHitboxSet)
		return false;

	auto pHitbox = pHitboxSet->GetHitbox(iHitboxIndex);

	if (!pHitbox)
		return false;

	bool bIsCapsule = pHitbox->m_radius != -1.0f;
	bool bHitIntersection = false;

	CGameTrace tr;

	vec3_t vecMin;
	vec3_t vecMax;

	math::VectorTransform(math::vector_rotate(pHitbox->m_mins, pHitbox->m_angle), pRecord->m_bones[pHitbox->m_bone], vecMin);
	math::VectorTransform(math::vector_rotate(pHitbox->m_mins, pHitbox->m_angle), pRecord->m_bones[pHitbox->m_bone], vecMax);

	bHitIntersection = bIsCapsule ?
		math::IntersectSegmentToSegment(vecEyePos, vecEnd, vecMin, vecMax, pHitbox->m_radius * 1.01f) : math::IntersectionBoundingBox(vecEyePos, vecEnd, vecMin, vecMax);//( tr.hit_entity == pRecord->player && ( tr.hitgroup >= Hitgroup_Head && tr.hitgroup <= Hitgroup_RightLeg ) || tr.hitgroup == Hitgroup_Gear );

	return bHitIntersection;
};

bool IWS2000Target::GetBestAimPosition(vec3_t& aim, float& damage, int& hitbox, int& hitgroup, IWSRecord* record) {
	bool                  skip_other_points{ false }, skip_other_hitboxes{ false };
	float                 dmg, pendmg;
	HitscanData_t         scan;
	std::vector< vec3_t > points;

	// get player hp.
	int hp = std::min(100, m_player->m_iHealth());

	if (g_cl.m_weapon_id == ZEUS) {
		dmg = pendmg = hp + 1;
	}
	else {
		dmg = g_iws.m_damage_toggle ? g_menu.main.aimbot.override_dmg_value.get() : g_menu.main.aimbot.minimal_damage.get();

		if (g_menu.main.aimbot.minimal_damage_hp.get())
			dmg = std::ceil((dmg / 100.f) * hp);

		if (dmg > 100)
			dmg = m_player->m_iHealth() + g_iws.m_damage_toggle ? g_menu.main.aimbot.override_dmg_value.get() : g_menu.main.aimbot.minimal_damage.get();

		// fuck pen damage 
		pendmg = dmg;
	}

	// write all data of this record l0l.
	record->cache();

	bool done = false;

	// iterate hitboxes.
	for (const auto& it : m_hitboxes) {
		done = false;

		// setup points on hitbox.
		if (!SetupHitboxPoints(record, record->m_bones, it.m_index, points))
			continue;

		// iterate points on hitbox.
		for (const auto& point : points) {
			penetration::PenetrationInput_t in;

			in.m_damage = dmg;
			in.m_damage_pen = pendmg;
			in.m_can_pen = g_cl.m_weapon_id == ZEUS ? false : true;
			in.m_target = m_player;
			in.m_from = g_cl.m_local;
			in.m_pos = point;

			// ignore mindmg.
			if (it.m_mode == HitscanMode::LETHAL || it.m_mode == HitscanMode::LETHAL2)
				in.m_damage = in.m_damage_pen = 1.f;

			penetration::PenetrationOutput_t out;

			// we can hit p!
			if (penetration::run(&in, &out)) {

				// fix head behind body situations
				if (it.m_index <= HITBOX_LOWER_NECK && out.m_hitgroup != HITGROUP_HEAD && g_menu.main.aimbot.overlap.get())
					continue;

				// nope we did not hit head..
				if (it.m_index == HITBOX_HEAD && out.m_hitgroup != HITGROUP_HEAD)
					continue;

				// prefered hitbox, just stop now.
				if (it.m_mode == HitscanMode::PREFER)
					done = true;

				// this hitbox requires lethality to get selected, if that is the case.
				// we are done, stop now.
				else if (it.m_mode == HitscanMode::LETHAL && out.m_damage >= m_player->m_iHealth())
					done = true;

				// 2 shots will be sufficient to kill.
				else if (it.m_mode == HitscanMode::LETHAL2 && (out.m_damage * 2.f) >= m_player->m_iHealth())
					done = true;

				// this hitbox has normal selection, it needs to have more damage.
				else if (it.m_mode == HitscanMode::NORMAL) {
					// we did more damage.
					if (out.m_damage >= scan.m_damage) {
						// save new best data.
						scan.m_damage = out.m_damage;
						scan.m_pos = point;
						scan.m_hitbox = it.m_index;
						scan.m_hitgroup = out.m_hitgroup;

						// if the first point is lethal
						// screw the other ones.
						if (point == points.front() && out.m_damage >= m_player->m_iHealth())
							break;
					}
				}

				// we found a preferred / lethal hitbox.
				if (done) {
					// save new best data.
					scan.m_damage = out.m_damage;
					scan.m_pos = point;
					scan.m_hitbox = it.m_index;
					scan.m_hitgroup = out.m_hitgroup;
					break;
				}
			}
		}

		// ghetto break out of outer loop.
		if (done)
			break;
	}

	// we found something that we can damage.
	// set out vars.
	if (scan.m_damage > 0.f) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		hitbox = scan.m_hitbox;
		hitgroup = scan.m_hitgroup;
		return true;
	}

	return false;
}

void IWS2000::apply() {
	bool attack, attack2;

	// attack states.
	attack = (g_cl.m_cmd->m_buttons & IN_ATTACK);
	attack2 = (g_cl.m_weapon_id == REVOLVER && g_cl.m_cmd->m_buttons & IN_ATTACK2);

	auto is_sniper = g_cl.m_weapon_id == 9 || g_cl.m_weapon_id == 11 || g_cl.m_weapon_id == 38 || g_cl.m_weapon_id == 40;
	auto is_pistol = g_cl.m_weapon_id == 1 || g_cl.m_weapon_id == 2 || g_cl.m_weapon_id == 3 || g_cl.m_weapon_id == 4 || g_cl.m_weapon_id == 30 || g_cl.m_weapon_id == 32 || g_cl.m_weapon_id == 36 || g_cl.m_weapon_id == 63 || g_cl.m_weapon_id == 64 || g_cl.m_weapon_id == 4;

	// ensure we're attacking.
	if (attack || attack2) {
		// choke every shot.
		*g_cl.m_packet = true;

		if (m_target) {
			if (g_menu.main.aimbot.aimbottweaks.get(3) && m_record->broke_lc())
				return;

			if (m_record && !m_record->m_broke_lc)
				g_cl.m_cmd->m_tick = game::TIME_TO_TICKS(m_record->m_sim_time + g_cl.m_lerp);

			g_cl.m_cmd->m_view_angles = m_angle;

			//g_visuals.AddMatrix(m_target, m_target->bone_cache().base());

			// if not silent aim, apply the viewangles.
			if (!g_menu.main.aimbot.silent.get())
				g_csgo.m_engine->SetViewAngles(m_angle);

			if (g_menu.main.aimbot.silent.get() and g_menu.main.aimbot.silentwep.get(1) and is_pistol) { // pistol
				g_csgo.m_engine->SetViewAngles(m_angle);
			}

			if (g_menu.main.aimbot.silent.get() and g_menu.main.aimbot.silentwep.get(0) and is_sniper) { // sniper
				g_csgo.m_engine->SetViewAngles(m_angle);
			}

			if (g_menu.main.aimbot.debugaim.get())
				g_visuals.DrawHitboxMatrix(m_record, g_menu.main.aimbot.debugaimcol.get(), g_menu.main.aimbot.matrix_duration.get(), g_menu.main.aimbot.matrix_duration.get());
		}

		// norecoil.
		if (g_menu.main.aimbot.norecoil.get())
			g_cl.m_cmd->m_view_angles -= g_cl.m_local->m_aimPunchAngle() * g_csgo.weapon_recoil_scale->GetFloat();

		// store fired shot.
		g_shots.OnShotFire(m_target ? m_target : nullptr, m_target ? m_damage : -1.f, g_cl.m_weapon_info->m_bullets, m_target ? m_record : nullptr, m_target ? m_hitbox : -1, m_target ? m_hitgroup : -1, m_target ? m_angle : ang_t(0, 0, 0));

		// set that we fired.
		g_cl.m_shot = true;
	}
}

void IWS2000::NoSpread() {
	bool    attack2;
	vec3_t  spread, forward, right, up, dir;

	// revolver state.
	attack2 = (g_cl.m_weapon_id == REVOLVER && (g_cl.m_cmd->m_buttons & IN_ATTACK2));

	// get spread.
	spread = g_cl.m_weapon->CalculateSpread(g_cl.m_cmd->m_random_seed, attack2);

	// compensate.
	g_cl.m_cmd->m_view_angles -= { -math::rad_to_deg(std::atan(spread.length_2d())), 0.f, math::rad_to_deg(std::atan2(spread.x, spread.y)) };
}