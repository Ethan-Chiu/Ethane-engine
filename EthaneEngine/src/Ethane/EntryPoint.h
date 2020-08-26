#pragma once

#ifdef ETH_PLATFORM_WINDOWS

extern Ethane::Application* Ethane::CreateApplication();

int main(int argc, char** argv) {

    auto app = Ethane::CreateApplication();
    app->Run();
    delete app;

}


#endif 

//EntryPoint_h