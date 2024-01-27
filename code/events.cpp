#include "gln.h"
#include "backends/imgui_impl_sdl2.h"
#include "events.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include "editor.h"

CEventQueue events;

typedef struct
{
	const char *name;
	uint32_t keynum;
} keyname_t;

static const keyname_t keynames[] = {
	{"MOUSE_LEFT", KEY_MOUSE_LEFT},
	{"MOUSE_MIDDLE", KEY_MOUSE_MIDDLE},
	{"MOUSE_RIGHT", KEY_MOUSE_RIGHT},
	{"a", KEY_A},
	{"b", KEY_B},
	{"c", KEY_C},
	{"d", KEY_D},
	{"e", KEY_E},
	{"f", KEY_F},
	{"g", KEY_G},
	{"h", KEY_H},
	{"i", KEY_I},
	{"j", KEY_J},
	{"k", KEY_K},
	{"l", KEY_L},
	{"m", KEY_M},
	{"n", KEY_N},
	{"o", KEY_O},
	{"p", KEY_P},
	{"q", KEY_Q},
	{"r", KEY_R},
	{"s", KEY_S},
	{"t", KEY_T},
	{"u", KEY_U},
	{"v", KEY_V},
	{"w", KEY_W},
	{"x", KEY_X},
	{"y", KEY_Y},
	{"z", KEY_Z},

	{"SHIFT", KEY_SHIFT},
	{"CTRL", KEY_CTRL},
	{"TAB", KEY_TAB},
	{"ENTER", KEY_ENTER},
	{"BACKSPACE", KEY_BACKSPACE},

	{"CONSOLE", KEY_CONSOLE},
	{"PRINTSCREEN", KEY_SCREENSHOT},

	{"UPARROW", KEY_UP},
	{"DOWNARROW", KEY_DOWN},
	{"RIGHTARROW", KEY_RIGHT},
	{"LEFTARROW", KEY_LEFT},

	{"F1", KEY_F1},
	{"F2", KEY_F2},
	{"F3", KEY_F3},
	{"F4", KEY_F4},
	{"F5", KEY_F5},
	{"F6", KEY_F6},
	{"F7", KEY_F7},
	{"F8", KEY_F8},
	{"F9", KEY_F9},
	{"F10", KEY_F10},
	{"F11", KEY_F11},
	{"F12", KEY_F12},

    {"WHEELUP", KEY_WHEEL_UP},
    {"WHEELDOWN", KEY_WHEEL_DOWN},

	{NULL, 0}
};

nkey_t keys[NUMKEYS];

CEventQueue::CEventQueue(void)
	: mLastEvent{ mEventQueue.data() + MAX_EVENT_QUEUE - 1 },
    mEventHead{ 0 }, mEventTail{ 0 },
	mPushedEventsHead{ 0 }, mPushedEventsTail{ 0 }
{
	memset(mEventQueue.data(), 0, sizeof(sysEvent_t) * mEventQueue.size());
	memset(mPushedEvents.data(), 0, sizeof(sysEvent_t) * mPushedEvents.size());
}

CEventQueue::~CEventQueue()
{
}

bool Key_IsDown(uint32_t keynum)
{
    if (keynum >= NUMKEYS)
        return false;
    
    return keys[keynum].down;
}

static const char *EventName(sysEventType_t evType)
{
	static const char *evNames[SE_MAX] = {
		"SE_NONE",
		"SE_KEY",
		"SE_MOUSE",
		"SE_JOYSTICK_AXIS",
		"SE_CONSOLE",
		"SE_WINDOW"
	};

	if ((unsigned)evType >= arraylen(evNames))
		return "SE_UNKOWN";
	else
		return evNames[evType];
}

