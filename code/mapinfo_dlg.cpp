#include "editor.h"

CMapInfoDlg::CMapInfoDlg( void )
{
    m_bHasSpawnWindow = false;
    m_bHasCheckpointWindow = false;

    m_bTilesetModified = false;
    m_bMapModified = false;
    m_bMapNameUpdated = false;

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

void CMapInfoDlg::SetCurrent( mapData_t *data )
{
    for ( auto& it : m_MapList ) {
        if ( !it.m_bUsed ) {
            it.m_bUsed = true;
            it.m_pMapData = data;
            return;
        }
    }

    mapData = data;
    m_MapList.emplace_back( data );
}

void CMapInfoDlg::Draw( void )
{
    char mapnameTemp[MAX_NPATH];
    bool open;
    uint32_t i;
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                                | ImGuiTreeNodeFlags_FramePadding;

    if ( m_bMapModified && !m_bMapNameUpdated ) {
		char tempName[sizeof(mapData->name)+6];
		snprintf( tempName, sizeof(tempName), "%s*", mapData->name );
		m_bMapNameUpdated = true;
		Sys_SetWindowTitle( tempName );
	}

    if ( m_bHasCheckpointWindow ) {
		if ( ImGui::Begin( va( "checkpoint %u", (unsigned)( m_pCheckpointEdit - mapData->checkpoints ) ), &m_bHasCheckpointWindow,
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize ) )
		{
			DrawVec3Control( "position", m_pCheckpointEdit->xyz );
			ImGui::End();
		} else {
			m_bHasCheckpointWindow = false;
		}
	}

	if ( ImGui::BeginPopupModal( "Map Name Error" ) ) {
		ImGui::TextUnformatted( "Map names require the .map extension, do not remove it" );
		ImGui::EndPopup();
	}

    if ( m_bShow && ImGui::Begin( "Map Info", NULL, ImGuiWindowFlags_NoCollapse ) ) {
        if ( ImGui::BeginTabBar( "##MapSelector" ) ) {
            for ( auto& it : m_MapList ) {
                if ( !it.m_bUsed ) {
                    continue;
                }

                if ( ImGui::BeginTabItem( it.m_pMapData->name, &it.m_bUsed ) ) {
                    mapData = it.m_pMapData;
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

		if ( ImGui::IsWindowHovered() || ImGui::IsWindowFocused() ) {
			g_pEditor->m_bWindowFocused = true;
			g_pEditor->m_InputFocus = EditorInputFocus::ToolFocus;
		}

		ImGui::SeparatorText( "General" );

		ImGui::TextUnformatted( "Name: " );
		ImGui::SameLine();
		if ( ImGui::InputText( "##MapName", mapnameTemp, sizeof(mapnameTemp) - 1, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
			if ( !strrchr( mapnameTemp, '.' ) || N_stricmp( COM_GetExtension( mapnameTemp ), "map" ) ) {
				ImGui::OpenPopup( "Map Name Error" );
			}
			else {
				N_strncpyz( mapData->name, mapnameTemp, sizeof(mapData->name) );
				m_bMapNameUpdated = false;
				m_bMapModified = true;
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

			if ( mapData->height < 0 ) {
				mapData->height = 0;
			} else if ( mapData->height > MAX_MAP_HEIGHT ) {
				mapData->height = MAX_MAP_HEIGHT;
			}
		}

		ImGui::NewLine();

		ImGui::SeparatorText( "Checkpoints" );
	    for ( i = 0; i < mapData->numCheckpoints; i++ ) {
			ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 4 ) );

			open = ImGui::TreeNodeEx( (void *)(uintptr_t)&mapData->checkpoints[i], treeNodeFlags, "checkpoint %u", i );
			ImGui::SameLine();
			if ( ImGui::Button( "+" ) ) {
				m_pCheckpointEdit = &mapData->checkpoints[i];
				m_bHasCheckpointWindow = true;
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
				m_bHasSpawnWindow = true;
			}

			if ( open ) {
				DrawVec3Control( "position", mapData->spawns[i].xyz );
				if ( ImGui::BeginCombo( "Entity ID",
					va( "%s", m_nSelectedSpawnEntityId != -1 ?
					g_pPrefsDlg->m_EntityLists[m_nSelectedSpawnEntityType][m_nSelectedSpawnEntityId].m_Name.c_str() : "Entity ID" ) ) )
				{
					for ( uint32_t a = 0; a < g_pPrefsDlg->m_EntityLists[m_nSelectedSpawnEntityType].size(); a++ ) {
						if ( ImGui::Selectable( g_pPrefsDlg->m_EntityLists[m_nSelectedSpawnEntityType][a].m_Name.c_str(), ( m_nSelectedSpawnEntityId == a ) ) ) {
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

		if ( m_bTilesetModified ) {
			if ( ImGui::Button( "Build Tileset" ) ) {
                m_bTilesetModified = false;
				Map_BuildTileset();
			}
		}
		ImGui::End();
	}
}
