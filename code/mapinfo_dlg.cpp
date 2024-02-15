#include "editor.h"
#include "gui.h"
#include "misc/cpp/imgui_stdlib.h"

#define SPECULAR_SCALE_RGBGLOSS 0
#define SPECULAR_SCALE_MS 1
#define SPECULAR_SCALE_RGB 2

#define NORMAL_SCALE_XY 0
#define NORMAL_SCALE_XY_HEIGHT 1

constexpr const byte null_sides[DIR_NULL] = { 0 };

typedef struct {
	bool active;

	Walnut::waveForm_t rgbWave;
	Walnut::colorGen_t rgbGen;

	Walnut::waveForm_t alphaWave;
	Walnut::alphaGen_t alphaGen;

	Walnut::texCoordGen_t tcGen;
	Walnut::texMod_t tcMod;

	Walnut::texModInfo_t texModInfo;

	byte constantColor[4];

	int blendFuncSrc;
	int blendFuncDest;

	uint32_t textureType;

	char texture[MAX_NPATH];

	vec4_t normalScale;
	vec4_t specularScale;

	uint32_t normalScaleType;
	uint32_t specularScaleType;
} stageEdit_t;

typedef struct
{
	stageEdit_t stages[MAX_SHADER_STAGES];
} shaderEdit_t;

static shaderEdit_t shaderEdit;

CMapInfoDlg::CMapInfoDlg( void )
{
    m_bHasSpawnWindow = false;
    m_bHasCheckpointWindow = false;

    m_bTilesetModified = false;
    m_bMapModified = false;

    m_bActive = true;
    m_bShow = true;
}

static void DrawVec3Control( const char *label, uvec3_t values, float resetValue = 0.0f )
{
	ImGuiIO& io = ImGui::GetIO();
	float lineHeight;
	ImVec2 buttonSize;

	ImGui::PushID( label );

	ImGui::Columns( 2 );
	ImGui::SetColumnWidth( 0, 100.0f );
	ImGui::TextUnformatted( label );
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 4 ) );

	lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	buttonSize = ImVec2( lineHeight + 3.0f, lineHeight );

	ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
	if ( ImGui::Button( "X", buttonSize ) ) {
		values[0] = resetValue;
	}
	ImGui::PopStyleColor( 3 );

	ImGui::SameLine();
	ImGui::DragInt( "##X", (int *)&values[0], 1, 0, mapData->width, "%u" );
	if ( values[0] < 0 ) {
		values[0] = 0;
	} else if ( values[0] > mapData->width ) {
		values[0] = mapData->width - 1;
	}

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
	if ( ImGui::Button( "Y", buttonSize ) ) {
		values[1] = resetValue;
	}
	ImGui::PopStyleColor( 3 );

	ImGui::SameLine();
	ImGui::DragInt( "##Y", (int *)&values[1], 1, 0, mapData->height, "%u" );
	if ( values[1] < 0 ) {
		values[1] = 0;
	} else if ( values[1] > mapData->height ) {
		values[1] = mapData->height - 1;
	}

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f  } );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
	if ( ImGui::Button( "Z", buttonSize ) ) {
		values[2] = resetValue;
	}
	ImGui::PopStyleColor( 3 );

	ImGui::SameLine();
	ImGui::DragInt( "##Z", (int *)&values[2], 1, 0, 1, "%u" );
	if ( values[2] < 0 ) {
		values[2] = 0;
	} else if ( values[2] > 1 ) {
		values[2] = 1;
	}

	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns( 1 );

	ImGui::PopID();
}

void CMapInfoDlg::AddMob( void )
{
	if ( !m_bHasAddMobWindow ) {
		return;
	}
	bool isUnique;

	ImGui::Begin( "Add Mob", &m_bHasAddMobWindow, ImGuiWindowFlags_NoCollapse );

	ImGui::TextUnformatted( "Name: " );
	ImGui::SameLine();
	ImGui::InputText( "##AddMobName", m_szAddMobName, sizeof(m_szAddMobName) - 1 );

	ImGui::TextUnformatted( "ID: " );
	ImGui::SameLine();
	ImGui::InputInt( "##AddMobId", &m_nAddMobId );
	if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
		ImGui::SetItemTooltip( "mob ID's are used in the game's scripting code to easily distinguish mobs from one another" );
	}

	if ( ButtonWithTooltip( "Generate a Mob ID", "automatically generate a unique mob id" ) ) {
		m_nAddMobId = g_pProjectManager->GetProject()->m_EntityList[ ET_MOB ].size();
	}

	ImGui::Separator();

	isUnique = true;
	for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_MOB ] ) {
		if ( it.m_Id == m_nAddMobId || !N_stricmp( it.m_Name.c_str(), m_szAddMobName ) ) {
			isUnique = false;
		}
	}

	if ( !m_szAddMobName[0] || !isUnique ) {
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
	}
	if ( !isUnique ) {
		ButtonWithTooltip( "DONE", "mob entity doesn't have a unique name and/or id!" );
	}
	else if ( !m_szAddMobName[0] ) {
		ButtonWithTooltip( "DONE", "mob entity has no name" );
	}
	else {
		if ( ImGui::Button( "DONE" ) ) {
			Log_Printf( "Adding new mob '%s' to project with ID %i...\n", m_szAddMobName, m_nAddMobId );
			g_pProjectManager->GetProject()->m_EntityList[ET_MOB].emplace_back( entityInfo_t( m_szAddMobName, m_nAddMobId ) );
			m_bHasAddMobWindow = false;
			g_pEditor->m_bProjectModified = true;
		}	
	}
	if ( !m_szAddMobName[0] || !isUnique ) {
		ImGui::PopStyleColor( 3 );
	}
	ImGui::SameLine();
	if ( ImGui::Button( "CANCEL" ) ) {
		m_bHasAddMobWindow = false;
	}

	ImGui::End();
}

void CMapInfoDlg::AddItem( void )
{
	if ( !m_bHasAddItemWindow ) {
		return;
	}

	bool isUnique;

	ImGui::Begin( "Add Item", &m_bHasAddItemWindow, ImGuiWindowFlags_NoCollapse );

	ImGui::TextUnformatted( "Name: " );
	ImGui::SameLine();
	ImGui::InputText( "##AddItemName", m_szAddItemName, sizeof(m_szAddItemName) - 1 );

	ImGui::TextUnformatted( "ID: " );
	ImGui::SameLine();
	ImGui::InputInt( "##AddItemId", &m_nAddItemId );

	if ( ButtonWithTooltip( "Generate an Item ID", "automatically generate a unique item id" ) ) {
		m_nAddItemId = g_pProjectManager->GetProject()->m_EntityList[ ET_ITEM ].size();
	}

	ImGui::Separator();

	isUnique = true;
	for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_ITEM ] ) {
		if ( it.m_Id == m_nAddItemId || !N_stricmp( it.m_Name.c_str(), m_szAddItemName ) ) {
			isUnique = false;
		}
	}

	if ( !m_szAddItemName[0] || !isUnique ) {
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
	}
	if ( !isUnique ) {
		ButtonWithTooltip( "DONE", "item entity doesn't have a unique name and/or id!" );
	}
	else if ( !m_szAddItemName[0] ) {
		ButtonWithTooltip( "DONE", "item entity has no name" );
	}
	else {
		if ( ImGui::Button( "DONE" ) ) {
			Log_Printf( "Adding new item '%s' to project with ID %i...\n", m_szAddItemName, m_nAddItemId );
			g_pProjectManager->GetProject()->m_EntityList[ET_ITEM].emplace_back( entityInfo_t( m_szAddItemName, m_nAddItemId ) );
			m_bHasAddItemWindow = false;
			g_pEditor->m_bProjectModified = true;
		}	
	}
	if ( !m_szAddItemName[0] || !isUnique ) {
		ImGui::PopStyleColor( 3 );
	}
	ImGui::SameLine();
	if ( ImGui::Button( "CANCEL" ) ) {
		m_bHasAddItemWindow = false;
	}

	ImGui::End();
}

