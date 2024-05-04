#include "gln.h"
#include "editor.h"
#include "Walnut/Random.h"

ContentBrowserPanel::ContentBrowserPanel( void )
	: m_BaseDirectory( g_pProjectManager->GetAssetDirectory() ), m_CurrentDirectory( m_BaseDirectory )
{
}

ContentBrowserPanel::~ContentBrowserPanel()
{
}

void ContentBrowserPanel::OnAttach( void ) {
	m_pFileIcon = new Walnut::Image( va( "%sbitmaps/FileIcon.png", g_pEditor->m_CurrentPath.c_str() ) );
	m_pDirectoryIcon = new Walnut::Image( va( "%sbitmaps/DirectoryIcon.png", g_pEditor->m_CurrentPath.c_str() ) );
}

void ContentBrowserPanel::OnDetach( void ) {
	delete m_pFileIcon;
	delete m_pDirectoryIcon;
}

void ContentBrowserPanel::OnUIRender( void )
{
	static float thumbnailSize = 132.0f;
	const float cellSize = thumbnailSize;
	const float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount;

	ImGui::Begin( "Content Browser" );

	if ( m_CurrentDirectory != m_BaseDirectory ) {
		if ( ImGui::Button( "<-" ) ) {
			m_CurrentDirectory = std::filesystem::path( m_CurrentDirectory ).parent_path();
		}
	}

	columnCount = (int)( panelWidth / cellSize );

	if ( columnCount < 1 ) {
		columnCount = 1;
    }

    ImGui::Columns( columnCount, NULL, false );

	for ( const auto& directoryEntry : std::filesystem::directory_iterator{ m_CurrentDirectory } ) {
		const auto& path = directoryEntry.path();
		std::string filenameString = path.string();
		
		ImGui::PushID( filenameString.c_str() );
		Walnut::Image *icon = directoryEntry.is_directory() ? m_pDirectoryIcon : m_pFileIcon;
        const ImGuiID id = directoryEntry.is_directory() ? m_pDirectoryIcon->GetRendererID() : m_pFileIcon->GetRendererID();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
		if ( ImGui::ImageButtonEx( id, (ImTextureID)(intptr_t)icon->GetID(), { thumbnailSize, thumbnailSize }, { 0, 0 }, { 1, 1 }, { 0, 0, 0, 0 }, { 1, 1, 1, 1 } ) ) {
			m_ItemPath = filenameString;
			m_bItemSelected = true;
		}
		
        if ( ImGui::BeginDragDropSource() ) {
            const char *itemPath = path.c_str();
            ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM", itemPath, ( strlen( itemPath ) + 1 ) * sizeof(char) );
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

	// TODO: status bar
	ImGui::End();
}