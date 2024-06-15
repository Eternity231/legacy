#include "includes.h"

void Hooks::OverrideView( CViewSetup* view ) {
	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

    // call original.
	g_hooks.m_client_mode.GetOldMethod< OverrideView_t >( IClientMode::OVERRIDEVIEW )( this, view );

	for (auto hk : g_lua_hook.getHooks("on_override_view"))
		hk.func(view);
}

bool Hooks::CreateMove(float time, CUserCmd* cmd) {
	Stack   stack;
	// called from CInput::ExtraMouseSample -> return original.
	if (!cmd || !cmd->m_command_number)
		return g_hooks.m_client_mode.GetOldMethod< CreateMove_t >(IClientMode::CREATEMOVE)(this, time, cmd);

	// if we arrived here, called from -> CInput::CreateMove
	// call EngineClient::SetViewAngles according to what the original returns.
	if (g_hooks.m_client_mode.GetOldMethod< CreateMove_t >(IClientMode::CREATEMOVE)(this, time, cmd))
		g_csgo.m_engine->SetViewAngles(cmd->m_view_angles);

	//add lua manager

	// random_seed isn't generated in ClientMode::CreateMove yet, we must set generate it ourselves.
	cmd->m_random_seed = g_csgo.MD5_PseudoRandom(cmd->m_command_number) & 0x7fffffff;

	// get bSendPacket off the stack.
	g_cl.m_packet = stack.next().local(0x1c).as< bool* >();

	// get bFinalTick off the stack.
	g_cl.m_final_packet = stack.next().local(0x1b).as< bool* >();

	return false;
}