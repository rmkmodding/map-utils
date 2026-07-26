#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>

#include <Windows.h>
#include <cstdint>

// Fixed-point world coordinate scale: int32 / 131072.0 = world units.
constexpr float kWorldPosScale = 131072.0f;

// Known offsets inside engine structs (from the RED4ext.SDK headers):
constexpr uintptr_t kIScriptable_NativeType   = 0x30; // CClass* nativeType
constexpr uintptr_t kCClass_Parent            = 0x10; // CClass* parent
constexpr uintptr_t kEntity_Components        = 0xA0; // DynArray<Handle<IComponent>>
constexpr uintptr_t kEntity_ComponentsSize    = 0xA8; // DynArray size (uint32)
constexpr uintptr_t kPlaced_LocalTransformPos = 0xC0; // WorldTransform.Position (FixedPoint x/y/z)
constexpr uintptr_t kHandleStride             = 0x10; // sizeof(Handle<T>)

// Returns true if aPtr points to committed, readable memory.
inline bool IsReadable(const void* aPtr) noexcept
{
    if (!aPtr)
        return false;
    MEMORY_BASIC_INFORMATION mbi{};
    return VirtualQuery(aPtr, &mbi, sizeof(mbi)) >= sizeof(mbi) &&
           mbi.State == MEM_COMMIT &&
           (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ |
                           PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_WRITECOPY));
}

inline void* ReadPtr(const void* aBase, uintptr_t aOffset) noexcept
{
    return *reinterpret_cast<void* const*>(reinterpret_cast<const uint8_t*>(aBase) + aOffset);
}

inline uint32_t ReadU32(const void* aBase, uintptr_t aOffset) noexcept
{
    return *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(aBase) + aOffset);
}

inline int32_t ReadI32(const void* aBase, uintptr_t aOffset) noexcept
{
    return *reinterpret_cast<const int32_t*>(reinterpret_cast<const uint8_t*>(aBase) + aOffset);
}

// Walks the parent chain of aClass looking for aTarget. Max depth 24.
inline bool DerivesFrom(const RED4ext::CClass* aClass, const RED4ext::CClass* aTarget) noexcept
{
    for (int i = 0; i < 24 && aClass; ++i)
    {
        if (aClass == aTarget)
            return true;

        const auto* parent =
            static_cast<const RED4ext::CClass*>(ReadPtr(aClass, kCClass_Parent));
        if (parent == aClass)
            break;

        aClass = parent;
    }
    return false;
}

// Reads a placed component's local world position from its WorldTransform.
inline RED4ext::Vector3 ReadPlacedPosition(const void* aPlacedComponent) noexcept
{
    const int32_t x = ReadI32(aPlacedComponent, kPlaced_LocalTransformPos + 0x0);
    const int32_t y = ReadI32(aPlacedComponent, kPlaced_LocalTransformPos + 0x4);
    const int32_t z = ReadI32(aPlacedComponent, kPlaced_LocalTransformPos + 0x8);
    return {static_cast<float>(x) / kWorldPosScale,
            static_cast<float>(y) / kWorldPosScale,
            static_cast<float>(z) / kWorldPosScale};
}
