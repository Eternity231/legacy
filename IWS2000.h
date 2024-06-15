#pragma once

enum HitscanMode : int {
	NORMAL  = 0,
	LETHAL  = 1,
	LETHAL2 = 3,
	PREFER  = 4
};

struct AnimationBackup_t {
	vec3_t           m_origin, m_abs_origin;
	vec3_t           m_velocity, m_abs_velocity;
	int              m_flags, m_eflags;
	float            m_duck, m_body;
	C_AnimationLayer m_layers[ 13 ];
};

struct HitscanData_t {
	float  m_damage;
	int    m_hitbox;
	int    m_hitgroup;
	vec3_t m_pos;

	__forceinline HitscanData_t() : m_damage{ 0.f }, m_pos{}, m_hitbox{}, m_hitgroup{}{}
};

struct HitscanBox_t {
	int         m_index;
	HitscanMode m_mode;

	__forceinline bool operator==( const HitscanBox_t& c ) const {
		return m_index == c.m_index && m_mode == c.m_mode;
	}
};

struct HitscanPoint_t {
	int         m_index;
	vec3_t      m_point;
	bool        m_center;
	bool        m_not_capsule;
	int       m_damage;


	__forceinline bool operator==(HitscanPoint_t& c) {
		return m_index == c.m_index && m_point == c.m_point && m_center == c.m_center && m_not_capsule == c.m_not_capsule;
	}

};

struct interpolated_t
{
	__forceinline constexpr interpolated_t() = default;

	__forceinline constexpr interpolated_t(
		const float sim_time, const float duck_amount,
		const int flags, const vec3_t& velocity
	) : m_sim_time{ sim_time }, m_duck_amount{ duck_amount },
		m_flags{ flags }, m_velocity{ velocity } {}

	float				m_sim_time{},
		m_duck_amount{};

	int	m_flags{};
	vec3_t				m_velocity{};

};

class IWS2000Targer {
public:
	using records_t   = std::deque< std::shared_ptr< IWSRecord > >;
	using hitboxcan_t = stdpp::unique_vector< HitscanBox_t >;

public:
	float m_last_shot_time;

	// essential data.
	Player* m_player;
	float	  m_spawn_time;
	records_t m_records;

	// aimbot data.
	hitboxcan_t m_hitboxes;

	std::string hitscan_mode;

	// resolve data.
	int       m_shots;
	float m_spawn;
	int       m_missed_shots;
	vec3_t      m_pos{};
	float     m_body_update;
	bool      m_moved;
	float     m_alive_loop_cycle;


	int m_logic_index;
	int m_reversefs_index;
	bool m_should_correct;
	int m_testfs_index;
	int m_spin_index;
	int m_reversefs_at_index;
	int m_stand3_reversefs;
	int m_stand3_back;
	int m_airlast_index;
	int m_air_index;
	int m_airlby_index;
	int m_airback_index;
	int m_back_index;
	int m_back_at_index;
	int m_lastmove_index;
	int m_sidelast_index;
	int m_lby_index;
	int m_lowlby_index;
	int m_stand_index1;
	int m_stand_index2;
	int m_stand_index3;
	int m_stand_index4;
	int m_body_index;
	int m_test_index;
	int m_moving_index;
	int m_experimental_index;

	// data about the freestand resolver
	bool m_freestand_found = false;
	float m_freestand_angle = 0.f;
	bool  m_blacklist_back = false;
	bool  m_blacklist_fs = false;

	// data about the LBY proxy.
	float m_body_proxy = 0.f;
	float m_body_proxy_old = 0.f;
	bool  m_body_proxy_updated = false;

	// data about last moving.
	float m_moving_time = -1.f;
	float m_moving_body = 0.f;
	vec3_t m_moving_origin = vec3_t();
	float m_valid_last_moving = false;


	// shot data
	float m_last_valid_pitch = 89.f;
	int m_ticks_since_dormant = 0;

	bool m_prefer_body = false;
	bool m_prefer_head = false;
	std::vector< interpolated_t >					m_simulation_data{};

	//std::deque< float >            m_lbyt_update;
	//std::deque< float >			   m_prefer_stand;
	//std::deque< float >            m_prefer_air;

public:
	void CorrectAnimlayers(IWSRecord* record);
	void CorrectVelocity(Player* m_player, IWSRecord* record, IWSRecord* previous, IWSRecord* pre_previous, float max_speed);
	void UpdateAnimations( IWSRecord* record );
	void OnNetUpdate( Player* player );
	void OnRoundStart( Player* player );
	void SetupHitboxes( IWSRecord* record, bool history );
	bool SetupHitboxPoints(IWSRecord* record, BoneArray* bones, int index, std::vector< vec3_t >& points);
	bool GetBestAimPosition( vec3_t& aim, float& damage, int& hitbox, int& hitgroup, IWSRecord* record );

public:
	void reset() {
		m_player = nullptr;
		m_spawn_time = 0.f;
		m_shots = 0;
		m_missed_shots = 0;
		m_spawn = 0;

		m_records.clear();
		m_hitboxes.clear();
	}
};

class IWS2000 {
private:
	struct target_t {
		Player*    m_player;
		IWS2000Targer* m_data;
	};

	struct knife_target_t {
		target_t  m_target;
		IWSRecord m_record;
	};

