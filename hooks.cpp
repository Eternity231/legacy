#include "includes.h"

Hooks                g_hooks{ };;

void Hooks::init() {
	// hook wndproc.
	auto m_hWindow = FindWindowA(XOR("Valve001"), NULL);
	m_old_wndproc = (WNDPROC)g_winapi.SetWindowLongA(m_hWindow, GWL_WNDPROC, util::force_cast<LONG>(Hooks::WndProc));

	// setup normal VMT hooks.
	m_panel.init(g_csgo.m_panel);
	m_panel.add(IPanel::PAINTTRAVERSE, util::force_cast(&Hooks::PaintTraverse));

	m_client.init(g_csgo.m_client);
	m_client.add(CHLClient::FRAMESTAGENOTIFY, util::force_cast(&Hooks::FrameStageNotify));

	m_client_mode.init(g_csgo.m_client_mode);
	m_client_mode.add(IClientMode::OVERRIDEVIEW, util::force_cast(&Hooks::OverrideView));
	m_client_mode.add(IClientMode::CREATEMOVE, util::force_cast(&Hooks::CreateMove));

	m_surface.init(g_csgo.m_surface);
	m_surface.add(ISurface::LOCKCURSOR, util::force_cast(&Hooks::LockCursor));
	m_surface.add(ISurface::ONSCREENSIZECHANGED, util::force_cast(&Hooks::OnScreenSizeChanged));

	m_model_render.init(g_csgo.m_model_render);
	m_render_view.init(g_csgo.m_render_view);
	m_shadow_mgr.init(g_csgo.m_shadow_mgr);
	m_view_render.init(g_csgo.m_view_render);
	m_match_framework.init(g_csgo.m_match_framework);
	m_material_system.init(g_csgo.m_material_system);
	m_fire_bullets.init(g_csgo.TEFireBullets);

	m_device.init(g_csgo.m_device);
	m_device.add(17, util::force_cast(&Hooks::Present));
	m_device.add(16, util::force_cast(&Hooks::Reset));

	// cvar hooks.
	m_debug_spread.init(g_csgo.weapon_debug_spread_show);
	m_net_show_fragments.init(g_csgo.net_showfragments);
}