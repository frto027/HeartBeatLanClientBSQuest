#pragma once



struct Lang{
#define V(key, v) const char * key;
#include "langs/english.inc"
#undef  V
};

namespace I18N{
    void Setup();
}

extern Lang *LANG;