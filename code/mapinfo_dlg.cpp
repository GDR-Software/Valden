#include "editor.h"
#include "gui.h"

#define SPECULAR_SCALE_RGBGLOSS 0
#define SPECULAR_SCALE_MS 1
#define SPECULAR_SCALE_RGB 2

#define NORMAL_SCALE_XY 0
#define NORMAL_SCALE_XY_HEIGHT 1

typedef struct
{
	bool active;

	Walnut::waveForm_t rgbWave;
	Walnut::colorGen_t rgbGen;

	Walnut::waveForm_t alphaWave;
	Walnut::alphaGen_t alphaGen;

	byte constantColor[4];

	int blendFuncSrc;
	int blendFuncDest;

	uint32_t textureType;

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

static const char *EntityTypeToString( entitytype_t type )
{
    switch ( type ) {
    case ET_PLAYR: return "Player";
    case ET_MOB: return "Mob";
    case ET_BOT: return "Bot";
    case ET_ITEM: return "Item";
    case ET_WEAPON: return "Weapon";
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

static bool MapIsInProjectList( const char *name )
{
	if ( g_pProjectManager->GetProject().get() ) {
		for ( const auto& it : g_pProjectManager->GetProject()->m_MapList ) {
			if ( strcmp( it->name, name ) == 0 ) {
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
	if ( !present ) {
		g_pEditor->m_RecentFiles.emplace_back( data->name );
	}

	if ( !MapIsInProjectList( data->name ) && g_pProjectManager->GetProject().get() ) {
		g_pProjectManager->GetProject()->m_MapList.emplace_back( data );
	}

	N_strncpyz( m_szTempMapName, mapData->name, sizeof(m_szTempMapName) );
}

static void ShaderEdit( void );

void CMapInfoDlg::Draw( void )
{
    char mapnameTemp[MAX_NPATH];
    bool open;
    uint32_t i;
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                                | ImGuiTreeNodeFlags_FramePadding;

	if ( ImGui::BeginPopup( "TileMode" ) ) {
		const int x = g_pMapDrawer->m_nTileSelectX;
		const int y = g_pMapDrawer->m_nTileSelectY;

		ImGui::Text( "Editing Tile At %ix%i", x, y );
		if ( mapData->tiles[y * mapData->width + x].flags & TILETYPE_CHECKPOINT ) {
			ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
		}
		if ( ImGui::Button( "Set Checkpoint" ) ) {
			mapData->tiles[y * mapData->width + x].flags |= TILETYPE_CHECKPOINT;
			m_bMapModified = true;
			m_bMapNameUpdated = false;
		}
		if ( mapData->tiles[y * mapData->width + x].flags & TILETYPE_CHECKPOINT ) {
			ImGui::PopStyleColor();
		}

		if ( mapData->tiles[y * mapData->width + x].flags & TILETYPE_SPAWN ) {
			ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
		}
		if ( ImGui::Button( "Set Checkpoint" ) ) {
			mapData->tiles[y * mapData->width + x].flags |= TILETYPE_SPAWN;
			m_bMapModified = true;
			m_bMapNameUpdated = false;
		}
		if ( mapData->tiles[y * mapData->width + x].flags & TILETYPE_SPAWN ) {
			ImGui::PopStyleColor();
		}


		if ( ImGui::Button( "CANCEL" ) ) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ShaderEdit();

	if ( ImGui::BeginPopup( "CheckpointEdit", ImGuiWindowFlags_AlwaysAutoResize ) ) {
		ImGui::SetWindowFocus();
		ImGui::SeparatorText( va( "checkpoint %u", (unsigned)( m_pCheckpointEdit - mapData->checkpoints ) ) );
		DrawVec3Control( "position", m_pCheckpointEdit->xyz );
		if ( ImGui::Button( "delete" ) ) {
			mapData->numCheckpoints--;
			m_bMapModified = true;
			ImGui::CloseCurrentPopup();
		}
		if ( ImGui::Button( "CANCEL" ) ) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if ( ImGui::BeginPopup( "SpawnEdit", ImGuiWindowFlags_AlwaysAutoResize ) ) {
		ImGui::SetWindowFocus();
		ImGui::SeparatorText( va( "spawn %u", (unsigned)( m_pSpawnEdit - mapData->spawns ) ) );
		DrawVec3Control( "position", m_pSpawnEdit->xyz );
		if ( ImGui::Button( "delete" ) ) {
			mapData->numSpawns--;
			m_bMapModified = true;
			ImGui::CloseCurrentPopup();
		}
		if ( ImGui::Button( "CANCEL" ) ) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if ( ImGui::BeginPopup( "LightEdit", ImGuiWindowFlags_AlwaysAutoResize ) ) {
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
			ImGui::CloseCurrentPopup();
		}
		if ( ImGui::Button( "CANCEL" ) ) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
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
			if ( strcmp( m_szTempMapName, mapData->name ) != 0 ) {
				snprintf( mapData->name, sizeof(mapData->name), "%s.map", m_szTempMapName );

				m_bMapModified = true;

				Sys_SetWindowTitle( va( "%s*", mapData->name ) );
			}
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

			open = ImGui::TreeNodeEx( (void *)(uintptr_t)&mapData->lights[i], treeNodeFlags, "spawn %u", i );
			ImGui::SameLine();
			if ( ImGui::Button( "+" ) ) {
				m_pLightEdit = &mapData->lights[i];
				ImGui::OpenPopup( "LightEdit" );
			}

			if ( open ) {
				DrawVec3Control( "position", mapData->checkpoints[i].xyz );
				ImGui::TreePop();
			}
			ImGui::PopStyleVar();
		}

		ImGui::SeparatorText( "Checkpoints" );
	    for ( i = 0; i < mapData->numCheckpoints; i++ ) {
			ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 4 ) );

			open = ImGui::TreeNodeEx( (void *)(uintptr_t)&mapData->checkpoints[i], treeNodeFlags, "checkpoint %u", i );
			ImGui::SameLine();
			if ( ImGui::Button( "+" ) ) {
				m_pCheckpointEdit = &mapData->checkpoints[i];
				ImGui::OpenPopup( "CheckpointEdit" );
			}

			if ( open ) {
				DrawVec3Control( "position", mapData->checkpoints[i].xyz );
				ImGui::TreePop();
			}
			ImGui::PopStyleVar();
		}

		ImGui::SeparatorText( "Spawns" );
		for ( i = 0; i < mapData->numSpawns; i++ ) {
			ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 4 ) );

			open = ImGui::TreeNodeEx( (void *)(uintptr_t)&mapData->spawns[i], treeNodeFlags, "spawn %u", i );
			ImGui::SameLine();
			if ( ImGui::Button( "+" ) ) {
				m_pSpawnEdit = &mapData->spawns[i];
				ImGui::OpenPopup( "SpawnEdit" );
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
					if ( ImGui::Selectable( "weapon", ( m_nSelectedSpawnEntityType == ET_WEAPON ) ) ) {
						m_nSelectedSpawnEntityType = ET_WEAPON;
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
		
		ImGui::SeparatorText( "Tileset" );
		ImGui::TextUnformatted( "Name: " );
		ImGui::SameLine();
		if ( ImGui::InputText( "##TilesetName", mapData->tileset.texture, sizeof(mapData->tileset.texture), ImGuiInputTextFlags_EnterReturnsTrue ) ) {
			m_bMapModified = true;
			m_bTilesetModified = true;
		}

		ImGui::Text( "Image Width: %s", mapData->textures[TU_DIFFUSEMAP] ? va( "%i", mapData->textures[TU_DIFFUSEMAP]->GetWidth() ) : "N/A" );
		ImGui::Text( "Image Height: %s", mapData->textures[TU_DIFFUSEMAP] ? va( "%i", mapData->textures[TU_DIFFUSEMAP]->GetHeight() ) : "N/A" );
		
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

        const char *textureFileDlgFilters =
            ".jpg,.jpeg,.png,.bmp,.tga,.webp,"
			"Jpeg Files (*.jpeg *.jpg){.jpeg,.jpg},"
			"Image Files (*.jpg *.jpeg *.png *.bmp *.tga *.webp){.jpg,.jpeg,.png,.bmp,.tga,.webp}";

		ImGui::TextUnformatted( "Diffuse Map: " );
		ImGui::SameLine();
		if ( ImGui::Button( mapData->textures[Walnut::TB_DIFFUSEMAP] ? mapData->textures[Walnut::TB_DIFFUSEMAP]->GetName().c_str() : "None##DiffuseMap" ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectDiffuseMapDlg", "Select Diffuse Texture File",
                textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		ImGui::TextUnformatted( "Normal Map: " );
		ImGui::SameLine();
		if ( ImGui::Button( mapData->textures[Walnut::TB_NORMALMAP] ? mapData->textures[Walnut::TB_NORMALMAP]->GetName().c_str() : "None##NormalMap" ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectNormalMapDlg", "Select Normal Texture File",
				textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		ImGui::TextUnformatted( "Specular Map: " );
		ImGui::SameLine();
		if ( ImGui::Button( mapData->textures[Walnut::TB_SPECULARMAP] ? mapData->textures[Walnut::TB_SPECULARMAP]->GetName().c_str() : "None##SpecularMap" ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectSpecularMapDlg", "Select Specular Texture File",
				textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		ImGui::TextUnformatted( "Ambient Occulusion Map: " );
		ImGui::SameLine();
		if ( ImGui::Button( mapData->textures[Walnut::TB_LIGHTMAP] ? mapData->textures[Walnut::TB_LIGHTMAP]->GetName().c_str() : "None##AmbientOcclusionMap" ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "SelectAmbientOccMapDlg", "Select Ambient Occlusion Texture File",
				textureFileDlgFilters,
				va( "%s%ctextures%c", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP ) );
		}

		if ( !mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
			ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
		}
		if ( ImGui::Button( "Edit Shader" ) && mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
			ImGui::OpenPopup( "EditShader" );
		}
		if ( !mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
			ImGui::PopStyleColor();
		}

		if ( m_bTilesetModified ) {
			if ( ImGui::Button( "Build Tileset" ) ) {
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
	if ( ImGui::Begin( va( "Edit Shader %s",  ), NULL, ImGuiWindowFlags_None ) ) {
		ImGui::SeparatorText( "Edit Shader" );

		for ( uint32_t i = 0; i < MAX_SHADER_STAGES; i++ ) {
			if ( ImGui::CollapsingHeader( va( "Shader Stage %i", i ) ) ) {
				if ( ImGui::Checkbox( "Active", &shaderEdit.stages[i].active ) ) {
					g_pMapInfoDlg->m_bMapModified = true;
					g_pMapInfoDlg->m_bTilesetModified = true;
				}
				if ( ImGui::BeginCombo( "Specular Scale Type", SpecularScaleTypeString( shaderEdit.stages[i].specularScaleType ) ) ) {
					if ( ImGui::Selectable( "<Metallic> <Smoothness> (Requires r_pbr == 1 in game)",
						shaderEdit.stages[i].specularScaleType == SPECULAR_SCALE_MS ) )
					{
						shaderEdit.stages[i].specularScaleType =  SPECULAR_SCALE_MS;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "<r> <g> <b>", shaderEdit.stages[i].specularScaleType == SPECULAR_SCALE_RGB ) ) {
						shaderEdit.stages[i].specularScaleType = SPECULAR_SCALE_RGB;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "<r> <g> <b> <gloss>", shaderEdit.stages[i].specularScaleType == SPECULAR_SCALE_RGBGLOSS ) ) {
						shaderEdit.stages[i].specularScaleType = SPECULAR_SCALE_RGBGLOSS;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					ImGui::EndCombo();
				}


				if ( ImGui::BeginCombo( "Normal Scale Type", NormalScaleTypeString( shaderEdit.stages[i].normalScaleType ) ) ) {
					if ( ImGui::Selectable( "<X> <Y>", shaderEdit.stages[i].normalScaleType == NORMAL_SCALE_XY ) ) {
						shaderEdit.stages[i].normalScaleType = NORMAL_SCALE_XY;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "<X> <Y> <Height>", shaderEdit.stages[i].normalScaleType == NORMAL_SCALE_XY_HEIGHT ) ) {
						shaderEdit.stages[i].normalScaleType = NORMAL_SCALE_XY_HEIGHT;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					ImGui::EndCombo();
				}

				if ( ImGui::BeginCombo( "RGB Gen", RGBGenToString( shaderEdit.stages[i].rgbGen ) ) ) {
					
					if ( ImGui::Selectable( "Waveform", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_WAVEFORM ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_WAVEFORM;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Constant", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_CONST ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_CONST;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Identity", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_IDENTITY ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_IDENTITY;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Identity Lighting", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_IDENTITY_LIGHTING ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_IDENTITY_LIGHTING;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Entity", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_ENTITY ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_ENTITY;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "One Minus Entity", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_ONE_MINUS_ENTITY ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_ONE_MINUS_ENTITY;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "One Minus Vertex", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_ONE_MINUS_VERTEX ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_ONE_MINUS_VERTEX;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Vertex", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_VERTEX ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_VERTEX;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Exact Vertex", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_EXACT_VERTEX ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_EXACT_VERTEX;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Exact Vertex Lighting", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_EXACT_VERTEX_LIT ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_EXACT_VERTEX_LIT;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Vertex Lighting", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_VERTEX_LIT ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_VERTEX_LIT;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}
					if ( ImGui::Selectable( "Diffuse Lighting", ( shaderEdit.stages[i].rgbGen == Walnut::CGEN_LIGHTING_DIFFUSE ) ) ) {
						shaderEdit.stages[i].rgbGen = Walnut::CGEN_LIGHTING_DIFFUSE;
						g_pMapInfoDlg->m_bMapModified = true;
						g_pMapInfoDlg->m_bTilesetModified = true;
					}

					ImGui::EndCombo();
				}
				if ( ImGui::BeginCombo( "Alpha Gen", AlphaGenToString( shaderEdit.stages[i].alphaGen ) ) ) {
					ImGui::EndCombo();
				}
				if ( ImGui::BeginCombo( "Texture Coordinate Generate (tcGen)", "" ) ) {
					ImGui::EndCombo();
				}
				if ( ImGui::BeginCombo( "Texture Coordinate Modification (tcMod)", "" ) ) {
					ImGui::EndCombo();
				}
			}
		}

		ImGui::End();
	}
}
