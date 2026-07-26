#include <RED4ext/RED4ext.hpp>

#include "version.h"
#include "NavPath.hpp"
#include "WorldMap.hpp"

// Captures the world-map preview camera position from native memory (there is no
// RTTI getter for it) and hides the GPS route line. Both are exposed to Redscript.
// See MapUtils.reds for the script API.

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle,
                                        RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        MapUtils::Init(aSdk, aHandle);

        aSdk->logger->Info(aHandle, "MapUtils: loaded.");

        aSdk->scripts->Add(aHandle, L"MapUtils.reds");

        NavPath::AttachHook();

        auto* rtti = RED4ext::CRTTISystem::Get();
        rtti->AddPostRegisterCallback(MapUtils::PostRegisterTypes);
        rtti->AddPostRegisterCallback(NavPath::PostRegisterTypes);
        break;
    }
    case RED4ext::v1::EMainReason::Unload:
        NavPath::DetachHook();
        break;
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name    = L"HardcoreMapAndNavigation";
    aInfo->author  = L"RMK";
    aInfo->version = RED4EXT_V1_SEMVER(VER_MAJOR, VER_MINOR, VER_PATCH);
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_LATEST;
    aInfo->sdk     = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1;
}
