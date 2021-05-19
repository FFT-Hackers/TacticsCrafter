#include <QCoreApplication>
#include <QDir>
#include <TacticsCrafter/Core/ScriptManager.h>
#include <TacticsCrafter/Core/State.h>
#include <TacticsCrafter/API/API.h>

ScriptManager::ScriptManager()
{
    /* Create the lua state */
    _lua = luaL_newstate();
    luaL_openlibs(_lua);

    /* Set the search path */
    QString luaPath = QCoreApplication::applicationDirPath() + "/data/?.lua";
    lua_getglobal(_lua, "package");
    lua_pushstring(_lua, luaPath.toStdString().c_str());
    lua_setfield(_lua, -2, "path");
    lua_pop(_lua, 1);

    /* Load core scripts */
    QString dirPath = QCoreApplication::applicationDirPath() + "/data/core/";
    QDir coreDir(dirPath);
    auto coreScripts = coreDir.entryList(QStringList("*.lua"), QDir::Filter::Files, QDir::SortFlag::Name);
    for (const auto& s : coreScripts)
    {
        load(dirPath + s, true);
    }

    /* Run the pipeline once to process the core scripts */
    run();
}

ScriptManager::~ScriptManager()
{
    lua_close(_lua);
}

void ScriptManager::load(const QString& path, bool core)
{
    _scripts.push_back(std::make_unique<Script>(_lua, path, core));
}

const Changeset& ScriptManager::run()
{
    State state;

    _changes.clear();
    state.changeset = &_changes;
    API::init(_lua, &state);

    for (auto& ss : _scripts)
    {
        /* Reset the script metadata (name, author, etc.) */
        state.scriptMeta.name = "Unknown";
        state.scriptMeta.author = "Unknown";
        state.scriptMeta.version = "0.0.0";
        state.scriptMeta.description = "N/A";

        /* Execute the script */
        auto& s = *ss.get();
        state.script = &s;
        s.exec();

        /* Store the new metadata */
        s.setMeta(state.scriptMeta);
    }
    emit update();

    return _changes;
}