void CEventQueue::PushEvent(const sysEvent_t *event)
{
	sysEvent_t *ev;
	static bool printedWarning = false;

	ev = &mPushedEvents[mPushedEventsTail & (MAX_EVENT_QUEUE - 1)];

	if (mPushedEventsHead - mPushedEventsTail >= MAX_EVENT_QUEUE) {
		// don't print the warning constantly, or it can give time for more...
		if (!printedWarning) {
			printedWarning = true;
			Log_Printf("WARNING: Com_PushEvent: overflow");
		}

		if (ev->evPtr) {
			FreeMemory(ev->evPtr);
		}
		mPushedEventsTail++;
	}
	else {
		printedWarning = false;
	}

	*ev = *event;
	mPushedEventsHead++;
}

#define MAX_CONSOLE_KEYS 16

static keynum_t Com_TranslateSDL2ToQ3Key( SDL_Keysym *keysym, qboolean down )
{
	keynum_t key = (keynum_t)0;

	if ( keysym->scancode >= SDL_SCANCODE_1 && keysym->scancode <= SDL_SCANCODE_0 ) {
		// Always map the number keys as such even if they actually map
		// to other characters (eg, "1" is "&" on an AZERTY keyboard).
		// This is required for SDL before 2.0.6, except on Windows
		// which already had this behavior.
		if ( keysym->scancode == SDL_SCANCODE_0 ) {
			key = '0';
		} else {
			key = '1' + keysym->scancode - SDL_SCANCODE_1 ;
		}
	} else {
		if ( keysym->scancode >= SDL_SCANCODE_A && keysym->scancode <= SDL_SCANCODE_Z ) {
			key = 'a' + keysym->scancode - SDL_SCANCODE_A;
		} else {
			switch ( keysym->scancode ) {
			case SDL_SCANCODE_MINUS:        key = '-';  break;
			case SDL_SCANCODE_EQUALS:       key = '=';  break;
			case SDL_SCANCODE_LEFTBRACKET:  key = '[';  break;
			case SDL_SCANCODE_RIGHTBRACKET: key = ']';  break;
			case SDL_SCANCODE_NONUSBACKSLASH:
			case SDL_SCANCODE_BACKSLASH:    key = '\\'; break;
			case SDL_SCANCODE_SEMICOLON:    key = ';';  break;
			case SDL_SCANCODE_APOSTROPHE:   key = '\''; break;
			case SDL_SCANCODE_COMMA:        key = ',';  break;
			case SDL_SCANCODE_PERIOD:       key = '.';  break;
			case SDL_SCANCODE_SLASH:        key = '/';  break;
			default:
				/* key = 0 */
				break;
			};
		}
	}

	if ( !key && keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE ) {
		// These happen to match the ASCII chars
		key = (uint32_t)keysym->sym;
	}
	else if ( !key ) {
		switch ( keysym->sym ) {
			case SDLK_PAGEUP:       key = KEY_PAGEUP;      		break;
			case SDLK_KP_9:         key = KEY_KP_PAGEUP;    	break;
			case SDLK_PAGEDOWN:     key = KEY_PAGEDOWN;      	break;
			case SDLK_KP_3:         key = KEY_KP_PAGEDOWN;      break;
			case SDLK_KP_7:         key = KEY_KP_HOME;       	break;
			case SDLK_HOME:         key = KEY_HOME;          	break;
			case SDLK_KP_1:         key = KEY_KP_END;        	break;
			case SDLK_END:          key = KEY_END;           	break;
			case SDLK_KP_4:         key = KEY_KP_LEFT;  		break;
			case SDLK_LEFT:         key = KEY_LEFT;     		break;
			case SDLK_KP_6:         key = KEY_KP_RIGHT; 		break;
			case SDLK_RIGHT:        key = KEY_RIGHT;    		break;
			case SDLK_KP_2:         key = KEY_KP_DOWN;  		break;
			case SDLK_DOWN:         key = KEY_DOWN;     		break;
			case SDLK_KP_8:         key = KEY_KP_UP;    		break;
			case SDLK_UP:           key = KEY_UP;       		break;
			case SDLK_ESCAPE:       key = KEY_ESCAPE;        	break;
			case SDLK_KP_ENTER:     key = KEY_KP_ENTER;      	break;
			case SDLK_RETURN:       key = KEY_ENTER;         	break;
			case SDLK_TAB:          key = KEY_TAB;           	break;
			case SDLK_F1:           key = KEY_F1;            	break;
			case SDLK_F2:           key = KEY_F2;            	break;
			case SDLK_F3:           key = KEY_F3;            	break;
			case SDLK_F4:           key = KEY_F4;            	break;
			case SDLK_F5:           key = KEY_F5;            	break;
			case SDLK_F6:           key = KEY_F6;            	break;
			case SDLK_F7:           key = KEY_F7;            	break;
			case SDLK_F8:           key = KEY_F8;            	break;
			case SDLK_F9:           key = KEY_F9;            	break;
			case SDLK_F10:          key = KEY_F10;           	break;
			case SDLK_F11:          key = KEY_F11;           	break;
			case SDLK_F12:          key = KEY_F12;           	break;

			case SDLK_BACKSPACE:    key = KEY_BACKSPACE;     	break;
			case SDLK_KP_PERIOD:    key = KEY_KP_DELETE;        break;
			case SDLK_DELETE:       key = KEY_DELETE;           break;
			case SDLK_PAUSE:        key = KEY_PAUSE;         	break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:       key = KEY_SHIFT;         	break;

			case SDLK_LCTRL:
			case SDLK_RCTRL:        key = KEY_CTRL;          	break;

#ifdef __APPLE__
			case SDLK_RGUI:
			case SDLK_LGUI:         key = KEY_COMMAND;       	break;
#else
			case SDLK_RGUI:
			case SDLK_LGUI:         key = KEY_SUPER;         	break;
#endif

			case SDLK_RALT:
			case SDLK_LALT:         key = KEY_ALT;           	break;

			case SDLK_KP_5:         key = KEY_KP_5;          	break;
			case SDLK_INSERT:       key = KEY_INSERT;           break;
			case SDLK_KP_0:         key = KEY_KP_INSERT;        break;
			case SDLK_KP_MULTIPLY:  key = '*'; /*K_KP_STAR;*/ 	break;
			case SDLK_KP_PLUS:      key = KEY_KP_PLUS;       	break;
			case SDLK_KP_MINUS:     key = KEY_KP_MINUS;      	break;
			case SDLK_KP_DIVIDE:    key = KEY_KP_SLASH;      	break;

			case SDLK_MODE:         key = KEY_MODE;          	break;
			case SDLK_HELP:         key = KEY_HELP;          	break;
			case SDLK_PRINTSCREEN:  key = KEY_SCREENSHOT;       break;
			case SDLK_SYSREQ:       key = KEY_SYSREQ;        	break;
			case SDLK_MENU:         key = KEY_MENU;          	break;
			case SDLK_APPLICATION:	key = KEY_MENU;          	break;
			case SDLK_POWER:        key = KEY_POWER;         	break;
			case SDLK_UNDO:         key = KEY_UNDO;          	break;
			case SDLK_SCROLLLOCK:   key = KEY_SCROLLOCK;     	break;
			case SDLK_NUMLOCKCLEAR: key = KEY_KP_NUMLOCK;    	break;
			case SDLK_CAPSLOCK:     key = KEY_CAPSLOCK;      	break;

			default:
#if 1
				key = 0;
#else
				if ( !( keysym->sym & SDLK_SCANCODE_MASK ) && keysym->scancode <= 95 ) {
					// Map Unicode characters to 95 world keys using the key's scan code.
					// FIXME: There aren't enough world keys to cover all the scancodes.
					// Maybe create a map of scancode to quake key at start up and on
					// key map change; allocate world key numbers as needed similar
					// to SDL 1.2.
					key = KEY_WORLD_0 + (int)keysym->scancode;
				}
#endif
				break;
		}
	}

	if ( keysym->scancode == SDL_SCANCODE_GRAVE ) {
		//SDL_Keycode translated = SDL_GetKeyFromScancode( SDL_SCANCODE_GRAVE );

		//if ( translated == SDLK_CARET )
		{
			// Console keys can't be bound or generate characters
			key = KEY_CONSOLE;
		}
	}

	return key;
}

