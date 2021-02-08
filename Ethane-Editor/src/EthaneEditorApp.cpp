#include <Ethane.h>
//-----EntryPoint-----
#include <Ethane/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Ethane {

	class EthaneEditor : public Application
	{
	public:
		EthaneEditor()
			:Application("Ethane Editor")
		{
			PushLayer(new EditorLayer());
		}

		~EthaneEditor()
		{

		}
	};

	Application* CreateApplication()
	{
		return new EthaneEditor();
	}

}