#pragma once
#include "includes.h"

class external_visuals {
public:
	external_visuals( );
	static external_visuals& get( ) {
		external_visuals instance;
		return instance;
	}
public:
	void supremacy_visuals( Player* player );
	void initialize( Entity* ent );
    void think( );

	std::array< bool, 64 > m_draw;
	std::array< float, 2048 > m_opacities;

    std::unordered_map< int, char > m_weapon_icons = {
    { DEAGLE, 'F' },
    { ELITE, 'S' },
    { FIVESEVEN, 'U' },
    { GLOCK, 'C' },
    { AK47, 'B' },
    { AUG, 'E' },
    { AWP, 'R' },
    { FAMAS, 'T' },
    { G3SG1, 'I' },
    { GALIL, 'V' },
    { M249, 'Z' },
    { M4A4, 'W' },
    { MAC10, 'L' },
    { P90, 'M' },
    { UMP45, 'Q' },
    { XM1014, ']' },
    { BIZON, 'D' },
    { MAG7, 'K' },
    { NEGEV, 'Z' },
    { SAWEDOFF, 'K' },
    { TEC9, 'C' },
    { ZEUS, 'Y' },
    { P2000, 'Y' },
    { MP7, 'X' },
    { MP9, 'D' },
    { NOVA, 'K' },
    { P250, 'Y' },
    { SCAR20, 'I' },
    { SG553, '[' },
    { SSG08, 'N' },
    { KNIFE_CT, 'J' },
    { FLASHBANG, 'G' },
    { HEGRENADE, 'H' },
    { SMOKE, 'P' },
    { MOLOTOV, 'H' },
    { DECOY, 'G' },
    { FIREBOMB, 'H' },
    { C4, '\\' },
    { KNIFE_T, 'J' },
    { M4A1S, 'W' },
    { USPS, 'Y' },
    { CZ75A, 'Y' },
    { REVOLVER, 'F' },
    { KNIFE_BAYONET, 'J' },
    { KNIFE_FLIP, 'J' },
    { KNIFE_GUT, 'J' },
    { KNIFE_KARAMBIT, 'J' },
    { KNIFE_M9_BAYONET, 'J' },
    { KNIFE_HUNTSMAN, 'J' },
    { KNIFE_FALCHION, 'J' },
    { KNIFE_BOWIE, 'J' },
    { KNIFE_BUTTERFLY, 'J' },
    { KNIFE_SHADOW_DAGGERS, 'J' },
    };

};