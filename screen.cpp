//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <ctime>

#include "screen.h"
#include "sound.h"
#include "game.h"
#include "utils.h"
#include "gamestate.h"
#include "image.h"
#include "sector.h"
#include "panel.h"
#include "gui.h"
#include "player.h"

//---------------------------------------------------------------------------

Gigalomania::Screen::Screen() : m_pos_x(0), m_pos_y(0), m_down_left(false), m_down_middle(false), m_down_right(false) {
#if SDL_MAJOR_VERSION == 1
	surface = NULL;
#else
	sdlWindow = NULL;
	sdlRenderer = NULL;
	width = 0;
	height = 0;
#endif
}

Gigalomania::Screen::~Screen() {
}

bool Gigalomania::Screen::canOpenFullscreen(int width, int height) {
#if SDL_MAJOR_VERSION == 1
	if( SDL_VideoModeOK(width, height, 32, SDL_HWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN) != 0 )
		return true;
	return false;
#else
	return true; // assume always true?
#endif
}

bool Gigalomania::Screen::open(int screen_width, int screen_height, bool fullscreen) {
	LOG("Screen::open(%d, %d, %s)\n", screen_width, screen_height, fullscreen?"fullscreen":"windowed");
#if SDL_MAJOR_VERSION == 1
	if( fullscreen )
		surface = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_HWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN);
	else
		surface = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_HWSURFACE|SDL_HWPALETTE);
	if( surface == NULL ) {
		LOG("failed to open screen at this resolution\n");
		return false;
	}
#else
	if( SDL_CreateWindowAndRenderer(screen_width, screen_height, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE, &sdlWindow, &sdlRenderer) != 0 ) {
		LOG("failed to open screen at this resolution\n");
		return false;
	}
	this->width = screen_width;
	this->height = screen_height;
	{
		SDL_BlendMode blendMode = SDL_BLENDMODE_NONE;
		SDL_GetRenderDrawBlendMode(sdlRenderer, &blendMode);
		LOG("render draw blend mode: %d\n", blendMode);
	}
	SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND); // needed for Screen::fillRectWithAlpha, as blending is off by default for drawing functions
#endif
	LOG("screen opened ok\n");

	if( game_g->isMobileUI() ) {
		SDL_ShowCursor(false); // comment out to test with system cursor, for testing purposes when ran on non-touchscreen devices
	}
	else {
		//SDL_ShowCursor(false);
		// Set up a blank cursor
		// We use this hacky way due to a bug in SDL_ShowCursor(false), where when alt-tabbing out of full screen mode in Windows, the mouse pointer is
		// trapped in the top left corner. See 
		unsigned char data[1] = "";
		unsigned char mask[1] = "";
		SDL_Cursor *cursor = SDL_CreateCursor(data, mask, 8, 1, 0, 0);
		SDL_Cursor *old_cursor = SDL_GetCursor();
		SDL_SetCursor(cursor);
		SDL_FreeCursor(old_cursor);
	}

#if SDL_MAJOR_VERSION == 1
	Image::setGraphicsOutput(surface);
#else
	Image::setGraphicsOutput(sdlRenderer);
#endif

	return true;
}

#if SDL_MAJOR_VERSION == 1
#else
void Gigalomania::Screen::setLogicalSize(int width, int height, bool smooth) {
	this->width = width;
	this->height = height;
	LOG("width, height: %d, %d\n", width, height);
	if( smooth ) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
	}
	else {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	}
	SDL_RenderSetLogicalSize(sdlRenderer, width, height);
}
#endif

void Gigalomania::Screen::setTitle(const char *title) {
#if SDL_MAJOR_VERSION == 1
	SDL_WM_SetCaption(title, "");
#else
	SDL_SetWindowTitle(sdlWindow, title);
#endif
}

void Gigalomania::Screen::clear() {
#if SDL_MAJOR_VERSION == 1
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = getWidth();
	rect.h = getHeight();
	SDL_FillRect(surface, &rect, 0);
#else
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);
#endif
}

void Gigalomania::Screen::refresh() {
#if SDL_MAJOR_VERSION == 1
	SDL_Flip(surface);
#else
	SDL_RenderPresent(sdlRenderer);
#endif
}

int Gigalomania::Screen::getWidth() const {
#if SDL_MAJOR_VERSION == 1
	return surface->w;
#else
	/*int w = 0, h = 0;
	SDL_RenderGetLogicalSize(sdlRenderer, &w, &h);
	return w;*/
	//LOG("width = %d\n", width);
	return this->width;
#endif
}

int Gigalomania::Screen::getHeight() const {
#if SDL_MAJOR_VERSION == 1
	return surface->h;
#else
	/*int w = 0, h = 0;
	SDL_RenderGetLogicalSize(sdlRenderer, &w, &h);
	return h;*/
	//LOG("height = %d\n", height);
	return this->height;
#endif
}

