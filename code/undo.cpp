#include "gln.h"
#include "map.h"
#include "undo.h"

typedef struct undo_s
{
    double time;
    int id;
    int done;
    const char *operation;

    void *data;

    struct undo_s *next;
    struct undo_s *prev;
} undo_t;

undo_t *g_pUndoList;                        // first undo in the list
undo_t *g_pLastUndo;                        // last undo in the list
undo_t *g_pRedoList;                        // first redo in the list
undo_t *g_pLastRedo;                        // last redo in list
int g_nUndoMaxSize = 64;                    // maximum number of undos
int g_nUndoSize = 0;                        // number of undos in the list
int g_nUndoMaxMemorySize = 2 * 1024 * 1024; // maximum undo memory (default 2 MiB)
int g_nUndoMemorySize = 0;                  // memory size of undo buffer
int g_nUndoId = 1;                          // current undo ID (zero is invalid id)
int g_nRedoId = 1;                          // current redo ID (zero is invalid id)

int Undo_MemorySize( void ) {
    return g_nUndoMemorySize;
}

void Undo_ClearRedo( void )
{
    undo_t *redo, *nextredo;

    for ( redo = g_pRedoList; redo; redo = nextredo ) {
        nextredo = redo->next;
        
        free( redo->data );
        free( redo );
    }

    g_pRedoList = NULL;
    g_pLastRedo = NULL;
    g_nRedoId = 1;
}

/*
* Undo_Clear: clears the undo buffer
*/
void Undo_Clear( void )
{
    undo_t *undo, *nextundo;

    Undo_ClearRedo();
    for ( undo = g_pUndoList; undo; undo = nextundo ) {
        nextundo = undo->next;
        g_nUndoMemorySize -= sizeof(undo_t);
        free( undo );
    }

    g_pUndoList = NULL;
    g_pLastUndo = NULL;
    g_nUndoSize = 0;
    g_nUndoMemorySize = 0;
    g_nUndoId = 1;
}

void Undo_SetMaxSize( int size )
{
	Undo_Clear();
	if ( size < 1 ) {
		g_nUndoMaxSize = 1;
	}
	else {
        g_nUndoMaxSize = size;
    }
}

int Undo_GetMaxSize( void ) {
	return g_nUndoMaxSize;
}

void Undo_SetMaxMemorySize( int size )
{
	Undo_Clear();
	if ( size < 1024 ) {
		g_nUndoMaxMemorySize = 1024;
	}
	else {
        g_nUndoMaxMemorySize = size;
    }
}

int Undo_GetMaxMemorySize( void ) {
    return g_nUndoMaxMemorySize;
}

void Undo_FreeFirstUndo( void )
{
    undo_t *undo;

    // remove the oldest undo from the undo buffer
    undo = g_pUndoList;
    g_pUndoList = g_pUndoList->next;
    g_pUndoList->prev = NULL;

    g_nUndoMemorySize -= sizeof(undo_t);
    free( undo );
    g_nUndoSize--;
}

void Undo_GeneralStart( const char *operation )
{
	undo_t *undo;

	if ( g_pLastUndo ) {
		if ( !g_pLastUndo->done ) {
			Log_FPrintf( SYS_WRN, "WARNING last undo not finished.\n" );
		}
	}

	undo = (undo_t *) malloc( sizeof( undo_t ) );
	if ( !undo ) {
		return;
	}
	memset( undo, 0, sizeof( undo_t ) );
	if ( g_pLastUndo ) {
		g_pLastUndo->next = undo;
	}
	else {
		g_pUndoList = undo;
	}
	undo->prev = g_pLastUndo;
	undo->next = NULL;
	g_pLastUndo = undo;

	undo->time = Sys_DoubleTime();
	//
	if ( g_nUndoId > g_nUndoMaxSize * 2 ) {
		g_nUndoId = 1;
	}
	if ( g_nUndoId <= 0 ) {
		g_nUndoId = 1;
	}
	undo->id = g_nUndoId++;
	undo->done = false;
	undo->operation = operation;

	g_nUndoMemorySize += sizeof( undo_t );
	g_nUndoSize++;
	
    // undo buffer is bound to a max
	if ( g_nUndoSize > g_nUndoMaxSize ) {
		Undo_FreeFirstUndo();
	}
}

void Undo_AddTile( maptile_t *tile )
{

}

void Undo_AddSpawn( mapspawn_t *spawn )
{

}
