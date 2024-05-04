#include "Application.h"
#include "editor.h"
#include "events.h"
#include "gui.h"

#include "backends/imgui_impl_opengl3.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include "backends/imgui_impl_sdl2.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"

#include "gln.h"
#include <iostream>

// Emedded font
#include "Walnut/ImGui/Roboto-Regular.embed"

static void GL_ErrorCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam )
{
    switch ( type ) {
    case GL_DEBUG_TYPE_ERROR:
        Error( "[OpenGL error] %u: %s", id, message );
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        Log_Printf( "[OpenGL perfomance] %u: %s\n", id, message );
        break;
    };
}


// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#define IMGUI_UNLIMITED_FRAME_RATE

static std::vector<std::function<void()>> s_ResourceFreeQueue;
static Walnut::Application* s_Instance = NULL;

namespace Walnut {

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		s_Instance = this;

		m_ConsoleHeight = 528;
		Init();
	}

	Application::~Application()
	{
		Shutdown();

		s_Instance = NULL;
	}

	Application& Application::Get( void )
	{
		return *s_Instance;
	}

	void Application::Init( void )
	{
    	Log_Printf( "[Application::Init] Setting up GUI\n" );

	    m_WindowHandle = SDL_CreateWindow( m_Specification.Name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_Specification.Width, m_Specification.Height,
	        SDL_WINDOW_OPENGL | SDL_WINDOW_MOUSE_CAPTURE );
	    if ( !m_WindowHandle ) {
	        Error( "[Application::Init] Failed to create SDL2 window, reason: %s", SDL_GetError() );
	    }
		m_GLContext = SDL_GL_CreateContext( m_WindowHandle );
	    if ( !m_GLContext ) {
	        Error( "[Application::Init] Failed to create SDL_GLContext, reason: %s", SDL_GetError() );
	    }
	    SDL_GL_MakeCurrent( m_WindowHandle, m_GLContext );

		SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
	    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
	    SDL_GL_SetSwapInterval( -1 );

	    Log_Printf( "[Application::Init] loading gl procs\n" );

	    if ( !gladLoadGL( (GLADloadfunc)SDL_GL_GetProcAddress ) ) {
	        Error( "Failed to init GLAD2" );
	    }

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
//		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		io.ConfigViewportsNoTaskBarIcon = true;
		io.WantSaveIniSettings = true;
		io.IniFilename = CopyString( va( "%svalden-imgui.ini", g_pEditor->m_CurrentPath.c_str() ) );

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		ImGui_ImplSDL2_InitForOpenGL( m_WindowHandle, m_GLContext );

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		glEnable( GL_DEBUG_OUTPUT );
	    glDebugMessageCallback( GL_ErrorCallback, NULL );
	    uint32_t unusedIds = 0;
	    glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, (GLuint *)&unusedIds, GL_TRUE );


		// Load default font
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		ImFont* robotoFont = io.Fonts->AddFontFromMemoryTTF( (void *)g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &fontConfig );
		io.FontDefault = robotoFont;

		ImGui_ImplOpenGL3_Init( "#version 330" );
		ImGui_ImplOpenGL3_CreateDeviceObjects();
		ImGui_ImplOpenGL3_CreateFontsTexture();
	}

	void Application::Shutdown( void )
	{
		for ( auto& layer : m_LayerStack ) {
			layer->OnDetach();
		}

		g_pMapDrawer->OnDetach();
		m_LayerStack.clear();

		// Free resources in queue
		for (auto& func : s_ResourceFreeQueue) {
			func();
		}
		s_ResourceFreeQueue.clear();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
		
		SDL_GL_MakeCurrent( m_WindowHandle, NULL );
		SDL_GL_DeleteContext( m_GLContext );
		SDL_DestroyWindow( m_WindowHandle );

		SDL_Quit();
	}

	void Application::Run( void )
	{
		m_Running = true;
		ImGuiIO& io = ImGui::GetIO();

		// Main loop
		while ( m_Running ) {
			glClear( GL_COLOR_BUFFER_BIT );
			glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );
			glViewport( 0, 0, m_Specification.Width, m_Specification.Height );

			io.DisplaySize.x = m_Specification.Width;
			io.DisplaySize.y = m_Specification.Height;

			{
				ImGui::NewFrame();
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame();

				events.EventLoop();

				g_pMapDrawer->OnUpdate( m_TimeStep );
				for ( auto& layer : m_LayerStack ) {
					layer->OnUpdate( m_TimeStep );
				}

				{
					ImVec2 size;
					const ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
					std::unique_lock<std::mutex> lock{ g_ImGuiLock };
					const ImGuiViewport* viewport = ImGui::GetMainViewport();

					size = { viewport->WorkSize.x / 2.0f, viewport->WorkSize.y };

					// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
					// because it would be confusing to have two docking targets within each others.
					ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

					ImGui::SetNextWindowPos( { 0, 24 } );
					ImGui::SetNextWindowSize( size );
					ImGui::SetNextWindowViewport( viewport->ID );
					ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
					window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
					window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

					// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
					// and handle the pass-thru hole, so we ask Begin() to not render a background.
					if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
						window_flags |= ImGuiWindowFlags_NoBackground;

					// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
					// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
					// all active windows docked into it will lose their parent and become undocked.
					// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
					// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
					ImGui::Begin("DockSpace Demo", nullptr, window_flags);
					m_DockspaceWidth = ImGui::GetWindowSize().x;
					ImGui::PopStyleVar();
					
					size = ImGui::GetWindowSize();
					if ( size.x > viewport->WorkSize.x * 0.75f ) {
						size.x = viewport->WorkSize.x * 0.75f;
					} else if ( size.x < viewport->WorkSize.x * 0.25f ) {
						size.x = viewport->WorkSize.x * 0.25f;
					}

					ImGui::PopStyleVar( 2 );

					// Submit the DockSpace
					ImGuiIO& io = ImGui::GetIO();
					if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
					{
						ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
						ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
					}

					lock.unlock();

					ImGui::End();
				}

				if ( m_MenubarCallback ) {
					if ( ImGui::BeginMainMenuBar() ) {
						m_MenubarCallback();
						ImGui::EndMainMenuBar();
					}
				}

				g_pMapDrawer->OnUIRender();
				for ( auto& layer : m_LayerStack ) {
					layer->OnUIRender();
				}

				// Rendering
				ImGui::Render();
				ImDrawData* main_draw_data = ImGui::GetDrawData();
				const bool main_is_minimized = ( main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f );
				if ( !main_is_minimized ) {
					ImGui_ImplOpenGL3_RenderDrawData( main_draw_data );
				}

				// Update and Render additional Platform Windows
				if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) {
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
				}
			}

			float time = GetTime();
			m_FrameTime = time - m_LastFrameTime;
			m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
			m_LastFrameTime = time;

			SDL_GL_SwapWindow( m_WindowHandle );
		}
	}

	void Application::Close( void )
	{
		if ( this != NULL ) {
			m_Running = false;
		}
	}

	float Application::GetTime( void )
	{
		return (float)SDL_GetTicks();
	}

	void Application::SubmitResourceFree(std::function<void()>&& func)
	{
		s_ResourceFreeQueue.emplace_back(func);
	}

}