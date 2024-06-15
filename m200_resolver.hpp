#pragma once

namespace M200 {
	inline std::array<C_AnimationLayer, 64> layer3 = { };
	inline std::array<float, 64> last_moving_lby = { };
	inline std::array<float, 64> last_moving_time = { };
	inline std::array<float, 64> last_moving_lby_time = { };
	inline std::array<float, 64> last_lby = { };
	inline std::array<float, 64> next_lby_update_time = { };
	inline std::array<bool, 64> triggered_balance_adjust = { };
	inline std::array<bool, 64> has_real_jitter = { };

	enum ResolveMode {
		None = 0,
		LBY,
		PositiveLBY,
		NegativeLBY,
		LowLBY,
		Freestand,
		LastMovingLBY,
		Backwards,
	};

	inline const char* resolver_mode_names[ ] = {
		"None",
		"LBY",
		"PositiveLBY",
		"NegativeLBY",
		"LowLBY",
		"Freestand",
		"LastMovingLBY",
		"Backwards",
	};

	inline std::array<int, 64> resolver_mode = { };
	inline std::array<float, 64> last_last_moving_lby = { };
	inline std::array<int, 64> lby_updates_within_jitter_range = { };

	void resolve_player( Player* player, IWSRecord* record, bool& lby_updated_out );
	void run_antifreestand(Player* player, IWSRecord* record);
}