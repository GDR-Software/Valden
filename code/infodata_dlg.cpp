#include "infodata_dlg.h"

levelData_t *g_pLevelData;

CInfoDataDlg::CInfoDataDlg( void )
{
    m_LevelDatas.resize( g_MapCache.size() );
}

void CInfoDataDlg::CreateLevelData( void )
{
    m_LevelDatas.emplace_back( GetMemory( sizeof( levelData_t ) ) );
    m_pCurrent = m_LevelDatas.end() - 1;

    g_pMapInfoDlg->SetModified( true, false );
}

void CInfoDataDlg::Draw( void )
{
    static const char *difTable[] = {
        "Very Easy (Noob)",
        "Easy (Recruit)",
        "Medium/Standard (Mercenary)",
        "Hard (Nomad)",
        "Very Hard/Brutal (The Black Death)",
        "Meme Mode"
    };

    if ( m_bShow && ImGui::Begin( "##InfoDataDlgWindow", NULL, ImGuiWindowFlags_NoCollapse ) ) {
        bool empty = false;

        for ( const auto& it : m_LevelDatas ) {
            if ( it.bUsed ) {
                empty = true;
                break;
            }
        }

        if ( empty ) {
            ImGui::TextUnformatted( "No Level Datas" );
        }
        else if ( ImGui::BeginTabBar( "Level Datas" ) ) {
            for ( auto it = m_LevelDatas.begin(); it != m_LevelDatas.end(); it++ ) {
                if ( !it->bUsed ) {
                    continue;
                }

                if ( ImGui::BeginTabItem( it->pData->name, &it->bUsed ) ) {
                    g_pLevelData = it->pData;
					m_pCurrent = it;
                    ImGui::EndTabItem();
                }

				if ( !it->bUsed ) {
					it->pData = NULL;
					if ( it == m_pCurrent ) {
						if ( m_pCurrent == m_LevelDatas.begin() && m_LevelDatas.size() > 1 ) {
							m_pCurrent++;
						} else if ( m_pCurrent == m_LevelDatas.end() - 1 && m_LevelDatas.size() > 1 ) {
							m_pCurrent--;
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

        if ( ImGui::BeginCombo( "Level Map", m_pCurrent->pData->mapData ? "None" : m_pCurrent->pData->mapData->name ) ) {
            for ( auto& it : g_MapCache ) {
                if ( ImGui::Selectable( it.name, ( m_pCurrent->pData->mapData == std::addressof( it ) ) ) ) {
                    m_pCurrent->pData->mapData = std::addressof( it );
                    g_pMapInfoDlg->SetModified( true, false );
                }
            }
            ImGui::EndCombo();
        }

        if ( ImGui::BeginCombo( "Difficulty", difTable[ m_pCurrent->pData->difficulty ] ) ) {
            for ( int i = 0; i < arraylen( difTable ); i++ ) {
                if ( ImGui::Selectable( difTable[i], ( m_pCurrent->pData->difficulty == i ) ) ) {
                    m_pCurrent->pData->difficulty = i;
                    g_pMapInfoDlg->SetModified( true, false );
                }
            }
            ImGui::EndCombo();
        }

        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
        if ( ImGui::Button( "Delete Level Info" ) ) {
            
        }
        ImGui::PopStyleColor( 3 );

        ImGui::End();
    }
}