void CMapInfoDlg::AddBot( void )
{
	if ( !m_bHasAddBotWindow ) {
		return;
	}

	bool isUnique;

	ImGui::Begin( "Add Bot", &m_bHasAddBotWindow, ImGuiWindowFlags_NoCollapse );

	ImGui::TextUnformatted( "Name: " );
	ImGui::SameLine();
	ImGui::InputText( "##AddBotName", m_szAddBotName, sizeof(m_szAddBotName) - 1 );

	ImGui::TextUnformatted( "ID: " );
	ImGui::SameLine();
	ImGui::InputInt( "##AddBotId", &m_nAddBotId );

	if ( ButtonWithTooltip( "Generate a Bot ID", "automatically generate a unique bot id" ) ) {
		m_nAddBotId = g_pProjectManager->GetProject()->m_EntityList[ ET_BOT ].size();
	}

	ImGui::Separator();

	isUnique = true;
	for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_BOT ] ) {
		if ( it.m_Id == m_nAddBotId || !N_stricmp( it.m_Name.c_str(), m_szAddBotName ) ) {
			isUnique = false;
		}
	}

	if ( !m_szAddBotName[0] || !isUnique ) {
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
	}
	if ( !isUnique ) {
		ButtonWithTooltip( "DONE", "bot entity doesn't have a unique name and/or id!" );
	}
	else if ( !m_szAddBotName[0] ) {
		ButtonWithTooltip( "DONE", "bot entity has no name" );
	}
	else {
		if ( ImGui::Button( "DONE" ) ) {
			Log_Printf( "Adding new bot '%s' to project with ID %i...\n", m_szAddBotName, m_nAddBotId );
			g_pProjectManager->GetProject()->m_EntityList[ET_MOB].emplace_back( entityInfo_t( m_szAddBotName, m_nAddBotId ) );
			m_bHasAddBotWindow = false;
			g_pEditor->m_bProjectModified = true;
		}	
	}
	if ( !m_szAddBotName[0] || !isUnique ) {
		ImGui::PopStyleColor( 3 );
	}
	ImGui::SameLine();
	if ( ImGui::Button( "CANCEL" ) ) {
		m_bHasAddBotWindow = false;
	}

	ImGui::End();
}


void CMapInfoDlg::ModifyMob( void )
{
	if ( !m_bHasEditMobWindow ) {
		return;
	}
	bool isUnique;

	ImGui::Begin( "Modify Mob", &m_bHasEditMobWindow, ImGuiWindowFlags_NoCollapse );

	ImGui::TextUnformatted( "Name: " );
	ImGui::SameLine();
	ImGui::InputText( "##ModifyMobName", m_szEditMobName, sizeof(m_szEditMobName) - 1 );

	ImGui::TextUnformatted( "ID: " );
	ImGui::SameLine();
	ImGui::InputInt( "##AddMobId", &m_nEditMobId );
	if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
		ImGui::SetItemTooltip( "mob ID's are used in the game's scripting code to easily distinguish mobs from one another" );
	}

	if ( ButtonWithTooltip( "Generate a Mob ID", "automatically generate a unique mob id" ) ) {
		m_nEditMobId = g_pProjectManager->GetProject()->m_EntityList[ ET_MOB ].size();
	}

	ImGui::Separator();

	isUnique = true;
	for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_MOB ] ) {
		if ( it.m_Id == m_nEditMobId || !N_stricmp( it.m_Name.c_str(), m_szEditMobName ) ) {
			isUnique = false;
		}
	}

	if ( !m_szEditMobName[0] || !isUnique ) {
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
	}
	if ( !isUnique ) {
		ButtonWithTooltip( "DONE", "mob entity doesn't have a unique name and/or id!" );
	}
	else if ( !m_szEditMobName[0] ) {
		ButtonWithTooltip( "DONE", "mob entity has no name" );
	}
	else {
		if ( ImGui::Button( "DONE" ) ) {
			Log_Printf( "Adding new mob '%s' to project with ID %i...\n", m_szEditMobName, m_nEditMobId );
			g_pProjectManager->GetProject()->m_EntityList[ET_MOB].emplace_back( entityInfo_t( m_szEditMobName, m_nEditMobId ) );
			m_bHasEditMobWindow = false;
			g_pEditor->m_bProjectModified = true;
		}	
	}
	if ( !m_szEditMobName[0] || !isUnique ) {
		ImGui::PopStyleColor( 3 );
	}
	ImGui::SameLine();
	if ( ImGui::Button( "CANCEL" ) ) {
		m_bHasEditMobWindow = false;
	}
	ImGui::SameLine();
	if ( ImGui::Button( "DELETE" ) ) {
		m_bHasEditMobWindow = false;
		g_pProjectManager->GetProject()->m_EntityList[ ET_MOB ].erase( m_pCurrentMob );
		g_pEditor->m_bProjectModified = true;
	}

	ImGui::End();
}

void CMapInfoDlg::ModifyItem( void )
{
	if ( !m_bHasEditItemWindow ) {
		return;
	}

	bool isUnique;

	ImGui::Begin( "Modify Item", &m_bHasEditItemWindow, ImGuiWindowFlags_NoCollapse );

	ImGui::TextUnformatted( "Name: " );
	ImGui::SameLine();
	ImGui::InputText( "##ModifyItemName", m_szEditItemName, sizeof(m_szEditItemName) - 1 );

	std::string& type = g_pProjectManager->GetProject()->m_EntityList[ ET_ITEM ][ m_nEditItemId ].m_Properties["item_type"];
	if ( ImGui::BeginCombo( "Type", type.c_str() ) ) {
		if ( ImGui::Selectable( "Weapon", type == "Weapon" ) ) {
			type = "Weapon";
		}
		if ( ImGui::Selectable( "Powerup", type == "Powerup" ) ) {
			type = "Powerup";
		}
		if ( ImGui::Selectable( "None", type == "None" ) ) {
			type = "None";
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();

	isUnique = true;
	for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_ITEM ] ) {
		if ( !N_stricmp( it.m_Name.c_str(), m_szEditItemName ) ) {
			isUnique = false;
		}
	}

	if ( !m_szEditItemName[0] || !isUnique ) {
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
	}
	if ( !isUnique ) {
		ButtonWithTooltip( "DONE", "item entity doesn't have a unique name!" );
	}
	else if ( !m_szEditItemName[0] ) {
		ButtonWithTooltip( "DONE", "item entity has no name" );
	}
	else {
		if ( ImGui::Button( "DONE" ) ) {
			m_bHasEditItemWindow = false;
			m_pCurrentItem->m_Name = m_szEditItemName;
			m_pCurrentItem->m_Id = m_nEditItemId;
			g_pEditor->m_bProjectModified = true;
		}	
	}
	if ( !m_szEditItemName[0] || !isUnique ) {
		ImGui::PopStyleColor( 3 );
	}
	ImGui::SameLine();
	if ( ImGui::Button( "CANCEL" ) ) {
		m_bHasEditItemWindow = false;
	}
	ImGui::SameLine();
	if ( ImGui::Button( "DELETE" ) ) {
		g_pProjectManager->GetProject()->m_EntityList[ ET_ITEM ].erase( m_pCurrentItem );
		m_bHasEditItemWindow = false;
		g_pEditor->m_bProjectModified = true;
	}

	ImGui::End();
}

