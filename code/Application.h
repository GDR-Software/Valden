#pragma once

#include "Walnut/Layer.h"

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <SDL2/SDL.h>

#include "imgui.h"

namespace Walnut {

	struct ApplicationSpecification
	{
		std::string Name = "Walnut App";
		uint32_t Width = 1600;
		uint32_t Height = 900;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification());
		~Application();

		static Application& Get( void );

		void Run( void );
		void SetMenubarCallback(const std::function<void()>& menubarCallback) { m_MenubarCallback = menubarCallback; }
		
		template<typename T>
		void PushLayer( void )
		{
			static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
			m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
		}

		void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

		void Close( void );

		float GetTime( void );
		SDL_Window* GetWindowHandle() const { return m_WindowHandle; }

		static void SubmitResourceFree(std::function<void()>&& func);
	private:
		void Init();
		void Shutdown();
	public:
		ApplicationSpecification m_Specification;
		std::vector<std::shared_ptr<Layer>> m_LayerStack;

		bool m_Running = false;

		float m_TimeStep = 0.0f;
		float m_FrameTime = 0.0f;
		float m_LastFrameTime = 0.0f;

		int m_DockspaceWidth;
		int m_ConsoleHeight;

		std::function<void()> m_MenubarCallback;

		SDL_Window *m_WindowHandle;
		SDL_GLContext m_GLContext;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}