static void KeyDownEvent(uint32_t key)
{
	keys[key].down = true;
	keys[key].bound = false;
	keys[key].repeats++;

	// console key is hardcoded, so the user can never unbind it
	if (key == KEY_CONSOLE || (keys[KEY_SHIFT].down && key == KEY_ESCAPE)) {
		g_pEditor->m_bShowConsole = !g_pEditor->m_bShowConsole;
		return;
	}

	// only let the console process the event if its open
	if (g_pEditor->m_bShowConsole) {
		keys[key].down = false;
		keys[key].repeats--;
		return;
	}
}

static void KeyUpEvent(uint32_t key)
{
	const bool bound = keys[key].bound;

	keys[key].repeats = 0;
	keys[key].down = false;
	keys[key].bound = false;

	// don't process key-up events for the console key
	if (key == KEY_CONSOLE || (key == KEY_ESCAPE && keys[KEY_SHIFT].down)) {
		return;
	}

	// hardcoded screenshot key
	if (key == KEY_SCREENSHOT) {
		return;
	}
}

void KeyEvent(uint32_t key, bool down)
{
	if (down)
		KeyDownEvent(key);
	else
		KeyUpEvent(key);
}

typedef struct {
	int x, y;
	glm::ivec2 lastPos;
	glm::vec2 offset, delta;
} mouse_t;
mouse_t mouseState;

