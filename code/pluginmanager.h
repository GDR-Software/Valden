#ifndef __PLUGIN_MANAGER__
#define __PLUGIN_MANAGER__

#pragma once

#define MAX_APINAME 128
typedef struct
{
    char major_version[MAX_APINAME];

    char minor_version[MAX_APINAME];
} APIDescriptor_t;

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