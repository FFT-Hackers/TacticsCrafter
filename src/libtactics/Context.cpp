#include <filesystem>
#include <libtactics/Context.h>
#include <libtactics/API/API.h>

void ltcImplPipelineRun(LTC_Context* ctx)
{
    lua_State* L = ctx->L;
    bool error{};

    /* Reset our state */
    ctx->changeset.clear();
    ctx->symbols.clear();
    ctx->extraMemory = 0;

    /* Create a new lua ENV */
    lua_newtable(L);
    lua_newtable(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    for (LTC_Script script : ctx->pipeline)
    {
        /* Flag the script as current for the lua API */
        Script* s = ctx->scripts.get(script);
        ctx->currentScript = s;

        if (s->func == LUA_NOREF)
        {
            error = true;
        }
        else
        {
            /* Reset the script metadata */
            s->name = "Unknown";
            s->author = "Unknown";
            s->version = "0.0.0";
            s->description = "N/A";

            /* Load the script function */
            lua_rawgeti(L, LUA_REGISTRYINDEX, s->func);

            /* Set the env */
            lua_pushvalue(L, 1);
            lua_setupvalue(L, -2, 1);

            /* Call the func */
            s->error = false;
            s->log = "";
            if (lua_pcall(L, 0, 0, 0))
            {
                s->error = true;
                s->log = lua_tostring(L, -1);
                lua_pop(L, -1);
                error = true;
            }
        }

        if (error)
            break;
    }

    /* Remove the env */
    lua_pop(L, 1);
}

static LTC_Script ltcImplPipelineLoadScript(LTC_Context* ctx, const char* path, bool core)
{
    lua_State* L = ctx->L;
    LTC_Script script;
    Script* s;

    /* Alloc the script */
    script = ctx->scripts.alloc();
    s = ctx->scripts.get(script);

    /* Create the script function */
    if (luaL_loadfile(L, path))
    {
        s->func = LUA_NOREF;
        s->log = lua_tostring(L, -1);
        s->error = true;
    }
    else
    {
        s->func = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    s->core = core;
    ctx->pipeline.push_back(script);

    return script;
}

LTC_API void ltcRemoveScript(LTC_Context* ctx, LTC_Script script)
{
    lua_State* L = ctx->L;
    Script* s = ctx->scripts.get(script);

    if (!s || s->core)
        return;

    /* Remove options */
    for (auto o : s->options)
        ctx->options.dealloc(o);

    /* Remove the lua handle */
    if (s->func != LUA_NOREF)
        luaL_unref(L, LUA_REGISTRYINDEX, s->func);

    /* Remove the script from the pipeline */
    auto it = std::find(ctx->pipeline.begin(), ctx->pipeline.end(), script);
    if (it != ctx->pipeline.end())
        ctx->pipeline.erase(it);

    /* Remove the script itself */
    ctx->scripts.dealloc(script);
}

LTC_API void ltcMoveScript(LTC_Context* ctx, LTC_Script script, int delta)
{
    lua_State* L = ctx->L;
    Script* s = ctx->scripts.get(script);

    if (!s || s->core)
        return;

    int index{};
    for (index = 0; index < ctx->pipeline.size(); ++index)
    {
        if (ctx->pipeline[index] == script)
            break;
    }
    int index2 = index + delta;
    if (index2 < 0 || index2 >= ctx->pipeline.size())
        return;
    Script* s2 = ctx->scripts.get(ctx->pipeline[index2]);
    if (!s2 || s2->core)
        return;

    ctx->pipeline[index] = ctx->pipeline[index2];
    ctx->pipeline[index2] = script;
}

LTC_API LTC_Context* ltcCreateContext(const char* dataPath)
{
    LTC_Context* ctx;
    lua_State* L;
    luaL_Buffer b;

    ctx = new LTC_Context{};

    /* Create the lua state */
    L = luaL_newstate();
    luaL_openlibs(L);

    /* Set the search path */
    lua_getglobal(L, "package");
    luaL_buffinit(L, &b);
    luaL_addstring(&b, dataPath);
    luaL_addstring(&b, "/?.lua");
    luaL_pushresult(&b);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);

    /* Set the lua state */
    ctx->L = L;

    /* Load our API */
    API::init(ctx);

    /* Load core scripts */
    std::vector<std::string> coreScripts;
    std::string corePath = std::string(dataPath) + "/core/";
    for (const auto& e : std::filesystem::directory_iterator(corePath))
    {
        if (e.is_regular_file() && e.path().extension() == ".lua")
        {
            coreScripts.push_back(e.path().string());
        }
    }
    std::sort(coreScripts.begin(), coreScripts.end());
    for (auto& e : coreScripts)
    {
        ltcImplPipelineLoadScript(ctx, e.c_str(), true);
    }

    /* Run the pipeline once to process the core scripts */
    ltcImplPipelineRun(ctx);

    return ctx;
}

LTC_API void ltcDestroyContext(LTC_Context* ctx)
{
    lua_close(ctx->L);
    delete ctx;
}

LTC_API LTC_Script ltcLoadScript(LTC_Context* ctx, const char* path)
{
    return ltcImplPipelineLoadScript(ctx, path, false);
}

LTC_API int ltcGetScriptCount(LTC_Context* ctx)
{
    return (int)ctx->pipeline.size();
}

LTC_API LTC_Script ltcGetScriptHandle(LTC_Context* ctx, int index)
{
    if (index < 0 || index >= ctx->pipeline.size())
        return 0;
    return ctx->pipeline[index];
}

LTC_API void ltcRunPipeline(LTC_Context* ctx)
{
    ltcImplPipelineRun(ctx);
}