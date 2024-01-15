#ifndef __UNDO__
#define __UNDO__

#pragma once

// start operation
void Undo_Start( const char *operation );
// end operation
void Undo_End( void );
// add tile to undo
void Undo_AddTile( maptile_t *tile );
// undo last operation (bSilent == true -> will not print the "undone blah blah message")
void Undo_Undo( qboolean bSilent = false );
// redo last undone operation
void Undo_Redo( void );


#endif