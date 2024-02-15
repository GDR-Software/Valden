#include "editor.h"

static CCommand *cmdlist;

static char *G_CopyString(const char *str)
{
    uint64_t len;
    char *out;

    len = strlen(str);
    out = (char *)GetMemory(len + 1);
    memset(out, 0, len + 1);
    memcpy(out, str, len);

    return out;
}

static void Cmd_List_f(void)
{
    uint64_t numCommands;

    Log_Printf("Command list:\n");
    numCommands = 0;
    for (CCommand *cmd = cmdlist; cmd; cmd = cmd->mNext) {
        if (!cmd->mName)
            continue;

        Log_Printf("%s\n", cmd->mName);
        numCommands++;
    }
    Log_Printf("Number of commands: %lu\n", numCommands);
}


/*
Cmd_Init: initializes all the common commands that can be found in the editor
*/
void Cmd_Init(void)
{
    cmdlist = (CCommand *)GetMemory(sizeof(*cmdlist));

    Cmd_AddCommand("list", Cmd_List_f);
}

static CCommand *Cmd_FindCommand(const char *name)
{
    CCommand *cmd;

    for (cmd = cmdlist; cmd; cmd = cmd->mNext) {
        if (!N_stricmp(name, cmd->mName)) {
            return cmd;
        }
    }
    return NULL;
}

void Cmd_Shutdown(void)
{
}

#define BIG_INFO_STRING 8192
#define MAX_STRING_TOKENS 1024
#define MAX_HISTORY 32
#define MAX_CMD_BUFFER  65536

static uint32_t cmd_argc;
static char CCommandokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];
static char *cmd_argv[MAX_STRING_TOKENS];
static char cmd_cmd[BIG_INFO_STRING];

static char cmd_history[MAX_HISTORY][BIG_INFO_STRING];
static uint32_t cmd_historyused;

const char *Argv(uint32_t index)
{
    if (index >= cmd_argc)
        return "";
    
    return cmd_argv[index];
}

uint32_t Argc(void)
{
    return cmd_argc;
}

void TokenizeString(const char *str, bool ignoreQuotes)
{
	const char *p;
	char *tok;

    memset(cmd_cmd, 0, sizeof(cmd_cmd));
    memset(CCommandokenized, 0, sizeof(CCommandokenized));
    cmd_argc = 0;

    strncpy(cmd_cmd, str, sizeof(cmd_cmd));
	p = str;
	tok = CCommandokenized;

	while (1) {
		if (cmd_argc >= arraylen(cmd_argv)) {
			return; // usually something malicious
		}
		while (*p && *p <= ' ') {
			p++; // skip whitespace
		}
		if (!*p) {
			break; // end of string
		}
		// handle quoted strings
		if (!ignoreQuotes && *p == '\"') {
			cmd_argv[cmd_argc] = tok;
            cmd_argc++;
			p++;
			while (*p && *p != '\"') {
				*tok++ = *p++;
			}
			if (!*p) {
				return; // end of string
			}
			p++;
			continue;
		}

		// regular stuff
		cmd_argv[cmd_argc] = tok;
        cmd_argc++;

		// skip until whitespace, quote, or command
		while (*p > ' ') {
			if (!ignoreQuotes && p[0] == '\"') {
				break;
			}

			if (p[0] == '/' && p[1] == '/') {
				// accept protocol headers (e.g. http://) in command lines that match "*?[a-z]://" pattern
				if (p < cmd_cmd + 3 || p[-1] != ':' || p[-2] < 'a' || p[-2] > 'z') {
					break;
				}
			}

			// skip /* */ comments
			if (p[0] == '/' && p[1] == '*') {
				break;
			}

			*tok++ = *p++;
		}
		*tok++ = '\0';
		if (!*p) {
			return; // end of string
		}
	}
}

void Cmd_ExecuteText(const char *text)
{
    CCommand *cmd;
    const char *cmdname;

    if ( *text == '/' || *text == '\\' ) {
        text++;
    }

    TokenizeString(text, false);
    cmdname = Argv(0);

    cmd = Cmd_FindCommand(cmdname);
    if (!cmd) {
        Log_Printf("No such command '%s'\n", cmdname);
        return;
    }
    
    cmd->mFunc();
}

void Cmd_AddCommand(const char *name, cmdfunc_t func)
{
    CCommand *cmd;

    cmd = Cmd_FindCommand(name);
    if (cmd) {
        Log_Printf("Command '%s' already registered\n", cmd->mName);
        return;
    }

    cmd = new CCommand( name, func );
    cmd->mName = name;
    cmd->mFunc = func;
    cmd->mNext = cmdlist;
    cmdlist = cmd;
}
