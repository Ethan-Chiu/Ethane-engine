#include "Ethane.h"
//-----EntryPoint-----
#include "Ethane/Core/EntryPoint.h"

#include "EditorLayer.h"
#include "TriangleLayer.h"

namespace Ethane {

	class EthaneEditor : public Application
	{
	public:
		EthaneEditor(ApplicationCommandLineArgs args)
			:Application("Ethane Editor", args)
		{
			PushLayer(new EditorLayer());
			PushLayer(new TriangleLayer());
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