void CMapInfoDlg::ModifyBot( void )
{
	if ( !m_bHasEditBotWindow ) {
		return;
	}

	bool isUnique;

	ImGui::Begin( "Modify Bot", &m_bHasEditBotWindow, ImGuiWindowFlags_NoCollapse );

	ImGui::TextUnformatted( "Name: " );
	ImGui::SameLine();
	ImGui::InputText( "##ModifyBotName", m_szEditBotName, sizeof(m_szEditBotName) - 1 );

	ImGui::Separator();

	isUnique = true;
	for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_BOT ] ) {
		if ( !N_stricmp( it.m_Name.c_str(), m_szEditBotName ) ) {
			isUnique = false;
		}
	}

	if ( !m_szEditBotName[0] || !isUnique ) {
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
	}
	if ( !isUnique ) {
		ButtonWithTooltip( "DONE", "bot entity doesn't have a unique name!" );
	}
	else if ( !m_szEditBotName[0] ) {
		ButtonWithTooltip( "DONE", "bot entity has no name" );
	}
	else {
		if ( ImGui::Button( "DONE" ) ) {
			m_bHasEditBotWindow = false;
			m_pCurrentBot->m_Name = m_szEditBotName;
			m_pCurrentBot->m_Id = m_nEditBotId;
			g_pEditor->m_bProjectModified = true;
		}	
	}
	if ( !m_szEditBotName[0] || !isUnique ) {
		ImGui::PopStyleColor( 3 );
	}
	ImGui::SameLine();
	if ( ImGui::Button( "CANCEL" ) ) {
		m_bHasEditBotWindow = false;
	}
	ImGui::SameLine();
	if ( ImGui::Button( "DELETE" ) ) {
		g_pProjectManager->GetProject()->m_EntityList[ ET_BOT ].erase( m_pCurrentBot );
		m_bHasEditBotWindow = false;
		g_pEditor->m_bProjectModified = true;
	}

	ImGui::End();
}

void CMapInfoDlg::ProjectSettingsDlg( void )
{
	if ( !m_bHasProjectWindow ) {
		return;
	}

	if ( ImGui::Begin( "Project Settings", &m_bHasProjectWindow, ImGuiWindowFlags_NoCollapse ) ) {
		if ( ImGui::IsWindowFocused() ) {
			g_pEditor->m_bWindowFocused = true;
			g_pEditor->m_InputFocus = EditorInputFocus::ToolFocus;
		}

		ImGui::TextUnformatted( "Name: " );
		ImGui::SameLine();
		if ( ImGui::InputText( "##ProjectName", m_szProjectName, sizeof(m_szProjectName), ImGuiInputTextFlags_EnterReturnsTrue ) ) {
			g_pEditor->m_bProjectModified = true;
			g_pProjectManager->GetProject()->m_Name = m_szProjectName;
		}

		if ( ImGui::Button( "Add Map To Project" ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "AddMapToProjectDlg", "Open Map File", ".map, .bmf, Map Files (*.map *.bmf){*.map, *.bmf}",
				g_pEditor->m_RecentDirectory );
		}

		if ( ImGui::BeginCombo( "Map List", mapData->name ) ) {

			for ( auto it : g_pProjectManager->GetProject()->m_MapList ) {
				if ( !it->name[0] ) {
					continue;
				}
				if ( ImGui::Selectable( it->name, ( mapData == it ) ) ) {
					if ( ::ConfirmModified() ) {
						g_pMapInfoDlg->SetCurrent( it );
					}
				}
			}

			ImGui::EndCombo();
		}

		if ( ImGui::BeginCombo( "Mob List", "Mob List" ) ) {
			std::vector<entityInfo_t>& entityList = g_pProjectManager->GetProject()->m_EntityList[ ET_MOB ];
			for ( auto it = entityList.begin(); it != entityList.end(); it++ ) {
				if ( ImGui::Selectable( va( "%s (ID: %i)", it->m_Name.c_str(), it->m_Id ), ( m_pCurrentMob == it ) ) ) {
					m_pCurrentMob = it;
					m_bHasEditMobWindow = true;
				}
			}
			if ( ImGui::Button( "Add Mob" ) ) {
				m_bHasAddMobWindow = true;
			}

			ImGui::EndCombo();
		}

		if ( ImGui::BeginCombo( "Item List", "Item List" ) ) {
			std::vector<entityInfo_t>& entityList = g_pProjectManager->GetProject()->m_EntityList[ ET_ITEM ];
			for ( auto it = entityList.begin(); it != entityList.end(); it++ ) {
				if ( ImGui::Selectable( va( "%s (ID: %i)", it->m_Name.c_str(), it->m_Id ), ( m_pCurrentItem == it ) ) ) {
					m_pCurrentItem = it;
					m_bHasEditItemWindow = true;
				}
			}
			if ( ImGui::Button( "Add Item" ) ) {
				m_bHasAddItemWindow = true;
			}

			ImGui::EndCombo();
		}

		if ( ImGui::BeginCombo( "Bot List", "Bot List" ) ) {
			std::vector<entityInfo_t>& entityList = g_pProjectManager->GetProject()->m_EntityList[ ET_MOB ];
			for ( auto it = entityList.begin(); it != entityList.end(); it++ ) {
				if ( ImGui::Selectable( va( "%s (ID: %i)", it->m_Name.c_str(), it->m_Id ), ( m_pCurrentBot == it ) ) ) {
					m_pCurrentBot = it;
					m_bHasEditBotWindow = true;
				}
			}
			if ( ImGui::Button( "Add Bot" ) ) {
				m_bHasAddBotWindow = true;
			}

			ImGui::EndCombo();
		}

		ImGui::End();
	}
}

static const char *EntityTypeToString( entitytype_t type )
{
    switch ( type ) {
    case ET_PLAYR: return "Player";
    case ET_MOB: return "Mob";
    case ET_BOT: return "Bot";
    case ET_ITEM: return "Item";
    default:
        break;
    };

    Error( "EntityTypeToString: invalid entity type %i", (int)type );
    return NULL;
}

void CMapInfoDlg::CreateSpawn( void )
{
	memset( &mapData->spawns[mapData->numSpawns], 0, sizeof(mapspawn_t) );
	m_bMapModified = true;
	m_bMapNameUpdated = false;
	mapData->numSpawns++;
}

