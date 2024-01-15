#ifndef __COMMAND__
#define __COMMAND__

#pragma once

typedef void (*cmdfunc_t)(void);

class CCommand
{
public:
    CCommand(const char *name, cmdfunc_t func)
        : mName{ name }, mFunc{ func } { }
    ~CCommand() { }

    const char *mName;
    cmdfunc_t mFunc;
    CCommand *mNext;
};

// command.cpp
void Cmd_Init(void);
void Cmd_Shutdown(void);
void Cmd_ExecuteText(const char *text);
void Cmd_AddCommand(const char *name, cmdfunc_t func);
void TokenizeString(const char *str, bool ignoreQuotes);
uint32_t Argc(void);
const char *Argv(uint32_t index);

#endif