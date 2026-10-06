#pragma once

#include "BaseTypes.h"

class XMLNode {
public:
    struct CXMLAttribute {
        RCString name;
        RCString value;
    };

    CHandle* script_handle_;
    XMLNode* parent_;
    XMLNode* first_child_;
    RCString tag_name_;
    char* body_text_;
    uint32_t body_length_;
    TSGrowableArray<CXMLAttribute> attributes_;
    uint32_t parent_body_length_;
    XMLNode* next_sibling_;
    XMLNode* next_free_;

    // key, value
    HOOKKIT_HOOK(ctor, 0x00814AD0, hookkit::Conv::eThiscall, XMLNode*, XMLNode*, int, const char*);
    HOOKKIT_HOOK(setValue, 0x00814C40, hookkit::Conv::eThiscall, void, XMLNode*, const char*, const char*);
};
