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

// Registers the world map controller so its GPS widgets are re-hidden each frame.
// Its initial line is drawn by a native pull that bypasses the hooks, so without
// this it would show on first open until a quest is (re)tracked.
@addMethod(WorldMapMenuGameController)
public native func MapUtils_RegisterNavPathContainer() -> Void
