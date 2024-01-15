#include "editor.h"

CAssetManagerDlg::CAssetManagerDlg( void )
{
}

CAssetManagerDlg::~CAssetManagerDlg()
{
}

static void FindShaderFiles( const std::filesystem::path& currentPath, std::vector<std::filesystem::path>& shaderList )
{
	for ( const auto& it : std::filesystem::directory_iterator{ currentPath } ) {
		if ( it.is_directory() ) {
			FindShaderFiles( it, shaderList );
		} else if ( !N_stricmp( COM_GetExtension( it.path().filename().c_str() ), "shader" ) && !it.is_directory() ) {
			shaderList.emplace_back( it.path() );
		}
	}
}

void CAssetManagerDlg::OnAttach( void )
{
	m_CurrentDirectory = g_pProjectManager->GetProject()->m_AssetDirectory;
	m_BaseDirectory = m_CurrentDirectory;

	m_pDirectoryIcon = new Walnut::Image( va( "%sData/Icons/DirectoryIcon.png", g_pEditor->m_CurrentPath.c_str() ) );
    m_pFileIcon = new Walnut::Image( va( "%sData/Icons/FileIcon.png", g_pEditor->m_CurrentPath.c_str() ) );

	FindShaderFiles( m_BaseDirectory, m_ShaderList );
}

void CAssetManagerDlg::OnDetach( void )
{
	delete m_pDirectoryIcon;
	delete m_pFileIcon;
}

void CAssetManagerDlg::OnUIRender( void )
{
    static float padding = 16.0f;
	static float thumbnailSize = 128.0f;
	const float cellSize = thumbnailSize + padding;
	const float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount;
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                                | ImGuiTreeNodeFlags_FramePadding;

    if ( ImGui::Begin( "Asset Manager" ) ) {
		if ( ImGui::TreeNodeEx( (void *)(uintptr_t)"Shaders", treeNodeFlags, "Shaders" ) ) {
			if ( ImGui::Button( "Reload Shader List") ) {
				m_ShaderList.clear();
				Log_Printf( "[CAssetManagerDlg:OnUIRender] Reloading shader file list...\n" );
				FindShaderFiles( m_BaseDirectory, m_ShaderList );
			}
			for ( auto& it : m_ShaderList ) {
				if ( ImGui::MenuItem( it.filename().c_str() ) ) {
					g_pEditor->OnTextEdit( it );
				}
			}
			ImGui::TreePop();
		}
		if ( ImGui::TreeNodeEx( (void *)(uintptr_t)"Tileset", treeNodeFlags, "Tileset" ) ) {
			if ( ImGui::Button( "Diffuse Map" ) ) {
				if ( ImGui::BeginDragDropTarget() ) {
					if ( const ImGuiPayload *payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM" ) ) {
					}
					ImGui::EndDragDropTarget();
				}
			}
			if ( ImGui::Button( "Normal Map" ) ) {

			}
			ImGui::TreePop();
		}

        ImGui::End();
    }
}
