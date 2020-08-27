#pragma once

#ifdef ETH_PLATFORM_WINDOWS

extern Ethane::Application* Ethane::CreateApplication();

int main(int argc, char** argv) {

    Ethane::Log::Init();
    Ethane::Log::GetCoreLogger()->warn("Initialized log");
    int a = 2;
    Ethane::Log::GetClientLogger()->info("Initialized log Var = {0}", a);
    auto app = Ethane::CreateApplication();
    app->Run();
    delete app;

}


#endif 

//EntryPoint_h