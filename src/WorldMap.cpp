#include "WorldMap.hpp"
#include "MemUtils.hpp"
#include "RttiHelpers.hpp" // MapUtils::Rtti::ClassFn

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>

// State (definitions for externs declared in WorldMap.hpp)

namespace MapUtils
{

const RED4ext::v1::Sdk*   s_sdk    = nullptr;
RED4ext::v1::PluginHandle s_handle = nullptr;

const RED4ext::CClass* s_mapObjectClass   = nullptr;
const RED4ext::CClass* s_targetPointClass = nullptr;

void Init(const RED4ext::v1::Sdk* aSdk, RED4ext::v1::PluginHandle aHandle)
{
    s_sdk    = aSdk;
    s_handle = aHandle;
}

} // namespace MapUtils

// Private helpers (TU-local)

namespace
{

// Reads the gameuiWorldMapGameObject entity handle at controller+0x128, then walks
// its components for entTargetPointComponent (the pan centre). SEH-guarded; no scan
// loop, so no AV exceptions in normal operation.
bool TryDirectAccess(RED4ext::IScriptable* aController, RED4ext::Vector3* aOut) noexcept
{
    __try
    {
        // controller+0x128 = Handle<gameuiWorldMapGameObject>.instance
        void* entity = ReadPtr(aController, 0x128);
        if (!entity || !IsReadable(entity))
            return false;

        auto* cls =
            static_cast<const RED4ext::CClass*>(ReadPtr(entity, kIScriptable_NativeType));
        if (!cls || !DerivesFrom(cls, MapUtils::s_mapObjectClass))
            return false;

        void*    entries = ReadPtr(entity, kEntity_Components);
        uint32_t count   = ReadU32(entity, kEntity_ComponentsSize);
        if (!entries || count == 0 || count > 256)
            return false;

        for (uint32_t i = 0; i < count && i < 48; ++i)
        {
            void* comp = ReadPtr(entries, static_cast<uintptr_t>(i) * kHandleStride);
            if (!comp)
                continue;
            auto* compCls =
                static_cast<const RED4ext::CClass*>(ReadPtr(comp, kIScriptable_NativeType));
            if (DerivesFrom(compCls, MapUtils::s_targetPointClass))
            {
                *aOut = ReadPlacedPosition(comp);
                return true;
            }
        }
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// Native function implementations

// MapUtils_GetWorldMapCameraPos() -> Vector3 - the world-map pan centre, or
// (0,0,0) on failure. Reads the entTargetPointComponent in the controller's map
// entity.
void GetWorldMapCameraPos(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame,
                          RED4ext::Vector3* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(a4);

    aFrame->code++; // skip ParamEnd

    if (aOut)
    {
        *aOut = {};
        if (aContext)
            TryDirectAccess(aContext, aOut);
    }
}

} // anonymous namespace

// RTTI registration

namespace MapUtils
{

void PostRegisterTypes()
{
    auto* rtti = RED4ext::CRTTISystem::Get();

    s_mapObjectClass   = rtti->GetClass("gameuiWorldMapGameObject");
    s_targetPointClass = rtti->GetClass("entTargetPointComponent");

    // Newer game versions expose the controller as gameuiWorldMapPreviewGameController;
    // fall back to the legacy inkWorldMapPreviewGameController name.
    const bool registered =
        Rtti::ClassFn("gameuiWorldMapPreviewGameController", "MapUtils_GetWorldMapCameraPos",
                      &GetWorldMapCameraPos, {}, "Vector3")
        || Rtti::ClassFn("inkWorldMapPreviewGameController", "MapUtils_GetWorldMapCameraPos",
                         &GetWorldMapCameraPos, {}, "Vector3");

    if (!registered && s_sdk)
        s_sdk->logger->Error(s_handle,
            "PostRegisterTypes: gameuiWorldMapPreviewGameController not found");
}

} // namespace MapUtils