void CEventQueue::PumpKeyEvents(void)
{
	SDL_Event event;
	SDL_PumpEvents();

	while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

		switch (event.type) {
		case SDL_KEYDOWN:
            QueueEvent(SE_KEY, Com_TranslateSDL2ToQ3Key( &event.key.keysym, qtrue ), qtrue, 0, NULL);
			break;
		case SDL_KEYUP:
			QueueEvent(SE_KEY, Com_TranslateSDL2ToQ3Key( &event.key.keysym, qfalse ), qfalse, 0, NULL);
			break;
		case SDL_QUIT:
			QueueEvent(SE_WINDOW, event.type, 0, 0, NULL);
			break;
		case SDL_MOUSEMOTION:
//			mouseState.x = event.motion.x;
//			mouseState.y = event.motion.y;
//
//			mouseState.offset.x = mouseState.x - mouseState.lastPos.x;
//			mouseState.offset.y = mouseState.y - mouseState.lastPos.y;
//
//			mouseState.lastPos.x = mouseState.x;
//			mouseState.lastPos.y = mouseState.y;
//
//			mouseState.delta.x = ((float)mouseState.offset.x) / gui->mWindowWidth * 2;
//			mouseState.delta.y = ((float)mouseState.offset.y) / gui->mWindowHeight * 2;
//			mouseState.delta = glm::normalize(mouseState.delta);
//
//			if (Key_IsDown(KEY_MOUSE_LEFT) && editor->mode == MODE_MAP) {
//				gui->mCameraPos.x -= (mouseState.delta.x / 4);
//				gui->mCameraPos.y += (mouseState.delta.y / 4);
//			}
            QueueEvent(SE_MOUSE, event.motion.xrel, event.motion.yrel, 0, NULL);
            break;
		case SDL_MOUSEBUTTONUP:
			switch ( event.button.button ) {
			case SDL_BUTTON_LEFT:
				QueueEvent( SE_KEY, KEY_MOUSE_LEFT, qfalse, 0, NULL );
				break;
			case SDL_BUTTON_MIDDLE:
				QueueEvent( SE_KEY, KEY_MOUSE_MIDDLE, qfalse, 0, NULL );
				break;
			case SDL_BUTTON_RIGHT:
				QueueEvent( SE_KEY, KEY_MOUSE_RIGHT, qfalse, 0, NULL );
				break;
			};
            break;
		case SDL_MOUSEBUTTONDOWN:
            switch ( event.button.button ) {
			case SDL_BUTTON_LEFT:
				QueueEvent( SE_KEY, KEY_MOUSE_LEFT, qtrue, 0, NULL );
				break;
			case SDL_BUTTON_MIDDLE:
				QueueEvent( SE_KEY, KEY_MOUSE_MIDDLE, qtrue, 0, NULL );
				break;
			case SDL_BUTTON_RIGHT:
				QueueEvent( SE_KEY, KEY_MOUSE_RIGHT, qtrue, 0, NULL );
				break;
			};
            break;
		case SDL_MOUSEWHEEL:
			if ( event.wheel.y > 0 ) {
				QueueEvent( SE_KEY, KEY_WHEEL_UP, qtrue, 0, NULL );
				QueueEvent( SE_KEY, KEY_WHEEL_UP, qfalse, 0, NULL );
			}
			if ( event.wheel.y < 0 ) {
				QueueEvent( SE_KEY, KEY_WHEEL_DOWN, qtrue, 0, NULL );
				QueueEvent( SE_KEY, KEY_WHEEL_DOWN, qfalse, 0, NULL );
			}
			break;
		};
	}
}

