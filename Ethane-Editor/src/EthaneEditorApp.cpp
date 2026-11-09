#include "Ethane.h"
//-----EntryPoint-----
#include "Ethane/Core/EntryPoint.h"

#include "EditorLayer.h"
#include "TriangleLayer.h"
#include "TexturedQuadLayer.h"
#include "CubeLayer.h"

namespace Ethane {

	class EthaneEditor : public Application
	{
	public:
		EthaneEditor(ApplicationCommandLineArgs args)
			:Application("Ethane Editor", args)
		{
			PushLayer(new EditorLayer());
			// 2D demos: they draw at fixed depths, so they'd cover the 3D scene. Push them instead of CubeLayer to see them.
			// PushLayer(new TriangleLayer());
			// PushLayer(new TexturedQuadLayer());
			PushLayer(new CubeLayer());
		}

		~EthaneEditor()
		{

		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		return new EthaneEditor(args);
	}

}