void CMapInfoDlg::CreateCheckpoint( void )
{
	memset( &mapData->checkpoints[mapData->numCheckpoints], 0, sizeof(mapcheckpoint_t) );
	m_bMapModified = true;
	m_bMapNameUpdated = false;
	mapData->numCheckpoints++;
}

void CMapInfoDlg::CreateLight( void )
{
	memset( &mapData->lights[mapData->numLights], 0, sizeof(maplight_t) );
	m_bMapModified = true;
	m_bMapNameUpdated = false;
	mapData->numLights++;
}

static bool MapIsInProjectList( const char *name )
{
	if ( g_pProjectManager->GetProject().get() ) {
		for ( const auto& it : g_pProjectManager->GetProject()->m_MapList ) {
			if ( it && strcmp( it->name, name ) == 0 ) {
				return true;
			}
		}
		return false;
	}
	return false;
}

void CMapInfoDlg::SetCurrent( mapData_t *data )
{
    for ( auto& it : m_MapList ) {
		if ( it.m_pMapData && strcmp( it.m_pMapData->name, data->name ) == 0 ) {
			const bool inProjectList = MapIsInProjectList( data->name );

			if ( inProjectList && g_pProjectManager->GetProject().get() ) {
				// already in the list, don't let the user add it twice
				return;
			} else if ( !inProjectList && g_pProjectManager->GetProject().get() ) {
				mapData = data;
				g_pProjectManager->GetProject()->m_MapList.emplace_back( data );
				return;
			}
		}
        if ( !it.m_bUsed ) {
            it.m_bUsed = true;
            it.m_pMapData = data;
            return;
        }
    }

    mapData = data;
    m_MapList.emplace_back( data );

	bool present = false;

	if ( g_pEditor->m_RecentFiles.size() ) {
		for ( const auto& it : g_pEditor->m_RecentFiles ) {
			if ( strcmp( it.c_str(), data->name ) == 0 ) {
				present = true;
				break;
			}
		}
	}
	if ( !present && data && data->name[0] ) {
		g_pEditor->m_RecentFiles.emplace_back( data->name );
	}

	if ( !MapIsInProjectList( data->name ) && g_pProjectManager->GetProject().get() ) {
		g_pProjectManager->GetProject()->m_MapList.emplace_back( data );
	}

	N_strncpyz( m_szTempMapName, mapData->name, sizeof(m_szTempMapName) );
	N_strncpyz( m_szProjectName, g_pProjectManager->GetProject()->m_Name.c_str(), sizeof(m_szProjectName) );
}

static void ShaderEdit( void );

static inline void ImGui_Quad_TexCoords( const spriteCoord_t *texcoords, ImVec2& min, ImVec2& max )
{
    min = { (*texcoords)[3][0], (*texcoords)[3][1] };
    max = { (*texcoords)[1][0], (*texcoords)[1][1] };
}

