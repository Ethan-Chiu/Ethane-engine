#include <Ethane.h>

class Sandbox : public Ethane::Application
{
public:
	Sandbox()
	{

	}
	~Sandbox()
	{

	}
};

Ethane::Application* Ethane::CreateApplication()
{
	return new Sandbox();
}