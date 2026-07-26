#include "NavPath.hpp"
#include "RttiHelpers.hpp" // MapUtils::Rtti::GlobalFn / ClassFn
#include "WorldMap.hpp" // MapUtils::s_sdk / MapUtils::s_handle

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Relocation.hpp>
#include <RED4ext/Handle.hpp>
#include <RED4ext/Scripting/Natives/inkWidget.hpp>
#include <RED4ext/Scripting/Natives/inkWidgetReference.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ui/MappinsContainerController.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

// MappinsContainerController::OnGPSPathChanged hash (from in_world_navigation),
// resolved at runtime via the RED4ext address library to survive game updates.
namespace
{
constexpr uint32_t kOnGPSPathChangedHash = 1268721260u;

// Per-route hide flags, one bit per ETargetType index. Touched from both the game
// thread (hook) and script thread (natives), so keep it atomic.
std::atomic<uint32_t> g_hideMask{0};

// Hooked native member fn (__fastcall). We only read the route type and forward
// the opaque pointers unchanged.
using OnGPSPathChanged_t = void (*)(void* aThis, uint32_t aType, void* aPathData);
OnGPSPathChanged_t OnGPSPathChanged_Original = nullptr;

void* g_target = nullptr;

// Per-frame GPS line enforcement. The world map draws its line via a native pull
// on open that bypasses the hook, so skipping OnGPSPathChanged isn't enough. We
// track the container controllers and every frame re-read their GPS line widgets,
// forcing hidden routes invisible - self-healing, no event dependency. Weak
// handles so we never touch a destroyed controller.
std::mutex g_trackedMutex;
std::vector<RED4ext::WeakHandle<RED4ext::ISerializable>> g_containers;

// Force a line widget invisible and fully transparent (so it can't flash if some
// other pass toggles visibility mid-frame).
void HideWidget(RED4ext::ink::Widget* aWidget)
{
    if (aWidget)
    {
        aWidget->visible = false;
        aWidget->opacity = 0.0f;
    }
}

void ShowWidget(RED4ext::ink::Widget* aWidget)
{
    if (aWidget)
    {
        aWidget->visible = true;
        aWidget->opacity = 1.0f;
    }
}

// Hide the container's GPS line widgets whose route bit is set in aMask, reading
// each reference fresh so it works the moment they bind.
void HideContainerWidgets(RED4ext::game::ui::MappinsContainerController* aController, uint32_t aMask)
{
    RED4ext::ink::WidgetReference* refs[4] = {
        &aController->gpsQuestPathWidget,
        &aController->gpsPlayerTrackedPathWidget,
        &aController->gpsDelamainPathWidget,
        &aController->autodrivePathWidget,
    };

    for (uint32_t route = 0; route < 4; ++route)
    {
        if ((aMask & (1u << route)) != 0)
            HideWidget(refs[route]->widget.instance);
    }
}

// Restore visibility for routes set in aRestoreMask, so widgets aren't stuck
// invisible waiting for the next OnGPSPathChanged (the minimap has no independent
// native redraw).
void ShowContainerWidgets(RED4ext::game::ui::MappinsContainerController* aController,
                          uint32_t aRestoreMask)
{
    RED4ext::ink::WidgetReference* refs[4] = {
        &aController->gpsQuestPathWidget,
        &aController->gpsPlayerTrackedPathWidget,
        &aController->gpsDelamainPathWidget,
        &aController->autodrivePathWidget,
    };

    for (uint32_t route = 0; route < 4; ++route)
    {
        if ((aRestoreMask & (1u << route)) != 0)
            ShowWidget(refs[route]->widget.instance);
    }
}

// Track a container controller (by weak handle) so the per-frame updater keeps its
// GPS line widgets hidden. Deduped by instance pointer.
void RegisterContainer(RED4ext::IScriptable* aController)
{
    if (!aController)
        return;

    const RED4ext::WeakHandle<RED4ext::ISerializable>& ref = aController->ref;
    if (!ref.instance)
        return;

    std::lock_guard<std::mutex> lock(g_trackedMutex);
    for (const auto& c : g_containers)
    {
        if (c.instance == ref.instance)
            return; // already tracked
    }
    g_containers.push_back(ref);
}

// Runs every frame in the Running state: re-reads each tracked container's GPS
// line widgets, force-hides or restores routes, and prunes dead controllers.
bool OnRunningUpdate(RED4ext::CGameApplication*)
{
    const uint32_t mask = g_hideMask.load();

    // Routes that went hidden -> shown this frame need an explicit restore (the
    // minimap only redraws via OnGPSPathChanged).
    static uint32_t s_prevMask = 0;
    const uint32_t restored = s_prevMask & ~mask; // bits that were set and are now clear
    s_prevMask = mask;

    if (mask == 0 && restored == 0)
        return false;

    std::lock_guard<std::mutex> lock(g_trackedMutex);
    for (size_t i = 0; i < g_containers.size();)
    {
        auto handle = g_containers[i].Lock();
        auto* serializable = handle.GetPtr();
        if (!serializable)
        {
            g_containers[i] = std::move(g_containers.back());
            g_containers.pop_back();
            continue;
        }

        auto* controller = static_cast<RED4ext::game::ui::MappinsContainerController*>(serializable);

        if (mask != 0)
            HideContainerWidgets(controller, mask);
        if (restored != 0)
            ShowContainerWidgets(controller, restored);

        ++i;
    }

    return false; // keep updating every frame
}

void OnGPSPathChanged_Hook(void* aThis, uint32_t aType, void* aPathData)
{
    auto* controller = static_cast<RED4ext::game::ui::MappinsContainerController*>(aThis);

    // Track this container so the per-frame updater keeps its GPS line hidden.
    if (controller)
        RegisterContainer(controller);

    // Only the low byte of the route enum is meaningful.
    const uint32_t type = aType & 0xFFu;
    const uint32_t mask = g_hideMask.load();
    const bool     suppress = (type < 32) && (mask & (1u << type)) != 0;

    // Skip the original so the line widget gets no geometry.
    if (suppress)
        return;

    if (OnGPSPathChanged_Original)
        OnGPSPathChanged_Original(aThis, aType, aPathData);
}

// Native function implementations

// MapUtils_SetNavPathHidden(hidden: Bool) - master toggle for all routes.
void SetNavPathHidden(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame,
                      void* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(aOut);
    RED4EXT_UNUSED_PARAMETER(a4);

    bool hidden = false;
    RED4ext::GetParameter(aFrame, &hidden);
    aFrame->code++; // skip ParamEnd

    const uint32_t mask = hidden ? 0xFFFFFFFFu : 0u;
    g_hideMask.store(mask);

    if (MapUtils::s_sdk)
        MapUtils::s_sdk->logger->InfoF(MapUtils::s_handle,
            "NavPath: SetNavPathHidden(%d) -> mask=0x%08X", hidden ? 1 : 0, mask);
}

// MapUtils_SetNavPathTypeHidden(type: Int32, hidden: Bool) - per-route toggle.
void SetNavPathTypeHidden(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame,
                          void* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(aOut);
    RED4EXT_UNUSED_PARAMETER(a4);

    int32_t type   = 0;
    bool    hidden = false;
    RED4ext::GetParameter(aFrame, &type);
    RED4ext::GetParameter(aFrame, &hidden);
    aFrame->code++; // skip ParamEnd

    if (type < 0 || type >= 32)
        return;

    const uint32_t bit = 1u << static_cast<uint32_t>(type);
    if (hidden)
        g_hideMask.fetch_or(bit);
    else
        g_hideMask.fetch_and(~bit);
}

// MapUtils_IsNavPathHidden() -> Bool - true if any route is currently hidden.
void IsNavPathHidden(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame,
                     bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    aFrame->code++; // skip ParamEnd

    if (aOut)
        *aOut = g_hideMask.load() != 0;
}

// MapUtils_RegisterNavPathContainer() - world map controller method. Called from
// Redscript OnInitialize so the per-frame updater can hide its line from first
// open (the world map's initial line bypasses our hooks).
void RegisterNavPathContainer(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame,
                              void* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aOut);
    RED4EXT_UNUSED_PARAMETER(a4);

