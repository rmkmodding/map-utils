#pragma once

#include <RED4ext/RED4ext.hpp>

// NavPath - hides the GPS route line on the world map and minimap.
//
// Both container controllers derive from gameuiMappinsContainerController, whose
// OnGPSPathChanged callback feeds the line widgets, so one hook covers both. When
// a route's hide bit is set the hook skips the original, so the line gets no new
// geometry. A per-frame pass also re-hides widgets on redraw paths that bypass the
// hook (world map). Toggled at runtime from Redscript.

namespace NavPath
{

// Resolve OnGPSPathChanged and attach/detach the hook. Call MapUtils::Init first.
void AttachHook();
void DetachHook();

// Registers the MapUtils_SetNavPath* / MapUtils_IsNavPathHidden natives. Pass to
// CRTTISystem::AddPostRegisterCallback in Main -> Load.
void PostRegisterTypes();

} // namespace NavPath
