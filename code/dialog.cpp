#include "editor.h"

CFileDialogLayer::CFileDialogLayer( void )
{
    m_FileDlgStack.reserve( 64 );
}

void CFileDialogLayer::OnAttach( void ) {
}

void CFileDialogLayer::OnDetach( void ) {
}

void CFileDialogLayer::OnUpdate( float timestep ) {
}

void CFileDialogLayer::OnUIRender( void )
{
    std::vector<FileDialog>::const_iterator it;

    for ( it = m_FileDlgStack.cbegin(); it != m_FileDlgStack.cend(); it++ ) {
        if ( ImGuiFileDialog::Instance()->IsOpened( it->m_pId ) ) {
            ImGui::SetNextWindowViewport( ImGui::GetMainViewport()->ID );
            if ( ImGuiFileDialog::Instance()->Display( it->m_pId, ImGuiWindowFlags_NoResize, FileDlgWindowSize, FileDlgWindowSize ) ) {
                if ( ImGuiFileDialog::Instance()->IsOk() ) {
                    it->m_Activate( ImGuiFileDialog::Instance()->GetFilePathName() );
                    m_FileDlgStack.erase( it );
                }
                ImGuiFileDialog::Instance()->Close();
            }
        } else {
            m_FileDlgStack.erase( it );
        }
    }
}

void CTextEditDlg::OnAttach( void )
{
    m_pDirectoryIcon = new Walnut::Image( va( "%sData/Icons/DirectoryIcon.png", g_pEditor->m_CurrentPath.c_str() ) );
    m_pFileIcon = new Walnut::Image( va( "%sData/Icons/FileIcon.png", g_pEditor->m_CurrentPath.c_str() ) );

    m_CurrentDirectory = va( "%s%cScripts", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP );
    m_BaseDirectory = m_CurrentDirectory;
}

void CTextEditDlg::OnDetach( void )
{
    delete m_pFileIcon;
    delete m_pDirectoryIcon;
}

void CTextEditDlg::OnUIRender( void )
{
    static float padding = 16.0f;
	static float thumbnailSize = 128.0f;
	const float cellSize = thumbnailSize + padding;
	const float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount;

	ImGui::Begin( m_Label.c_str() );

	if ( m_CurrentDirectory != std::filesystem::path( m_BaseDirectory ) ) {
		if ( ImGui::Button( "<-" ) ) {
			m_CurrentDirectory = m_CurrentDirectory.parent_path();
		}
	}

	columnCount = (int)( panelWidth / cellSize );

	if ( columnCount < 1 ) {
		columnCount = 1;
    }

    ImGui::Columns( columnCount, NULL, false );

	for ( const auto& directoryEntry : std::filesystem::directory_iterator{ m_CurrentDirectory } ) {
		const auto& path = directoryEntry.path();
		std::string filenameString = path.filename().string();
		
		ImGui::PushID( filenameString.c_str() );
		Walnut::Image *icon = directoryEntry.is_directory() ? m_pDirectoryIcon : m_pFileIcon;
        const ImGuiID id = directoryEntry.is_directory() ? m_pDirectoryIcon->GetRendererID() : m_pFileIcon->GetRendererID();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
		if ( ImGui::ImageButtonEx( id, (ImTextureID)(intptr_t)icon->GetID(), { thumbnailSize, thumbnailSize }, { 0, 0 }, { 1, 1 }, { 0, 0, 0, 0 }, { 1, 1, 1, 1 } ) ) {
		}
		
        if ( ImGui::BeginDragDropSource() ) {
			std::filesystem::path relativePath( path );
        #ifdef _WIN32
			const wchar_t* itemPath = relativePath.c_str();
			ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM", itemPath, ( wcslen(itemPath) + 1 ) * sizeof(wchar_t) );
        #else
            const char *itemPath = relativePath.c_str();
            ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM", itemPath, ( strlen( itemPath ) + 1 ) * sizeof(char) );
        #endif
			ImGui::EndDragDropSource();
		}

		ImGui::PopStyleColor();
		if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) ) {
			if ( directoryEntry.is_directory() ) {
				m_CurrentDirectory = path;
            } else {

            }
		}

        const char *name = strrchr( filenameString.c_str(), PATH_SEP );
		ImGui::TextWrapped( name ? name + 1 : filenameString.c_str() );
		ImGui::NextColumn();
		ImGui::PopID();
	}

	ImGui::Columns( 1 );
	ImGui::SliderFloat( "Thumbnail Size", &thumbnailSize, 16, 512 );