sysEvent_t CEventQueue::GetSystemEvent(void)
{
	sysEvent_t ev;
	const char *s;

	// return if we have data
	if (mEventHead - mEventTail > 0)
		return mEventQueue[(mEventTail++) & MASK_QUEUED_EVENTS];
	
	PumpKeyEvents();

	// check for console commands

	// return if we have data
	if (mEventHead - mEventTail > 0)
		return mEventQueue[(mEventTail++) & MASK_QUEUED_EVENTS];
	
	// create a new empty event to return
	memset(&ev, 0, sizeof(ev));

	return ev;
}

sysEvent_t CEventQueue::GetRealEvent(void)
{
	return GetSystemEvent();
}

void CEventQueue::QueueEvent(sysEventType_t evType, int evValue, int evValue2, uint32_t ptrLength, void *ptr)
{
	sysEvent_t *ev;

	// try to combine all sequential mouse moves in one event
	if (evType == SE_MOUSE && mLastEvent->evType == SE_MOUSE && mEventHead != mEventTail) {
		mLastEvent->evValue += evValue;
		mLastEvent->evValue2 += evValue2;
		return;
	}

	ev = &mEventQueue[mEventHead & MASK_QUEUED_EVENTS];

	if (mEventHead - mEventTail >= MAX_EVENT_QUEUE) {
		Log_Printf("%s(type=%s,keys=(%i,%i)): overflow", __func__, EventName(evType), evValue, evValue2);
		// we are discarding an event, but avoid leaking memory
		if (ev->evPtr) {
			FreeMemory(ev->evPtr);
		}
		mEventTail++;
	}

	mEventHead++;

	ev->evType = evType;
	ev->evValue = evValue;
	ev->evValue2 = evValue2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;

	mLastEvent = ev;
}

sysEvent_t CEventQueue::GetEvent(void)
{
	if (mPushedEventsHead - mPushedEventsTail > 0) {
		return mPushedEvents[(mPushedEventsTail++) & (MAX_EVENT_QUEUE - 1)];
	}

	return GetRealEvent();
}

static void WindowEvent(uint32_t value)
{
	if (value == SDL_QUIT) {
		g_pApplication->Close();
	}
}

uint64_t CEventQueue::EventLoop(void)
{
	sysEvent_t ev;

	while (1) {
		ev = GetEvent();

		// no more events are available
		if (ev.evType == SE_NONE) {
			return 0;
		}

		switch (ev.evType) {
		case SE_KEY:
			KeyEvent(ev.evValue, ev.evValue2);
			break;
		case SE_WINDOW:
			WindowEvent(ev.evValue);
			break;
		case SE_MOUSE:
//			MouseEvent(ev.evValue, ev.evValue2);
			break;
		case SE_CONSOLE:
			g_pEditor->m_bShowConsole = !g_pEditor->m_bShowConsole;
			break;
		default:
			Error("Com_EventLoop: bad event type %i", ev.evType);
		};

		// free any block data
		if ( ev.evPtr ) {
			FreeMemory(ev.evPtr);
			ev.evPtr = NULL;
		}
	}

	return 0; // never reached
}