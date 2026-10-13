#include "$RUNTIMEMODULEModule.h"

#include "Core/Module/ModuleManager.h"
#include "Log/Log.h"

using namespace Lumina;

IMPLEMENT_MODULE(F$RUNTIMEMODULEModule, "$RUNTIMEMODULE");

void F$RUNTIMEMODULEModule::StartupModule()
{
    LOG_INFO("[$PLUGINNAME] Runtime module ready");

    // Register gameplay here: reflected components/systems, script bindings, etc.
}

void F$RUNTIMEMODULEModule::ShutdownModule()
{
    LOG_INFO("[$PLUGINNAME] Runtime module shutdown");
}