	struct table_t {
		uint8_t swing[ 2 ][ 2 ][ 2 ]; // [ first ][ armor ][ back ]
		uint8_t stab[ 2 ][ 2 ];		  // [ armor ][ back ]
	};

	const table_t m_knife_dmg{ { { { 25, 90 }, { 21, 76 } }, { { 40, 90 }, { 34, 76 } } }, { { 65, 180 }, { 55, 153 } } };

	std::array< ang_t, 12 > m_knife_ang{
		ang_t{ 0.f, 0.f, 0.f }, ang_t{ 0.f, -90.f, 0.f }, ang_t{ 0.f, 90.f, 0.f }, ang_t{ 0.f, 180.f, 0.f },
		ang_t{ -80.f, 0.f, 0.f }, ang_t{ -80.f, -90.f, 0.f }, ang_t{ -80.f, 90.f, 0.f }, ang_t{ -80.f, 180.f, 0.f },
		ang_t{ 80.f, 0.f, 0.f }, ang_t{ 80.f, -90.f, 0.f }, ang_t{ 80.f, 90.f, 0.f }, ang_t{ 80.f, 180.f, 0.f }
	};

public:
	std::array< IWS2000Targer, 64 > m_players;
	std::vector< IWS2000Targer* >   m_targets;

	BackupRecord m_backup[ 64 ];

	// target selection stuff.
	float m_best_dist;
	float m_best_fov;
	float m_best_damage;
	int   m_best_hp;
	float m_best_lag;
	float m_best_height;
	
	// found target stuff.
	Player*    m_target;
	ang_t      m_angle;
	int        m_hitbox, m_hitgroup;
	vec3_t     m_aim;
	float      m_damage;
	bool       m_damage_toggle;
	IWSRecord* m_record;
	bool	   m_force_body;

	// fake latency stuff.
	bool       m_fake_latency;

	bool m_stop;

public:
	__forceinline void reset( ) {
		// reset aimbot data.
		init( );

		// reset all players data.
		for( auto& p : m_players )
			p.reset( );
	}

	__forceinline bool IsValidTarget( Player* player ) {
		if (!player)
			return false;

		if (!player->IsPlayer())
			return false;

		if (!player->alive())
			return false;

		if (player->m_bIsLocalPlayer())
			return false;

		if (!player->enemy(g_cl.m_local))
			return false;

		if (player->dormant())
			return false;

		if (!player->GetClientClass())
			return false;

		if (player->m_fImmuneToGunGameDamageTime())
			return false;

		return true;
	}

public:
	// aimbot.
	void init( );
	void StripAttack( );
	void think( );
	bool SelectTarget(IWSRecord* record, const vec3_t& aim, float damage);
	bool CanHit(const vec3_t start, const vec3_t end, IWSRecord* animation, int box, bool in_shot, BoneArray* bones);
	bool CanHitPlayer(IWSRecord* pRecord, int iSide, const vec3_t& vecEyePos, const vec3_t& vecEnd, int iHitboxIndex);
	void find( );
	bool CheckHitchance(Player* player, const ang_t& angle, int seeds);
	void apply( );
	void start_target_selection();
	void finish_target_selection();
	void NoSpread( );

	// knifebot.
	void knife( );
	bool CanKnife( IWSRecord* record, ang_t angle, bool& stab );
	bool KnifeTrace( vec3_t dir, bool stab, CGameTrace* trace );
	bool KnifeIsBehind( IWSRecord* record );
};

static int m_static_seeds[] = {
0x2 ,
0x3 ,
0x4 ,
0x0A,
0x0D,
0x0F,
0x1B,
0x1D,
0x25,
0x26,
0x29,
0x2E,
0x2F,
0x31,
0x32,
0x34,
0x38,
0x39,
0x3A,
0x3E,
0x42,
0x44,
0x46,
0x48,
0x4C,
0x53,
0x57,
0x61,
0x6C,
0x6D,
0x6E,
0x6F,
0x71,
0x79,
0x7A,
0x7D,
0x87,
0x1 ,
0x5 ,
0x6 ,
0x8 ,
0x11,
0x19,
0x1A,
0x1C,
0x1F,
0x20,
0x24,
0x27,
0x2C,
0x33,
0x3D,
0x43,
0x47,
0x4A,
0x4D,
0x4E,
0x4F,
0x50,
0x51,
0x54,
0x59,
0x5A,
0x5B,
0x5C,
0x60,
0x62,
0x65,
0x66,
0x68,
0x6B,
0x72,
0x73,
0x77,
0x9 ,
0x0B,
0x10,
0x13,
0x14,
0x16,
0x1E,
0x30,
0x36,
0x3B,
0x3C,
0x41,
0x45,
0x49,
0x4B,
0x52,
0x55,
0x56,
0x58,
0x5E,
0x63,
0x67,
0x69,
0x70,
0x74,
0x75,
0x76,
0x7C,
0x0 ,
0x7 ,
0x0C,
0x0E,
0x12,
0x15,
0x17,
0x18,
0x21,
0x22,
0x23,
0x28,
0x2A,
0x2B,
0x2D,
0x35,
0x37,
0x3F,
0x40,
0x5D,
0x5F,
0x64,
0x6A,
0x78,
0x85,
0x89
};

extern IWS2000 g_aimbot;