void CMapInfoDlg::Draw( void )
{
    char mapnameTemp[MAX_NPATH];
    bool open;
    uint32_t i;
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                                | ImGuiTreeNodeFlags_FramePadding;

	if ( m_bMapModified && !m_bMapNameUpdated ) {
		Sys_SetWindowTitle( va( "%s*", mapData->name ) );
		m_bMapNameUpdated = true;
	}

	AddMob();
	AddItem();
	AddBot();

	ModifyMob();
	ModifyItem();
	ModifyBot();

	if ( m_bHasTileWindow && g_pMapDrawer->m_bTileSelectOn ) {
		if ( ImGui::Begin( "##TileMode", &m_bHasTileWindow, ImGuiWindowFlags_AlwaysAutoResize  ) ) {
			int& x = g_pMapDrawer->m_nTileSelectX;
			int& y = g_pMapDrawer->m_nTileSelectY;

			if ( ImGui::IsKeyPressed( ImGuiKey_W, false ) ) {
				y--;
			}
			if ( ImGui::IsKeyPressed( ImGuiKey_S, false ) ) {
				y++;
			}
			if ( ImGui::IsKeyPressed( ImGuiKey_A, false ) ) {
				x--;
			}
			if ( ImGui::IsKeyPressed( ImGuiKey_D, false ) ) {
				x++;
			}
			if ( ImGui::IsKeyPressed( ImGuiKey_UpArrow, false ) ) {
				m_nTileY--;
			}
			if ( ImGui::IsKeyPressed( ImGuiKey_DownArrow, false ) ) {
				m_nTileY++;
			}
			if ( ImGui::IsKeyPressed( ImGuiKey_LeftArrow, false ) ) {
				m_nTileX--;
			}
			if ( ImGui::IsKeyPressed( ImGuiKey_RightArrow, false ) ) {
				m_nTileX++;
			}
			if ( ImGui::IsKeyPressed( ImGuiKey_Enter, false ) || ImGui::IsKeyPressed( ImGuiKey_KeypadEnter, false ) ) {
				maptile_t *t = &mapData->tiles[ y * mapData->width + x ];
				t->index = m_nTileY * mapData->tileset.tileCountX + m_nTileX;
				memcpy( t->texcoords, mapData->texcoords[ t->index ], sizeof(spriteCoord_t) );
			}

			y = clamp( y, 0, mapData->height );
			x = clamp( x, 0, mapData->width );

			ImGui::Text( "Editing Tile At %ix%i", x, y );

			if ( ImGui::CollapsingHeader( "Set Tile Sprite" ) ) {
				uint32_t tileY, tileX;
				spriteCoord_t *tile;
				const Walnut::Image *texture = mapData->textures[Walnut::TB_DIFFUSEMAP];
				ImVec2 min, max;

				if ( !texture ) {
					ImGui::TextUnformatted( "No Tileset in Use" );
				}
				else {
					for ( tileY = 0; tileY < mapData->tileset.tileCountY; tileY++ ) {
						for ( tileX = 0; tileX < mapData->tileset.tileCountX; tileX++ ) {
							tile = &mapData->texcoords[tileY * mapData->tileset.tileCountX + tileX];
							ImGui_Quad_TexCoords( tile, min, max );

							ImGui::PushID( (uintptr_t)tile );
							if ( ImGui::ImageButton( (ImTextureID)(uintptr_t)texture->GetID(), { 64.0f, 64.0f }, min, max ) ) {
								maptile_t *t = &mapData->tiles[ y * mapData->width + x ];
								t->index = tileY * mapData->tileset.tileCountX + tileX;
								memcpy( t->texcoords, *tile, sizeof(*tile) );
								m_bMapModified = true;
								m_bMapNameUpdated = false;
								(void)0; // NEVER remove this dead code, for some reason, g++ WILL NOT compile it in
							}
							ImGui::PopID();
							ImGui::SameLine();
						}
						ImGui::NewLine();
					}
				}
			}

			if ( ImGui::CollapsingHeader( "Tile Physics Sides" ) ) {
				ImGui::SeparatorText( "Sides" );
		        ImGui::BeginTable( " ", 3 );
		        {
		            const ImVec2 buttonSize = { 86, 48 };

					auto sideButton = [&]( const char *name, dirtype_t dir ) {
						const bool color = mapData->tiles[ y * mapData->width + x ].sides[dir];
						ImGui::TableNextColumn();
						if ( color ) {
							ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
							ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
							ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
						}
						if ( ImGui::Button( name, buttonSize ) ) {
							mapData->tiles[ y * mapData->width + x ].sides[dir] = true;
							m_bMapModified = true;
							m_bMapNameUpdated = false;
						}
						if ( color ) {
							ImGui::PopStyleColor( 3 );
						}
					};

					sideButton( "North West", DIR_NORTH_WEST );
					sideButton( "North", DIR_NORTH );
					sideButton( "North East", DIR_NORTH_EAST );
					sideButton( "West", DIR_WEST );
					sideButton( "Inside", NUMDIRS );
					sideButton( "East", DIR_EAST );
					sideButton( "South West", DIR_SOUTH_WEST );
					sideButton( "South", DIR_SOUTH );
					sideButton( "South East", DIR_SOUTH_EAST );
		        }
		        ImGui::EndTable();

				const bool clear = memcmp( mapData->tiles[ y * mapData->width + x ].sides, null_sides, sizeof(null_sides) ) == 0;
				if ( clear ) {
					ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
					ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
					ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
				}
		        if ( ImGui::Button( "Clear Collision Sides" ) && !clear ) {
		            memset( mapData->tiles[ y * mapData->width + x ].sides, 0, sizeof(mapData->tiles[ y * mapData->width + x ].sides) );
		        }
				if ( clear ) {
					ImGui::PopStyleColor( 3 );
				}
			}

			const bool wasCheckpoint = mapData->tiles[y * mapData->width + x].flags & TILETYPE_CHECKPOINT;
			if ( wasCheckpoint ) {
				ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
			}
			if ( ImGui::Button( "Set Checkpoint" ) ) {
				mapData->tiles[y * mapData->width + x].flags |= TILETYPE_CHECKPOINT;
				m_bMapModified = true;
				m_bMapNameUpdated = false;
			}
			if ( wasCheckpoint ) {
				ImGui::PopStyleColor();
			}

			const bool wasSpawn = mapData->tiles[y * mapData->width + x].flags & TILETYPE_SPAWN;
			if ( wasSpawn ) {
				ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
			}
			if ( ImGui::Button( "Set Spawn" ) ) {
				mapData->tiles[y * mapData->width + x].flags |= TILETYPE_SPAWN;
				m_bMapModified = true;
				m_bMapNameUpdated = false;
			}
			if ( wasSpawn ) {
				ImGui::PopStyleColor();
			}

			if ( ImGui::Button( "Done" ) ) {
				g_pMapDrawer->m_bTileSelectOn = false;
				m_bHasTileWindow = false;
			}

			if ( ImGui::Button( "CANCEL" ) ) {
				m_bHasTileWindow = false;
			}
			ImGui::End();
		}
	}

	if ( m_bHasCheckpointWindow ) {
		if ( ImGui::Begin( "Editing Checkpoint", &m_bHasCheckpointWindow, ImGuiWindowFlags_AlwaysAutoResize ) ) {
			ImGui::SetWindowFocus();
			ImGui::SeparatorText( va( "checkpoint %u", (unsigned)( m_pCheckpointEdit - mapData->checkpoints ) ) );
			DrawVec3Control( "position", m_pCheckpointEdit->xyz );
			if ( ImGui::Button( "delete" ) ) {
				mapData->numCheckpoints--;
				m_bMapModified = true;
				m_bHasCheckpointWindow = false;
			}
			if ( ImGui::Button( "CANCEL" ) ) {
				m_bHasCheckpointWindow = false;
			}
			ImGui::End();
		}
	}

	if ( m_bHasSpawnWindow ) {
		if ( ImGui::Begin( "Editing Spawn", &m_bHasSpawnWindow, ImGuiWindowFlags_AlwaysAutoResize ) ) {
			ImGui::SetWindowFocus();
			ImGui::SeparatorText( va( "spawn %u", (unsigned)( m_pSpawnEdit - mapData->spawns ) ) );
			DrawVec3Control( "position", m_pSpawnEdit->xyz );
			if ( ImGui::Button( "delete" ) ) {
				mapData->numSpawns--;
				m_bMapModified = true;
				m_bHasSpawnWindow = false;
			}
			if ( ImGui::Button( "CANCEL" ) ) {
				m_bHasSpawnWindow = false;
			}
			ImGui::End();
		}
	}

	if ( m_bHasLightWindow ) {
		if ( ImGui::Begin( "Editing Light", &m_bHasLightWindow, ImGuiWindowFlags_AlwaysAutoResize ) ) {
			ImGui::SetWindowFocus();
			ImGui::SeparatorText( va( "light %u", (unsigned)( m_pLightEdit - mapData->lights ) ) );
			DrawVec3Control( "position", m_pLightEdit->origin );
			if ( ImGui::SliderFloat( "brightness", &m_pLightEdit->range, 0.5f, 200.0f ) ) {
				m_bMapModified = true;
			}
			if ( ImGui::ColorEdit3( "color", m_pLightEdit->color ) ) {
				m_bMapModified = true;
			}
			if ( ImGui::Button( "delete" ) ) {
				mapData->numLights--;
				m_bMapModified = true;
				m_bHasLightWindow = false;
			}
			if ( ImGui::Button( "CANCEL" ) ) {
				m_bHasLightWindow = false;
			}
			ImGui::End();
		}
	}
	
	if ( ImGui::BeginPopupModal( "Map Name Error" ) ) {
		ImGui::TextUnformatted( "Map names require the .map extension, do not remove it" );
		ImGui::EndPopup();
	}

    if ( m_bShow && ImGui::Begin( "Map Info", NULL, ImGuiWindowFlags_NoCollapse ) ) {
		bool empty = true;

		for ( const auto& it : m_MapList ) {
			if ( it.m_bUsed ) {
				empty = false;
				break;
			}
		}
		if ( empty ) {
			ImGui::TextUnformatted( "No Maps Open" );
		}
        else if ( ImGui::BeginTabBar( "##MapSelector" ) ) {
            for ( auto it = m_MapList.begin(); it != m_MapList.end(); it++ ) {
                if ( !it->m_bUsed ) {
                    continue;
                }

                if ( ImGui::BeginTabItem( it->m_pMapData->name, &it->m_bUsed ) ) {
                    mapData = it->m_pMapData;
					m_pCurrentMap = it;
                    ImGui::EndTabItem();
                }

				if ( !it->m_bUsed ) {
					it->m_pMapData = NULL;
					if ( it == m_pCurrentMap ) {
						if ( m_pCurrentMap == m_MapList.begin() && m_MapList.size() > 1 ) {
							m_pCurrentMap++;
						} else if ( m_pCurrentMap == m_MapList.end() - 1 && m_MapList.size() > 1 ) {
							m_pCurrentMap--;
						}
					}
				}
            }
            ImGui::EndTabBar();
        }

		if ( ImGui::IsWindowHovered() || ImGui::IsWindowFocused() ) {
			g_pEditor->m_bWindowFocused = true;
			g_pEditor->m_InputFocus = EditorInputFocus::ToolFocus;
		}

		if ( empty ) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText( "General" );

		ImGui::TextUnformatted( "Name: " );
		ImGui::SameLine();
		if ( ImGui::InputText( "##MapName", m_szTempMapName, MAX_NPATH - strlen( MAP_FILE_EXT ), ImGuiInputTextFlags_EnterReturnsTrue ) ) {
			snprintf( mapData->name, sizeof(mapData->name), "%s.map", m_szTempMapName );
			m_bMapModified = true;
			m_bMapNameUpdated = false;
		}

		ImGui::TextUnformatted( "Width: " );
		ImGui::SameLine();
		if ( ImGui::InputInt( "##MapWidth", &mapData->width, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
			m_bMapModified = g_pEditor->m_nOldMapWidth != mapData->width;
			
			if ( mapData->width < 0 ) {
				mapData->width = 0;
			} else if ( mapData->width > MAX_MAP_WIDTH ) {
				mapData->width = MAX_MAP_WIDTH;
			}
		}
	
		ImGui::TextUnformatted( "Height: " );
		ImGui::SameLine();
		if ( ImGui::InputInt( "##MapHeight", &mapData->height, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
			m_bMapModified = g_pEditor->m_nOldMapHeight != mapData->height;
			m_bMapNameUpdated = false;

			if ( mapData->height < 0 ) {
				mapData->height = 0;
			} else if ( mapData->height > MAX_MAP_HEIGHT ) {
				mapData->height = MAX_MAP_HEIGHT;
			}
		}

		if ( ImGui::ColorEdit3( "Ambient Light Color", mapData->ambientColor, ImGuiColorEditFlags_HDR ) ) {
			m_bMapModified = true;
			m_bMapNameUpdated = false;
		}

		ImGui::NewLine();

		ImGui::SeparatorText( "Lights" );
		for ( i = 0; i < mapData->numLights; i++ ) {
			ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 4 ) );

			open = ImGui::TreeNodeEx( (void *)(uintptr_t)&mapData->lights[i], treeNodeFlags, "light %u", i );
			ImGui::SameLine();
			if ( ButtonWithTooltip( "+", "Edit Light" ) ) {
				m_pLightEdit = &mapData->lights[i];
				m_bHasLightWindow = true;
			}

			if ( open ) {
				DrawVec3Control( "position", mapData->checkpoints[i].xyz );
				ImGui::TreePop();
			}
			ImGui::PopStyleVar();
		}
		if ( ButtonWithTooltip( "Add Light", "Add a light object to the map" ) ) {
			CreateLight();
		}

		ImGui::SeparatorText( "Checkpoints" );
	    for ( i = 0; i < mapData->numCheckpoints; i++ ) {
			ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 4 ) );

			open = ImGui::TreeNodeEx( (void *)(uintptr_t)&mapData->checkpoints[i], treeNodeFlags, "checkpoint %u", i );
			ImGui::SameLine();
			if ( ButtonWithTooltip( "+", "Edit Checkpoint" ) ) {
				m_pCheckpointEdit = &mapData->checkpoints[i];
				m_bHasCheckpointWindow = true;
			}

			if ( open ) {
				DrawVec3Control( "position", mapData->checkpoints[i].xyz );
				ImGui::TreePop();
			}
			ImGui::PopStyleVar();
		}
		if ( ButtonWithTooltip( "Add Checkpoint", "Add a checkpoint to the map" ) ) {
			CreateCheckpoint();
		}

		ImGui::SeparatorText( "Spawns" );
		for ( i = 0; i < mapData->numSpawns; i++ ) {
			ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 4 ) );

			open = ImGui::TreeNodeEx( (void *)(uintptr_t)&mapData->spawns[i], treeNodeFlags, "spawn %u", i );
			ImGui::SameLine();
			if ( ButtonWithTooltip( "+", "Edit Spawn" ) ) {
				m_pSpawnEdit = &mapData->spawns[i];
				m_bHasSpawnWindow = true;
			}

			if ( open ) {
				DrawVec3Control( "position", mapData->spawns[i].xyz );
				if ( ImGui::BeginCombo( "Entity ID",
					va( "%s", m_nSelectedSpawnEntityId != -1 ?
					g_pProjectManager->GetProject()->m_EntityList[m_nSelectedSpawnEntityType][m_nSelectedSpawnEntityId].m_Name.c_str() : "Entity ID" ) ) )
				{
					for ( uint32_t a = 0; a < g_pProjectManager->GetProject()->m_EntityList[m_nSelectedSpawnEntityType].size(); a++ ) {
						if ( ImGui::Selectable( g_pProjectManager->GetProject()->m_EntityList[m_nSelectedSpawnEntityType][a].m_Name.c_str(),
						( m_nSelectedSpawnEntityId == a ) ) )
						{
							m_nSelectedSpawnEntityId = a;
							m_bMapModified = true;
						}
					}
					ImGui::EndCombo();
				}
				if ( ImGui::BeginCombo( "Entity Type", EntityTypeToString( (entitytype_t)m_nSelectedSpawnEntityType ) ) ) {
					if ( ImGui::Selectable( "item", ( m_nSelectedSpawnEntityType == ET_ITEM ) ) ) {
						m_nSelectedSpawnEntityType = ET_ITEM;
						m_bMapModified = true;
					}
					if ( ImGui::Selectable( "mob", ( m_nSelectedSpawnEntityType == ET_MOB ) ) ) {
						m_nSelectedSpawnEntityType = ET_MOB;
						m_bMapModified = true;
					}
					if ( ImGui::Selectable( "bot", ( m_nSelectedSpawnEntityType == ET_BOT ) ) ) {
						m_nSelectedSpawnEntityType = ET_BOT;
						m_bMapModified = true;
					}
					ImGui::EndCombo();
				}
				ImGui::TreePop();
			}
			ImGui::PopStyleVar();
	    }
		if ( ButtonWithTooltip( "Add Spawn", "Add an entity spawn to the map" ) ) {
			CreateSpawn();
		}
		
		ImGui::SeparatorText( "Tileset" );
		ImGui::TextUnformatted( "Name: " );
		ImGui::SameLine();
		if ( ImGui::InputText( "##TilesetName", mapData->tileset.texture, sizeof(mapData->tileset.texture), ImGuiInputTextFlags_EnterReturnsTrue ) ) {
			m_bMapModified = true;
			m_bTilesetModified = true;
		}

		ImGui::Text( "Image Width: %i", mapData->textureWidth );
		ImGui::Text( "Image Height: %i", mapData->textureHeight );
		
		ImGui::TextUnformatted( "Tile Width: " );
		ImGui::SameLine();
		if ( mapData->tileset.texture[0] ) {
			if ( ImGui::InputInt( "##TileWidth", (int *)&mapData->tileset.tileWidth, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
				m_bMapModified = true;
				m_bTilesetModified = true;
			}
		} else {
			ImGui::TextUnformatted( "N/A" );
		}

		ImGui::TextUnformatted( "Tile Height: " );
		ImGui::SameLine();
		if ( mapData->tileset.texture[0] ) {
			if ( ImGui::InputInt( "##TileHeight", (int *)&mapData->tileset.tileHeight, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
				m_bMapModified = true;
				m_bTilesetModified = true;
			}
		} else {
			ImGui::TextUnformatted( "N/A" );
		}
		
		ImGui::Text( "Number of Tiles: %u", mapData->tileset.numTiles );

		const char *name;
        const char *textureFileDlgFilters =
            ".jpg,.jpeg,.png,.bmp,.tga,.webp,"
			"Jpeg Files (*.jpeg *.jpg){.jpeg,.jpg},"
			"Image Files (*.jpg *.jpeg *.png *.bmp *.tga *.webp){.jpg,.jpeg,.png,.bmp,.tga,.webp}";

		ImGui::TextUnformatted( "Diffuse Map: " );
		ImGui::SameLine();

		if ( mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
			name = mapData->textures[Walnut::TB_DIFFUSEMAP]->GetName().c_str();
			name = strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name;
		} else {
			name = "None##DiffuseMap";
		}

		if ( ImGui::Button( name ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectDiffuseMapDlg", "Select Diffuse Texture File",
	            textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		ImGui::TextUnformatted( "Normal Map: " );
		ImGui::SameLine();

		if ( mapData->textures[Walnut::TB_NORMALMAP] ) {
			name = mapData->textures[Walnut::TB_NORMALMAP]->GetName().c_str();
			name = strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name;
		} else {
			name = "None##NormalMap";
		}

		if ( ImGui::Button( name ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectNormalMapDlg", "Select Normal Texture File",
				textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		ImGui::TextUnformatted( "Specular Map: " );
		ImGui::SameLine();

		if ( mapData->textures[Walnut::TB_SPECULARMAP] ) {
			name = mapData->textures[Walnut::TB_SPECULARMAP]->GetName().c_str();
			name = strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name;
		} else {
			name = "None##SpecularMap";
		}

		if ( ImGui::Button( name ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectSpecularMapDlg", "Select Specular Texture File",
				textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		ImGui::TextUnformatted( "Light Map: " );
		ImGui::SameLine();

		if ( mapData->textures[Walnut::TB_LIGHTMAP] ) {
			name = mapData->textures[Walnut::TB_LIGHTMAP]->GetName().c_str();
			name = strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name;
		} else {
			name = "None##LightMap";
		}

		if ( ImGui::Button( name ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectLightMapDlg", "Select Light Texture File",
				textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		ImGui::TextUnformatted( "Shadow Map: " );
		ImGui::SameLine();

		if ( mapData->textures[Walnut::TB_SHADOWMAP] ) {
			name = mapData->textures[Walnut::TB_SHADOWMAP]->GetName().c_str();
			name = strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name;
		} else {
			name = "None##ShadowMap";
		}

		if ( ImGui::Button( name ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectShadowMapDlg", "Select Shadow Texture File",
				textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		if ( !mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
			ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
		}
//		if ( ImGui::Button( "Edit Shader" ) && mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
//			ImGui::OpenPopup( "EditShader" );
//		}
		if ( !mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
			ImGui::PopStyleColor();
		}

		mapData->numTiles = mapData->width * mapData->height;

		if ( m_bTilesetModified ) {
			if ( ButtonWithTooltip( "Build Tileset", "Calculate tile data for the engine to process" ) ) {
                m_bTilesetModified = false;
				Map_BuildTileset();
			}
		}
		ImGui::End();
	}
}

inline const char *AlphaGenToString( Walnut::alphaGen_t alphaGen ) {
	switch ( alphaGen ) {
	case Walnut::AGEN_CONST: return "Constant";
	case Walnut::AGEN_ENTITY: return "Entity";
	case Walnut::AGEN_IDENTITY: return "Identity";
	case Walnut::AGEN_LIGHTING_SPECULAR: return "Specular Lighting";
	case Walnut::AGEN_WAVEFORM: return "Waveform";
	case Walnut::AGEN_ONE_MINUS_VERTEX: return "One Minus Vertex";
	case Walnut::AGEN_ONE_MINUS_ENTITY: return "One Minus Entity";
	case Walnut::AGEN_SKIP: return "Skip";
	case Walnut::AGEN_VERTEX: return "Vertex";
	default:
		return "Unknown";
	};
}

inline const char *RGBGenToString( Walnut::colorGen_t rgbGen ) {
	switch ( rgbGen ) {
	case Walnut::CGEN_CONST: return "Constant";
	case Walnut::CGEN_ENTITY: return "Entity";
	case Walnut::CGEN_EXACT_VERTEX: return "Exact Vertex";
	case Walnut::CGEN_EXACT_VERTEX_LIT: return "Exact Vertex Lighting";
	case Walnut::CGEN_IDENTITY: return "Identity";
	case Walnut::CGEN_IDENTITY_LIGHTING: return "Identity Lighting";
	case Walnut::CGEN_LIGHTING_DIFFUSE: return "Lighting Diffuse";
	case Walnut::CGEN_WAVEFORM: return "Waveform";
	case Walnut::CGEN_VERTEX_LIT: return "Vertex Lighting";
	case Walnut::CGEN_VERTEX: return "Vertex";
	case Walnut::CGEN_ONE_MINUS_VERTEX: return "One Minus Vertex";
	case Walnut::CGEN_ONE_MINUS_ENTITY: return "One Minus Entity";
	default:
		return "Unknown";
	};
}

inline const char *NormalScaleTypeString( uint32_t type ) {
	switch ( type ) {
	case NORMAL_SCALE_XY: return "<X> <Y>";
	case NORMAL_SCALE_XY_HEIGHT: return "<X> <Y> <Height>";
	default:
		return "Unknown";
	};
}

inline const char *SpecularScaleTypeString( uint32_t type ) {
	switch ( type ) {
	case SPECULAR_SCALE_MS: return "<Mellatic> <Smoothness> (Requires r_pbr == 1 in game)";
	case SPECULAR_SCALE_RGB: return "<r> <g> <b>";
	case SPECULAR_SCALE_RGBGLOSS: return "<r> <g> <b> <gloss>";
	default:
		return "Unknown";
	};
}

inline const char *TCGenToString( Walnut::texCoordGen_t tcGen ) {
	switch ( tcGen ) {
	case Walnut::TCGEN_VECTOR: return "Vector";
	case Walnut::TCGEN_TEXTURE: return "Texture";
	case Walnut::TCGEN_IDENTITY: return "Identity";
	case Walnut::TCGEN_LIGHTMAP: return "Lightmap";
	case Walnut::TCGEN_BAD:
	default:
		return "Invalid";
	};
}

inline const char *TCModToString( Walnut::texMod_t tcMod ) {
	switch ( tcMod ) {
	case Walnut::TMOD_ENTITY_TRANSLATE: return "Entity Translate";
	case Walnut::TMOD_OFFSET: return "Offset";
	case Walnut::TMOD_ROTATE: return "Rotate";
	case Walnut::TMOD_SCALE: return "Scale";
	case Walnut::TMOD_SCALE_OFFSET: return "Scale Offset";
	case Walnut::TMOD_TURBULENT: return "Turbulent";
	case Walnut::TMOD_SCROLL: return "Scroll";
	case Walnut::TMOD_TRANSFORM: return "Transform";
	case Walnut::TMOD_STRETCH: return "Stretch";
	case Walnut::TMOD_NONE:
	default:
		return "None";
	};
}

static void ShaderEdit( void )
{
	if ( ImGui::Begin( va( "Edit Shader %s", g_pMapInfoDlg->m_szCurrentShader ), NULL, ImGuiWindowFlags_None ) ) {
		for ( uint32_t i = 0; i < MAX_SHADER_STAGES; i++ ) {
			if ( ImGui::CollapsingHeader( va( "Shader Stage %i", i ) ) ) {
				if ( ImGui::Checkbox( "Active", &shaderEdit.stages[i].active ) ) {
					g_pMapInfoDlg->SetModified( true, true );
				}

				if ( ImGui::BeginCombo( "Specular Scale Type", SpecularScaleTypeString( shaderEdit.stages[i].specularScaleType ) ) ) {
					if ( ImGui::Selectable( "<Metallic> <Smoothness> (Requires r_pbr == 1 in game)",
						shaderEdit.stages[i].specularScaleType == SPECULAR_SCALE_MS ) )
					{
						shaderEdit.stages[i].specularScaleType =  SPECULAR_SCALE_MS;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "<r> <g> <b>", shaderEdit.stages[i].specularScaleType == SPECULAR_SCALE_RGB ) ) {
						shaderEdit.stages[i].specularScaleType = SPECULAR_SCALE_RGB;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "<r> <g> <b> <gloss>", shaderEdit.stages[i].specularScaleType == SPECULAR_SCALE_RGBGLOSS ) ) {
						shaderEdit.stages[i].specularScaleType = SPECULAR_SCALE_RGBGLOSS;
						g_pMapInfoDlg->SetModified( true, true );
					}
					ImGui::EndCombo();
				}


				if ( ImGui::BeginCombo( "Normal Scale Type", NormalScaleTypeString( shaderEdit.stages[i].normalScaleType ) ) ) {
					if ( ImGui::Selectable( "<X> <Y>", shaderEdit.stages[i].normalScaleType == NORMAL_SCALE_XY ) ) {
						shaderEdit.stages[i].normalScaleType = NORMAL_SCALE_XY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "<X> <Y> <Height>", shaderEdit.stages[i].normalScaleType == NORMAL_SCALE_XY_HEIGHT ) ) {
						shaderEdit.stages[i].normalScaleType = NORMAL_SCALE_XY_HEIGHT;
						g_pMapInfoDlg->SetModified( true, true );
					}
					ImGui::EndCombo();
				}

				if ( ImGui::BeginCombo( "RGB Gen", RGBGenToString( shaderEdit.stages[i].rgbGen ) ) ) {
					
					if ( ImGui::Selectable( "Waveform", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_WAVEFORM ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_WAVEFORM;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Constant", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_CONST ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_CONST;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Identity", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_IDENTITY ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_IDENTITY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Identity Lighting", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_IDENTITY_LIGHTING ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_IDENTITY_LIGHTING;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Entity", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_ENTITY ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_ENTITY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "One Minus Entity", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_ONE_MINUS_ENTITY ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_ONE_MINUS_ENTITY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "One Minus Vertex", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_ONE_MINUS_VERTEX ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_ONE_MINUS_VERTEX;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Vertex", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_VERTEX ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_VERTEX;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Exact Vertex", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_EXACT_VERTEX ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_EXACT_VERTEX;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Exact Vertex Lighting", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_EXACT_VERTEX_LIT ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_EXACT_VERTEX_LIT;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Vertex Lighting", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_VERTEX_LIT ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_VERTEX_LIT;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Diffuse Lighting", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_LIGHTING_DIFFUSE ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_LIGHTING_DIFFUSE;
						g_pMapInfoDlg->SetModified( true, true );
					}

					ImGui::EndCombo();
				}
				if ( ImGui::BeginCombo( "Alpha Gen", AlphaGenToString( shaderEdit.stages[i].alphaGen ) ) ) {
					if ( ImGui::Selectable( "Constant", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_CONST ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_CONST;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Entity", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_ENTITY ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_ENTITY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Identity", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_IDENTITY ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_IDENTITY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Specular Lighting", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_LIGHTING_SPECULAR ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_LIGHTING_SPECULAR;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "One Minus Entity", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_ONE_MINUS_ENTITY ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_ONE_MINUS_ENTITY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "One Minus Vertex", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_ONE_MINUS_VERTEX ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_ONE_MINUS_VERTEX;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Skip", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_SKIP ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_SKIP;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Vertex", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_VERTEX ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_VERTEX;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Waveform", ( shaderEdit.stages[i].alphaGen == Walnut::AGEN_WAVEFORM ) ) ) {
						shaderEdit.stages[i].alphaGen = Walnut::AGEN_WAVEFORM;
						g_pMapInfoDlg->SetModified( true, true );
					}
					ImGui::EndCombo();
				}
				if ( ImGui::BeginCombo( "Texture Coordinate Generate (tcGen)", TCGenToString( shaderEdit.stages[i].tcGen ) ) ) {
					if ( ImGui::Selectable( "Identity", ( shaderEdit.stages[i].tcGen == Walnut::TCGEN_IDENTITY ) ) ) {
						shaderEdit.stages[i].tcGen == Walnut::TCGEN_IDENTITY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Light Map", ( shaderEdit.stages[i].tcGen == Walnut::TCGEN_LIGHTMAP ) ) ) {
						shaderEdit.stages[i].tcGen == Walnut::TCGEN_LIGHTMAP;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Texture", ( shaderEdit.stages[i].tcGen == Walnut::TCGEN_TEXTURE ) ) ) {
						shaderEdit.stages[i].tcGen == Walnut::TCGEN_TEXTURE;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Vector", ( shaderEdit.stages[i].tcGen == Walnut::TCGEN_VECTOR ) ) ) {
						shaderEdit.stages[i].tcGen == Walnut::TCGEN_IDENTITY;
						g_pMapInfoDlg->SetModified( true, true );
					}
					ImGui::EndCombo();
				}
				if ( ImGui::BeginCombo( "Texture Coordinate Modification (tcMod)", TCModToString( shaderEdit.stages[i].tcMod ) ) ) {
					if ( ImGui::Selectable( "None", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_NONE ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_NONE;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Entity Translate", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_ENTITY_TRANSLATE ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_ENTITY_TRANSLATE;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Offset", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_OFFSET ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_OFFSET;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Scale", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_SCALE ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_SCALE;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Rotate", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_ROTATE ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_ROTATE;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Scale Offset", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_SCALE_OFFSET ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_SCALE_OFFSET;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Stretch", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_STRETCH ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_STRETCH;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Scroll", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_SCROLL ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_SCROLL;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Transform", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_TRANSFORM ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_TRANSFORM;
						g_pMapInfoDlg->SetModified( true, true );
					}
					if ( ImGui::Selectable( "Turbulent", ( shaderEdit.stages[i].tcMod == Walnut::TMOD_TURBULENT ) ) ) {
						shaderEdit.stages[i].tcMod = Walnut::TMOD_TURBULENT;
						g_pMapInfoDlg->SetModified( true, true );
					}
					ImGui::EndCombo();
				}
			}
		}

		ImGui::End();
	}
}
