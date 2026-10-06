#pragma once

#include "Common/Common.h"
#include "FrameScript/FrameScript.h"

class CSimpleFrame;

class CScriptObject : public FrameScriptObject {
public:
    ~CScriptObject() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override = 0;                                       // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    virtual CSimpleFrame* getScriptObjectParent() = 0;   // 5
    virtual bool isMatchingObjectType(const char* str);  // 6
    virtual const char* getObjectTypeStr();              // 7

    RCString name_;

    // name
    HOOKKIT_HOOK(registerObj, 0x00819880, hookkit::Conv::eThiscall, void, CScriptObject*, const char*);
    HOOKKIT_HOOK(getRefTable, 0x00488380, hookkit::Conv::eThiscall, int, CScriptObject*);
};
