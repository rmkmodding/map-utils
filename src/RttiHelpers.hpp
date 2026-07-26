#pragma once

#include <RED4ext/RED4ext.hpp>

#include <initializer_list>

// RttiHelpers - thin wrappers over the RED4ext RTTI registration API.
//
// Collapse the repetitive Create -> set flags -> AddParam -> SetReturnType ->
// RegisterFunction sequence into a single call. The scripting-function template
// parameter T (the aOut type of the native handler) is deduced from the passed
// function pointer, so callers just pass the handler by address.

namespace MapUtils::Rtti
{

// A single Redscript parameter: RTTI type name + parameter name.
struct Param
{
    const char* type;
    const char* name;
};

// Register a global native function (isNative + isStatic). Optionally give it
// parameters and a return type.
template<typename T>
void GlobalFn(const char* aName, RED4ext::ScriptingFunction_t<T> aFunc,
              std::initializer_list<Param> aParams = {}, const char* aReturnType = nullptr)
{
    auto* func  = RED4ext::CGlobalFunction::Create(aName, aName, aFunc);
    func->flags = {.isNative = true, .isStatic = true};
    for (const auto& p : aParams)
        func->AddParam(p.type, p.name);
    if (aReturnType)
        func->SetReturnType(aReturnType);
    RED4ext::CRTTISystem::Get()->RegisterFunction(func);
}

// Register a native method on an existing class. Returns false (registering
// nothing) if the class is not present in the RTTI system, so callers can log.
template<typename T>
bool ClassFn(const char* aClassName, const char* aName, RED4ext::ScriptingFunction_t<T> aFunc,
             std::initializer_list<Param> aParams = {}, const char* aReturnType = nullptr)
{
    auto* cls = RED4ext::CRTTISystem::Get()->GetClass(aClassName);
    if (!cls)
        return false;

    auto* func  = RED4ext::CClassFunction::Create(cls, aName, aName, aFunc);
    func->flags = {.isNative = true};
    for (const auto& p : aParams)
        func->AddParam(p.type, p.name);
    if (aReturnType)
        func->SetReturnType(aReturnType);
    cls->RegisterFunction(func);
    return true;
}

} // namespace MapUtils::Rtti