void Gigalomania::Screen::fillRect(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b) {
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
#if SDL_MAJOR_VERSION == 1
	Uint32 col = SDL_MapRGB(surface->format, r, g, b);
	SDL_FillRect(surface, &rect, col);
#else
	SDL_SetRenderDrawColor(sdlRenderer, r, g, b, 255);
	SDL_RenderFillRect(sdlRenderer, &rect);
#endif
}

#if SDL_MAJOR_VERSION == 1
// not supported with SDL 1.2 (as SDL_FillRect can't do blending)!
#else
void Gigalomania::Screen::fillRectWithAlpha(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha) {
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	//LOG("fill rect %d %d %d %d\n", r, g, b, alpha);
	SDL_SetRenderDrawColor(sdlRenderer, r, g, b, alpha);
	SDL_RenderFillRect(sdlRenderer, &rect);
}
#endif

#if SDL_MAJOR_VERSION == 1
// not supported with SDL 1.2
#else
void Gigalomania::Screen::drawLine(short x1, short y1, short x2, short y2, unsigned char r, unsigned char g, unsigned char b) {
	SDL_SetRenderDrawColor(sdlRenderer, r, g, b, 255);
	SDL_RenderDrawLine(sdlRenderer, x1, y1, x2, y2);
}
#endif

#if SDL_MAJOR_VERSION == 1
// not supported with SDL 1.2
#else
void Gigalomania::Screen::convertWindowToLogical(int *m_x, int *m_y) const {
	SDL_Rect rect;
	SDL_RenderGetViewport(sdlRenderer, &rect);
	float scale_x = 0.0f, scale_y = 0.0f;
	SDL_RenderGetScale(sdlRenderer, &scale_x, &scale_y);
	//LOG("viewport: %d x %d : %d, %d\n", rect.x, rect.y, rect.w, rect.h);
	//LOG("render scale: %f x %f\n", scale_x, scale_y);
	*m_x = (int)(*m_x / scale_x);
	*m_y = (int)(*m_y / scale_y);
	*m_x -= rect.x;
	*m_y -= rect.y;
}

void Gigalomania::Screen::getWindowSize(int *window_width, int *window_height) const {
	SDL_GetWindowSize(sdlWindow, window_width, window_height);
}
#endif


void Gigalomania::Screen::getMouseCoords(int *m_x, int *m_y) const {
	// SDL_GetMouseState doesn't play well with touch events
	/*SDL_GetMouseState(m_x, m_y);
	// need to convert from window space to logical space
#if SDL_MAJOR_VERSION == 1
#else
	this->convertWindowToLogical(m_x, m_y);
#endif*/
	*m_x = this->m_pos_x;
	*m_y = this->m_pos_y;
	//LOG("Screen::getMouseCoords: %d, %d\n", *m_x, *m_y);
}

bool Gigalomania::Screen::getMouseState(int *m_x, int *m_y, bool *m_left, bool *m_middle, bool *m_right) const {
	// SDL_GetMouseState doesn't play well with touch events
	/*int m_b = SDL_GetMouseState(m_x, m_y);
	*m_left = ( m_b & SDL_BUTTON(1) ) != 0;
	*m_middle = ( m_b & SDL_BUTTON(2) ) != 0;
	*m_right = ( m_b & SDL_BUTTON(3) ) != 0;
	// need to convert from window space to logical space
#if SDL_MAJOR_VERSION == 1
#else
	this->convertWindowToLogical(m_x, m_y);
#endif*/
	*m_x = this->m_pos_x;
	*m_y = this->m_pos_y;
	*m_left = this->m_down_left;
	*m_middle = this->m_down_middle;
	*m_right = this->m_down_right;
	/*if( *m_left ) {
		LOG("mouse down at: %d, %d\n", *m_x, *m_y);
	}*/
	//LOG("Screen::getMouseState: %d, %d\n", *m_x, *m_y);
	return ( *m_left || *m_middle || *m_right );
}

Application::Application() : quit(false), blank_mouse(false), compute_fps(false), fps(0.0f), last_time(0) {
	// uncomment to display fps
	//compute_fps = true;
}

Application::~Application() {
	LOG("quit SDL\n");
	SDL_Quit();
}

bool Application::init() {
#if defined(_WIN32)
	_putenv("SDL_VIDEO_CENTERED=0,0");
#elif defined(__MORPHOS__)
#else
	setenv("SDL_VIDEO_CENTERED", "0,0", 1);
#endif
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) == -1 ) {
		LOG("SDL_Init failed\n");
		return false;
	}
	return true;
}

unsigned int Application::getTicks() const {
	return SDL_GetTicks();
}

void Application::delay(unsigned int time) {
	SDL_Delay(time);
}

