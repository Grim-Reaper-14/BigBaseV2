#include "core/application.hpp"

#include <Windows.h>

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        reaper::core::application::instance().start();
        break;
    case DLL_PROCESS_DETACH:
        reaper::core::application::instance().request_stop();
        break;
    default:
        break;
    }

    return TRUE;
}