    aFrame->code++; // skip ParamEnd

    RegisterContainer(aContext);
}

} // anonymous namespace

// Hooking

namespace NavPath
{

void AttachHook()
{
    // Register the per-frame enforcer first, so line widgets stay hidden even on
    // redraw paths that bypass the hook (world map).
    if (MapUtils::s_sdk)
    {
        RED4ext::v1::GameState runState;
        runState.OnEnter  = nullptr;
        runState.OnUpdate = &OnRunningUpdate;
        runState.OnExit   = nullptr;
        MapUtils::s_sdk->gameStates->Add(MapUtils::s_handle, RED4ext::EGameStateType::Running, &runState);
    }

    RED4ext::UniversalRelocFunc<OnGPSPathChanged_t> reloc(kOnGPSPathChangedHash);
    g_target = reinterpret_cast<void*>(static_cast<OnGPSPathChanged_t>(reloc));

    if (!g_target)
    {
        if (MapUtils::s_sdk)
            MapUtils::s_sdk->logger->Error(MapUtils::s_handle,
                "NavPath: failed to resolve OnGPSPathChanged address");
        return;
    }

    const bool ok = MapUtils::s_sdk->hooking->Attach(MapUtils::s_handle, g_target,
        reinterpret_cast<void*>(&OnGPSPathChanged_Hook),
        reinterpret_cast<void**>(&OnGPSPathChanged_Original));

    if (ok)
        MapUtils::s_sdk->logger->Info(MapUtils::s_handle,
            "NavPath: OnGPSPathChanged hook attached.");
    else
        MapUtils::s_sdk->logger->Error(MapUtils::s_handle,
            "NavPath: failed to attach OnGPSPathChanged hook.");
}

void DetachHook()
{
    if (g_target && MapUtils::s_sdk)
        MapUtils::s_sdk->hooking->Detach(MapUtils::s_handle, g_target);
    g_target = nullptr;
}

// RTTI registration

void PostRegisterTypes()
{
    using namespace MapUtils::Rtti;

    GlobalFn("MapUtils_SetNavPathHidden", &SetNavPathHidden, {{"Bool", "hidden"}});
    GlobalFn("MapUtils_SetNavPathTypeHidden", &SetNavPathTypeHidden,
             {{"Int32", "type"}, {"Bool", "hidden"}});
    GlobalFn("MapUtils_IsNavPathHidden", &IsNavPathHidden, {}, "Bool");

    // Non-static method: Redscript calls it from OnInitialize to register the
    // world map controller for per-frame hiding.
    if (!ClassFn("gameuiWorldMapMenuGameController", "MapUtils_RegisterNavPathContainer",
                 &RegisterNavPathContainer)
        && MapUtils::s_sdk)
    {
        MapUtils::s_sdk->logger->Error(MapUtils::s_handle,
            "NavPath: gameuiWorldMapMenuGameController not found for container registration");
    }
}

} // namespace NavPath
