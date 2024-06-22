#include "cmenu.hpp"
#include "bg_texture.hpp"
#include "fonts.hpp"
#include <shellapi.h>

void Menu::initialize(IDirect3DDevice9* pDevice) {
	if (this->m_bInitialized)
		return;

	ui::CreateContext();
	auto io = ui::GetIO();
	auto style = &ui::GetStyle();

	style->WindowRounding = 0.f;
	style->AntiAliasedLines = true;
	style->AntiAliasedFill = true;
	style->ScrollbarRounding = 0.f;
	style->ScrollbarSize = 6.f;
	style->WindowPadding = ImVec2(0, 0);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(45 / 255.f, 45 / 255.f, 45 / 255.f, 1.f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(65 / 255.f, 65 / 255.f, 65 / 255.f, 1.f);

	this->m_bIsOpened = true;

	IDirect3DSwapChain9* pChain = nullptr;
	D3DPRESENT_PARAMETERS pp = {};
	D3DDEVICE_CREATION_PARAMETERS param = {};
	pDevice->GetCreationParameters(&param);
	pDevice->GetSwapChain(0, &pChain);

	if (pChain)
		pChain->GetPresentParameters(&pp);

	ImGui_ImplWin32_Init(param.hFocusWindow);
	ImGui_ImplDX9_Init(pDevice);

	D3DXCreateTextureFromFileInMemoryEx(
		pDevice, texture, sizeof(texture), 4096, 4096, D3DX_DEFAULT, NULL,
		pp.BackBufferFormat, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, NULL, NULL, NULL, &this->m_pTexture);

	ImFontConfig cfg;
	io.Fonts->AddFontFromFileTTF("C:/windows/fonts/verdana.ttf", 12.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
	io.Fonts->AddFontFromFileTTF("C:/windows/fonts/verdanab.ttf", 12.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
	io.Fonts->AddFontFromMemoryTTF(keybinds_font, 25600, 10.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
	io.Fonts->AddFontDefault();

	ImGuiFreeType::BuildFontAtlas(io.Fonts, 0x00);

	this->m_bInitialized = true;
}

void Menu::draw_begin() {
	if (!this->m_bInitialized)
		return;

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ui::NewFrame();
}

static void dmt(std::string key) {
	if (g_config.b[XOR("lua_devmode")] && ui::IsItemHovered()) {
		ui::BeginTooltip();
		ui::Text(key.c_str());
		ui::EndTooltip();
	}
}

static void draw_lua_items(std::string tab, std::string container) {
	for (auto i : g_lua.menu_items[tab][container]) {
		if (!i.is_visible)
			continue;

		auto type = i.type;
		switch (type)
		{
		case MENUITEM_CHECKBOX:
			if (ui::Checkbox(i.label.c_str(), &g_config.b[i.key])) {
				if (i.callback != sol::nil)
					i.callback(g_config.b[i.key]);
			}

			dmt(i.key);
			break;
		case MENUITEM_SLIDERINT:
			if (ui::SliderInt(i.label.c_str(), &g_config.i[i.key], i.i_min, i.i_max, i.format.c_str())) {
				if (i.callback != sol::nil)
					i.callback(g_config.i[i.key]);
			}

			dmt(i.key);
			break;
		case MENUITEM_SLIDERFLOAT:
			if (ui::SliderFloat(i.label.c_str(), &g_config.f[i.key], i.f_min, i.f_max, i.format.c_str())) {
				if (i.callback != sol::nil)
					i.callback(g_config.f[i.key]);
			}

			dmt(i.key);
			break;
		case MENUITEM_KEYBIND:
			if (ui::Keybind(i.label.c_str(), &g_config.i[i.key], i.allow_style_change ? &g_config.i[i.key + XOR("style")] : NULL)) {
				if (i.callback != sol::nil)
					i.callback(g_config.i[i.key], g_config.i[i.key + XOR("style")]);
			}

			dmt(i.key + (i.allow_style_change ? " | " + i.key + XOR("style") : XOR("") ));
			break;
		case MENUITEM_TEXT:
			ui::Text(i.label.c_str());
			break;
		case MENUITEM_SINGLESELECT:
			if (ui::SingleSelect(i.label.c_str(), &g_config.i[i.key], i.items)) {
				if (i.callback != sol::nil)
					i.callback(g_config.i[i.key]);
			}

			dmt(i.key);
			break;
		case MENUITEM_MULTISELECT:
			if (ui::MultiSelect(i.label.c_str(), &g_config.m[i.key], i.items)) {
				if (i.callback != sol::nil)
					i.callback(g_config.m[i.key]);
			}

			dmt(i.key);
			break;
		case MENUITEM_COLORPICKER:
			if (ui::ColorEdit4(i.label.c_str(), g_config.c[i.key])) {
				if (i.callback != sol::nil)
					i.callback(g_config.c[i.key][0] * 255, g_config.c[i.key][1] * 255, g_config.c[i.key][2] * 255, g_config.c[i.key][3] * 255);
			}

			dmt(i.key);
			break;
		case MENUITEM_BUTTON:
			if (ui::Button(i.label.c_str())) {
				if (i.callback != sol::nil)
					i.callback();
			}
			break;
		default:
			break;
		}
	}
}

void Menu::draw( ) {
	if ( !this->m_bIsOpened && ui::GetStyle( ).Alpha > 0.f ) {
		float fc = 255.f / 0.2f * ui::GetIO( ).DeltaTime;
		ui::GetStyle( ).Alpha = std::clamp( ui::GetStyle( ).Alpha - fc / 255.f, 0.f, 1.f );
	}

	if ( this->m_bIsOpened && ui::GetStyle( ).Alpha < 1.f ) {
		float fc = 255.f / 0.2f * ui::GetIO( ).DeltaTime;
		ui::GetStyle( ).Alpha = std::clamp( ui::GetStyle( ).Alpha + fc / 255.f, 0.f, 1.f );
	}

	if ( !this->m_bIsOpened && ui::GetStyle( ).Alpha == 0.f )
		return;

	ui::SetNextWindowSizeConstraints( ImVec2( 560, 500 ), ImVec2( 4096, 4096 ) );
	ui::Begin( "anime.today | lua addon", 0, ImGuiWindowFlags_NoTitleBar );

	ui::TabButton("IWS2000", &this->m_nCurrentTab, 0, 2);
	ui::TabButton("Lycoris", &this->m_nCurrentTab, 1, 2);
	ui::TabButton("Visuals", &this->m_nCurrentTab, 2, 2);
	ui::TabButton( "Misc", &this->m_nCurrentTab, 3, 2 );
	ui::TabButton( "Config", &this->m_nCurrentTab, 4, 2 );

	static auto calculateChildWindowPosition = [ ]( int num ) -> ImVec2 {
		return ImVec2( ui::GetWindowPos( ).x + 26 + ( ui::GetWindowSize( ).x / 3 - 31 ) * num + 20 * num, ui::GetWindowPos( ).y + 52 );
	};

	static auto calculateChildWindowPositionDouble = [ ]( int num ) -> ImVec2 {
		return ImVec2( ui::GetWindowPos( ).x + 26 + ( ui::GetWindowSize( ).x / 2 - 36 ) * num + 20 * num, ui::GetWindowPos( ).y + 52 );
	};

	auto child_size = ImVec2( ui::GetWindowSize( ).x / 3 - 31, ui::GetWindowSize( ).y - 80 );
	auto child_size_d = ImVec2( ui::GetWindowSize( ).x / 2 - 36, ui::GetWindowSize( ).y - 80 );
	auto cfg = g_config;

	switch ( this->m_nCurrentTab ) {
		case 1: {

			ui::BeginChild("Rage", ImVec2(ui::GetWindowSize().x / 2 - 21, ui::GetWindowSize().y - 80)); {
				ui::Checkbox("Enable", &g_config.b[XOR("iws_enable")]);

			} ui::EndChild();
			ui::SameLine();
			ui::BeginChild("other", ImVec2(ui::GetWindowSize().x / 2 - 21, ui::GetWindowSize().y - 80)); {

			} ui::EndChild();

		} break;
		case 2: {

			ui::BeginChild( "main", ImVec2( ui::GetWindowSize( ).x / 2 - 21, ui::GetWindowSize( ).y - 80 ) ); {
				ui::Checkbox( "Dormant", &g_config.b[ XOR( "esp_dormant" ) ] );

				ui::Checkbox( "Bounding box", &g_config.b[ XOR( "esp_box" ) ] );
				ui::ColorEdit4( "##esp_box_col", g_config.c[ XOR( "esp_box_col" ) ] ); dmt( "esp_box_col" );

				ui::Checkbox( "Name", &g_config.b[ XOR( "esp_name" ) ] );
				ui::ColorEdit4( "##esp_name_col", g_config.c[ XOR( "esp_name_col" ) ] ); dmt( "esp_name_col" );

				ui::Checkbox( "Health", &g_config.b[ XOR( "esp_health" ) ] );

				ui::MultiSelect( "Weapon", &g_config.m[ XOR( "esp_weapon" ) ], { XOR( "Text" ), XOR( "Icon" ), } ); dmt( "esp_weapon" );
				ui::ColorEdit4( "##esp_weapon_col", g_config.c[ XOR( "esp_weapon_col" ) ] ); dmt( "esp_weapon_col" );

				ui::Checkbox( "Ammo bar", &g_config.b[ XOR( "esp_ammo" ) ] ); dmt( "esp_ammo" );
				ui::ColorEdit4( "##esp_ammo_col", g_config.c[ XOR( "esp_ammo_col" ) ] );

				ui::MultiSelect( "Flags", &g_config.m[ XOR( "esp_flags" ) ], { XOR( "Money" ),  XOR( "Armor" ),  XOR( "Scoped" ),  XOR( "Flashed" ),  XOR( "Reload" ),  XOR( "Bomb" ) } ); dmt( "esp_flags" );

				ui::Checkbox( "Ping bar", &g_config.b[ XOR( "esp_ping_bar" ) ] ); dmt( "esp_ping_bar" );
				ui::ColorEdit4( "##esp_ping_bar_col", g_config.c[ XOR( "esp_ping_bar_col" ) ] ); dmt( "esp_ping_bar_col" );
			} ui::EndChild( );
			ui::SameLine( );
			ui::BeginChild( "other", ImVec2( ui::GetWindowSize( ).x / 2 - 21, ui::GetWindowSize( ).y - 80 ) ); {

			} ui::EndChild( );

		} break;
		case 4: {
			ui::BeginChild( "config", ImVec2( ui::GetWindowSize( ).x / 2 - 21, ui::GetWindowSize( ).y - 80 ) ); {
				ui::Text( "menu key" );
				ui::Keybind( "menukey", &g_config.i[ XOR( "misMenukey" ) ] ); dmt( "misMenukey" );
				ui::Text( "menu color" );
				ui::ColorEdit4( "##menucolor", g_config.c[ XOR( "menu_color" ) ] ); dmt( "menu_color" );

				ui::SingleSelect( "preset", &g_config.i[ "_preset" ], g_config.presets ); dmt( "_preset" );
				ui::Keybind( "##presetkey", &g_config.i[ "_preset_" + std::to_string( g_config.i[ "_preset" ] ) ] ); dmt( "_preset_" + std::to_string( g_config.i[ "_preset" ] ) );

				if ( ui::Button( "load" ) )
					g_config.load( );

				if ( ui::Button( "save" ) )
					g_config.save( );

				if ( ui::Button( "open settings folder" ) )
					ShellExecuteA( 0, "open", "C:/anime.today", NULL, NULL, SW_NORMAL );

				draw_lua_items( "misc", "cheat settings" );
			} ui::EndChild( );
			ui::SameLine( );
			ui::BeginChild( "lua manager", ImVec2( ui::GetWindowSize( ).x / 2 - 21, ui::GetWindowSize( ).y - 80 ) ); {
				ui::Checkbox( "developer mode", &g_config.b[ "lua_devmode" ] ); dmt( "lua_devmode" );
				if ( ui::Button( "refresh scripts" ) )
					g_lua.refresh_scripts( );
				if ( ui::Button( "reload active scripts" ) )
					g_lua.reload_all_scripts( );
				if ( ui::Button( "unload all" ) )
					g_lua.unload_all_scripts( );

				ui::ListBoxHeader( "##urnn", ImVec2( 0, 160 ) );
				{
					for (auto& s : g_lua.scripts)
					{
						if ( ui::Selectable( s.c_str( ), g_lua.loaded.at( g_lua.get_script_id( s ) ), NULL, ImVec2( 0, 0 ) ) ) {
							auto scriptId = g_lua.get_script_id( s );
							if ( g_lua.loaded.at( scriptId ) )
								g_lua.unload_script( scriptId );
							else
								g_lua.load_script( scriptId );
						}
					}
				}
				ui::ListBoxFooter( );
				draw_lua_items( "misc", "lua extensions" );
			} ui::EndChild( );
		} break;
	}


	ui::End( );
}

void Menu::draw_end() {
	if (!this->m_bInitialized)
		return;

	ui::EndFrame();
	ui::Render();
	ImGui_ImplDX9_RenderDrawData(ui::GetDrawData());
}

bool Menu::is_menu_initialized() {
	return this->m_bInitialized;
}

bool Menu::is_menu_opened() {
	return this->m_bIsOpened;
}

void Menu::set_menu_opened(bool v) {
	this->m_bIsOpened = v;
}

IDirect3DTexture9* Menu::get_texture_data() {
	return this->m_pTexture;
}

ImColor Menu::get_accent_color() {
	return ImColor(
		g_config.c[XOR("menu_color")][0],
		g_config.c[XOR("menu_color")][1],
		g_config.c[XOR("menu_color")][2],
		ui::GetStyle().Alpha
	);
}