const int TICK_INTERVAL = 16; // 62.5 fps max

void Application::wait() {
	unsigned int now = game_g->getApplication()->getTicks();
	// use unsigned int and compare by subtraction to avoid overflow problems - see http://blogs.msdn.com/b/oldnewthing/archive/2005/05/31/423407.aspx, http://www.gammon.com.au/millis
	unsigned int elapsed = now - last_time;
	unsigned int delay = 0;
	if( elapsed >= TICK_INTERVAL ) {
		// fine, we've already passed TICK_INTERVAL
	}
	else {
		delay = TICK_INTERVAL - elapsed;
		game_g->getApplication()->delay(delay);
	}
	last_time = now + delay;
}

void Application::runMainLoop() {
	unsigned int elapsed_time = game_g->getApplication()->getTicks();

	SDL_Event event;
	quit = false;
	int last_fps_time = clock();
	const int fps_frames_c = 50;
	int frames = 0;
	while(!quit) {
		if( compute_fps && frames == fps_frames_c ) {
			int new_fps_time = clock();
			float t = ((float)(new_fps_time - last_fps_time)) / (float)CLOCKS_PER_SEC;
			this->fps = fps_frames_c / t;
			//LOG("FPS: %f\n", fps);
			frames = 0;
			last_fps_time = new_fps_time;
		}
		frames++;

		updateSound();

		// draw screen
		game_g->drawGame();

		/* wait() to avoid 100% CPU - it's debatable whether we should do this,
		 * due to risk of SDL_Delay waiting too long, but since Gigalomania
		 * doesn't really need high frame rate, might as well avoid using full
		 * CPU. Also good for battery-life on mobile platforms.
		 * This also has the side-effect of meaning we don't call updateGame()
		 * with too small timestep - we have a minimum step of at least
		 * TICK_INTERVAL.
		 */
		wait();

		unsigned int new_time = game_g->getApplication()->getTicks();
		//LOG("%d, %d\n", new_time, new_time - elapsed_time);
		if( !game_g->isPaused() ) {
			game_g->updateTime(new_time - elapsed_time);
		}
		else
			game_g->updateTime(0);
		elapsed_time = new_time;

		// user input
		while( SDL_PollEvent(&event) == 1 ) {
			switch (event.type) {
			case SDL_QUIT:
				// SDL_QUIT may mean a message from the OS (e.g., OS shutting down) - so we quit immediately, saving the current state
				// It will also be sent if the user clicks the window close button, but reasonable to also interpret this as "quit immediately"
				// Also important for Android to quit immediately, as SDL_QUIT is sent when the screen is locked - if we don't quit immediately,
				// the app hangs when the screen is unlocked.
				game_g->requestQuit(true);
				break;
			case SDL_KEYDOWN:
				{
#if SDL_MAJOR_VERSION == 1
					SDL_keysym key = event.key.keysym;
#else
					SDL_Keysym key = event.key.keysym;
#endif
					if( key.sym == SDLK_ESCAPE || key.sym == SDLK_q
#if SDL_MAJOR_VERSION == 1
#else
						|| key.sym == SDLK_AC_BACK // SDLK_AC_BACK required for Android
#endif
						) {
						game_g->requestQuit(false);
					}
					else if( key.sym == SDLK_p ) {
						game_g->togglePause();
					}
					else if( key.sym == SDLK_RETURN ) {
						game_g->keypressReturn();
					}
					break;
				}
			case SDL_MOUSEBUTTONDOWN:
				{
#if SDL_MAJOR_VERSION == 1
#else
					if( event.motion.which == SDL_TOUCH_MOUSEID ) {
						// touchscreens handled below; n.b., for Windows requires SDL 2.0.4 or later otherwise "which" isn't set to SDL_TOUCH_MOUSEID
						break;
					}
#endif
					//LOG("received mouse down\n");
					// if this code is changed, consider whether SDL_FINGERDOWN also needs updating
					int m_x = event.button.x;
					int m_y = event.button.y;
					game_g->getScreen()->setMousePos(m_x, m_y);
					bool m_left = false, m_middle = false, m_right = false;
					Uint8 button = event.button.button;
					if( button == SDL_BUTTON_LEFT ) {
						m_left = true;
						game_g->getScreen()->setMouseLeft(true);
					}
					else if( button == SDL_BUTTON_MIDDLE ) {
						m_middle = true;
						game_g->getScreen()->setMouseMiddle(true);
					}
					else if( button == SDL_BUTTON_RIGHT ) {
						m_right = true;
						game_g->getScreen()->setMouseRight(true);
					}

					if( game_g->isPaused() ) {
						// click automatically unpaused (needed to work without keyboard!)
						game_g->togglePause();
					}
					else if( m_left || m_middle || m_right ) {
						/*int m_x = 0, m_y = 0;
						game_g->getScreen()->getMouseCoords(&m_x, &m_y);*/
						//LOG("received mouse click: %d, %d\n", m_x, m_y);
						game_g->mouseClick(m_x, m_y, m_left, m_middle, m_right, true);
					}

					break;
				}
			case SDL_MOUSEBUTTONUP:
				{
#if SDL_MAJOR_VERSION == 1
#else
					if( event.motion.which == SDL_TOUCH_MOUSEID ) {
						// touchscreens handled below; n.b., for Windows requires SDL 2.0.4 or later otherwise "which" isn't set to SDL_TOUCH_MOUSEID
						break;
					}
#endif
					//LOG("received mouse up\n");
					// if this code is changed, consider whether SDL_FINGERUP also needs updating
					Uint8 button = event.button.button;
					if( button == SDL_BUTTON_LEFT ) {
						game_g->getScreen()->setMouseLeft(false);
					}
					else if( button == SDL_BUTTON_MIDDLE ) {
						game_g->getScreen()->setMouseMiddle(false);
					}
					else if( button == SDL_BUTTON_RIGHT ) {
						game_g->getScreen()->setMouseRight(false);
					}
					break;
				}
			case SDL_MOUSEMOTION:
				{
#if SDL_MAJOR_VERSION == 1
#else
					if( event.motion.which == SDL_TOUCH_MOUSEID ) {
						// touchscreens handled below; n.b., for Windows requires SDL 2.0.4 or later otherwise "which" isn't set to SDL_TOUCH_MOUSEID
						break;
					}
#endif
					// if this code is changed, consider whether SDL_FINGERMOTION also needs updating
					int m_x = event.motion.x;
					int m_y = event.motion.y;
					this->blank_mouse = false;
					game_g->getScreen()->setMousePos(m_x, m_y);
					break;
				}
#if SDL_MAJOR_VERSION == 1
#else
			// support for touchscreens
			case SDL_FINGERDOWN:
				{
					//LOG("received fingerdown: %f , %f\n", event.tfinger.x, event.tfinger.y);
					int window_width = 0, window_height = 0;
					game_g->getScreen()->getWindowSize(&window_width, &window_height);
					int m_x = (int)(event.tfinger.x*window_width);
					int m_y = (int)(event.tfinger.y*window_height);
					//LOG("    %d, %d\n", m_x, m_y);
					game_g->getScreen()->convertWindowToLogical(&m_x, &m_y);
					//LOG("    logical %d, %d\n", m_x, m_y);
					game_g->getScreen()->setMousePos(m_x, m_y);
					game_g->getScreen()->setMouseLeft(true);
					this->blank_mouse = true;

					if( game_g->isPaused() ) {
						// click automatically unpaused (needed to work without keyboard!)
						game_g->togglePause();
					}
					else {
						game_g->mouseClick(m_x, m_y, true, false, false, true);
					}
					break;
				}
			case SDL_FINGERUP:
				{
					//LOG("received fingerup\n");
					game_g->getScreen()->setMouseLeft(false);
					this->blank_mouse = true;
					break;
				}
			case SDL_FINGERMOTION:
				{
					//LOG("received fingermotion: %f , %f\n", event.tfinger.x, event.tfinger.y);
					int window_width = 0, window_height = 0;
					game_g->getScreen()->getWindowSize(&window_width, &window_height);
					int m_x = (int)(event.tfinger.x*window_width);
					int m_y = (int)(event.tfinger.y*window_height);
					//LOG("    %d, %d\n", m_x, m_y);
					game_g->getScreen()->convertWindowToLogical(&m_x, &m_y);
					//LOG("    logical %d, %d\n", m_x, m_y);
					game_g->getScreen()->setMousePos(m_x, m_y);
					this->blank_mouse = true;
					break;
				}
#endif
#if SDL_MAJOR_VERSION == 1
			case SDL_ACTIVEEVENT:
#ifndef AROS
				// disabled for AROS, as we receive inactive events when the mouse goes outside the window!
				if( (event.active.state & SDL_APPINPUTFOCUS) != 0 || (event.active.state & SDL_APPACTIVE) != 0 ) {
					if( event.active.gain == 1 ) {
						// activate
						game_g->activate();
					}
					else if( event.active.gain == 0 ) {
						// inactive
						game_g->deactivate();
					}
				}
#endif
				break;
#else
			case SDL_WINDOWEVENT:
				if( event.window.event == SDL_WINDOWEVENT_SHOWN || event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ) {
					// activate
					game_g->activate();
				}
				else if( event.window.event == SDL_WINDOWEVENT_HIDDEN || event.window.event == SDL_WINDOWEVENT_FOCUS_LOST ) {
					// inactive
					game_g->deactivate();
				}
				break;
#endif
			}
		}
		SDL_PumpEvents();

		game_g->updateGame();
	}
}

//#endif
