#pragma once

#include <RED4ext/RED4ext.hpp>

namespace MapUtils
{

// Plugin-level state (defined in WorldMap.cpp)

extern const RED4ext::v1::Sdk*   s_sdk;
extern RED4ext::v1::PluginHandle s_handle;

// Cached RTTI class pointers - populated in PostRegisterTypes, used by
// TryDirectAccess.

extern const RED4ext::CClass* s_mapObjectClass;   // gameuiWorldMapGameObject
extern const RED4ext::CClass* s_targetPointClass; // entTargetPointComponent

// Lifetime

/// Stores the plugin handle and SDK pointer. Call from Main -> Load before any
/// other MapUtils operations.
void Init(const RED4ext::v1::Sdk* aSdk, RED4ext::v1::PluginHandle aHandle);

// RTTI registration callback. Pass to CRTTISystem::AddPostRegisterCallback in
// Main -> Load.

void PostRegisterTypes();

} // namespace MapUtils
