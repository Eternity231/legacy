#include "visuals.hpp"

external_visuals::external_visuals() { }
bool get_box_rect( Player* player, Rect& box ) {
	vec3_t origin, mins, maxs;
	vec2_t bottom, top;

	// get interpolated origin.
	origin = player->GetAbsOrigin( );

	// get hitbox bounds.
	player->ComputeHitboxSurroundingBox( &mins, &maxs );

	// correct x and y coordinates.
	mins = { origin.x, origin.y, mins.z };
	maxs = { origin.x, origin.y, maxs.z + 8.f };

	if ( !render::WorldToScreen( mins, bottom ) || !render::WorldToScreen( maxs, top ) )
		return false;

	box.h = bottom.y - top.y;
	box.w = box.h / 2.f;
	box.x = bottom.x - ( box.w / 2.f );
	box.y = bottom.y - box.h;

	return true;
}


void external_visuals::supremacy_visuals( Player* player ) {
	constexpr float MAX_DORMANT_TIME = 10.f;
	constexpr float DORMANT_FADE_TIME = MAX_DORMANT_TIME / 2.f;

	Rect		  box;
	player_info_t info;
	Color		  color;

	// get player index.
	int index = player->index( );

	// get reference to array variable.
	float& opacity = m_opacities[ index - 1 ];
	bool& draw = m_draw[ index - 1 ];

	// opacity should reach 1 in 300 milliseconds.
	constexpr int frequency = 1.f / 0.3f;

	// the increment / decrement per frame.
	float step = frequency * g_csgo.m_globals->m_frametime;

	// is player enemy.
	bool enemy = player->enemy( g_cl.m_local );
	bool dormant = player->dormant( );

	if ( !enemy )
		return;

	if ( g_config.b[ "other_radar" ] && !dormant )
		player->m_bSpotted( ) = true;

	// we can draw this player again.
	if ( !dormant )
		draw = true;

	if ( !draw )
		return;

	// if non-dormant	-> increment
	// if dormant		-> decrement
	dormant ? opacity -= step : opacity += step;

	// is dormant esp enabled for this player.
	bool dormant_esp = enemy && g_config.b[ "esp_dormant" ];

	// clamp the opacity.
	math::clamp( opacity, 0.f, 1.f );
	if ( !opacity && !dormant_esp )
		return;

	// stay for x seconds max.
	float dt = g_csgo.m_globals->m_curtime - player->m_flSimulationTime( );
	if ( dormant && dt > MAX_DORMANT_TIME )
		return;

	// calculate alpha channels.
	int alpha = ( int )( 255.f * opacity );
	int low_alpha = ( int )( 179.f * opacity );

	// get color based on enemy or not.
	color = g_config.imcolor_to_ccolor( g_config.c[ "esp_box_col" ] );

	if ( dormant && dormant_esp ) {
		alpha = 112;
		low_alpha = 80;

		// fade.
		if ( dt > DORMANT_FADE_TIME ) {
			// for how long have we been fading?
			float faded = ( dt - DORMANT_FADE_TIME );
			float scale = 1.f - ( faded / DORMANT_FADE_TIME );

			alpha *= scale;
			low_alpha *= scale;
		}

		// override color.
		color = { 112, 112, 112 };
	}

	// override alpha.
	color.a( ) = alpha;

	// get player info.
	if ( !g_csgo.m_engine->GetPlayerInfo( index, &info ) )
		return;

	// attempt to get player box.
	if ( !get_box_rect( player, box ) ) {
		return;
	}

	//DrawSkeleton( player, alpha );
	//DrawHistorySkeleton( player, alpha );

	if ( g_config.b[ "esp_box" ] )
		render::rect_outlined( box.x, box.y, box.w, box.h, color, { 10, 10, 10, low_alpha } );

	int name_offset{ 0 };

	if ( g_config.b[ XOR( "esp_ping_bar" ) ] )
	{
		const auto ping_pointer = ( *g_csgo.m_resource )->get_ping( player->index( ) ) / 1000.f;
		const auto ping = std::clamp( ping_pointer, 0.0f, 1.f );
		if ( ping >= 0.2f )
		{
			const auto width = box.w * ping;

			Color clr = g_config.imcolor_to_ccolor( g_config.c[ XOR( "esp_ping_bar_col" ) ] );
			clr.a( ) = alpha;

			// draw.
			render::rect_filled( box.x, box.y - 6, box.w, 4, { 10, 10, 10, low_alpha } );
			render::rect( box.x + 1, box.y - 5, width, 2, clr );
			name_offset += 6;
		}
	}

	// draw name.
	if ( g_config.b[ "esp_name" ] ) {
		// fix retards with their namechange meme 
		// the point of this is overflowing unicode compares with hardcoded buffers, good hvh strat
		std::string name{ std::string( info.m_name ).substr( 0, 24 ) };

		Color clr = g_config.imcolor_to_ccolor( g_config.c[ "esp_name_col" ] );
		if ( dormant ) {
			clr.r( ) = 130;
			clr.g( ) = 130;
			clr.b( ) = 130;
		}
		// override alpha.
		clr.a( ) = low_alpha;

		render::esp.string( box.x + box.w / 2, box.y - render::esp.m_size.m_height - name_offset, clr, name, render::ALIGN_CENTER );
	}

	// is health esp enabled for this player.
	if ( g_config.b[ "esp_health" ] ) {
		int y = box.y + 1;
		int h = box.h - 2;

		// retarded servers that go above 100 hp..
		int hp = std::min( 100, player->m_iHealth( ) );

		// calculate hp bar color.
		int r = std::min( ( 510 * ( 100 - hp ) ) / 100, 255 );
		int g = std::min( ( 510 * hp ) / 100, 255 );

		// get hp bar height.
		int fill = ( int )std::round( hp * h / 100.f );

		// render background.
		render::rect_filled( box.x - 6, y - 2, 4, h + 3 + 1, { 10, 10, 10, low_alpha } );

		// render actual bar.
		if ( dormant )
			render::rect( box.x - 5, y + h - fill - 1, 2, fill + 2, { 110, 130, 110 , alpha } );
		else
			render::rect( box.x - 5, y + h - fill - 1, 2, fill + 2, { r, g, 0, alpha } );

		// if hp is below max, draw a string.
		if ( dormant ) {
			if ( hp < 90 )
				render::esp_small.string( box.x - 5, y + ( h - fill ) - 5, { 130, 130, 130, low_alpha }, std::to_string( hp ), render::ALIGN_CENTER );
		}
		else {
			if ( hp < 90 )
				render::esp_small.string( box.x - 5, y + ( h - fill ) - 5, { 255, 255, 255, low_alpha }, std::to_string( hp ), render::ALIGN_CENTER );
		}
	}


	// draw flags.
	{
		std::vector< std::pair< std::string, Color > > flags;

		// NOTE FROM NITRO TO DEX -> stop removing my iterator loops, i do it so i dont have to check the size of the vector
		// with range loops u do that to do that.


		// money.
		if ( g_config.m[ "esp_flags" ][ 0 ] )
			if ( dormant )
				flags.push_back( { tfm::format( XOR( "$%i" ), player->m_iAccount( ) ), { 130,130,130, low_alpha } } );
			else
				flags.push_back( { tfm::format( XOR( "$%i" ), player->m_iAccount( ) ), { 150, 200, 60, low_alpha } } );

		// armor.
		if ( g_config.m[ "esp_flags" ][ 1 ] ) {
			// helmet and kevlar.
			if ( player->m_bHasHelmet( ) && player->m_ArmorValue( ) > 0 )
				if ( dormant )
					flags.push_back( { XOR( "HK" ), { 130,130,130, low_alpha } } );
				else
					flags.push_back( { XOR( "HK" ), { 255, 255, 255, low_alpha } } );
			// only helmet.
			else if ( player->m_bHasHelmet( ) )
				if ( dormant )
					flags.push_back( { XOR( "HK" ), { 130,130,130, low_alpha } } );
				else
					flags.push_back( { XOR( "HK" ), { 255, 255, 255, low_alpha } } );

			// only kevlar.
			else if ( player->m_ArmorValue( ) > 0 )
				if ( dormant )
					flags.push_back( { XOR( "K" ), { 130,130,130, low_alpha } } );
				else
					flags.push_back( { XOR( "K" ), { 255, 255, 255, low_alpha } } );
		}

		// scoped.
		if ( g_config.m[ "esp_flags" ][ 2 ] && player->m_bIsScoped( ) )
			if ( dormant )
				flags.push_back( { XOR( "ZOOM" ), { 130,130,130, low_alpha } } );
			else
				flags.push_back( { XOR( "ZOOM" ), { 60, 180, 225, low_alpha } } );

		// flashed.
		if ( g_config.m[ "esp_flags" ][ 3 ] && player->m_flFlashBangTime( ) > 0.f )
			if ( dormant )
				flags.push_back( { XOR( "BLIND" ), { 130,130,130, low_alpha } } );
			else
				flags.push_back( { XOR( "BLIND" ), { 60, 180, 225, low_alpha } } );

		// reload.
		if ( g_config.m[ "esp_flags" ][ 4 ] ) {
			// get ptr to layer 1.
			C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];

			// check if reload animation is going on.
			if ( layer1->m_weight != 0.f && player->GetSequenceActivity( layer1->m_sequence ) == 967 /* ACT_CSGO_RELOAD */ )
				if ( dormant )
					flags.push_back( { XOR( "RELOAD" ), { 130,130,130, low_alpha } } );
				else
					flags.push_back( { XOR( "RELOAD" ), { 60, 180, 225, low_alpha } } );
		}

		// bomb.
		if ( g_config.m[ "esp_flags" ][ 5 ] && player->HasC4( ) )
			if ( dormant )
				flags.push_back( { XOR( "BOMB" ), { 130,130,130, low_alpha } } );
			else
				flags.push_back( { XOR( "BOMB" ), { 255, 0, 0, low_alpha } } );

		// iterate flags.
		for ( size_t i{ }; i < flags.size( ); ++i ) {
			// get flag job (pair).
			const auto& f = flags[ i ];

			int offset = i * ( render::esp_small.m_size.m_height - 1 );

			// draw flag.
			render::esp_small.string( box.x + box.w + 2, box.y + offset, f.second, f.first );
		}
	}

	// draw bottom bars.
	{
		int  offset1{ 0 };
		int  offset3{ 0 };
		int  offset{ 0 };
		int  distance1337{ 0 };

		// draw weapon.
		{
			Weapon* weapon = player->GetActiveWeapon( );
			if ( weapon ) {
				WeaponInfo* data = weapon->GetWpnData( );
				if ( data ) {
					int bar;
					float scale;

					// the maxclip1 in the weaponinfo
					int max = data->m_max_clip1;
					int current = weapon->m_iClip1( );

					C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];

					// set reload state.
					bool reload = ( layer1->m_weight != 0.f ) && ( player->GetSequenceActivity( layer1->m_sequence ) == 967 );

					// ammo bar.
					if ( max != -1 && g_config.b[ "esp_ammo" ] ) {
						// check for reload.
						if ( reload )
							scale = layer1->m_cycle;

						// not reloading.
						// make the division of 2 ints produce a float instead of another int.
						else
							scale = ( float )current / max;

						// relative to bar.
						bar = ( int )std::round( ( box.w - 2 ) * scale );

						// draw.
						render::rect_filled( box.x - 1, box.y + box.h + 2 + offset, box.w + 2, 4, { 10, 10, 10, low_alpha } );

						Color clr = g_config.imcolor_to_ccolor( g_config.c[ "esp_ammo_col" ] );
						if ( dormant ) {
							clr.r( ) = 120;
							clr.g( ) = 125;
							clr.b( ) = 130;//95, 174, 227,
						}
						clr.a( ) = alpha;
						render::rect( box.x, box.y + box.h + 3 + offset, bar + 2, 2, clr );

						// less then a 5th of the bullets left.
						if ( current <= ( int )std::round( max / 5 ) && !reload )
							if ( dormant )
								render::esp_small.string( box.x + bar, box.y + box.h + offset, { 130, 130, 130, low_alpha }, std::to_string( current ), render::ALIGN_CENTER );
							else
								render::esp_small.string( box.x + bar, box.y + box.h + offset, { 255, 255, 255, low_alpha }, std::to_string( current ), render::ALIGN_CENTER );

						offset += 6;
					}


					auto color = g_config.imcolor_to_ccolor( g_config.c[ XOR( "esp_weapon_col" ) ] );

					// text.
					if ( g_config.m[ "esp_weapon" ][ 0 ] ) {
						offset1 -= 9;
						// construct std::string instance of localized weapon name.
						std::string name{ weapon->GetLocalizedName( ) };

						// smallfonts needs upper case.
						std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );


						if ( dormant )
							render::esp_small.string( box.x + box.w / 2, box.y + box.h + offset + distance1337, { 130, 130, 130, low_alpha }, name, render::ALIGN_CENTER );
						else
							render::esp_small.string( box.x + box.w / 2, box.y + box.h + offset + distance1337, color, name, render::ALIGN_CENTER );

					}

					// icons.
					if ( g_config.m[ "esp_weapon" ][ 1 ] ) {
						offset -= 5;
						// icons are super fat..
						// move them back up.

						std::string icon = tfm::format( XOR( "%c" ), m_weapon_icons[ weapon->m_iItemDefinitionIndex( ) ] );
						if ( dormant )
							render::cs.string( box.x + box.w / 2, box.y + box.h + offset - offset1 + distance1337 + 2, { 130,130,130, low_alpha }, icon, render::ALIGN_CENTER );
						else
							render::cs.string( box.x + box.w / 2, box.y + box.h + offset - offset1 + distance1337 + 2, color, icon, render::ALIGN_CENTER );
					}
				}
			}
		}
	}
}

void external_visuals::initialize( Entity* ent ) {
	if ( ent->IsPlayer( ) ) {
		Player* player = ent->as< Player* >( );

		// dont draw dead players.
		if ( !player->alive( ) )
			return;

		if ( player->m_bIsLocalPlayer( ) )
			return;

		// draw player esp.
		supremacy_visuals( player );
	}

}

void external_visuals::think( ) {
	// draw esp on ents.
	for ( int i{ 1 }; i <= g_csgo.m_entlist->GetHighestEntityIndex( ); ++i ) {
		Entity* ent = g_csgo.m_entlist->GetClientEntity( i );
		if ( !ent )
			continue;

		initialize( ent );
	}
}