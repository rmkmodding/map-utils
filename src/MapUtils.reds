// World-map pan centre from native memory. Call while the map is open; returns
// (0,0,0) on failure.
@addMethod(inkWorldMapPreviewGameController)
public native func MapUtils_GetWorldMapCameraPos() -> Vector3

// GPS route line visibility on both the world map and minimap. Suppresses
// OnGPSPathChanged and force-hides the line widgets each frame. Quest tracking is
// unaffected.

// Hide or show all GPS route lines.
public native func MapUtils_SetNavPathHidden(hidden: Bool) -> Void

// Per-route toggle; type is the ETargetType index
// (0=Quest, 1=PlayerTracked, 2=Delamain, 3=Autodrive).
public native func MapUtils_SetNavPathTypeHidden(type: Int32, hidden: Bool) -> Void

// True if any GPS route line is currently hidden.
public native func MapUtils_IsNavPathHidden() -> Bool

// Registers a mappins container (world map or minimap) so its GPS widgets are
// re-hidden each frame. The initial line is drawn by a native pull that bypasses the
// hooks, so without this it shows until a quest path is (re)tracked. Declared on the
// shared base so both the world map and minimap inherit it.
@addMethod(MappinsContainerController)
public native func MapUtils_RegisterNavPathContainer() -> Void
