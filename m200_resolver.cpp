#include "includes.h"

float yaw_diff( float yaw1, float yaw2 ) {
	return abs( math::normalize( math::normalize( yaw2 ) - math::normalize( yaw1 ) ) );
}

bool is_angle_within_range( float yaw1, float yaw2, float range ) {
	return yaw_diff( yaw1, yaw2 ) < range;
}

bool is_angle_same( float yaw1, float yaw2 ) {
	return is_angle_within_range( yaw1, yaw2, 35.0f ); // 35 degrees off center
}

void M200::resolve_player( Player* player, IWSRecord* record, bool& lby_updated_out ) {
	static auto resolver_enabled = g_menu.main.aimbot.correct.get( );

	const auto index = player->index( ) + 1;
	const float simtime = player->m_flSimulationTime( );
	const float lby = math::normalize( player->m_flLowerBodyYawTarget( ) );
	const auto on_ground = !!( player->m_fFlags( ) & FL_ONGROUND );
	const auto animlayers = player->layers( ); /* this should work fine */
	float speed = record->m_anim_velocity.length();

	if ((record->m_flags & FL_ONGROUND) && speed > 0.1f && !record->m_fake_walk)
		record->m_resolved = true;
	if (!(record->m_flags & FL_ONGROUND))
		record->m_resolved = false;

	/* store info on players */
	if ( on_ground && animlayers ) {
		layer3[ index - 1 ] = animlayers[ 3 ];

		if ( animlayers[ 6 ].m_weight > 0.0f ) {
			if ( last_moving_lby[ index - 1 ] != std::numeric_limits<float>::max( )
				 && last_last_moving_lby[ index - 1 ] != std::numeric_limits<float>::max( ) ) {
				if ( !is_angle_within_range( lby, last_moving_lby[ index - 1 ], 2.0f )
					 //&& !is_angle_within_range ( lby, last_last_moving_lby [ index - 1 ], 2.0f )
					 && !is_angle_within_range( last_moving_lby[ index - 1 ], last_last_moving_lby[ index - 1 ], 2.0f )
					 && is_angle_within_range( lby, last_moving_lby[ index - 1 ], 35.0f )
					 && is_angle_within_range( lby, last_last_moving_lby[ index - 1 ], 35.0f )
					 && is_angle_within_range( last_moving_lby[ index - 1 ], last_last_moving_lby[ index - 1 ], 35.0f ) ) {
					has_real_jitter[ index - 1 ] = true;
				} else if ( is_angle_within_range( lby, last_moving_lby[ index - 1 ], 5.0f )
						   && is_angle_within_range( lby, last_last_moving_lby[ index - 1 ], 5.0f )
						   && is_angle_within_range( last_moving_lby[ index - 1 ], last_last_moving_lby[ index - 1 ], 5.0f ) ) {
					has_real_jitter[ index - 1 ] = false;
				}
			}

			last_moving_time[ index - 1 ] = simtime;
			last_last_moving_lby[ index - 1 ] = last_moving_lby[ index - 1 ];
			last_moving_lby[ index - 1 ] = lby;

			last_moving_lby_time[ index - 1 ] = simtime;
			next_lby_update_time[ index - 1 ] = simtime + 0.22f;
			triggered_balance_adjust[ index - 1 ] = false;
			lby_updates_within_jitter_range[ index - 1 ] = 0;

			lby_updated_out = true;
		}
		else if ( !is_angle_within_range( lby, last_lby[ index - 1 ], 35.0f ) // lby != last_lby [ index - 1 ] // If LBY changed, we know it updated, and the next update must be 1.1f second from this one
				  // ^^ Also, we can correct the LBY timer if it goes out of sync
				  || simtime >= next_lby_update_time[ index - 1 ] ) { // If time since updated elapsed, this is probably an update
			 next_lby_update_time[ index - 1 ] = simtime + 1.1f;
			 lby_updated_out = true;
			 
			 // If the LBY changed but still within 35 degrees of the original value,
			 // The angle was probably around the same but within 35 degrees
			 // So they probably have something like jitter on their real
			 const bool lby_update_is_close = is_angle_within_range( lby, last_lby[ index - 1 ], 35.0f );

			 if ( lby != last_lby[ index - 1 ]
				 && lby_update_is_close ) {
				lby_updates_within_jitter_range[ index - 1 ]++;
			 } else if ( !lby_update_is_close ) {
				lby_updates_within_jitter_range[ index - 1 ] = 0;
			 }

			 // LBY changed to very close amounts 3 times in a row, so they might have jitter on their real
			 if ( lby_updates_within_jitter_range[ index - 1 ] >= 3 ) {
				has_real_jitter[ index - 1 ] = true;
			 }

			 // Sequence 4 on layer 3 is balance adjust
			 const float time_since_moved = abs( simtime - last_moving_lby_time[ index - 1 ] );

			 // Balance adjust was triggered after the first LBY update
			 // and before too long (so we don't record balance adjusts for the player simply rotating their view)
			 if ( animlayers[ 3 ].m_sequence == 4 && time_since_moved > 0.22f && time_since_moved < 3.0f ) {
				triggered_balance_adjust[ index - 1 ] = true;
			 }
		}
	}
	else {
		// Reset LBY updates information
		last_moving_lby_time[ index - 1 ] = next_lby_update_time[ index - 1 ] = std::numeric_limits<float>::max( );
		last_last_moving_lby[ index - 1 ] = last_moving_lby[ index - 1 ] = std::numeric_limits<float>::max( );
		triggered_balance_adjust[ index - 1 ] = false;
		lby_updates_within_jitter_range[ index - 1 ] = 0;
		//has_real_jitter [ index - 1 ] = false;
	}

	// Store old values for later
	last_lby[ index - 1 ] = lby;

	float final_angle = record->m_eye_angles.y;

	std::vector<float> possible_resolver_angles = {};

	possible_resolver_angles.push_back( final_angle );
	possible_resolver_angles.push_back( lby );
	possible_resolver_angles.push_back( math::normalize( lby + 120.0f ) );
	possible_resolver_angles.push_back( math::normalize( lby - 120.0f ) );
	possible_resolver_angles.push_back( math::normalize( lby ) ); // !!!!
	possible_resolver_angles.push_back( last_last_moving_lby[ index - 1 ] );
	possible_resolver_angles.push_back( math::calc_angle( g_cl.m_local->m_vecOrigin( ), player->m_vecOrigin( ) ).y );

	// Updated LBY is nearby last moving lby within first update, they are probably breaking with low delta
	if ( last_moving_lby_time[ index - 1 ] != std::numeric_limits<float>::max( )
		 && yaw_diff( lby, last_moving_lby[ index - 1 ] ) < 60.0f ) {
		// Pull LBY towards last moving LBY
		possible_resolver_angles[ 4 ] = math::normalize( lby + copysign( 60.0f, math::normalize( last_moving_lby[ index - 1 ] - lby ) ) );
		resolver_mode[ index - 1 ] = ResolveMode::LowLBY;
	}

	if ( lby_updated_out ) {
		final_angle = lby;

		if ( animlayers[ 6 ].m_weight == 0.0f /*&& animlayers [ 3 ].m_sequence == 4*/ ) {
			// Updated LBY is nearby last moving lby within first update, they are probably breaking with low delta
			if ( last_moving_lby_time[ index - 1 ] != std::numeric_limits<float>::max( )
				 && yaw_diff( lby, last_moving_lby[ index - 1 ] ) < 60.0f ) {
				resolver_mode[ index - 1 ] = ResolveMode::LowLBY;
			}
		}
	} else {
		// If freestand is the same as last moving lby, use last moving lby as more accurate guess
		//if ( last_freestand_time[ index - 1 ] != std::numeric_limits<float>::max( )
		//	 && last_moving_lby_time[ index - 1 ] != std::numeric_limits<float>::max( )
		//	 && is_angle_same( last_freestanding[ index - 1 ], last_moving_lby[ index - 1 ] ) ) {
		//	resolver_mode[ index - 1 ] = ResolveMode::LastMovingLBY;
		//}

		// No lby breaker, so they probably have no fake
		if ( on_ground
			 && last_moving_lby_time[ index - 1 ] != std::numeric_limits<float>::max( )
			 && abs( simtime - last_moving_lby_time[ index - 1 ] ) > 0.5f
			 && is_angle_same( lby, last_moving_lby[ index - 1 ] ) ) {
			if ( is_angle_same( lby, record->m_eye_angles.y ) )
				resolver_mode[ index - 1 ] = ResolveMode::None;
			else
				resolver_mode[ index - 1 ] = ResolveMode::LBY;
		}

		// Real while moving has low variance, and we don't know their resolve mode
		// Just force LBY and hope for the best
		if ( on_ground
			 && !has_real_jitter[ index - 1 ]
			 && ( resolver_mode[ index - 1 ] == ResolveMode::None || resolver_mode[ index - 1 ] == ResolveMode::Backwards ) ) {
			resolver_mode[ index - 1 ] = ResolveMode::LBY;
		}

		// In air for more than 3 seconds or don't have a good side to hide the head on
		if ( ( !on_ground && abs( simtime - last_moving_time[ index - 1 ] ) > 3.0f )
			 ) {
			resolver_mode[ index - 1 ] = ResolveMode::Backwards;
		}

		// Apply wanted yaw
		final_angle = possible_resolver_angles[ resolver_mode[ index - 1 ] ];

		// Apply jitter if needed
		if ( has_real_jitter[ index - 1 ] ) {
			const bool rand_sign = ( rand( ) % 2 == 0 ) ? -1.0f : 1.0f;
			const float jitter_range = 35.0f;
			const float rand_factor = static_cast< float >( rand( ) ) / static_cast< float >( RAND_MAX );

			final_angle += rand_factor * ( jitter_range / 2.0f ) * rand_sign;
		}
	}

	// Only apply final angle if resolver is enabled
	if ( resolver_enabled ) {
		record->m_eye_angles.y = math::normalize( final_angle );
	}
}
