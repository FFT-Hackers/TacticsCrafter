#ifndef TC_STATE_STATE_H
#define TC_STATE_STATE_H

#include <TacticsCrafter/State/StatePatch.h>
#include <TacticsCrafter/State/StateScript.h>
#include <TacticsCrafter/State/StateSymbols.h>

struct State
{
    State();
    void apply(lua_State* L);

    StatePatch      patch;
    StateScript     script;
    StateSymbols    symbols;
};

#endif /* TC_STATE_STATE_H */