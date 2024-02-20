#ifndef __PLUGIN_MANAGER__
#define __PLUGIN_MANAGER__

#pragma once

class CPluginManager
{
public:
    CPluginManager( void );

    void Init( void );      // go through the path where we will find modules and plugins
    void Cleanup( void );   // cleanup of data structures allocated for plugins, not a plugin reload
private:
    std::vector<void *> m_Plugins;
};

class CPluginSlot
{
public:
    CPluginSlot( void );
private:
    ValdenPluginTable m_PluginTable;
};

#endif