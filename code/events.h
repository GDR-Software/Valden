#ifndef __EVENTS__
#define __EVENTS__

#pragma once

#include "keycodes.h"
#include <array>

#define MAX_EVENT_QUEUE 256
#define MASK_QUEUED_EVENTS (MAX_EVENT_QUEUE - 1)
#define MAX_PUSHED_EVENTS 256

typedef enum {
  // bk001129 - make sure SE_NONE is zero
	SE_NONE = 0,		// evTime is still valid
	SE_KEY,				// evValue is a key code, evValue2 is whether its pressed or not
	SE_MOUSE,	    	// evValue and evValue2 are relative signed x / y moves
	SE_JOYSTICK_AXIS,	// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE,			// evPtr is a char*
	SE_WINDOW,			// really only used by the rendering engine for window resisizing
	SE_MAX,
} sysEventType_t;

typedef struct
{
	sysEventType_t evType;
	int evValue, evValue2;
	uint32_t evPtrLength;	// bytes of data pointed to by evPtr, for journaling
	void *evPtr;			// this must be manually freed if not NULL
} sysEvent_t;

typedef struct
{
	bool down;
	bool bound;
	uint32_t repeats;
	char *binding;
} nkey_t;

extern nkey_t keys[NUMKEYS];

class CEventQueue
{
public:
    CEventQueue(void);
    ~CEventQueue();

    uint64_t EventLoop(void);
private:
    sysEvent_t GetEvent(void);
    sysEvent_t GetRealEvent(void);
    sysEvent_t GetSystemEvent(void);
    void PumpKeyEvents(void);
    void PushEvent(const sysEvent_t *event);
    void QueueEvent(sysEventType_t evType, int evValue, int evValue2, uint32_t ptrLength, void *ptr);
    void SendKeyEvents(void);

    std::array<sysEvent_t, MAX_EVENT_QUEUE> mEventQueue;
    std::array<sysEvent_t, MAX_PUSHED_EVENTS> mPushedEvents;
    sysEvent_t *mLastEvent;
    uint32_t mEventHead;
    uint32_t mEventTail;
    uint32_t mPushedEventsHead;
    uint32_t mPushedEventsTail;
};

extern CEventQueue events;

bool Key_IsDown( uint32_t keynum );
uint32_t Key_StringToKeynum( const char *str );
const char *Key_KeynumToString (uint32_t keynum );
const char *Key_GetBinding( uint32_t keynum );
void Key_ClearStates(void);

#endif