//	ImGui::SliderFloat( "Padding", &padding, 0, 32 );

	// TODO: status bar
	ImGui::End();
}

CPopupDlg::CPopupDlg( void )
    : CEditorDialog()
{
}

void CPopupDlg::OnActivate( void )
{
    m_bActive = true;
    m_nOption = -1;
    ImGui::OpenPopup( m_Label.c_str() );
}

void CPopupDlg::Draw( void )
{
    if ( !m_bActive ) {
        return;
    }

    int buttonType;
    int iconType;

    buttonType = m_PopupFlags & MB_BUTTONBITS;
	iconType = m_PopupFlags & MB_ICONBITS;

    ImGui::BeginPopupModal( m_Label.c_str(), NULL, m_Flags );

    switch ( iconType ) {
    case MB_ICONINFORMATION:
        ImGui::TextUnformatted( "** NOTIFICATION **\n" );
        break;
    case MB_ICONERROR:
        ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "** ERROR **\n" );
        break;
    case MB_ICONWARNING:
        ImGui::TextColored( ImVec4( 1.0f, 0.75f, 0.0f, 1.0f ), "** WARNING **\n" );
    case MB_ICONQUESTION:
    default:
        break;
    };

    for ( auto *it : m_Widgets ) {
        it->Draw();
    }

    switch ( buttonType ) {
    case MB_OK:
        if ( ImGui::Button( "OK" ) ) {
            m_nOption = IDOK;
        }
        break;
    case MB_OKCANCEL:
        if ( ImGui::Button( "OK" ) ) {
            m_nOption = IDOK;
        }
        ImGui::SameLine();
        if ( ImGui::Button( "CANCEL" ) ) {
            m_nOption = IDCANCEL;
        }
        break;
    case MB_ABORTRETRYIGNORE:
        if ( ImGui::Button( "ABORT" ) ) {
            m_nOption = IDABORT;
        }
        ImGui::SameLine();
        if ( ImGui::Button( "RETRY" ) ) {
            m_nOption = IDRETRY;
        }
        ImGui::SameLine();
        if ( ImGui::Button( "IGNORE" ) ) {
            m_nOption = IDIGNORE;
        }
        break;
    case MB_YESNOCANCEL:
        if ( ImGui::Button( "YES" ) ) {
            m_nOption = IDYES;
        }
        ImGui::SameLine();
        if ( ImGui::Button( "NO" ) ) {
            m_nOption = IDNO;
        }
        ImGui::SameLine();
        if ( ImGui::Button( "CANCEL" ) ) {
            m_nOption = IDCANCEL;
        }
        break;
    case MB_YESNO:
        if ( ImGui::Button( "YES" ) ) {
            m_nOption = IDYES;
        }
        ImGui::SameLine();
        if ( ImGui::Button( "NO" ) ) {
            m_nOption = IDNO;
        }
        break;
    case MB_RETRYCANCEL:
        if ( ImGui::Button( "RETRY" ) ) {
            m_nOption = IDYES;
        }
        ImGui::SameLine();
        if ( ImGui::Button( "CANCEL" ) ) {
            m_nOption = IDCANCEL;
        }
        break;
    };

    if ( m_nOption != -1 ) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void CUIText::Draw( void )
{
    ImGui::TextUnformatted( m_TextBuf.c_str() );
}

void CUISlider::Draw( void )
{
    switch ( m_Type ) {
    case SLIDER_INT:
        ImGui::SliderInt( m_Label.c_str(), &m_Data.i, m_Min.i, m_Max.i );
        break;
    case SLIDER_FLOAT:
        ImGui::SliderFloat( m_Label.c_str(), &m_Data.f, m_Min.f, m_Max.f );
        break;
    case SLIDER_COLOR3:
        ImGui::ColorEdit3( m_Label.c_str(), m_Data.c3 );
        break;
    case SLIDER_COLOR4:
        ImGui::ColorEdit4( m_Label.c_str(), m_Data.c4 );
        break;
    default:
        Error( "CUISlider::Draw: invalid data type" );
    };
}
