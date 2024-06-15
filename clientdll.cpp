#include "includes.h"

void Hooks::FrameStageNotify( Stage_t stage ) {

	// save stage.
	if( stage != FRAME_START )
		g_cl.m_stage = stage;

	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	if( stage == FRAME_RENDER_START ) { }

	for (auto& hk : g_lua_hook.getHooks("on_frame_stage_notify"))
		hk.func((int)stage);

	// call og.
	g_hooks.m_client.GetOldMethod< FrameStageNotify_t >( CHLClient::FRAMESTAGENOTIFY )( this, stage );

	if( stage == FRAME_RENDER_START ) { }
	else if( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START ) { }
	else if( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END ) { }
	else if( stage == FRAME_NET_UPDATE_END ) { }
}