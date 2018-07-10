//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <cerrno> // n.b., needed on Linux at least

#include <stdexcept> // needed for Android at least

#include <sstream>

#include "gamestate.h"
#include "game.h"
#include "utils.h"
#include "sector.h"
#include "gui.h"
#include "player.h"
#include "tutorial.h"

#include "screen.h"
#include "image.h"
#include "sound.h"

//---------------------------------------------------------------------------

const int defenders_ticks_per_update_c = (int)(60.0 * ticks_per_frame_c * time_ratio_c); // consider a turn every this number of ticks
const int soldier_move_rate_c = (int)(1.8 * ticks_per_frame_c * time_ratio_c); // ticks per pixel - needs to be in sync with the animation!
const int cannon_move_rate_c = (int)(0.6 * ticks_per_frame_c * time_ratio_c); // ticks per pixel - needs to be in sync with the animation!
const int air_move_rate_c = (int)(0.2 * ticks_per_frame_c * time_ratio_c); // ticks per pixel
const int soldier_turn_rate_c = (int)(50.0 * ticks_per_frame_c * time_ratio_c); // mean ticks per turn

const int shield_step_y_c = 20;

class Soldier {
	static int sort_soldier_pair(const void *v1,const void *v2);
public:
	int player;
	int epoch;
	int xpos, ypos;
	AmmoDirection dir;
	Soldier(int player,int epoch,int xpos,int ypos) {
		ASSERT_S_EPOCH(epoch);
		this->player = player;
		this->epoch = epoch;
		this->xpos = xpos;
		this->ypos = ypos;
		this->dir = (AmmoDirection)(rand() % 4);
	}
	static void sortSoldiers(Soldier **soldiers,int n_soldiers) {
		qsort(soldiers, n_soldiers, sizeof( Soldier *), sort_soldier_pair);
	}
};

int Soldier::sort_soldier_pair(const void *v1,const void *v2) {
	Soldier *s1 = *(Soldier **)v1;
	Soldier *s2 = *(Soldier **)v2;
	/*if( s1->epoch >= 6 )
	return 1;
	else if( s2->epoch >= 6 )
	return -1;
	else*/
	return (s1->ypos - s2->ypos);
}

void Feature::draw() const {
	const int ticks_per_frame_c = 110; // tree animation looks better if offset from main animation, and if slightly slower
	int counter = ( game_g->getRealTime() * game_g->getTimeRate() ) / ticks_per_frame_c;
	image[counter % n_frames]->draw(xpos, ypos);
}

TimedEffect::TimedEffect() {
	this->timeset = game_g->getRealTime();
	this->func_finish = NULL;
}

TimedEffect::TimedEffect(int delay, void (*func_finish)()) {
	this->timeset = game_g->getRealTime() + delay;
	this->func_finish = func_finish;
}

const int ammo_time_c = 1000;
const float ammo_speed_c = 1.5f; // higher is faster

AmmoEffect::AmmoEffect(PlayingGameState *gamestate,int epoch, AmmoDirection dir, int xpos, int ypos) : TimedEffect(), gamestate(gamestate) {
	ASSERT_EPOCH(epoch);
	this->gametimeset = game_g->getGameTime();
	this->epoch = epoch;
	this->dir = dir;
	this->xpos = xpos;
	this->ypos = ypos;
}

bool AmmoEffect::render() const {
	int time = game_g->getRealTime() - this->timeset;
	if( time < 0 )
		return false;
	int gametime = game_g->getGameTime() - this->gametimeset;
	int x = xpos;
	int y = ypos;
	int dist = (int)(gametime * ammo_speed_c);
	if( dir == ATTACKER_AMMO_BOMB )
		dist /= 2;
	if( dir == ATTACKER_AMMO_DOWN )
		y += dist;
	else if( dir == ATTACKER_AMMO_UP )
		y -= dist;
	else if( dir == ATTACKER_AMMO_LEFT )
		x -= dist;
	else if( dir == ATTACKER_AMMO_RIGHT )
		x += dist;
	else if( dir == ATTACKER_AMMO_BOMB )
		y += dist;
	else {
		ASSERT(0);
	}
	Gigalomania::Image *image = game_g->attackers_ammo[epoch][dir];
	if( dir == ATTACKER_AMMO_BOMB && dist > 24 ) {
		if( game_g->explosions[0] != NULL ) {
			int w = image->getScaledWidth();
			int w2 = game_g->explosions[0]->getScaledWidth();
			gamestate->explosionEffect(offset_land_x_c + x + (w-w2)/2, offset_land_y_c + y);
		}
		return true;
	}
	if( x < 0 || y < 0 )
		return true;
	x += offset_land_x_c;
	y += offset_land_y_c;
	if( x + image->getScaledWidth() >= game_g->getScreen()->getWidth() || y + image->getScaledHeight() >= game_g->getScreen()->getHeight() )
		return true;
	image->draw(x, y);
	if( time > ammo_time_c )
		return true;
	return false;
}

const int fade_time_c = 1000;
const int whitefade_time_c = 1000;

FadeEffect::FadeEffect(bool white,bool out,int delay, void (*func_finish)()) : TimedEffect(delay, func_finish) {
	this->white = white;
	this->out = out;
#if SDL_MAJOR_VERSION == 1
	this->image = Gigalomania::Image::createBlankImage(game_g->getScreen()->getWidth(), game_g->getScreen()->getHeight(), 24);
	int r = 0, g = 0, b = 0;
	if( white ) {
		r = g = b = 255;
	}
	else {
		r = g = b = 0;
	}
	image->fillRect(0, 0, game_g->getScreen()->getWidth(), game_g->getScreen()->getHeight(), r, g, b);
	this->image->convertToDisplayFormat();
#else
	image = NULL;
#endif
}

FadeEffect::~FadeEffect() {
#if SDL_MAJOR_VERSION == 1
	delete image;
#endif
}

bool FadeEffect::render() const {
	int time = game_g->getRealTime() - this->timeset;
	int length = white ? whitefade_time_c : fade_time_c;
	if( time < 0 )
		return false;
	double alpha = 0.0;
	if( white ) {
		alpha = ((double)time) / (0.5 * (double)length);
		if( alpha > 2.0 )
			alpha = 2.0;
		if( time > length/2.0 ) {
			alpha = 2.0 - alpha;
		}
	}
	else {
		alpha = ((double)time) / (double)length;
		if( alpha > 1.0 )
			alpha = 1.0;
		if( !out )
			alpha = 1.0 - alpha;
	}
#if SDL_MAJOR_VERSION == 1
	image->drawWithAlpha(0, 0, (unsigned char)(alpha * 255));
#else
	unsigned char value = white ? 255 : 0;
	game_g->getScreen()->fillRectWithAlpha(0, 0, game_g->getScreen()->getWidth(), game_g->getScreen()->getHeight(), value, value, value, (unsigned char)(alpha * 255));
#endif
	if( time > length ) // we still need to draw the fade, on the last time
		return true;
	return false;
}

const int flashingsquare_flash_time_c = 250;
const int flashing_square_n_flashes_c = 8;

bool FlashingSquare::render() const {
	int time = game_g->getRealTime() - this->timeset;
	if( time < 0 )
		return false;
	if( time > flashingsquare_flash_time_c * flashing_square_n_flashes_c ) {
		return true;
	}

	bool flash = ( time / flashingsquare_flash_time_c ) % 2 == 0;
	if( flash ) {
		int map_x = offset_map_x_c + 16 * this->xpos;
		int map_y = offset_map_y_c + 16 * this->ypos;
		game_g->flashingmapsquare->draw(map_x, map_y);
	}
	return false;
}

bool AnimationEffect::render() const {
	int time = game_g->getRealTime() - this->timeset;
	if( time < 0 )
		return false;
	int frame = time / time_per_frame;
	if( frame >= n_images )
		return true;
	//this->finished = true;
	else {
		if( !dir )
			frame = n_images - 1 - frame;
		images[frame]->draw(xpos, ypos);
	}
	return false;
}

bool TextEffect::render() const {
	int time = game_g->getRealTime() - this->timeset;
	if( time < 0 )
		return false;
	else if( time > duration )
		return true;
	Gigalomania::Image::write(xpos, ypos, game_g->letters_small, text.c_str(), Gigalomania::Image::JUSTIFY_CENTRE);
	return false;
}

GameState::GameState(int client_player) : client_player(client_player) {
	this->fade = NULL;
	this->whitefade = NULL;
	this->screen_page = new PanelPage(0, 0);
	this->screen_page->setTolerance(0);
	this->mobile_ui_display_mouse = false;
	this->mouse_image = NULL;
	this->mouse_off_x = 0;
	this->mouse_off_y = 0;
	this->confirm_type = CONFIRMTYPE_UNKNOWN;
    this->confirm_window = NULL;
    this->confirm_button_1 = NULL;
    this->confirm_button_2 = NULL;
    this->confirm_button_3 = NULL;
}

GameState::~GameState() {
	LOG("~GameState()\n");
	if( fade != NULL ) {
		fade->clearFuncFinish();
		delete fade;
	}
	if( whitefade != NULL )
		delete whitefade;

	if( this->screen_page )
		delete screen_page;

	LOG("~GameState() done\n");
}

void GameState::reset() {
    //LOG("GameState::reset()\n");
	this->screen_page->free(true);
}

void GameState::setDefaultMouseImage() {
	if( game_g->isDemo() )
		mouse_image = game_g->mouse_pointers[0];
	else
		mouse_image = game_g->mouse_pointers[client_player];
	mobile_ui_display_mouse = false;
}

void GameState::draw() {
	if( mouse_image != NULL ) {
		bool touch_mode = game_g->isMobileUI() || game_g->getApplication()->isBlankMouse();
		if( touch_mode && mobile_ui_display_mouse ) {
			mouse_image->draw(default_width_c - mouse_image->getScaledWidth(), 0);
		}
		else if( !touch_mode ) {
			int m_x = 0, m_y = 0;
			game_g->getScreen()->getMouseCoords(&m_x, &m_y);
			m_x = (int)(m_x / game_g->getScaleWidth());
			m_y = (int)(m_y / game_g->getScaleHeight());
			m_x += mouse_off_x;
			m_y += mouse_off_y;
			mouse_image->draw(m_x, m_y);
		}
	}

	if( fade ) {
		if( fade->render() ) {
			FadeEffect *copy = fade;
			delete fade;
			if( copy == fade )
				fade = NULL;
		}
	}
	if( whitefade ) {
		if( whitefade->render() ) {
			FadeEffect *copy = whitefade;
			delete whitefade;
			if( copy == whitefade )
				whitefade = NULL;
		}
	}

	if( game_g->isPaused() ) {
		string str = game_g->isMobileUI() ? "touch screen\nto unpause game" : "press p or click\nmouse to unpause game";
		// n.b., don't use 120 for y pos, need to avoid collision with quit game message
		// and offset x pos slightly, to avoid overlapping with GUI
		Gigalomania::Image::write(120, 100, game_g->letters_large, str.c_str(), Gigalomania::Image::JUSTIFY_LEFT);
	}

	if( game_g->getApplication()->hasFPS() ) {
		float fps = game_g->getApplication()->getFPS();
		if( fps > 0.0f ) {
			stringstream str;
			str << fps;
			Gigalomania::Image::writeMixedCase(4, default_height_c - 16, game_g->letters_large, game_g->letters_small, game_g->numbers_white, str.str().c_str(), Gigalomania::Image::JUSTIFY_LEFT);
		}
	}

	game_g->getScreen()->refresh();
}

void GameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	this->screen_page->input(m_x, m_y, m_left, m_middle, m_right, click);
}

void GameState::requestQuit(bool force_quit) {
	game_g->getApplication()->setQuit();
}

void GameState::createQuitWindow() {
    if( confirm_window == NULL && !game_g->isStateChanged() ) {
		confirm_type = CONFIRMTYPE_QUITGAME;
		confirm_window = new PanelPage(0, 0, default_width_c, default_height_c);
		confirm_window->setBackground(0, 0, 0, 200);
		const int offset_x_c = 120, offset_y_c = 120;
		Button *text_button = new Button(offset_x_c, offset_y_c, "REALLY QUIT?", game_g->letters_large);
		confirm_window->add(text_button);
		confirm_button_1 = new Button(offset_x_c, offset_y_c+16, "YES", game_g->letters_large);
		confirm_window->add(confirm_button_1);
		confirm_button_2 = new Button(offset_x_c+32, offset_y_c+16, "NO", game_g->letters_large);
		confirm_window->add(confirm_button_2);
		screen_page->add(confirm_window);
	}
	else if( confirm_window != NULL && !game_g->isStateChanged() ) {
		closeConfirmWindow();
	}
}

void GameState::closeConfirmWindow() {
    //LOG("GameState::closeConfirmWindow()\n");
    if( confirm_window != NULL ) {
        delete confirm_window;
        confirm_window = NULL;
        confirm_button_1 = NULL;
        confirm_button_2 = NULL;
        confirm_button_3 = NULL;
    }
    //LOG("GameState::closeConfirmWindow() done\n");
}

ChooseGameTypeGameState::ChooseGameTypeGameState(int client_player) : GameState(client_player) {
	this->choosegametypePanel = NULL;
	T_ASSERT(game_g->getTutorial() == NULL);
}

ChooseGameTypeGameState::~ChooseGameTypeGameState() {
	LOG("~ChooseGameTypeGameState()\n");
	if( this->choosegametypePanel )
		delete choosegametypePanel;
	LOG("~ChooseGameTypeGameState() done\n");
}

ChooseGameTypePanel *ChooseGameTypeGameState::getChooseGameTypePanel() {
	return this->choosegametypePanel;
}

void ChooseGameTypeGameState::reset() {
    //LOG("ChooseGameTypeGameState::reset()\n");
	this->screen_page->free(true);

	if( this->choosegametypePanel != NULL ) {
		delete this->choosegametypePanel;
		this->choosegametypePanel = NULL;
	}
	this->choosegametypePanel = new ChooseGameTypePanel();
}

void ChooseGameTypeGameState::draw() {
	game_g->getScreen()->clear(); // SDL on Android and OS X at least require screen to be cleared (otherwise we get corrupt regions outside of the main area)
	game_g->background->draw(0, 0);

	this->choosegametypePanel->draw();

	this->screen_page->draw();
	//this->screen_page->drawPopups();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void ChooseGameTypeGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	this->choosegametypePanel->input(m_x, m_y, m_left, m_middle, m_right, click);
}

ChooseDifficultyGameState::ChooseDifficultyGameState(int client_player) : GameState(client_player) {
	this->choosedifficultyPanel = NULL;
}

ChooseDifficultyGameState::~ChooseDifficultyGameState() {
	LOG("~ChooseDifficultyGameState()\n");
	if( this->choosedifficultyPanel )
		delete choosedifficultyPanel;
	LOG("~ChooseDifficultyGameState() done\n");
}

ChooseDifficultyPanel *ChooseDifficultyGameState::getChooseDifficultyPanel() {
	return this->choosedifficultyPanel;
}

void ChooseDifficultyGameState::reset() {
    //LOG("ChooseDifficultyGameState::reset()\n");
	this->screen_page->free(true);

	if( this->choosedifficultyPanel != NULL ) {
		delete this->choosedifficultyPanel;
		this->choosedifficultyPanel = NULL;
	}
	this->choosedifficultyPanel = new ChooseDifficultyPanel();
}

void ChooseDifficultyGameState::draw() {
	game_g->getScreen()->clear(); // SDL on Android and OS X at least require screen to be cleared (otherwise we get corrupt regions outside of the main area)
	game_g->background->draw(0, 0);

	this->choosedifficultyPanel->draw();

	this->screen_page->draw();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void ChooseDifficultyGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	this->choosedifficultyPanel->input(m_x, m_y, m_left, m_middle, m_right, click);
}

ChoosePlayerGameState::ChoosePlayerGameState(int client_player) : GameState(client_player), button_red(NULL), button_yellow(NULL), button_green(NULL), button_blue(NULL) {
}

ChoosePlayerGameState::~ChoosePlayerGameState() {
	LOG("~ChoosePlayerGameState()\n");
	LOG("~ChoosePlayerGameState() done\n");
}

void ChoosePlayerGameState::reset() {
	this->screen_page->free(true);

	const int xpos = 64;
	int ypos = 48;
	const int ydiff = 48;
	const int xindent = 8;
	const int ylargediff = game_g->letters_large[0]->getScaledHeight() + 2;
	const int ysmalldiff = game_g->letters_small[0]->getScaledHeight() + 2;
	const int draw_offset_x = 32;

	button_red = new Button(xpos-draw_offset_x, ypos, draw_offset_x, ylargediff + 2*ysmalldiff, "CONTROLLER OF THE RED PEOPLE", game_g->letters_large);
	screen_page->add(new Button(xpos+xindent, ypos+ylargediff, "SPECIAL SKILL STRENGTH", game_g->letters_small));
	screen_page->add(new Button(xpos+xindent, ypos+ylargediff+ysmalldiff, "UNARMED MEN ARE STRONGER IN COMBAT", game_g->letters_small));
	ypos += ydiff;
	screen_page->add(button_red);

	button_green = new Button(xpos-draw_offset_x, ypos, draw_offset_x, ylargediff + 2*ysmalldiff, "CONTROLLER OF THE GREEN PEOPLE", game_g->letters_large);
	screen_page->add(new Button(xpos+xindent, ypos+ylargediff, "SPECIAL SKILL CONSTRUCTION", game_g->letters_small));
	screen_page->add(new Button(xpos+xindent, ypos+ylargediff+ysmalldiff, "FASTER AT BUILDING NEW TOWERS", game_g->letters_small));
	ypos += ydiff;
	screen_page->add(button_green);

	button_yellow = new Button(xpos-draw_offset_x, ypos, draw_offset_x, ylargediff + 2*ysmalldiff, "CONTROLLER OF THE YELLOW PEOPLE", game_g->letters_large);
	screen_page->add(new Button(xpos+xindent, ypos+ylargediff, "SPECIAL SKILL DIPLOMACY", game_g->letters_small));
	screen_page->add(new Button(xpos+xindent, ypos+ylargediff+ysmalldiff, "EASIER TO FORM ALLIANCES", game_g->letters_small));
	ypos += ydiff;
	screen_page->add(button_yellow);

	button_blue = new Button(xpos-draw_offset_x, ypos, draw_offset_x, ylargediff + 2*ysmalldiff, "CONTROLLER OF THE BLUE PEOPLE", game_g->letters_large);
	screen_page->add(new Button(xpos+xindent, ypos+ylargediff, "SPECIAL SKILL DEFENCE", game_g->letters_small));
	screen_page->add(new Button(xpos+xindent, ypos+ylargediff+ysmalldiff, "BUILDINGS STRONGER AGAINST ATTACK", game_g->letters_small));
	ypos += ydiff;
	screen_page->add(button_blue);
}

void ChoosePlayerGameState::draw() {
	game_g->getScreen()->clear(); // SDL on Android and OS X at least require screen to be cleared (otherwise we get corrupt regions outside of the main area)
	//player_select->draw(0, 0, false);
	game_g->background->draw(0, 0);
    Gigalomania::Image::writeMixedCase(160, 16, game_g->letters_large, game_g->letters_small, NULL, "Select a Player", Gigalomania::Image::JUSTIFY_CENTRE);

	const int y_offset = 2; // must be even, otherwise we have graphical problems when running at 1280x1024 mode
	if( game_g->player_heads_select[0] != NULL )
		game_g->player_heads_select[0]->draw(button_red->getLeft(), button_red->getTop()+y_offset);
	if( game_g->player_heads_select[1] != NULL )
		game_g->player_heads_select[1]->draw(button_green->getLeft(), button_green->getTop()+y_offset);
	if( game_g->player_heads_select[2] != NULL )
		game_g->player_heads_select[2]->draw(button_yellow->getLeft(), button_yellow->getTop()+y_offset);
	if( game_g->player_heads_select[3] != NULL )
		game_g->player_heads_select[3]->draw(button_blue->getLeft(), button_blue->getTop()+y_offset);

	this->screen_page->draw();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void ChoosePlayerGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	int player = -1;
    if( m_left && click && button_red->mouseOver(m_x, m_y) ) {
		player = 0;
	}
    else if( m_left && click && button_yellow->mouseOver(m_x, m_y) ) {
		player = 2;
	}
    else if( m_left && click && button_green->mouseOver(m_x, m_y) ) {
		player = 1;
	}
    else if( m_left && click && button_blue->mouseOver(m_x, m_y) ) {
		player = 3;
	}

	if( player != -1 ) {
		game_g->setClientPlayer(player);
		if( game_g->getGameType() == GAMETYPE_TUTORIAL ) {
			game_g->setGameStateID(GAMESTATEID_CHOOSETUTORIAL);
		}
		else {
			game_g->setGameStateID(GAMESTATEID_PLACEMEN);
			game_g->newGame();
		}
	}
}

ChooseTutorialGameState::ChooseTutorialGameState(int client_player) : GameState(client_player) {
}

void ChooseTutorialGameState::reset() {
	this->screen_page->free(true);

	const int xpos = 96;
	int ypos = 48;
	const int ydiff = 16;

	vector<TutorialInfo> infos = TutorialManager::getTutorialInfo();
	for(vector<TutorialInfo>::const_iterator iter = infos.begin(); iter != infos.end(); ++iter) {
		TutorialInfo info = *iter;
		Button *button = new Button(xpos, ypos, info.text.c_str(), game_g->letters_large);
		button->setId(info.id);
		ypos += ydiff;
		screen_page->add(button);
		buttons.push_back(button);
	}
}

void ChooseTutorialGameState::draw() {
	game_g->getScreen()->clear(); // SDL on Android and OS X at least require screen to be cleared (otherwise we get corrupt regions outside of the main area)
	game_g->background->draw(0, 0);
    Gigalomania::Image::writeMixedCase(160, 16, game_g->letters_large, game_g->letters_small, NULL, "Select a Tutorial", Gigalomania::Image::JUSTIFY_CENTRE);

	this->screen_page->draw();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void ChooseTutorialGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	for(vector<Button *>::const_iterator iter = buttons.begin(); iter != buttons.end(); ++iter) {
		const Button *button = *iter;
	    if( m_left && click && button->mouseOver(m_x, m_y) ) {
			game_g->setupTutorial(button->getId());
			game_g->setCurrentIsand(game_g->getTutorial()->getStartEpoch(), game_g->getTutorial()->getIsland());
			game_g->setupPlayers();
			game_g->setGameStateID(GAMESTATEID_PLAYING);
			break;
		}
	}
}

PlaceMenGameState::PlaceMenGameState(int client_player) : GameState(client_player), start_map_x(-1), start_map_y(-1) {
	this->off_x = 220;
	this->off_y = 32;
	this->choosemenPanel = NULL;
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			map_panels[x][y] = NULL;
		}
	}
}

PlaceMenGameState::~PlaceMenGameState() {
	LOG("~PlaceMenGameState()\n");
	if( this->choosemenPanel )
		delete choosemenPanel;
	LOG("~PlaceMenGameState() done\n");
}

ChooseMenPanel *PlaceMenGameState::getChooseMenPanel() {
	return this->choosemenPanel;
}

const PanelPage *PlaceMenGameState::getMapPanel(int x, int y) const {
	ASSERT( x >= 0 && x < map_width_c );
	ASSERT( y >= 0 && y < map_height_c );
	return this->map_panels[x][y];
}

PanelPage *PlaceMenGameState::getMapPanel(int x, int y) {
	ASSERT( x >= 0 && x < map_width_c );
	ASSERT( y >= 0 && y < map_height_c );
	return this->map_panels[x][y];
}

void PlaceMenGameState::reset() {
    //LOG("PlaceMenGameState::reset()\n");
	/*if( !_CrtCheckMemory() ) {
		throw "_CrtCheckMemory FAILED";
	}*/
	this->screen_page->free(true);

	if( this->choosemenPanel != NULL ) {
		delete this->choosemenPanel;
		this->choosemenPanel = NULL;
	}

	this->choosemenPanel = new ChooseMenPanel(this);

	// setup screen_page buttons
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			map_panels[x][y] = NULL;
			if( game_g->getMap()->isSectorAt(x, y) ) {
				//int map_x = offset_map_x_c + 16 * x;
				int map_x = this->off_x - 8 * map_width_c + 16 * x;
				int map_y = this->off_y + 16 * y;
				PanelPage *panel = new PanelPage(map_x, map_y, 16, 16);
				panel->setInfoLMB("place starting tower\nin this sector");
				panel->setVisible(false);
				screen_page->add(panel);
				map_panels[x][y] = panel;
			}
		}
	}
	/*if( !_CrtCheckMemory() ) {
		throw "_CrtCheckMemory FAILED";
	}*/
}

void PlaceMenGameState::draw() {
	char buffer[256] = "";

	game_g->getScreen()->clear(); // SDL on Android and OS X at least require screen to be cleared (otherwise we get corrupt regions outside of the main area)
	game_g->background_islands->draw(0, 0);

	if( !game_g->isUsingOldGfx() ) {
		if( patchVersion == 0 )
			sprintf(buffer, "Gigalomania v%d.%d", majorVersion, minorVersion);
		else
			sprintf(buffer, "Gigalomania v%d.%d.%d", majorVersion, minorVersion, patchVersion);
	    Gigalomania::Image::writeMixedCase(160, 228, game_g->letters_large, game_g->letters_small, game_g->numbers_white, buffer, Gigalomania::Image::JUSTIFY_CENTRE);
	}

    /*this->choosemenPanel->draw();
    this->screen_page->draw();
    GameState::draw();
    return;*/

    int l_h = game_g->letters_large[0]->getScaledHeight();
	int s_h = game_g->letters_small[0]->getScaledHeight();
	const int cx = this->off_x;
    int cy = this->off_y + 104;
	Gigalomania::Image::writeMixedCase(cx, cy, game_g->letters_large, game_g->letters_small, NULL, game_g->getMap()->getName(), Gigalomania::Image::JUSTIFY_CENTRE);
	cy += s_h + 2;
	Gigalomania::Image::writeMixedCase(cx, cy, game_g->letters_large, game_g->letters_small, NULL, "of the", Gigalomania::Image::JUSTIFY_CENTRE);
	cy += l_h + 2;
	sprintf(buffer, "%s AGE", epoch_names[game_g->getStartEpoch()]);
	Gigalomania::Image::writeMixedCase(cx, cy, game_g->letters_large, game_g->letters_small, NULL, buffer, Gigalomania::Image::JUSTIFY_CENTRE);
    cy += l_h + 2;

	int year = epoch_dates[game_g->getStartEpoch()];
	bool shiny = game_g->getStartEpoch() == n_epochs_c-1;
	Gigalomania::Image::writeNumbers(cx+8, cy, shiny ? game_g->numbers_largeshiny : game_g->numbers_largegrey, abs(year),Gigalomania::Image::JUSTIFY_RIGHT);
	Gigalomania::Image *era = ( year < 0 ) ? game_g->icon_bc :
		shiny ? game_g->icon_ad_shiny : game_g->icon_ad;
	if( era != NULL )
		era->draw(cx+8, cy);
	else {
		Gigalomania::Image::write(cx+8, cy, game_g->letters_small, ( year < 0 ) ? "BC" : "AD", Gigalomania::Image::JUSTIFY_LEFT);
	}
    cy += l_h + 2;

	if( !game_g->isDemo() && game_g->getGameType() == GAMETYPE_ALLISLANDS ) {
		int n_suspended = game_g->getNSuspended();
        if( n_suspended > 0 )
		{
			sprintf(buffer, "Saved Men %d", n_suspended);
            Gigalomania::Image::writeMixedCase(cx, cy, game_g->letters_large, game_g->letters_small, game_g->numbers_white, buffer, Gigalomania::Image::JUSTIFY_CENTRE);
		}
	}

    /*cy += l_h + 2;
	cy += l_h + 2;
	if( choosemenPanel->getPage() == ChooseMenPanel::STATE_LOADGAME ) {
		Gigalomania::Image::write(cx, cy, game_g->letters_large, "LOAD", Gigalomania::Image::JUSTIFY_CENTRE, true);
	}
	else if( choosemenPanel->getPage() == ChooseMenPanel::STATE_SAVEGAME ) {
		Gigalomania::Image::write(cx, cy, game_g->letters_large, "SAVE", Gigalomania::Image::JUSTIFY_CENTRE, true);
	}
    cy += s_h + 2;*/

	if( choosemenPanel->getPage() == ChooseMenPanel::STATE_CHOOSEMEN ) {
		const int ydiff = s_h + 2;
		cy = 140 - 5 * ydiff; // update this if we change the number of lines!
		const int xpos = 80;
		Gigalomania::Image::writeMixedCase(xpos, cy, game_g->letters_large, game_g->letters_small, NULL, "Click on the icon below", Gigalomania::Image::JUSTIFY_CENTRE);
		cy += ydiff;
		Gigalomania::Image::writeMixedCase(xpos, cy, game_g->letters_large, game_g->letters_small, NULL, "to choose how many men", Gigalomania::Image::JUSTIFY_CENTRE);
		cy += ydiff;
		Gigalomania::Image::writeMixedCase(xpos, cy, game_g->letters_large, game_g->letters_small, NULL, "to play with", Gigalomania::Image::JUSTIFY_CENTRE);
		cy += ydiff;
		Gigalomania::Image::writeMixedCase(xpos, cy, game_g->letters_large, game_g->letters_small, NULL, "then click on the map", Gigalomania::Image::JUSTIFY_CENTRE);
		cy += ydiff;
		Gigalomania::Image::writeMixedCase(xpos, cy, game_g->letters_large, game_g->letters_small, NULL, "to the right", Gigalomania::Image::JUSTIFY_CENTRE);
		cy += ydiff;
	}

	game_g->getMap()->draw(cx - 8*map_width_c, off_y);

	this->choosemenPanel->draw();
	//this->choosemenPanel->drawPopups();

    this->screen_page->draw();
	//this->screen_page->drawPopups();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void PlaceMenGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

    bool done = false;
    if( !done && m_left && click && confirm_button_1 != NULL && confirm_button_1->mouseOver(m_x, m_y) ) {
        LOG("confirm yes clicked\n");
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
		this->requestConfirm();
    }
    else if( !done && m_left && click && confirm_button_2 != NULL && confirm_button_2->mouseOver(m_x, m_y) ) {
        LOG("confirm no clicked\n");
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
        this->closeConfirmWindow();
    }

    if( !done && m_left && click && this->choosemenPanel->getPage() == ChooseMenPanel::STATE_CHOOSEMEN && this->choosemenPanel->getNMen() > 0 ) {
		bool found = false;
		int map_x = -1;
		int map_y = -1;
		for(int y=0;y<map_height_c && !found;y++) {
			for(int x=0;x<map_width_c && !found;x++) {
				if( game_g->getMap()->isSectorAt(x, y) ) {
					ASSERT( this->map_panels[x][y] != NULL );
					if( this->map_panels[x][y]->mouseOver(m_x, m_y) ) {
						found = true;
						map_x = x;
						map_y = y;
					}
				}
			}
		}
		if( found ) {
			LOG("starting epoch %d island %s at %d, %d\n", game_g->getStartEpoch(), game_g->getMap()->getName(), map_x, map_y);
			this->setStartMapPos(map_x, map_y);
			return;
		}
	}

    if( !done )
        this->choosemenPanel->input(m_x, m_y, m_left, m_middle, m_right, click);
}

void PlaceMenGameState::requestQuit(bool force_quit) {
	if( force_quit ) {
		game_g->saveState();
	    game_g->getApplication()->setQuit();
	}
	else if( choosemenPanel->getPage() == ChooseMenPanel::STATE_CHOOSEMEN && !game_g->isStateChanged() ) {
		choosemenPanel->setPage(ChooseMenPanel::STATE_CHOOSEISLAND);
	}
	else {
		this->createQuitWindow();
	}
}

void PlaceMenGameState::requestConfirm() {
	if( confirm_window != NULL ) {
        this->closeConfirmWindow();
		if( confirm_type == CONFIRMTYPE_NEWGAME ) {
			game_g->setGameStateID(GAMESTATEID_CHOOSEGAMETYPE);
		}
		else if( confirm_type == CONFIRMTYPE_QUITGAME ) {
	        game_g->getApplication()->setQuit();
		}
		else {
			T_ASSERT(false);
		}
	}
}

void PlaceMenGameState::setStartMapPos(int start_map_x, int start_map_y ) {
	this->start_map_x = start_map_x;
	this->start_map_y = start_map_y;
	if( !game_g->isDemo() ) {
		game_g->players[client_player]->setNMenForThisIsland( this->choosemenPanel->getNMen() );
		ASSERT( game_g->players[client_player]->getNMenForThisIsland() <= game_g->getMenAvailable() );
		LOG("human is player %d, starting with %d men\n", client_player, game_g->players[client_player]->getNMenForThisIsland());
	}
	else {
		LOG("DEMO mode\n");
		//placeTower(map_x, map_y, 0);
	}
	game_g->placeTower();
}

void PlaceMenGameState::requestNewGame() {
	if( confirm_window != NULL ) {
		this->closeConfirmWindow();
	}
	confirm_window = new PanelPage(0, 0, default_width_c, default_height_c);
	confirm_type = CONFIRMTYPE_NEWGAME;
	confirm_window->setBackground(0, 0, 0, 200);
	const int offset_x_c = 120, offset_y_c = 120;
	Button *text_button = new Button(offset_x_c, offset_y_c, "NEW GAME?", game_g->letters_large);
	confirm_window->add(text_button);
	confirm_button_1 = new Button(offset_x_c, offset_y_c+16, "YES", game_g->letters_large);
	confirm_window->add(confirm_button_1);
	confirm_button_2 = new Button(offset_x_c+32, offset_y_c+16, "NO", game_g->letters_large);
	confirm_window->add(confirm_button_2);
	this->screen_page->add(confirm_window);
}


PlayingGameState::PlayingGameState(int client_player) : GameState(client_player) {
	this->current_sector = NULL;
	this->flag_frame_step = 0;
	this->defenders_last_time_update = 0;
	this->soldier_last_time_moved_x = -1;
	this->soldier_last_time_moved_y = -1;
	this->cannon_last_time_moved_x = -1;
	this->cannon_last_time_moved_y = -1;
	this->air_last_time_moved = -1;
	this->soldiers_last_time_turned = 0;

	this->text_effect = NULL;
	this->speed_button = NULL;
	for(int i=0;i<n_players_c;i++) {
		this->shield_buttons[i] = NULL;
		this->shield_number_panels[i] = NULL;
	}
	this->shield_blank_button = NULL;
	this->land_panel = NULL;
	this->pause_button = NULL;
	this->quit_button = NULL;
	this->tutorial_next_button = NULL;
	this->gamePanel = NULL;
	this->selected_army = NULL;
	this->map_display = MAPDISPLAY_MAP;
	//this->map_display = MAPDISPLAY_UNITS;
	this->player_asking_alliance = -1;
	/*for(i=0;i<n_players_c;i++)
	this->n_soldiers[i] = 0;*/
	for(int i=0;i<n_players_c;i++)
		for(int j=0;j<=n_epochs_c;j++)
			this->n_deaths[i][j] = 0;
	//this->refreshSoldiers(false);
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			map_panels[x][y] = NULL;
		}
	}
	alliance_yes = NULL;
	alliance_no = NULL;

	game_g->setTimeRate(client_player == PLAYER_DEMO ? 5 : 1);
}

PlayingGameState::~PlayingGameState() {
	LOG("~PlayingGameState()\n");
	if( game_g->getMap() != NULL ) { // check needed if the current map failed to load, and we're resuming from saved state with that island
		game_g->getMap()->freeSectors(); // needed to avoid crash for tests, and exiting to desktop
	}
	game_g->s_biplane->fadeOut(500);
	game_g->s_jetplane->fadeOut(500);
	game_g->s_spaceship->fadeOut(500);
	for(size_t i=0;i<effects.size();i++) {
		TimedEffect *effect = effects.at(i);
		delete effect;
	}
	for(size_t i=0;i<ammo_effects.size();i++) {
		TimedEffect *effect = ammo_effects.at(i);
		delete effect;
	}
	if( text_effect != NULL ) {
		delete text_effect;
	}
	if( this->gamePanel )
		delete gamePanel;
	LOG("~PlayingGameState() done\n");
}

void PlayingGameState::createQuitWindow() {
    if( confirm_window == NULL && !game_g->isStateChanged() ) {
		confirm_type = CONFIRMTYPE_QUITGAME;
		confirm_window = new PanelPage(0, 0, default_width_c, default_height_c);
		confirm_window->setBackground(0, 0, 0, 200);
		const int offset_x_c = 80, offset_y_c = 100;
#if defined(__ANDROID__)
		confirm_button_1 = NULL; // if user wants to exit to homescreen, they can just press the Home button
#else
		confirm_button_1 = new Button(offset_x_c, offset_y_c, "SAVE GAME AND QUIT TO DESKTOP", game_g->letters_large);
		confirm_window->add(confirm_button_1);
#endif
		confirm_button_2 = new Button(offset_x_c, offset_y_c+16, "EXIT BATTLE", game_g->letters_large);
		confirm_window->add(confirm_button_2);
		confirm_button_3 = new Button(offset_x_c, offset_y_c+32, "CANCEL", game_g->letters_large);
		confirm_window->add(confirm_button_3);
		screen_page->add(confirm_window);
	}
	else if( confirm_window != NULL && !game_g->isStateChanged() ) {
		closeConfirmWindow();
	}
}

bool PlayingGameState::readSectorsProcessLine(Map *map, char *line, bool *done_header, int *sec_x, int *sec_y) {
	bool ok = true;
	line[ strlen(line) - 1 ] = '\0'; // trim new line
	line[ strlen(line) - 1 ] = '\0'; // trim carriage return
	if( !(*done_header) ) {
		if( line[0] != '#' ) {
			LOG("expected first character to be '#'\n");
			ok = false;
			return ok;
		}
		// ignore rest of header
		*done_header = true;
	}
	else {
		char *line_ptr = line;
		while( *line_ptr == ' ' || *line_ptr == '\t' ) // avoid initial whitespace
			line_ptr++;
		char *comment = strchr(line_ptr, '#');
		if( comment != NULL ) { // trim comments
			*comment = '\0';
		}
		char *ptr = strtok(line_ptr, " ");
		if( ptr == NULL ) {
			// this line may be a comment
		}
		else if( strcmp(ptr, "SECTOR") == 0 ) {
			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find sec_x\n");
				ok = false;
				return ok;
			}
			*sec_x = atoi(ptr);
			if( *sec_x < 0 || *sec_x >= map_width_c ) {
				LOG("invalid map x %d\n", *sec_x);
				ok = false;
				return ok;
			}

			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find sec_y\n");
				ok = false;
				return ok;
			}
			*sec_y = atoi(ptr);
			if( *sec_y < 0 || *sec_y >= map_height_c ) {
				LOG("invalid map y %d\n", *sec_y);
				ok = false;
				return ok;
			}
		}
		else if( strcmp(ptr, "ELEMENT") == 0 ) {
			if( *sec_x == -1 || *sec_y == -1 ) {
				LOG("sector not defined\n");
				ok = false;
				return ok;
			}
			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find element name\n");
				ok = false;
				return ok;
			}
			/*char elementname[MAX_LINE+1] = "";
			strcpy(elementname, ptr);*/
			string elementname = ptr;

			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find n_elements\n");
				ok = false;
				return ok;
			}
			int n_elements = atoi(ptr);

			Id element = UNDEFINED;
			for(int i=0;i<N_ID;i++) {
				if( strcmp( game_g->elements[i]->getName(), elementname.c_str() ) == 0 ) {
					element = (Id)i;
				}
			}
			if( element == UNDEFINED ) {
				LOG("unknown element: %s\n", elementname.c_str());
				ok = false;
				return ok;
			}

			game_g->getMap()->getSector(*sec_x, *sec_y)->setElements(element, n_elements);
		}
		else {
			LOG("unknown word: %s\n", ptr);
			ok = false;
			return ok;
		}
	}
	return ok;
}

bool PlayingGameState::readSectors(Map *map) {
	bool ok = true;
	const int MAX_LINE = 4096;
	char line[MAX_LINE+1] = "";
	int sec_x = -1, sec_y = -1;
	bool done_header = false;

    char fullname[4096] = "";
	sprintf(fullname, "%s/%s", maps_dirname.c_str(), game_g->getMap()->getFilename());
	// open in binary mode, so that we parse files in an OS-independent manner
	// (otherwise, Windows will parse "\r\n" as being "\n", but Linux will still read it as "\n")
	//FILE *file = fopen(fullname, "rb");
	SDL_RWops *file = SDL_RWFromFile(fullname, "rb");
#ifdef DATADIR
	if( file == NULL ) {
		LOG("searching in /usr/share/gigalomania/ for islands folder\n");
		sprintf(fullname, "%s/%s", alt_maps_dirname.c_str(), game_g->getMap()->getFilename());
		file = SDL_RWFromFile(fullname, "rb");
	}
#endif
	if( file == NULL ) {
		LOG("failed to open file: %s\n", fullname);
		//perror("perror returns: ");
		return false;
	}
	char buffer[MAX_LINE+1] = "";
	int buffer_offset = 0;
	bool reached_end = false;
	int newline_index = 0;
	while( ok ) {
		bool done = game_g->readLineFromRWOps(ok, file, buffer, line, MAX_LINE, buffer_offset, newline_index, reached_end);
		if( !ok )  {
			LOG("failed to read line\n");
		}
		else if( done ) {
			break;
		}
		else {
			ok = readSectorsProcessLine(map, line, &done_header, &sec_x, &sec_y);
		}
	}
	file->close(file);

	return ok;
}

void PlayingGameState::createSectors(int x, int y, int n_men) {
	LOG("PlayingGameState::createSectors(%d, %d, %d)\n", x, y, n_men);

	game_g->getMap()->createSectors(this, game_g->getStartEpoch());
	Sector *sector = game_g->getMap()->getSector(x, y);
	current_sector = sector;
	if( !game_g->isDemo() ) {
		sector->createTower(client_player, n_men);
	}
	//current_sector->createTower(human_player, 10);

	//current_sector->getArmy(enemy_player)->soldiers[10] = 0;
	//game_g->getMap()->sectors[2][2]->getArmy(enemy_player)->soldiers[0] = 10;

	//Sector *enemy_sector = game_g->getMap()->sectors[1][2];

	for(int i=0;i<n_players_c;i++) {
		if( i == client_player || game_g->players[i] == NULL )
			continue;
		int ex = 0, ey = 0;
		while( true ) {
			game_g->getMap()->findRandomSector(&ex, &ey);
			ASSERT( game_g->getMap()->isSectorAt(ex, ey) );
			if( game_g->getMap()->getSector(ex, ey)->getPlayer() == -1 && !game_g->getMap()->isReserved(ex, ey) )
				break;
		}
		//Sector *enemy_sector = game_g->getMap()->sectors[ex][ey];
		Sector *enemy_sector = game_g->getMap()->getSector(ex, ey);
		//enemy_sector->getArmy(enemy_player)->soldiers[10] = 30;
		//enemy_sector->createTower(enemy_player, 12);
		if( game_g->getStartEpoch() == end_epoch_c ) {
			//game_g->players[i]->n_men_for_this_island = n_suspended[i];
			game_g->players[i]->setNMenForThisIsland(100);
		}
		else {
			game_g->players[i]->setNMenForThisIsland(20 + 5*game_g->getStartEpoch());
			// total: 360*3 + 65 = 1145 men
		}
		LOG("Enemy %d created at %d , %d\n", i, ex, ey);
		enemy_sector->createTower(i, game_g->players[i]->getNMenForThisIsland());
		//enemy_sector->createTower(enemy_player, 20);
		//enemy_sector->createTower(enemy_player, 200);
	}

	if( !readSectors(game_g->getMap()) ) {
		LOG("failed to read map sector info!\n");
		ASSERT(false);
	}
}

GamePanel *PlayingGameState::getGamePanel() {
	return this->gamePanel;
}

/*Sector *PlayingGameState::getCurrentSector() {
	return current_sector;
}*/

const Sector *PlayingGameState::getCurrentSector() const {
	return current_sector;
}

bool PlayingGameState::viewingActiveClientSector() const {
	return this->getCurrentSector()->getActivePlayer() == client_player;
}

bool PlayingGameState::viewingAnyClientSector() const {
	// includes shutdown sectors
	return this->getCurrentSector()->getPlayer() == client_player;
}

bool PlayingGameState::openPitMine() {
	if( current_sector->getActivePlayer() != -1 && current_sector->getBuilding(BUILDING_MINE) == NULL && current_sector->getEpoch() >= 1 ) {
		for(int i=0;i<N_ID;i++) {
			if( current_sector->anyElements((Id)i) ) {
				Element *element = game_g->elements[i];
				if( element->getType() == Element::OPENPITMINE )
					return true;
			}
		}
	}
	return false;
}

void PlayingGameState::setupMapGUI() {
	if( alliance_yes != NULL ) {
		delete alliance_yes;
		alliance_yes = NULL;
	}
	if( alliance_no != NULL ) {
		delete alliance_no;
		alliance_no = NULL;
	}
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			if( map_panels[x][y] != NULL ) {
				delete map_panels[x][y];
				map_panels[x][y] = NULL;
			}
		}
	}
	if( this->player_asking_alliance != -1 ) {
		alliance_yes = new Button(24, 82, "YES", game_g->letters_large);
		alliance_yes->setInfoLMB("join the alliance");
		screen_page->add(alliance_yes);
		alliance_no = new Button(56, 82, "NO", game_g->letters_large);
		alliance_no->setInfoLMB("refuse the alliance");
		screen_page->add(alliance_no);
	}
	else if( this->map_display == MAPDISPLAY_MAP ) {
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				if( game_g->getMap()->isSectorAt(x, y) ) {
					int map_x = offset_map_x_c + 16 * x;
					int map_y = offset_map_y_c + 16 * y;
					PanelPage *panel = new PanelPage(map_x, map_y, 16, 16);
					panel->setTolerance(0); // since map sectors are aligned, better for touchscreens not to use the "tolerance"
					panel->setInfoLMB("view this sector");
					screen_page->add(panel);
					//game_g->getMap()->panels[x][y] = panel;
					map_panels[x][y] = panel;
					char buffer[256] = "";
					sprintf(buffer, "map_%d_%d", x, y);
					map_panels[x][y]->setId(buffer);
				}
			}
		}
	}
}

void PlayingGameState::reset() {
    //LOG("PlayingGameState::reset()\n");
	if( current_sector == NULL )
		return;

	this->screen_page->free(true);
	alliance_yes = NULL;
	alliance_no = NULL;
	tutorial_next_button = NULL;
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			map_panels[x][y] = NULL;
		}
	}
	confirm_type = CONFIRMTYPE_UNKNOWN;
	confirm_window = NULL;
	confirm_button_1 = NULL;
	confirm_button_2 = NULL;
	confirm_button_3 = NULL;

	if( this->gamePanel != NULL ) {
		delete this->gamePanel;
		this->gamePanel = NULL;
	}

	this->land_panel = new PanelPage(offset_land_x_c, offset_land_y_c, default_width_c - offset_land_x_c, default_height_c - offset_land_y_c - quit_button_offset_c);
	this->land_panel->setId("land_panel");
	screen_page->add(this->land_panel);

	// setup screen_page buttons
	this->setupMapGUI();

	if( !game_g->isDemo() ) {
		speed_button = new ImageButton(offset_map_x_c + 16 * map_width_c + 4, 4, game_g->icon_speeds[game_g->getTimeRate()-1]);
		speed_button->setId("speed_button");
		if( game_g->isOneMouseButton() ) {
			speed_button->setInfoLMB("cycle through different time rates");
		}
		else {
			speed_button->setInfoLMB("decrease the rate of time");
			speed_button->setInfoRMB("increase the rate of time");
		}
		screen_page->add(speed_button);

		//if( mobile_ui )
		{
			pause_button = new Button(default_width_c - 80, default_height_c - quit_button_offset_c, "PAUSE", game_g->letters_large);
			pause_button->setId("pause_button");
			screen_page->add(pause_button);
			quit_button = new Button(default_width_c - 32, default_height_c - quit_button_offset_c, "QUIT", game_g->letters_large);
			quit_button->setId("quit_button");
			screen_page->add(quit_button);
		}
	}

	for(int i=0;i<n_players_c;i++) {
		shield_buttons[i] = NULL;
		shield_number_panels[i] = NULL;
	}
	this->shield_blank_button = NULL;

	resetShieldButtons();

	for(int i=0;i<N_BUILDINGS;i++) {
		Building *building = current_sector->getBuilding((Type)i);
		if( building != NULL ) {
			addBuilding(building);
		}
	}

	// must be done after creating shield_number_panels
	this->refreshSoldiers(false);

	/*if( smokeParticleSystem != NULL ) {
		delete smokeParticleSystem;
	}
	Building *building_factory = current_sector->getBuilding(BUILDING_FACTORY);
	//Building *building_factory = current_sector->getBuilding(BUILDING_TOWER);
	if( building_factory != NULL ) {
		//smokeParticleSystem = new SmokeParticleSystem(smoke_image, offset_land_x_c + building_factory->getX(), offset_land_y_c + building_factory->getY());
		smokeParticleSystem = new SmokeParticleSystem(smoke_image, offset_land_x_c + building_factory->getX() + 20, offset_land_y_c + building_factory->getY());
	}*/
	/*if( smokeParticleSystem == NULL ) {
		smokeParticleSystem = new SmokeParticleSystem(smoke_image);
	}*/

	this->gamePanel = new GamePanel(this, this->client_player);
	// must call setup last, in case it recalls member functions of PlayingGameState, that requires the buttons to have been initialised
	this->gamePanel->setup();

	if( game_g->getTutorial() != NULL ) {
		const TutorialCard *card = game_g->getTutorial()->getCard();
		if( card != NULL ) {
			card->setGUI(this);
		}
		else {
			GUIHandler::resetGUI(this);
		}
	}
	if( LOGGING ) {
		current_sector->printDebugInfo();
	}
}

void PlayingGameState::resetShieldButtons() {
	bool done_shield[n_players_c];
	for(int i=0;i<n_players_c;i++)
		done_shield[i] = false;
	for(int i=0;i<n_players_c;i++) {
		if( shield_buttons[i] != NULL ) {
			screen_page->remove(shield_buttons[i]);
			delete shield_buttons[i];
			shield_buttons[i] = NULL;
		}
		if( shield_number_panels[i] != NULL ) {
			screen_page->remove(shield_number_panels[i]);
			delete shield_number_panels[i];
			shield_number_panels[i] = NULL;
		}
	}
	if( shield_blank_button != NULL ) {
		screen_page->remove(shield_blank_button);
		delete shield_blank_button;
		shield_blank_button = NULL;
	}
	int n_sides = 0;
	for(int i=0;i<n_players_c;i++) {
		if( !done_shield[i] && game_g->players[i] != NULL && !game_g->players[i]->isDead() ) {
			bool allied[n_players_c];
			for(int j=0;j<n_players_c;j++)
				allied[j] = false;
			allied[i] = true;
			done_shield[i] = true;
			for(int j=i+1;j<n_players_c;j++) {
				if( Player::isAlliance(i, j) ) {
					ASSERT( game_g->players[j] != NULL );
					ASSERT( !game_g->players[j]->isDead() );
					allied[j] = true;
					done_shield[j] = true;
				}
			}
			n_sides++;
			shield_buttons[i] = new ImageButton(offset_map_x_c + 16 * map_width_c + 4, offset_map_y_c + shield_step_y_c * i + 8, game_g->playershields[ Player::getShieldIndex(allied) ]);
			screen_page->add(shield_buttons[i]);
			//shield_number_panels[i] = new PanelPage(offset_map_x_c + 16 * map_width_c + 4 + 16, offset_map_y_c + shield_step_y_c * i + 8, 20, 10);
			shield_number_panels[i] = new PanelPage(offset_map_x_c + 16 * map_width_c + 4 + 16, offset_map_y_c + shield_step_y_c * i + 8, 20, shield_step_y_c);
			screen_page->add(shield_number_panels[i]);
		}
	}
	if( !game_g->isDemo() && n_sides > 2 ) {
		for(int i=0;i<n_players_c;i++) {
			if( shield_buttons[i] != NULL && i != client_player && !Player::isAlliance(i, client_player) ) {
				shield_buttons[i]->setInfoLMB("make an alliance");
			}
		}
	}
	bool any_alliances = false;
	for(int i=0;i<n_players_c && !any_alliances && !game_g->isDemo();i++) {
		if( i != client_player && Player::isAlliance(i, client_player) ) {
			any_alliances = true;
			ASSERT( game_g->players[i] != NULL );
			ASSERT( !game_g->players[i]->isDead() );
		}
	}
	if( any_alliances ) {
		shield_blank_button = new ImageButton(offset_map_x_c + 16 * map_width_c + 4, offset_map_y_c + 4*shield_step_y_c + 8, game_g->playershields[0]);
		shield_blank_button->setInfoLMB("break current alliance");
		screen_page->add(shield_blank_button);
	}

	refreshShieldNumberPanels();
}

void PlayingGameState::addBuilding(Building *building) {
	for(int j=0;j<building->getNTurrets();j++) {
		screen_page->add(building->getTurretButton(j));
	}
	if( building->getBuildingButton() != NULL )
		screen_page->add(building->getBuildingButton());
}

void PlayingGameState::setFlashingSquare(int xpos,int ypos) {
	if( this->player_asking_alliance == -1 && map_display == MAPDISPLAY_MAP ) {
		FlashingSquare *square = new FlashingSquare(xpos, ypos);
		this->effects.push_back(square);
	}
};

void GameState::fadeScreen(bool out, int delay, void (*func_finish)()) {
    if( fade != NULL )
        {delete fade;}
	if( game_g->isTesting() ) {
		if( func_finish != NULL ) {
			func_finish();
		}
	}
	else {
	    fade = new FadeEffect(false, out, delay, func_finish);
	}
}

void GameState::whiteFlash() {
	//ASSERT( whitefade == NULL );
    if( whitefade != NULL )
        {delete whitefade;}
	if( !game_g->isTesting() ) {
	    whitefade = new FadeEffect(true, false, 0, NULL);
	}
}

void PlayingGameState::getFlagOffset(int *offset_x, int *offset_y, int epoch) const {
	if( game_g->isUsingOldGfx() ) {
		*offset_x = 22;
		*offset_y = 6;
	}
	else if( epoch == 6  || epoch ==7 ) {
		// building towers have flat roof
		*offset_x = 22;
		*offset_y = 6;
	}
	else {
		*offset_x = 21;
		*offset_y = 2;
	}
}

void PlayingGameState::draw() {
	game_g->getScreen()->clear(); // SDL on Android and OS X at least require screen to be cleared (otherwise we get corrupt regions outside of the main area)
	game_g->background_stars->draw(0, 0);

	bool no_armies = true;
	for(int i=0;i<n_players_c && no_armies;i++) {
		const Army *army = current_sector->getArmy(i);
		if( army->getTotal() > 0 )  {
			no_armies = false;
		}
	}
	if( no_armies && this->map_display == MAPDISPLAY_UNITS ) {
		this->map_display = MAPDISPLAY_MAP;
		this->reset();
	}

	if( this->player_asking_alliance != -1 ) {
		// ask alliance
		if( game_g->player_heads_alliance[player_asking_alliance] != NULL ) {
			game_g->player_heads_alliance[player_asking_alliance]->draw(offset_map_x_c + 24, offset_map_y_c + 24);
		}
		stringstream str;
		str << PlayerType::getName((PlayerType::PlayerTypeID)player_asking_alliance);
		Gigalomania::Image::write(offset_map_x_c + 8, offset_map_y_c + 0, game_g->letters_small, str.str().c_str(), Gigalomania::Image::JUSTIFY_LEFT);
		str.str("asks for an");
		Gigalomania::Image::write(offset_map_x_c + 8, offset_map_y_c + 8, game_g->letters_small, str.str().c_str(), Gigalomania::Image::JUSTIFY_LEFT);
		str.str("alliance");
		Gigalomania::Image::write(offset_map_x_c + 8, offset_map_y_c + 16, game_g->letters_small, str.str().c_str(), Gigalomania::Image::JUSTIFY_LEFT);
	}
	else if( this->map_display == MAPDISPLAY_MAP ) {
		// map

		game_g->getMap()->draw(offset_map_x_c, offset_map_y_c);
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				if( game_g->getMap()->getSector(x, y) != NULL ) {
					int map_x = offset_map_x_c + 16 * x;
					int map_y = offset_map_y_c + 16 * y;
					//map_sq[15]->draw(map_x, map_y, true);
					if( game_g->getMap()->getSector(x, y)->getPlayer() != -1 ) {
						game_g->icon_towers[ game_g->getMap()->getSector(x, y)->getPlayer() ]->draw(map_x + 5, map_y + 5);
					}
					else if( game_g->getMap()->getSector(x, y)->isNuked() ) {
						game_g->icon_nuke_hole->draw(map_x + 4, map_y + 4);
					}
					for(int i=0;i<n_players_c;i++) {
						Army *army = game_g->getMap()->getSector(x, y)->getArmy(i);
						int n_army = army->getTotal();
						if( n_army > 0 ) {
							int off_step = 5;
							int off_step_x = ( i == 0 || i == 2 ) ? -off_step : off_step;
							int off_step_y = ( i == 0 || i == 1 ) ? -off_step : off_step;
							game_g->icon_armies[i]->draw(map_x + 6 + off_step_x, map_y + 6 + off_step_y);
						}
					}
				}
			}
		}
		int map_x = offset_map_x_c + 16 * current_sector->getXPos();
		int map_y = offset_map_y_c + 16 * current_sector->getYPos();
		game_g->mapsquare->draw(map_x, map_y);
	}
	else if( this->map_display == MAPDISPLAY_UNITS ) {
		// unit stats
		const int gap = 18;
		const int extra = 0;
		for(int i=0;i<=game_g->getNSubEpochs();i++) {
			Gigalomania::Image *image = (i==0) ? game_g->unarmed_man : game_g->numbered_weapons[game_g->getStartEpoch() + i - 1];
			//Gigalomania::Image *image = (i==0) ? men[game_g->getStartEpoch()] : numbered_weapons[game_g->getStartEpoch() + i - 1];
			image->draw(offset_map_x_c + gap * i + extra, offset_map_y_c + 2 - 16 + 8);
		}
		for(int i=0;i<n_players_c;i++) {
			if( shield_buttons[i] == NULL ) {
				continue;
			}
			int off = 0;
			for(int j=i;j<n_players_c;j++) {
				if( j == i || Player::isAlliance(i, j) ) {
					const Army *army = current_sector->getArmy(j);
					if( army->getTotal() > 0 ) {
						for(int k=0;k<=game_g->getNSubEpochs();k++) {
							int idx = (k==0) ? 10 : game_g->getStartEpoch() + k - 1;
							int n_men = army->getSoldiers(idx);
							if( n_men > 0 ) {
								//Gigalomania::Image::writeNumbers(offset_map_x_c + 16 * k + 4, offset_map_y_c + 2 + 16 * i + 8 * off + 8, game_g->numbers_small[j], n_men, Gigalomania::Image::JUSTIFY_LEFT, true);
								Gigalomania::Image::writeNumbers(offset_map_x_c + gap * k + extra, offset_map_y_c + 2 + 16 * i + 8 * off + 8, game_g->numbers_small[j], n_men, Gigalomania::Image::JUSTIFY_LEFT);
							}
						}
						off++;
					}
				}
			}
		}
	}

	// land area
	game_g->land[game_g->getMap()->getColour()]->draw(offset_land_x_c, offset_land_y_c);

	// trees etc (not at front)
	for(int i=0;i<current_sector->getNFeatures();i++) {
		const Feature *feature = current_sector->getFeature(i);
		if( !feature->isAtFront() ) {
			feature->draw();
		}
	}

	if( current_sector->getActivePlayer() != -1 )
	{
		if( openPitMine() )
			game_g->icon_openpitmine->draw(offset_land_x_c + offset_openpitmine_x_c, offset_land_y_c + offset_openpitmine_y_c);

		bool rotate_defenders = false;
		if( game_g->getGameTime() - defenders_last_time_update > defenders_ticks_per_update_c ) {
			rotate_defenders = true;
			defenders_last_time_update = game_g->getGameTime();
		}

		for(int i=0;i<N_BUILDINGS;i++) {
			Building *building = current_sector->getBuilding((Type)i);
			if( building == NULL )
				continue;

			// draw building
			Gigalomania::Image **images = building->getImages();
			images[ current_sector->getBuildingEpoch() ]->draw(offset_land_x_c + building->getX(), offset_land_y_c + building->getY());

			if( rotate_defenders )
				building->rotateDefenders();

			// draw defenders
			for(int j=0;j<building->getNTurrets();j++) {
				// uncomment to draw turrent button regions:
				/*{
					PanelPage *button = building->getTurretButton(j);
					game_g->getScreen()->fillRectWithAlpha(game_g->getScaleWidth()*button->getLeft(), game_g->getScaleHeight()*button->getTop(), game_g->getScaleWidth()*button->getWidth(), game_g->getScaleHeight()*button->getHeight(), 255, 0, 0, 127);
				}*/

				if( building->getTurretMan(j) != -1 ) {
					Gigalomania::Image *image = NULL;
					int defender_epoch = building->getTurretMan(j);
					if( defender_epoch == nuclear_epoch_c ) {
						image = game_g->nuke_defences[current_sector->getPlayer()];
					}
					else {
						image = game_g->defenders[current_sector->getPlayer()][defender_epoch][ building->getTurretManFrame(j) % game_g->n_defender_frames[defender_epoch] ];
					}
					image->draw(building->getTurretButton(j)->getLeft(), building->getTurretButton(j)->getTop() - 4);
				}
			}

			if( i == BUILDING_TOWER ) {
				int offset_x = 0, offset_y = 0;
				getFlagOffset(&offset_x, &offset_y, current_sector->getBuildingEpoch());
				game_g->flags[ current_sector->getPlayer() ][game_g->getFrameCounter() % n_flag_frames_c]->draw(offset_land_x_c + building->getX() + offset_x, offset_land_y_c + building->getY() + offset_y);
			}

			int width = game_g->building_health->getScaledWidth();
			int health = building->getHealth();
			int max_health = building->getMaxHealth();
			int offx = offset_land_x_c + building->getX() + 4;
			int offy = offset_land_y_c + building->getY() + images[ current_sector->getBuildingEpoch() ]->getScaledHeight() + 2;
			game_g->building_health->draw(offx, offy, (int)((width*health)/(float)max_health), game_g->building_health->getScaledHeight());
		}
	}
	else if( current_sector->getPlayer() != -1 ) {
		ASSERT( current_sector->isShutdown() );
		Building *building = current_sector->getBuilding(BUILDING_TOWER);
		ASSERT( building != NULL );
		Gigalomania::Image **images = building->getImages();
		images[ current_sector->getBuildingEpoch() ]->draw(offset_land_x_c + building->getX(), offset_land_y_c + building->getY());
		int offset_x = 0, offset_y = 0;
		getFlagOffset(&offset_x, &offset_y, current_sector->getBuildingEpoch());
		game_g->flags[ current_sector->getPlayer() ][game_g->getFrameCounter() % n_flag_frames_c]->draw(offset_land_x_c + building->getX() + offset_x, offset_land_y_c + building->getY() + offset_y);
	}

	//Vector soldier_list(n_players_c * 250);
	int n_total_soldiers = 0;
	for(int i=0;i<n_players_c;i++) {
		n_total_soldiers += soldiers[i].size();
	}
	Soldier **soldier_list = new Soldier *[n_total_soldiers];
	for(int i=0,c=0;i<n_players_c;i++) {
		//for(int j=0;j<n_soldiers[i];j++) {
		for(size_t j=0;j<soldiers[i].size();j++) {
			//Soldier *soldier = soldiers[i][j];
			//Soldier *soldier = (Soldier *)soldiers[i]->get(j);
			Soldier *soldier = soldiers[i].at(j);
			//soldier_list.add(soldier);
			soldier_list[c++] = soldier;
		}
	}
	//Soldier::sortSoldiers((Soldier **)soldier_list.getData(), soldier_list.size());
	Soldier::sortSoldiers(soldier_list, n_total_soldiers);
	// draw land units
	/*for(int i=0;i<soldier_list.size();i++) {
	Soldier *soldier = (Soldier *)soldier_list.elementAt(i);*/
	for(int i=0;i<n_total_soldiers;i++) {
		Soldier *soldier = soldier_list[i];
		ASSERT(soldier->epoch != nuclear_epoch_c);
		if( !isAirUnit(soldier->epoch) ) {
			//int frame = soldier->dir * 4 + ( game_g->getFrameCounter() % 3 );
			//Gigalomania::Image *image = attackers_walking[soldier->player][soldier->epoch][frame];
			int n_frames = game_g->n_attacker_frames[soldier->epoch][soldier->dir];
			Gigalomania::Image *image = game_g->attackers_walking[soldier->player][soldier->epoch][soldier->dir][game_g->getFrameCounter() % n_frames];
			image->draw(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos);
		}
	}

	// trees etc (at front)
	for(int i=0;i<current_sector->getNFeatures();i++) {
		const Feature *feature = current_sector->getFeature(i);
		if( feature->isAtFront() ) {
			feature->draw();
		}
	}

	for(int i=effects.size()-1;i>=0;i--) {
		TimedEffect *effect = effects.at(i);
		if( effect->render() ) {
			effects.erase(effects.begin() + i);
			delete effect;
		}
	}
	for(int i=ammo_effects.size()-1;i>=0;i--) {
		TimedEffect *effect = ammo_effects.at(i);
		if( effect->render() ) {
			ammo_effects.erase(ammo_effects.begin() + i);
			delete effect;
		}
	}

	// draw air units
	/*for(int i=0;i<soldier_list.size();i++) {
	Soldier *soldier = (Soldier *)soldier_list.elementAt(i);*/
	for(int i=0;i<n_total_soldiers;i++) {
		Soldier *soldier = soldier_list[i];
		ASSERT(soldier->epoch != nuclear_epoch_c);
		if( isAirUnit(soldier->epoch) ) {
			Gigalomania::Image *image = NULL;
			if( soldier->epoch == 6 || soldier->epoch == 7 ) {
				image = game_g->planes[soldier->player][soldier->epoch];
			}
			else if( soldier->epoch == 9 ) {
				int frame = game_g->getFrameCounter() % 3;
				image = game_g->saucers[soldier->player][frame];
			}
			ASSERT(image != NULL);
			image->draw(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos);
			if( soldier->epoch == 7 ) {
				if( current_sector->getJetParticleSystem() != NULL ) {
					current_sector->getJetParticleSystem()->draw(offset_land_x_c + soldier->xpos + 17, offset_land_y_c + soldier->ypos + 17);
				}
			}
		}
	}
	delete [] soldier_list;

	// nuke
	int nuke_time = -1;
	int nuke_by_player = current_sector->beingNuked(&nuke_time);
	if( nuke_by_player != -1 ) {
		int xpos = -1, ypos = -1;
		current_sector->getNukePos(&xpos, &ypos);
		game_g->nukes[nuke_by_player][1]->draw(xpos, ypos);
		if( current_sector->getNukeParticleSystem() != NULL ) {
			current_sector->getNukeParticleSystem()->draw(xpos + 23 - 4, ypos + 2 - 4);
		}
	}
	// nuke defence
	int nuke_defence_time = -1;
	int nuke_defence_x = 0;
	int nuke_defence_y = 0;
	if( current_sector->hasNuclearDefenceAnimation(&nuke_defence_time, &nuke_defence_x, &nuke_defence_y) ) {
		ASSERT( nuke_defence_time != -1 );
		float alpha = ((float)( game_g->getGameTime() - nuke_defence_time )) / (float)nuke_delay_c;
		ASSERT( alpha >= 0.0 );
		if( alpha > 1.0 )
			alpha = 1.0;
		int ey = nuke_defence_y - 200;
		int ypos = (int)(alpha * ey + (1.0 - alpha) * nuke_defence_y);
		game_g->nukes[current_sector->getPlayer()][0]->draw(nuke_defence_x, ypos);
		if( current_sector->getNukeDefenceParticleSystem() != NULL ) {
			current_sector->getNukeDefenceParticleSystem()->draw(nuke_defence_x + 4 - 4, ypos + 31 - 4);
		}
	}

	// playershields etc
	for(int i=0;i<n_players_c;i++) {
		if( game_g->players[i] != NULL && !game_g->players[i]->isDead() ) {
			//ASSERT( shield_buttons[i] != NULL );
			if( shield_buttons[i] == NULL ) {
				continue;
			}
			shield_number_panels[i]->setVisible(false);
			/*int n_allied = 1;
			for(j=i+1;j<n_players_c;j++) {
			if( Player::isAlliance(i, j) ) {
			n_allied++;
			}
			}*/
			//playershields[ game_g->players[i]->getShieldIndex()  ]->draw(offset_map_x_c + 16 * map_width_c + 4, offset_map_y_c + shield_step_y_c * i + 8, true);
			int off = 0;
			for(int j=i;j<n_players_c;j++) {
				if( j == i || Player::isAlliance(i, j) ) {
					const Army *army = current_sector->getArmy(j);
					int n_army = army->getTotal();
					if( n_army > 0 ) {
						shield_number_panels[i]->setVisible(true);
						//Gigalomania::Image::writeNumbers(offset_map_x_c + 16 * map_width_c + 20, offset_map_y_c + 2 + shield_step_y_c * i + 8, game_g->numbers_small[i], n_army, Gigalomania::Image::JUSTIFY_LEFT, true);
						Gigalomania::Image::writeNumbers(offset_map_x_c + 16 * map_width_c + 20, offset_map_y_c + 2 + shield_step_y_c * i + 8 * off + 8, game_g->numbers_small[j], n_army, Gigalomania::Image::JUSTIFY_LEFT);
						off++;
					}
				}
			}
		}
	}

	// panel
	if( game_g->getTutorial() != NULL ) {
		const TutorialCard *card = game_g->getTutorial()->getCard();
		if( card != NULL ) {
			const unsigned char tutorial_alpha_c = 127;
			int n_lines = 0, max_wid = 0;
			int s_w = game_g->letters_small[0]->getScaledWidth();
			int l_w = game_g->letters_large[0]->getScaledWidth();
			int l_h = game_g->letters_large[0]->getScaledHeight();
			textLines(&n_lines, &max_wid, card->getText().c_str(), s_w, l_w);

			Rect2D rect;
			rect.x = 100;
			rect.y = 130;
			rect.w = max_wid;
			rect.h = n_lines * (l_h + 2);
			const Gigalomania::Image *player_image = game_g->player_heads_alliance[client_player];
			if( player_image != NULL ) {
				player_image->draw(rect.x, rect.y - player_image->getScaledHeight());
			}
#if SDL_MAJOR_VERSION == 1
			Gigalomania::Image *fill_rect = Gigalomania::Image::createBlankImage(game_g->getScaleWidth()*rect.w, game_g->getScaleHeight()*rect.h, 24);
			fill_rect->fillRect(0, 0, game_g->getScaleWidth()*rect.w, game_g->getScaleHeight()*rect.h, 0, 0, 0);
			fill_rect->convertToDisplayFormat();
			fill_rect->drawWithAlpha(game_g->getScaleWidth()*rect.x, game_g->getScaleHeight()*rect.y, tutorial_alpha_c);
			delete fill_rect;
#else
			game_g->getScreen()->fillRectWithAlpha((short)(game_g->getScaleWidth()*rect.x), (short)(game_g->getScaleHeight()*rect.y), (short)(game_g->getScaleWidth()*rect.w), (short)(game_g->getScaleHeight()*rect.h), 0, 0, 0, tutorial_alpha_c);
#endif
			Gigalomania::Image::writeMixedCase(rect.x, rect.y, game_g->letters_large, game_g->letters_small, game_g->numbers_white, card->getText().c_str(), Gigalomania::Image::JUSTIFY_LEFT);
			if( card->hasArrow(this) ) {
				int arrow_x = card->getArrowX();
				int arrow_y = card->getArrowY();
				int src_x = rect.x - 4;
				int src_y = (int)(rect.y + 0.5*rect.h);
				if( arrow_x >= rect.x + rect.w ) {
					src_x = rect.x + rect.w + 4;
				}
				else if( arrow_x >= rect.x && arrow_x < rect.x + rect.w ) {
					src_x = (int)(rect.x + 0.5*rect.w);
					if( arrow_y < src_y ) {
						src_y = rect.y - 4;
					}
					else {
						src_y = rect.y + rect.h + 4;
					}
				}
				game_g->getScreen()->drawLine((short)(game_g->getScaleWidth()*src_x), (short)(game_g->getScaleHeight()*src_y), (short)(game_g->getScaleWidth()*arrow_x), (short)(game_g->getScaleHeight()*arrow_y), 255, 255, 255);
			}

			if( !card->autoProceed() && card->canProceed(this) ) {
				if( tutorial_next_button == NULL ) {
					textLines(&n_lines, &max_wid, card->getNextText().c_str(), l_w, l_w);
					tutorial_next_button = new Button(rect.x + rect.w - max_wid, rect.y + rect.h + 4, card->getNextText().c_str(), game_g->letters_large);
					tutorial_next_button->setBackground(0, 0, 0, tutorial_alpha_c);
					screen_page->add(tutorial_next_button);
				}
			}
			else {
				if( tutorial_next_button != NULL ) {
					delete tutorial_next_button;
					tutorial_next_button = NULL;
				}
			}
			if( card->autoProceed() && card->canProceed(this) ) {
				game_g->getTutorial()->proceed();
				const TutorialCard *new_card = game_g->getTutorial()->getCard();
				if( new_card != NULL ) {
					new_card->setGUI(this);
				}
				else {
					GUIHandler::resetGUI(this);
				}
			}
		}
	}

	this->gamePanel->draw();
	//this->gamePanel->drawPopups();

	this->screen_page->draw();
	//this->screen_page->drawPopups();

	/*if( smokeParticleSystem != NULL ) {
		const Building *building_factory = current_sector->getBuilding(BUILDING_FACTORY);
		//const Building *building_factory = current_sector->getBuilding(BUILDING_TOWER);
		if( building_factory != NULL ) {
			const SmokeParticleSystem *ps = ( current_sector->getWorkers() > 0 ) ? smokeParticleSystem_busy : smokeParticleSystem;
			ps->draw(offset_land_x_c + building_factory->getX() + 17, offset_land_y_c + building_factory->getY() + 2);
		}
	}*/
	if( current_sector->getSmokeParticleSystem() != NULL ) {
		const Building *building_factory = current_sector->getBuilding(BUILDING_FACTORY);
		//const Building *building_factory = current_sector->getBuilding(BUILDING_TOWER);
		if( building_factory != NULL ) {
			current_sector->getSmokeParticleSystem()->draw(offset_land_x_c + building_factory->getX() + 17, offset_land_y_c + building_factory->getY() + 2);
		}
	}

	if( text_effect != NULL && text_effect->render() ) {
		delete text_effect;
		text_effect = NULL;
	}

	// mouse pointer
	GameState::setDefaultMouseImage();
	mouse_off_x = 0;
	mouse_off_y = 0;
	if( game_g->getGameStateID() == GAMESTATEID_PLAYING ) {
		GamePanel::MouseState mousestate = gamePanel->getMouseState();
		if( mousestate == GamePanel::MOUSESTATE_DEPLOY_WEAPON || selected_army != NULL ) {
			ASSERT( mousestate != GamePanel::MOUSESTATE_DEPLOY_WEAPON || selected_army == NULL );
			bool bloody = false;
			const Sector *this_sector = ( selected_army == NULL ) ? current_sector : selected_army->getSector();
			if( this_sector->getPlayer() != client_player ) {
				if( this_sector->getPlayer() != PLAYER_NONE && !Player::isAlliance(this_sector->getPlayer(), client_player) )
					bloody = true;
				for(int i=0;i<n_players_c && !bloody;i++) {
					if( i != client_player && !Player::isAlliance(i, client_player) ) {
						if( this_sector->getArmy(i)->any(true) )
							bloody = true;
					}
				}
			}
			if( bloody )
				mouse_image = game_g->panel_bloody_attack;
			else
				mouse_image = game_g->panel_attack;
			mobile_ui_display_mouse = true;
		}
		else if( mousestate == GamePanel::MOUSESTATE_DEPLOY_DEFENCE ) {
			mouse_image = game_g->panel_defence;
			//m_x -= mouse_image->getScaledWidth() / 2;
			//m_y -= mouse_image->getScaledHeight() / 2;
			mouse_off_x = - mouse_image->getScaledWidth() / 2;
			mouse_off_y = - mouse_image->getScaledHeight() / 2;
			mobile_ui_display_mouse = true;
		}
		else if( mousestate == GamePanel::MOUSESTATE_DEPLOY_SHIELD ) {
			mouse_image = game_g->panel_shield;
			mobile_ui_display_mouse = true;
			//m_x -= mouse_image->getScaledWidth() / 2;
			//m_y -= mouse_image->getScaledHeight() / 2;
			mouse_off_x = - mouse_image->getScaledWidth() / 2;
			mouse_off_y = - mouse_image->getScaledHeight() / 2;
		}
		else if( mousestate == GamePanel::MOUSESTATE_SHUTDOWN ) {
			mouse_image = game_g->men[n_epochs_c-1];
			mouse_off_x = - mouse_image->getScaledWidth() / 2;
			mouse_off_y = - mouse_image->getScaledHeight() / 2;
			mobile_ui_display_mouse = true;
		}
	}

	GameState::draw();
}

void PlayingGameState::update() {
	/*if( this->smokeParticleSystem != NULL ) {
		if( current_sector->getWorkers() > 0 ) {
			this->smokeParticleSystem->setBirthRate(0.008f);
		}
		else {
			this->smokeParticleSystem->setBirthRate(0.004f);
		}
	}*/
	int move_soldier_step_x = 0;
	int move_soldier_step_y = 0;
	int move_cannon_step_x = 0;
	int move_cannon_step_y = 0;
	int move_air_step = 0;
	if( soldier_last_time_moved_x == -1 )
		soldier_last_time_moved_x = game_g->getGameTime();
	if( soldier_last_time_moved_y == -1 )
		soldier_last_time_moved_y = game_g->getGameTime();
	if( cannon_last_time_moved_x == -1 )
		cannon_last_time_moved_x = game_g->getGameTime();
	if( cannon_last_time_moved_y == -1 )
		cannon_last_time_moved_y = game_g->getGameTime();
	if( air_last_time_moved == -1 )
		air_last_time_moved = game_g->getGameTime();
	// move twice as fast in x direction, to simulate 3D look
	while( game_g->getGameTime() - soldier_last_time_moved_x > soldier_move_rate_c ) {
		move_soldier_step_x++;
		soldier_last_time_moved_x += soldier_move_rate_c;
	}
	while( game_g->getGameTime() - soldier_last_time_moved_y > 2*soldier_move_rate_c ) {
		move_soldier_step_y++;
		soldier_last_time_moved_y += 2*soldier_move_rate_c;
	}
	while( game_g->getGameTime() - cannon_last_time_moved_x > cannon_move_rate_c ) {
		move_cannon_step_x++;
		cannon_last_time_moved_x += cannon_move_rate_c;
	}
	while( game_g->getGameTime() - cannon_last_time_moved_y > 2*cannon_move_rate_c ) {
		move_cannon_step_y++;
		cannon_last_time_moved_y += 2*cannon_move_rate_c;
	}
	while( game_g->getGameTime() - air_last_time_moved > air_move_rate_c ) {
		move_air_step++;
		air_last_time_moved += air_move_rate_c;
	}
	/*bool move_soldiers = ( game_g->getGameTime() - soldiers_last_time_moved > soldier_move_rate_c );
	bool move_air = ( game_g->getGameTime() - air_last_time_moved > air_move_rate_c );
	if( move_soldiers )
	soldiers_last_time_moved = game_g->getGameTime();
	if( move_air )
	air_last_time_moved = game_g->getGameTime();*/
	int time_interval = game_g->getLoopTime();

	int n_armies = 0;
	for(int i=0;i<n_players_c;i++) {
		const Army *army = current_sector->getArmy(i);
		if( army->getTotal() > 0 )
			n_armies++;
	}
	bool combat = false;
	if( n_armies >= 2 || ( current_sector->getPlayer() != -1 && current_sector->enemiesPresent() ) ) {
		combat = true;
	}

	int fire_prob = poisson(soldier_turn_rate_c, time_interval);
	for(int i=0;i<n_players_c;i++) {
		//for(int j=0;j<n_soldiers[i];j++) {
		for(size_t j=0;j<soldiers[i].size();j++) {
			//Soldier *soldier = soldiers[i][j];
			//Soldier *soldier = (Soldier *)soldiers[i]->get(j);
			Soldier *soldier = soldiers[i].at(j);
			//if( soldier->epoch == 6 || soldier->epoch == 7 || soldier->epoch == 9 ) {
			if( isAirUnit(soldier->epoch) ) {
				// air unit
				if( move_air_step > 0 ) {
					soldier->xpos -= move_air_step;
					soldier->ypos -= move_air_step;
					while( soldier->xpos < - offset_land_x_c - 32 )
						soldier->xpos += default_width_c + 64;
					while( soldier->ypos < - offset_land_y_c - 32 )
						soldier->ypos += default_height_c + 64;
				}
				if( combat ) {
					int fire_random = rand() % RAND_MAX;
					if( fire_random <= fire_prob ) {
						// fire!
						AmmoEffect *ammoeffect = new AmmoEffect( this, soldier->epoch, ATTACKER_AMMO_BOMB, soldier->xpos + 4, soldier->ypos + 8 );
						this->ammo_effects.push_back(ammoeffect);
					}
				}
			}
			else {
				if( !validSoldierLocation(soldier->epoch,soldier->xpos, soldier->ypos) ) {
					/* Soldier is already invalid location. This usually happens if the scenery suddenly
					* changes (eg, new building appearing). If this happens, find a new valid locaation.
					*/
					bool found_loc = false;
					while(!found_loc) {
						soldier->xpos = rand() % land_width_c;
						soldier->ypos = rand() % land_height_c;
						found_loc = validSoldierLocation(soldier->epoch, soldier->xpos, soldier->ypos);
					}
				}
				/* Turns are modelled as a Poisson distribution - so soldier_turn_rate_c is the mean number of
				* ticks that elapse per turn. Therefore we are interested in the probability that at least one
				* turn occured within this time interval.
				*/
				/*double prob = 1.0 - exp( - ((double)time_interval) / soldier_turn_rate_c );
				double random = ((double)( rand() % RAND_MAX )) / (double)RAND_MAX;*/
				//double prob = RAND_MAX * ( 1.0 - exp( - ((double)time_interval) / soldier_turn_rate_c ) );
				int prob = poisson(soldier_turn_rate_c, time_interval);
				int random = rand() % RAND_MAX;
				if( random <= prob ) {
					// turn!
					soldier->dir = (AmmoDirection)(rand() % 4);
				}
				int move_step = 0;
				if( soldier->epoch == cannon_epoch_c )
					move_step = (soldier->dir == 0 || soldier->dir == 1) ? move_cannon_step_y : move_cannon_step_x;
				else
					move_step = (soldier->dir == 0 || soldier->dir == 1) ? move_soldier_step_y : move_soldier_step_x;
				if( move_step > 0  ) {
					int step_x = 0;
					int step_y = 0;
					if( soldier->dir == 0 )
						step_y = move_step;
					else if( soldier->dir == 1 )
						step_y = - move_step;
					else if( soldier->dir == 2 )
						step_x = move_step;
					else if( soldier->dir == 3 )
						step_x = - move_step;

					int new_xpos = soldier->xpos + step_x;
					int new_ypos = soldier->ypos + step_y;
					if( !validSoldierLocation(soldier->epoch,new_xpos, new_ypos) ) {
						// path blocked, so turn around
						new_xpos = soldier->xpos;
						new_ypos = soldier->ypos;
						if( soldier->dir == 0 )
							soldier->dir = (AmmoDirection)1;
						else if( soldier->dir == 1 )
							soldier->dir = (AmmoDirection)0;
						else if( soldier->dir == 2 )
							soldier->dir = (AmmoDirection)3;
						else if( soldier->dir == 3 )
							soldier->dir = (AmmoDirection)2;
					}
					soldier->xpos = new_xpos;
					soldier->ypos = new_ypos;
				}

				if( combat && soldier->epoch != n_epochs_c ) {
					int fire_random = rand() % RAND_MAX;
					if( fire_random <= fire_prob ) {
						// fire!
						Gigalomania::Image *image = game_g->attackers_walking[soldier->player][soldier->epoch][soldier->dir][0];
						int xpos = 0, ypos = 0;
						if( soldier->epoch == cannon_epoch_c ) {
							xpos = soldier->xpos;
							ypos = soldier->ypos;
							if( soldier->dir == ATTACKER_AMMO_LEFT ) {
								xpos = soldier->xpos;
								ypos = soldier->ypos;
							}
							else if( soldier->dir == ATTACKER_AMMO_RIGHT ) {
								xpos = soldier->xpos + image->getScaledWidth();
								ypos = soldier->ypos;
							}
							else if( soldier->dir == ATTACKER_AMMO_UP ) {
								xpos = soldier->xpos + image->getScaledWidth()/4;
								ypos = soldier->ypos;
							}
							else if( soldier->dir == ATTACKER_AMMO_DOWN ) {
								xpos = soldier->xpos + image->getScaledWidth()/4;
								ypos = soldier->ypos + image->getScaledHeight();
							}
						}
						else {
							xpos = soldier->xpos + image->getScaledWidth()/2;
							ypos = soldier->ypos;
						}
						AmmoEffect *ammoeffect = new AmmoEffect( this, soldier->epoch, soldier->dir, xpos, ypos );
						this->ammo_effects.push_back(ammoeffect);
					}
				}
			}
		}
	}
}

bool PlayingGameState::buildingMouseClick(int s_m_x,int s_m_y,bool m_left,bool m_right,Building *building) {
	bool done = false;
	if( building == NULL )
		return done;
	if( !m_left && !m_right )
		return done;

	//Gigalomania::Image *base_image = building->getImages()[0];
	Gigalomania::Image *base_image = building->getImages()[current_sector->getBuildingEpoch()];
	ASSERT( base_image != NULL );
	if( s_m_x < offset_land_x_c + building->getX() || s_m_x >= offset_land_x_c + building->getX() + base_image->getScaledWidth() ||
		s_m_y < offset_land_y_c + building->getY() || s_m_y >= offset_land_y_c + building->getY() + base_image->getScaledHeight() ) {
			return done;
	}
	LOG("clicked on building\n");

	if( !done && this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_SHUTDOWN && building->getType() == BUILDING_TOWER ) {
		//current_sector->shutdown();
		this->shutdown(current_sector->getXPos(), current_sector->getYPos());
		done = true;
	}

	for(int i=0;i<building->getNTurrets() && !done;i++) {
		if( m_left
			/*&& s_m_x >= offset_land_x_c + building->pos_x + building->turret_pos[i].x
			&& s_m_x < offset_land_x_c + building->pos_x + building->turret_pos[i].getRight()
			&& s_m_y >= offset_land_y_c + building->pos_y + building->turret_pos[i].y
			&& s_m_y < offset_land_y_c + building->pos_y + building->turret_pos[i].getBottom()*/
			&& s_m_x >= building->getTurretButton(i)->getLeft()
			&& s_m_x < building->getTurretButton(i)->getRight()
			&& s_m_y >= building->getTurretButton(i)->getTop()
			&& s_m_y < building->getTurretButton(i)->getBottom()
			) {
				//int n_population = current_sector->getPopulation();
				//int n_spare = current_sector->getSparePopulation();
				if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_DEFENCE ) {
					done = true;
					// set new defender
					int deploy_defence = this->getGamePanel()->getDeployDefence();
					ASSERT(deploy_defence != -1);
					//current_sector->deployDefender(building, i, deploy_defence);
					this->deployDefender(current_sector->getXPos(), current_sector->getYPos(), building->getType(), i, deploy_defence);
				}
				else if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_NORMAL && building->getTurretMan(i) != -1 ) {
					done = true;
					// remove existing defender
					// return current defender to stocks
					//current_sector->returnDefender(building, i);
					this->returnDefender(current_sector->getXPos(), current_sector->getYPos(), building->getType(), i);
				}
		}
	}

	if( !done && this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_SHIELD ) {
		LOG("try to deploy shield\n");
		if( building->getHealth() < building->getMaxHealth() ) {
			int deploy_shield = this->getGamePanel()->getDeployShield();
			ASSERT(deploy_shield != -1);
			this->useShield(current_sector->getXPos(), current_sector->getYPos(), building->getType(), deploy_shield);
		}
	}

	return done;
}

void PlayingGameState::moveTo(int map_x,int map_y) {
	current_sector = game_g->getMap()->getSector(map_x, map_y);
	if( this->getGamePanel() != NULL )
		this->getGamePanel()->setPage( GamePanel::STATE_SECTORCONTROL );
	this->reset();
	for(size_t i=0;i<effects.size();i++) {
		TimedEffect *effect = effects.at(i);
		delete effect;
	}
	effects.clear();
	for(size_t i=0;i<ammo_effects.size();i++) {
		TimedEffect *effect = ammo_effects.at(i);
		delete effect;
	}
	ammo_effects.clear();
}

bool PlayingGameState::canRequestAlliance(int player,int i) const {
	ASSERT(player != i);
	ASSERT(game_g->players[player] != NULL);
	ASSERT(!game_g->players[player]->isDead());
	bool ok = true;
	// check not already allied
	for(int j=0;j<n_players_c && ok;j++) {
		if( j == player || game_g->players[j] == NULL || game_g->players[j]->isDead() ) {
		}
		else if( j == i || Player::isAlliance(i, j) ) {
			if( Player::isAlliance(player, j) )
				ok = false;
		}
	}

	// check still two sides
	bool allied_all_others = ok;
	for(int j=0;j<n_players_c && allied_all_others;j++) {
		if( j == player || game_g->players[j] == NULL || game_g->players[j]->isDead() ) {
		}
		else if( j == i || Player::isAlliance(i, j) ) {
			// player on the side that we are requesting an alliance with
		}
		else if( !Player::isAlliance(player, j) ) {
			allied_all_others = false;
		}
	}
	if( allied_all_others )
		ok = false;
	return ok;
}

void PlayingGameState::requestAlliance(int player,int i,bool human) {
	// 'player' requests alliance with 'i'
	/*if( !human ) {
		// AIs only supported in non-player mode
		ASSERT(gameMode == GAMEMODE_SINGLEPLAYER);
	}*/
	ASSERT(game_g->getGameMode() == GAMEMODE_SINGLEPLAYER); // blocked for now
	ASSERT(player != i);
	ASSERT(game_g->players[player] != NULL);
	ASSERT(!game_g->players[player]->isDead());
	//ASSERT(i != human_player); // todo: for requesting with human player
	bool ok = true;
	bool ask_human_player = false;
	int playing_asking_human = -1; // which player do we need to ask the human?
	int human_player = -1;
	// okay to request?
	// check i, and those who are allied with i
	for(int j=0;j<n_players_c && ok;j++) {
		if( j == player || game_g->players[j] == NULL || game_g->players[j]->isDead() ) {
		}
		else if( j == i || Player::isAlliance(i, j) ) {
			//if( j == human_player ) {
			if( game_g->players[j]->isHuman() ) {
				// request if human is part of alliance
				ask_human_player = true;
				playing_asking_human = player;
				human_player = j;
			}
			else if( !game_g->players[j]->requestAlliance(player) ) {
				ok = false;
				if( human )
					playSample(game_g->s_alliance_no[j]);
			}
		}
	}
	// check those who are allied with player
	for(int j=0;j<n_players_c && ok;j++) {
		if( j == player || game_g->players[j] == NULL || game_g->players[j]->isDead() ) {
		}
		else if( Player::isAlliance(player, j) ) {
			//if( j == human_player ) {
			if( game_g->players[j]->isHuman() ) {
				// request if human is part of alliance
				//ok = false;
				ask_human_player = true;
				playing_asking_human = i;
				human_player = j;
			}
			else if( !game_g->players[j]->requestAlliance(i) ) {
				ok = false;
				if( human )
					playSample(game_g->s_alliance_no[j]);
			}
		}
	}

	if( ok && ask_human_player ) {
		if( this->player_asking_alliance != -1 ) {
			// someone else asking, so don't ask
		}
		else {
			// askHuman() is called to avoid the cpu player repeatedly asking
			if( game_g->players[playing_asking_human]->askHuman() && game_g->players[playing_asking_human]->requestAlliance(human_player) ) {
				playSample(game_g->s_alliance_ask[playing_asking_human]);
				this->player_asking_alliance = playing_asking_human;
				//this->reset();
				this->setupMapGUI(); // needed to change the map GUI to ask player; call this rather than reset(), to avoid resetting the entire GUI (which causes the GUI to return to main sector control)
			}
		}
	}
	else if( ok ) {
		if( human )
			playSample(game_g->s_alliance_yes[i]);
		makeAlliance(player, i);
	}
}

void PlayingGameState::makeAlliance(int player,int i) {
	for(int j=0;j<n_players_c;j++) {
		if( j == player || game_g->players[j] == NULL || game_g->players[j]->isDead() ) {
		}
		else if( j == i || Player::isAlliance(i, j) ) {
			// bring player j into the alliance
			for(int k=0;k<n_players_c;k++) {
				if( k != j && ( k == player || Player::isAlliance(k, player) ) ) {
					Player::setAlliance(k, j, true);
				}
			}
		}
	}
	//gamestate->reset(); // reset shield buttons
	//((PlayingGameState *)gamestate)->resetShieldButtons(); // needed to update player shield buttons
	this->resetShieldButtons(); // needed to update player shield buttons
	this->cancelPlayerAskingAlliance(); // need to do this even if AIs make an alliance between themselves, as it may mean the player-alliance is no longer possible!
}

void PlayingGameState::cancelPlayerAskingAlliance() {
	if( this->player_asking_alliance != -1 ) {
		this->player_asking_alliance = -1;
		//this->reset();
		this->setupMapGUI(); // call this rather than reset(), to avoid the GUI going back to main sector control!
	}
}

void PlayingGameState::refreshTimeRate() {
	speed_button->setImage( game_g->icon_speeds[ game_g->getTimeRate()-1 ] );
}

void PlayingGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	if( !game_g->isDemo() && game_g->players[client_player]->isDead() ) {
		return;
	}
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);
	int s_m_x = (int)(m_x / game_g->getScaleWidth());
	int s_m_y = (int)(m_y / game_g->getScaleHeight());

	bool done = false;
	bool clear_selected_army = true;

	//int s_m_x = m_x / 1;
	//int s_m_y = m_y / 1;
	int map_x = ( s_m_x - offset_map_x_c ) / 16;
	int map_y = ( s_m_y - offset_map_y_c ) / 16;
	/*if( m_x >= offset_map_x_c && m_x < offset_map_x_c + map_width_c * map_sq->getScaledWidth() &&
	m_y >= offset_map_y_c && m_y < offset_map_y_c + map_height_c * map_sq->getScaledHeight() ) {*/
	if( !done && m_left && click && this->player_asking_alliance != -1 ) {
		ASSERT( this->alliance_yes != NULL );
		ASSERT( this->alliance_no != NULL );
		if( this->alliance_yes->mouseOver(m_x, m_y) ) {
			ASSERT( game_g->players[player_asking_alliance] != NULL );
			ASSERT( !game_g->players[player_asking_alliance]->isDead() );
			this->makeAlliance(player_asking_alliance, client_player);
			// makeAlliance also cancels
			done = true;
		}
		else if( this->alliance_no->mouseOver(m_x, m_y ) ) {
			this->cancelPlayerAskingAlliance();
			done = true;
		}
		if( done ) {
			registerClick();
		}
	}
	if( !done && click && map_x >= 0 && map_x < map_width_c && map_y >= 0 && map_y < map_height_c ) {
		if( this->player_asking_alliance == -1 && map_display == MAPDISPLAY_MAP && game_g->getMap()->isSectorAt(map_x, map_y) && this->map_panels[map_x][map_y]->mouseOver(m_x, m_y) ) {
			// although the mouse should always be over the map square, we call mouseOver so that the enabled flag is checked
			done = true;
			if( m_left && selected_army != NULL ) {
				if( selected_army->getSector() != game_g->getMap()->getSector(map_x, map_y) ) {
					int n_nukes = selected_army->getSoldiers(nuclear_epoch_c);
					ASSERT( n_nukes == 0 );
					// move selected army
					/*if( game_g->getMap()->getSector(map_x, map_y)->moveArmy(selected_army) ) {
						this->moveTo(map_x,map_y);
					}*/
					if( this->moveArmyTo(selected_army->getSector()->getXPos(), selected_army->getSector()->getYPos(), map_x, map_y) ) {
						this->moveTo(map_x, map_y);
					}
					else {
						// (some of) army too far - don't lose selection
						clear_selected_army = false;
					}
				}
			}
			else if( m_left && this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_WEAPON ) {
				// deploy assembled army
				ASSERT( current_sector->getAssembledArmy() != NULL );
				Sector *target_sector = game_g->getMap()->getSector(map_x, map_y);
				if( target_sector->isNuked() ) {
					//clear_selected_army = false;
				}
				else {
					int n_nukes = current_sector->getAssembledArmy()->getSoldiers(nuclear_epoch_c);
					if( n_nukes > 0 ) {
						// nuke!
						LOG("nuke sector %d, %d (%d)\n", map_x, map_y, n_nukes);
						ASSERT( n_nukes == 1 );
						if( target_sector->getActivePlayer() != -1 && target_sector->getPlayer() == current_sector->getPlayer() ) {
							// don't nuke own sector
							LOG("don't nuke own sector: %d\n", target_sector->getActivePlayer());
						}
						else if( target_sector->getActivePlayer() != -1 && Player::isAlliance(current_sector->getPlayer(), target_sector->getPlayer()) ) {
							// don't nuke allied sectors
							LOG("don't nuke allied sector\n");
							playSample(game_g->s_cant_nuke_ally);
						}
						else {
							if( this->nukeSector(current_sector->getXPos(), current_sector->getYPos(), map_x, map_y) ) {
								this->moveTo(map_x,map_y);
							}
						}
					}
					//else if( game_g->getMap()->getSector(map_x, map_y)->moveArmy(current_sector->getAssembledArmy() ) ) {
					else if( this->moveAssembledArmyTo(current_sector->getXPos(), current_sector->getYPos(), map_x, map_y) ) {
						this->getGamePanel()->setMouseState(GamePanel::MOUSESTATE_NORMAL);
						this->moveTo(map_x,map_y);
					}
				}
			}
			else if( m_left ) {
				//if( game_g->getMap()->sectors[map_x][map_y] != current_sector )
				{
					// move to viewing a different sector
					if( current_sector->getPlayer() == client_player ) {
						//current_sector->returnAssembledArmy();
						this->returnAssembledArmy(current_sector->getXPos(), current_sector->getYPos());
					}
					this->getGamePanel()->setMouseState(GamePanel::MOUSESTATE_NORMAL);
					this->moveTo(map_x,map_y);
				}
			}
			else if( m_right && !game_g->isDemo() ) {
				// select an army
				Army *army = game_g->getMap()->getSector(map_x, map_y)->getArmy(client_player);
				if( army->getTotal() > 0 ) {
					done = true;
					selected_army = army;
					clear_selected_army = false;
				}
			}
		}
	}

	if( !done && ( m_left || m_right ) && click && speed_button != NULL && speed_button->mouseOver(m_x, m_y) ) {
        done = true;
        registerClick();
        if( game_g->oneMouseButtonMode() ) {
			// cycle through the speeds
			game_g->cycleTimeRate();
		}
		else {
			if( m_left ) {
				game_g->increaseTimeRate();
			}
			else if( m_right ) {
				game_g->decreaseTimeRate();
			}
		}
		LOG("set time_rate to %d\n", game_g->getTimeRate());
		refreshTimeRate();
		//processClick(buttonSpeedClick, this->screen_page, this, 0, speed_button, m_left, m_middle, m_right);
	}
	else if( !done && m_left && click && quit_button != NULL && quit_button->mouseOver(m_x, m_y) ) {
        done = true;
        registerClick();
        requestQuit(false);
	}
	else if( !done && m_left && click && confirm_button_1 != NULL && confirm_button_1->mouseOver(m_x, m_y) ) {
		// save game and quit to desktop
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
		game_g->saveState();
	    game_g->getApplication()->setQuit();
	}
	else if( !done && m_left && click && confirm_button_2 != NULL && confirm_button_2->mouseOver(m_x, m_y) ) {
		// exit battle
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
		this->requestConfirm();
	}
	else if( !done && m_left && click && confirm_button_3 != NULL && confirm_button_3->mouseOver(m_x, m_y) ) {
		// cancel
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
        this->closeConfirmWindow();
    }
	else if( !done && m_left && click && pause_button != NULL && pause_button->mouseOver(m_x, m_y) ) {
		// should always be non-paused if we are here!
		if( !game_g->isPaused() ) {
            done = true;
            registerClick();
			game_g->togglePause();
		}
	}
	else if( !done && m_left && click && tutorial_next_button != NULL && tutorial_next_button->mouseOver(m_x, m_y) ) {
		game_g->getTutorial()->proceed();
		const TutorialCard *new_card = game_g->getTutorial()->getCard();
		if( new_card != NULL ) {
			new_card->setGUI(this);
		}
		else {
			GUIHandler::resetGUI(this);
		}
		delete tutorial_next_button;
		tutorial_next_button = NULL;
		done = true;
        registerClick();
		// don't lose selection, important for tutorial:
		clear_selected_army = false;
	}

	// switch map display
	for(int i=0;i<n_players_c && !done && m_left && click;i++) {
		if( shield_buttons[i] == NULL ) {
			continue;
		}
		ASSERT( shield_number_panels[i] != NULL );
		/*int bx = offset_map_x_c + 16 * map_width_c + 20;
		int by = offset_map_y_c + 2 + 16 * i + 8;
		if( s_m_x >= bx && s_m_x < bx + 16 && s_m_y >= by && s_m_y < by + 16 ) {*/
		if( shield_number_panels[i]->mouseOver(m_x, m_y) ) {
			bool ok = false;
			for(int j=i;j<n_players_c && !ok;j++) {
				if( j == i || Player::isAlliance(i, j) ) {
					const Army *army = current_sector->getArmy(j);
					if( army->getTotal() > 0 )
						ok = true;
				}
			}

			if( ok ) {
                done = true;
                registerClick();
                if( this->map_display == MAPDISPLAY_MAP )
					this->map_display = MAPDISPLAY_UNITS;
				else if( this->map_display == MAPDISPLAY_UNITS )
					this->map_display = MAPDISPLAY_MAP;
				this->reset();
			}
		}
	}

	// alliances
	for(int i=0;i<n_players_c && !done && m_left && click && !game_g->isDemo();i++) {
		if( i != client_player && game_g->players[i] != NULL && !game_g->players[i]->isDead() ) {
			if( shield_buttons[i] != NULL && shield_buttons[i]->mouseOver(m_x, m_y) ) {
				if( this->player_asking_alliance != -1 && this->player_asking_alliance == i ) {
					// automatically accept
					this->makeAlliance(client_player, i);
				}
				else {
					// request alliance
					if( canRequestAlliance(client_player, i) ) {
						requestAlliance(client_player, i, true);
					}
				}
				// automatically cancel any alliance being asked for
				if( this->player_asking_alliance != -1 ) {
					this->cancelPlayerAskingAlliance();
				}
				done = true;
			}
		}
	}
	if( !done && m_left && click && shield_blank_button != NULL && shield_blank_button->mouseOver(m_x, m_y) ) {
		// break alliance
		bool any = false;
		for(int i=0;i<n_players_c;i++) {
			if( i != client_player && Player::isAlliance(i, client_player) ) {
				Player::setAlliance(i, client_player, false);
				any = true;
			}
		}
		ASSERT( any );
		//gamestate->reset(); // reset shield buttons
		//((PlayingGameState *)gamestate)->resetShieldButtons(); // needed to update player shield buttons
		this->resetShieldButtons(); // needed to update player shield buttons
		done = true;
	}

	//if( !done && s_m_x >= offset_land_x_c + 16 && s_m_y >= offset_land_y_c ) {
	if( !done && click && !game_g->isDemo() && this->land_panel->mouseOver(m_x, m_y) ) {
		const Army *army_in_sector = current_sector->getArmy(client_player);
		Building *building = current_sector->getBuilding(BUILDING_TOWER);
		bool clicked_fortress = building != NULL && s_m_x >= offset_land_x_c + building->getX() && s_m_x < offset_land_x_c + building->getX() + game_g->fortress[ current_sector->getBuildingEpoch() ]->getScaledWidth() &&
			s_m_y >= offset_land_y_c + building->getY() && s_m_y < offset_land_y_c + building->getY() + game_g->fortress[ current_sector->getBuildingEpoch() ]->getScaledHeight();
		if( m_left ) {
            if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_WEAPON ) {
                ASSERT( current_sector->getAssembledArmy() != NULL );
				int n_nukes = current_sector->getAssembledArmy()->getSoldiers(nuclear_epoch_c);
				if( n_nukes == 0 ) {
					// deploy assembled army
                    done = true;
                    registerClick();
                    //army_in_sector->add( current_sector->getAssembledArmy() );
					this->moveAssembledArmyTo(current_sector->getXPos(), current_sector->getYPos(), current_sector->getXPos(), current_sector->getYPos());
					this->getGamePanel()->setMouseState(GamePanel::MOUSESTATE_NORMAL);
                }
				else {
					// can't nuke own sector!
					int n_selected = current_sector->getAssembledArmy()->getTotal();
					ASSERT( n_nukes == 1 );
					ASSERT( n_selected == n_nukes );
				}
			}
			else if( selected_army != NULL ) {
				done = true;
                registerClick();
                // move selected army
				if( clicked_fortress && current_sector->getPlayer() == selected_army->getPlayer() ) {
					this->returnArmy(current_sector->getXPos(), current_sector->getYPos(), selected_army->getSector()->getXPos(), selected_army->getSector()->getYPos());
				}
				else {
					if( selected_army->getSector() != current_sector ) {
						// move selected army
						this->moveArmyTo(selected_army->getSector()->getXPos(), selected_army->getSector()->getYPos(), current_sector->getXPos(), current_sector->getYPos());
					}
				}
			}
		}
		if( !done && ( game_g->oneMouseButtonMode() ? m_left : m_right ) && !clicked_fortress && army_in_sector->getTotal() > 0 ) {
			done = true;
            registerClick();
            //selected_army = army_in_sector;
			selected_army = game_g->getMap()->getSector(current_sector->getXPos(), current_sector->getYPos())->getArmy(client_player);
			clear_selected_army = false;
			if( current_sector->getPlayer() == client_player ) {
				//current_sector->returnAssembledArmy();
				this->returnAssembledArmy(current_sector->getXPos(), current_sector->getYPos());
			}
			this->getGamePanel()->setMouseState(GamePanel::MOUSESTATE_NORMAL);
		}
	}

	if( current_sector->getPlayer() == client_player && click ) {
		for(int i=0;i<N_BUILDINGS && !done;i++) {
			done = buildingMouseClick(s_m_x, s_m_y, m_left, m_right, current_sector->getBuilding((Type)i));
		}
	}

	if( !done )
		this->getGamePanel()->input(m_x, m_y, m_left, m_middle, m_right, click);

	if( clear_selected_army && ( m_left || m_right ) && click ) {
		selected_army = NULL;
		refreshButtons();
	}
	else if( selected_army != NULL ) {
		refreshButtons();
	}

}

void PlayingGameState::requestQuit(bool force_quit) {
	if( force_quit ) {
		game_g->saveState();
	    game_g->getApplication()->setQuit();
	}
	else {
		this->createQuitWindow();
	}
}

void PlayingGameState::requestConfirm() {
	if( confirm_window != NULL ) {
        this->closeConfirmWindow();
        if( !game_g->isStateChanged() ) {
			game_g->setGameResult(GAMERESULT_QUIT);
			game_g->fadeMusic(1000);
			game_g->setStateChanged(true);
			this->fadeScreen(true, 0, endIsland_g);
		}
	}
}

bool PlayingGameState::validSoldierLocation(int epoch,int xpos,int ypos) {
	ASSERT_S_EPOCH(epoch);
	bool okay = true;
	if( epoch == 6 || epoch == 7 || epoch == 9 )
		return true;

	//int size_x = attackers_walking[0][ epoch ][0]->getScaledWidth();
	//int size_y = attackers_walking[0][ epoch ][0]->getScaledHeight();
	int size_x = game_g->attackers_walking[0][ epoch ][0][0]->getScaledWidth();
	int size_y = game_g->attackers_walking[0][ epoch ][0][0]->getScaledHeight();
	if( xpos < 0 || xpos + size_x >= land_width_c || ypos < 0 || ypos + size_y >= land_height_c )
		okay = false;
	else if( current_sector->getPlayer() != -1 ) {
		for(int i=0;i<N_BUILDINGS && okay;i++) {
			Building *building = current_sector->getBuilding((Type)i);
			if( building == NULL )
				continue;
			Gigalomania::Image *image = building->getImages()[current_sector->getBuildingEpoch()];
			if( xpos + size_x >= building->getX() && xpos < building->getX() + image->getScaledWidth() &&
				ypos + size_y >= building->getY() && ypos < building->getY() + image->getScaledHeight() )
				okay = false;
		}
		if( okay && openPitMine() && xpos + size_x >= offset_openpitmine_x_c && xpos < offset_openpitmine_x_c + game_g->icon_openpitmine->getScaledWidth() &&
			ypos + size_y >= offset_openpitmine_y_c && ypos < offset_openpitmine_y_c + game_g->icon_openpitmine->getScaledHeight() )
			okay = false;
		/*Building *building = current_sector->getBuilding(BUILDING_TOWER);
		Building *building_mine = current_sector->getBuilding(BUILDING_MINE);
		if( xpos + size_x >= building->pos_x && xpos < building->pos_x + fortress[ current_sector->getBuildingEpoch() ]->getScaledWidth() &&
		ypos + size_y >= building->pos_y && ypos < building->pos_y + fortress[ current_sector->getBuildingEpoch() ]->getScaledHeight() )
		okay = false;
		else if( building_mine != NULL && xpos + size_x >= building_mine->pos_x && xpos < building_mine->pos_x + mine[ current_sector->getBuildingEpoch() ]->getScaledWidth() &&
		ypos + size_y >= building_mine->pos_y && ypos < building_mine->pos_y + mine[ current_sector->getBuildingEpoch() ]->getScaledHeight() )
		okay = false;
		else if( openPitMine() && xpos + size_x >= offset_openpitmine_x_c && xpos < offset_openpitmine_x_c + icon_openpitmine->getScaledWidth() &&
		ypos + size_y >= offset_openpitmine_y_c && ypos < offset_openpitmine_y_c + icon_openpitmine->getScaledHeight() )
		okay = false;*/
	}
	return okay;
}

void PlayingGameState::refreshSoldiers(bool flash) {
	for(int i=0;i<n_players_c;i++) {
		int n_soldiers_type[n_epochs_c+1];
		for(int j=0;j<=n_epochs_c;j++)
			n_soldiers_type[j] = 0;
		for(size_t j=0;j<soldiers[i].size();j++) {
			Soldier *soldier = soldiers[i].at(j);
			n_soldiers_type[ soldier->epoch ]++;
		}
		const Army *army = current_sector->getArmy(i);
		for(int j=0;j<=n_epochs_c;j++) {
			int diff = army->getSoldiers(j) - n_soldiers_type[j];
			if( diff > 0 ) {
				// create some more
				for(int k=0;k<diff;k++) {
					int xpos = 0, ypos = 0;
					bool found_loc = false;
					while(!found_loc) {
						xpos = rand() % land_width_c;
						ypos = rand() % land_height_c;
						found_loc = validSoldierLocation(j, xpos, ypos);
					}
					Soldier *soldier = new Soldier(i, j, xpos, ypos);
					soldiers[i].push_back( soldier );
					if( flash && !isAirUnit( soldier->epoch ) ) {
						blueEffect(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos, true);
					}
				}
				if( j == biplane_epoch_c ) {
					playSample(game_g->s_biplane, SOUND_CHANNEL_BIPLANE, -1); // n.b., doesn't matter if this restarts the currently playing sample
				}
				else if( j == jetplane_epoch_c ) {
					playSample(game_g->s_jetplane, SOUND_CHANNEL_BOMBER, -1); // n.b., doesn't matter if this restarts the currently playing sample
				}
				else if( j == spaceship_epoch_c ) {
					playSample(game_g->s_spaceship, SOUND_CHANNEL_SPACESHIP, -1); // n.b., doesn't matter if this restarts the currently playing sample
				}
			}
			else if( diff < 0 ) {
				// remove some
				for(size_t k=0;k<soldiers[i].size();) {
					Soldier *soldier = soldiers[i].at(k);
					if( soldier->epoch == j ) {
						if( n_deaths[i][j] > 0 ) {
							if( flash && !isAirUnit( soldier->epoch ) ) {
								deathEffect(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos);
								if( !isPlaying(SOUND_CHANNEL_FX) ) {
									// only play if sound fx channel is free, to avoid too many death samples sounding
									playSample(game_g->s_scream, SOUND_CHANNEL_FX);
								}
							}
							n_deaths[i][j]--;
						}
						else if( flash && !isAirUnit( soldier->epoch ) ) {
							blueEffect(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos, false);
						}
						soldiers[i].erase(soldiers[i].begin() + k);
						delete soldier;
						diff++;
						if( diff == 0 )
							break;
					}
					else
						k++;
				}
				if( army->getSoldiers(j) == 0 ) {
					if( j == biplane_epoch_c ) {
						game_g->s_biplane->fadeOut(500);
					}
					else if( j == jetplane_epoch_c ) {
						game_g->s_jetplane->fadeOut(500);
					}
					else if( j == spaceship_epoch_c ) {
						game_g->s_spaceship->fadeOut(500);
					}
				}
			}
		}
	}

	//this->refreshShieldNumberPanels();
}

/*void GameState::clearSoldiers() {
for(int i=0;i<n_players_c;i++) {
n_soldiers[i] = 0;
}
}*/

void PlayingGameState::deathEffect(int xpos,int ypos) {
	AnimationEffect *animationeffect = new AnimationEffect(xpos, ypos, game_g->death_flashes, n_death_flashes_c, 100, true);
	this->effects.push_back(animationeffect);
}

void PlayingGameState::blueEffect(int xpos,int ypos,bool dir) {
	AnimationEffect *animationeffect = new AnimationEffect(xpos, ypos, game_g->blue_flashes, n_blue_flashes_c, 50, dir);
	this->effects.push_back(animationeffect);
}

void PlayingGameState::explosionEffect(int xpos,int ypos) {
	if( game_g->explosions[0] != NULL ) { // not available with "old" graphics
		AnimationEffect *animationeffect = new AnimationEffect(xpos, ypos, game_g->explosions, n_explosions_c, 50, true);
		this->effects.push_back(animationeffect);
	}
}

void PlayingGameState::refreshButtons() {
	for(int i=0;i<N_BUILDINGS;i++) {
		Building *building = current_sector->getBuilding((Type)i);
		if( building != NULL ) {
			for(int j=0;j<building->getNTurrets();j++) {
				PanelPage *panel = building->getTurretButton(j);
				panel->setInfoLMB("");
			}
			if( building->getBuildingButton() != NULL ) {
				building->getBuildingButton()->setInfoLMB("");
			}

			if( current_sector->getPlayer() != client_player ) {
				// no text
			}
			else if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_SHUTDOWN ) {
				if( building->getBuildingButton() != NULL ) {
					building->getBuildingButton()->setInfoLMB("shutdown the sector");
				}
			}
			else if ( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_SHIELD ) {
				if( building->getBuildingButton() != NULL ) {
					building->getBuildingButton()->setInfoLMB("deploy shield");
				}
			}
			else {
				for(int j=0;j<building->getNTurrets();j++) {
					PanelPage *panel = building->getTurretButton(j);
					if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_DEFENCE ) {
						panel->setInfoLMB("place a defender here");
					}
					else if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_NORMAL &&  building->getTurretMan(j) != -1 ) {
						panel->setInfoLMB("return defender to tower"); // todo: check text
					}
				}
			}
		}
	}

	if( selected_army != NULL || this->gamePanel->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_WEAPON ) {
		bool is_nukes = current_sector->getAssembledArmy() != NULL && current_sector->getAssembledArmy()->getSoldiers(nuclear_epoch_c) > 0;
		this->land_panel->setInfoLMB(is_nukes ? "" : "move army here");
		if( this->player_asking_alliance == -1 && this->map_display == MAPDISPLAY_MAP ) {
			for(int y=0;y<map_height_c;y++) {
				for(int x=0;x<map_width_c;x++) {
					if( game_g->getMap()->isSectorAt(x, y) ) {
						if( is_nukes ) {
							if( game_g->getMap()->getSector(x, y)->getPlayer() == current_sector->getPlayer() || game_g->getMap()->getSector(x, y)->isBeingNuked() || game_g->getMap()->getSector(x, y)->isNuked() ) {
								map_panels[x][y]->setInfoLMB("");
							}
							else {
								map_panels[x][y]->setInfoLMB("nuke sector");
							}
						}
						else {
							map_panels[x][y]->setInfoLMB("move army to this sector");
						}
						//map_panels[x][y]->setInfoLMB(!is_nukes ? "move army to this sector" : game_g->getMap()->getSector(x, y) == current_sector ? "" : "nuke sector");
					}
				}
			}
		}
	}
	else {
		this->land_panel->setInfoLMB("");
		if( this->player_asking_alliance == -1 && this->map_display == MAPDISPLAY_MAP ) {
			for(int y=0;y<map_height_c;y++) {
				for(int x=0;x<map_width_c;x++) {
					if( game_g->getMap()->isSectorAt(x, y) ) {
						map_panels[x][y]->setInfoLMB("view this sector");
					}
				}
			}
		}
	}
}

void PlayingGameState::refreshShieldNumberPanels() {
	if( this->map_display == MAPDISPLAY_MAP ) {
		for(int i=0;i<n_players_c;i++) {
			if( shield_number_panels[i] != NULL ) {
				//shield_number_panels[i]->setVisible(false);
				shield_number_panels[i]->setInfoLMB("display numbers in each army");
			}
		}
	}
	else {
		for(int i=0;i<n_players_c;i++) {
			if( shield_number_panels[i] != NULL ) {
				shield_number_panels[i]->setInfoLMB("display map");
			}
		}
	}
}

void PlayingGameState::setNDesigners(int sector_x, int sector_y, int n_designers) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->getCurrentDesign() != NULL ) {
			sector->setDesigners(n_designers);
		}
	}
}

void PlayingGameState::setNWorkers(int sector_x, int sector_y, int n_workers) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->getCurrentManufacture() != NULL ) {
			sector->setWorkers(n_workers);
		}
	}
}

void PlayingGameState::setFAmount(int sector_x, int sector_y, int n_famount) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->getCurrentManufacture() != NULL ) {
			sector->setFAmount(n_famount);
		}
	}
}

void PlayingGameState::setNMiners(int sector_x, int sector_y, Id element, int n_miners) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->canMine(element) ) {
			sector->setMiners(element, n_miners);
		}
	}
}

void PlayingGameState::setNBuilders(int sector_x, int sector_y, Type type, int n_builders) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->canBuild(type) ) {
			sector->setBuilders(type, n_builders);
		}
	}
}

void PlayingGameState::setCurrentDesign(int sector_x, int sector_y, Design *design) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->setCurrentDesign(design);
	}
}

void PlayingGameState::setCurrentManufacture(int sector_x, int sector_y, Design *design) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->setCurrentManufacture(design);
	}
}

void PlayingGameState::assembledArmyEmpty(int sector_x, int sector_y) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->getAssembledArmy()->empty();
	}
}

bool PlayingGameState::assembleArmyUnarmed(int sector_x, int sector_y, int n) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		int n_spare = sector->getAvailablePopulation();
		if( n_spare >= n ) {
			int n_population = sector->getPopulation();
			sector->getAssembledArmy()->add(n_epochs_c, n);
			sector->setPopulation(n_population - n);
			return true;
		}
	}
	return false;
}

bool PlayingGameState::assembleArmy(int sector_x, int sector_y, int epoch, int n) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->assembleArmy(epoch, n) ) {
			return true;
		}
	}
	return false;
}

bool PlayingGameState::assembleAll(int sector_x, int sector_y, bool include_unarmed) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->assembleAll(include_unarmed);
	}
	return false;
}

void PlayingGameState::returnAssembledArmy(int sector_x, int sector_y) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->returnAssembledArmy();
	}
}

bool PlayingGameState::returnArmy(int sector_x, int sector_y, int src_x, int src_y) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		Sector *src = game_g->getMap()->getSector(src_x, src_y);
		Army *army = src->getArmy(client_player);
		return sector->returnArmy(army);
	}
	return false;
}

bool PlayingGameState::moveArmyTo(int src_x, int src_y, int target_x, int target_y) {
	Sector *src = game_g->getMap()->getSector(src_x, src_y);
	Sector *target = game_g->getMap()->getSector(target_x, target_y);
	ASSERT(src != NULL);
	ASSERT(target != NULL);
	Army *army = src->getArmy(client_player);
	return target->moveArmy(army);
}

bool PlayingGameState::moveAssembledArmyTo(int src_x, int src_y, int target_x, int target_y) {
	Sector *src = game_g->getMap()->getSector(src_x, src_y);
	ASSERT(src != NULL);
	if( src->getActivePlayer() == client_player ) {
		Army *army = src->getAssembledArmy();
		Sector *target = game_g->getMap()->getSector(target_x, target_y);
		ASSERT(target != NULL);
		return target->moveArmy(army);
	}
	return false;
}

bool PlayingGameState::nukeSector(int src_x, int src_y, int target_x, int target_y) {
	Sector *src = game_g->getMap()->getSector(src_x, src_y);
	Sector *target = game_g->getMap()->getSector(target_x, target_y);
	ASSERT(src != NULL);
	ASSERT(target != NULL);
	if( src->getActivePlayer() == client_player ) {
		if( target->nukeSector(src) ) {
			this->assembledArmyEmpty(src_x, src_y);
			return true;
		}
	}
	return false;
}

void PlayingGameState::deployDefender(int sector_x, int sector_y, Type type, int turret, int epoch) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		Building *building = sector->getBuilding(type);
		if( building != NULL ) {
			sector->deployDefender(building, turret, epoch);
		}
	}
}

void PlayingGameState::returnDefender(int sector_x, int sector_y, Type type, int turret) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		Building *building = sector->getBuilding(type);
		if( building != NULL ) {
			sector->returnDefender(building, turret);
		}
	}
}

void PlayingGameState::useShield(int sector_x, int sector_y, Type type, int shield) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		Building *building = sector->getBuilding(type);
		if( building != NULL ) {
			sector->useShield(building, shield);
		}
	}
}

void PlayingGameState::trashDesign(int sector_x, int sector_y, Invention *invention) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->trashDesign(invention);
	}
}

void PlayingGameState::shutdown(int sector_x, int sector_y) {
	Sector *sector = game_g->getMap()->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->shutdown(client_player);
	}
}

void PlayingGameState::saveState(stringstream &stream) const {
	stream << "<playing_gamestate>\n";
	if( game_g->getGameType() == GAMETYPE_TUTORIAL ) {
		stream << "<tutorial ";
		stream << "name=\"" << game_g->getTutorial()->getId().c_str() << "\" ";
		if( game_g->getTutorial()->getCard() != NULL ) {
			stream << "current_card_name=\"" << game_g->getTutorial()->getCard()->getId().c_str() << "\" ";
		}
		stream << "/>\n";
	}
	stream << "<current_sector x=\"" << current_sector->getXPos() << "\" y=\"" << current_sector->getYPos() << "\" />\n";
	stream << "<game_panel page=\"" << this->gamePanel->getPage() << "\" />\n";
	stream << "<player_asking_alliance player_id=\"" << player_asking_alliance << "\" />\n";

	for(int i=0;i<n_players_c;i++) {
		if( game_g->players[i] != NULL ) {
			game_g->players[i]->saveState(stream);
		}
	}
	Player::saveStateAlliances(stream);
	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<n_epochs_c+1;j++) {
			stream << "<n_deaths player_id=\"" << i << "\" epoch=\"" << j << "\" n=\"" << n_deaths[i][j] << "\" />\n";
		}
	}
	game_g->getMap()->saveStateSectors(stream);
	stream << "</playing_gamestate>\n";
}

void PlayingGameState::loadStateParseXMLMapXY(int *map_x, int *map_y, const TiXmlAttribute *attribute) {
	*map_x = -1;
	*map_y = -1;
	while( attribute != NULL ) {
		const char *attribute_name = attribute->Name();
		if( strcmp(attribute_name, "x") == 0 ) {
			*map_x = atoi(attribute->Value());
		}
		else if( strcmp(attribute_name, "y") == 0 ) {
			*map_y = atoi(attribute->Value());
		}
		else {
			// skip the other sector attributes, only interested in x/y in this subfunction
		}
		attribute = attribute->Next();
	}
	if( *map_x < 0 || *map_x >= map_width_c || *map_y < 0 || *map_y >= map_height_c ) {
		throw std::runtime_error("current_sector invalid map reference");
	}
	else if( !game_g->getMap()->isSectorAt(*map_x, *map_y) ) {
		throw std::runtime_error("current_sector map reference doesn't exist");
	}
}

void PlayingGameState::loadStateParseXMLNode(const TiXmlNode *parent) {
	if( parent == NULL ) {
		return;
	}
	bool read_children = true;
	//throw std::runtime_error("blah"); // test failing to load state

	switch( parent->Type() ) {
		case TiXmlNode::TINYXML_DOCUMENT:
			break;
		case TiXmlNode::TINYXML_ELEMENT:
			{
				const char *element_name = parent->Value();
				const TiXmlElement *element = parent->ToElement();
				const TiXmlAttribute *attribute = element->FirstAttribute();
				if( strcmp(element_name, "playing_gamestate") == 0 ) {
					// handled entirely by caller
				}
				else if( strcmp(element_name, "tutorial") == 0 ) {
					if( game_g->getGameType() != GAMETYPE_TUTORIAL ) {
						throw std::runtime_error("wrong game type for tutorial");
					}
					bool has_card_name = false;
					string card_name;
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "name") == 0 ) {
							string name = attribute->Value();
							game_g->setupTutorial(name);
						}
						else if( strcmp(attribute_name, "current_card_name") == 0 ) {
							has_card_name = true;
							card_name = attribute->Value();
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown playinggamestate/tutorial attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
					if( game_g->getTutorial() == NULL ) {
						throw std::runtime_error("unknown tutorial name");
					}
					game_g->getTutorial()->initCards();
					if( has_card_name ) {
						if( !game_g->getTutorial()->jumpTo(card_name) ) {
							throw std::runtime_error("unknown tutorial card name");
						}
					}
					else
						game_g->getTutorial()->jumpToEnd();
				}
				else if( strcmp(element_name, "player_asking_alliance") == 0 ) {
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "player_id") == 0 ) {
							player_asking_alliance = atoi(attribute->Value());
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown playinggamestate/player_asking_alliance attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
				}
				else if( strcmp(element_name, "n_deaths") == 0 ) {
					int player_id = -1;
					int epoch = -1;
					int n = -1;
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "player_id") == 0 ) {
							player_id = atoi(attribute->Value());
						}
						else if( strcmp(attribute_name, "epoch") == 0 ) {
							epoch = atoi(attribute->Value());
						}
						else if( strcmp(attribute_name, "n") == 0 ) {
							n = atoi(attribute->Value());
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown playinggamestate/n_deaths attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
					if( player_id == -1 || epoch == -1 || n == -1 ) {
						throw std::runtime_error("n_deaths missing attributes");
					}
					else if( player_id < 0 || player_id >= n_players_c ) {
						throw std::runtime_error("n_deaths invalid player_id");
					}
					else if( epoch < 0 || epoch >= n_epochs_c+1 ) {
						throw std::runtime_error("n_deaths invalid epoch");
					}
					n_deaths[player_id][epoch] = n;
				}
				else if( strcmp(element_name, "current_sector") == 0 ) {
					int map_x = -1, map_y = -1;
					loadStateParseXMLMapXY(&map_x, &map_y, attribute);
					this->moveTo(map_x, map_y);
				}
				else if( strcmp(element_name, "game_panel") == 0 ) {
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "page") == 0 ) {
							int page = atoi(attribute->Value());
							if( page < 0 || page > GamePanel::N_STATES ) {
								throw std::runtime_error("game_panel invalid page");
							}
							this->gamePanel->setPage(page);
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown playinggamestate/game_panel attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
				}
				else if( strcmp(element_name, "sector") == 0 ) {
					int map_x = -1, map_y = -1;
					loadStateParseXMLMapXY(&map_x, &map_y, attribute);
					game_g->getMap()->getSector(map_x, map_y)->loadStateParseXMLNode(parent);
					read_children = false;
				}
				else if( strcmp(element_name, "player") == 0 ) {
					int player_id = -1;
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "player_id") == 0 ) {
							player_id = atoi(attribute->Value());
						}
						else {
							// everything else parsed by Player::loadStateParseXMLNode()
						}
						attribute = attribute->Next();
					}
					if( player_id < 0 || player_id >= n_players_c ) {
						throw std::runtime_error("player invalid player_id");
					}
					game_g->players[player_id] = new Player(player_id == this->client_player, player_id);
					game_g->players[player_id]->loadStateParseXMLNode(parent);
					read_children = false;
				}
				else if( strcmp(element_name, "player_alliances") == 0 ) {
					Player::loadStateParseXMLNodeAlliances(parent);
					read_children = false;
				}
				else {
					// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
					LOG("unknown playinggamestate tag at line %d col %d: %s\n", parent->Row(), parent->Column(), element_name);
					ASSERT(false);
				}
			}
			break;
		case TiXmlNode::TINYXML_COMMENT:
			break;
		case TiXmlNode::TINYXML_UNKNOWN:
			break;
		case TiXmlNode::TINYXML_TEXT:
			{
				/*
				// add to the existing node, not a child
				vi_tree->setData( parent->Value() );
				*/
				/*VI_XMLTreeNode *vi_child = new VI_XMLTreeNode();
				vi_child->setData( parent->Value() );
				// add to the tree
				vi_tree->addChild(vi_child);
				vi_tree = vi_child;*/
			}
			break;
		case TiXmlNode::TINYXML_DECLARATION:
			break;
	}

	for(const TiXmlNode *child=parent->FirstChild();child!=NULL && read_children;child=child->NextSibling())  {
		loadStateParseXMLNode(child);
	}
}

void EndIslandGameState::reset() {
    //LOG("EndIslandGameState::reset()\n");
	this->screen_page->free(true);
}

void EndIslandGameState::draw() {
	game_g->getScreen()->clear(); // SDL on Android and OS X at least require screen to be cleared (otherwise we get corrupt regions outside of the main area)
	game_g->background_stars->draw(0, 0);
	game_g->getScreen()->fillRectWithAlpha((short)(game_g->getScaleWidth()*40), (short)(game_g->getScaleHeight()*120), (short)(game_g->getScaleWidth()*240), (short)(game_g->getScaleHeight()*70), 0, 0, 0, 127);
	char text[4096] = "";
	if( game_g->getGameResult() == GAMERESULT_QUIT )
		strcpy(text, "QUITTER!");
	else if( game_g->getGameResult() == GAMERESULT_LOST )
		strcpy(text, "LOSER!");
	else if( game_g->getGameResult() == GAMERESULT_WON )
		strcpy(text, "CONGRATULATIONS!");
	else {
		ASSERT(false);
	}
	Gigalomania::Image::write(160, 122, game_g->letters_large, text, Gigalomania::Image::JUSTIFY_CENTRE);

	bool suspend = false;
	if( game_g->getStartEpoch() >= 6 && game_g->getGameResult() == GAMERESULT_WON )
		suspend = true;

	if( !game_g->isDemo() ) {
		if( game_g->player_heads_select[client_player] != NULL ) {
			game_g->player_heads_select[client_player]->draw(40, 96);
			if( game_g->getGameResult() == GAMERESULT_LOST ) {
				game_g->grave->draw(42, 64);
			}
		}
	}
	const int xstep = 40;
	for(int i=0,xpos=96;i<n_players_c;i++) {
		if( i == client_player || game_g->players[i] == NULL )
			continue;
		if( game_g->player_heads_select[i] != NULL ) {
			game_g->player_heads_select[i]->draw(xpos, 96);
			if( game_g->getGameResult() == GAMERESULT_WON || game_g->players[i]->getFinalMen() == 0 ) {
				game_g->grave->draw(xpos+2, 64);
			}
		}
		xpos += xstep;
	}

	Gigalomania::Image::write(40, 140, game_g->letters_small, "PLAYER", Gigalomania::Image::JUSTIFY_LEFT);
	Gigalomania::Image::write(100, 140, game_g->letters_small, "START", Gigalomania::Image::JUSTIFY_LEFT);
	Gigalomania::Image::write(140, 140, game_g->letters_small, "BIRTHS", Gigalomania::Image::JUSTIFY_LEFT);
	Gigalomania::Image::write(180, 140, game_g->letters_small, "DEATHS", Gigalomania::Image::JUSTIFY_LEFT);
	Gigalomania::Image::write(220, 140, game_g->letters_small, "END", Gigalomania::Image::JUSTIFY_LEFT);
	if( suspend )
		Gigalomania::Image::write(260, 140, game_g->letters_small, "SAVED", Gigalomania::Image::JUSTIFY_LEFT);

	int ypos = 150;
	int rect_y_offset = 2;
	//int r = 0, g = 0, b = 0, col = 0;
	int r = 0, g = 0, b = 0;
	int rect_x = (int)(20 * game_g->getScaleWidth());
	int rect_y = (int)((ypos-rect_y_offset) * game_g->getScaleHeight());
	int rect_w = (int)(16 * game_g->getScaleWidth());
	int rect_h = (int)(8 * game_g->getScaleHeight());

	if( !game_g->isDemo() ) {
		PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)client_player);
		/*col = SDL_MapRGB(game_g->getScreen()->getSurface()->format, r, g, b);
		SDL_FillRect(game_g->getScreen()->getSurface(), &rect, col);*/
		game_g->getScreen()->fillRect(rect_x, rect_y, rect_w, rect_h, r, g, b);

		//Gigalomania::Image::write(40, ypos, game_g->letters_small, "HUMAN", Gigalomania::Image::JUSTIFY_LEFT, true);
		Gigalomania::Image::write(40, ypos, game_g->letters_small, PlayerType::getName((PlayerType::PlayerTypeID)client_player), Gigalomania::Image::JUSTIFY_LEFT);

		Gigalomania::Image::writeNumbers(110, ypos, game_g->numbers_yellow, game_g->players[client_player]->getNMenForThisIsland(), Gigalomania::Image::JUSTIFY_LEFT);
		Gigalomania::Image::writeNumbers(150, ypos, game_g->numbers_yellow, game_g->players[client_player]->getNBirths(), Gigalomania::Image::JUSTIFY_LEFT);
		Gigalomania::Image::writeNumbers(190, ypos, game_g->numbers_yellow, game_g->players[client_player]->getNDeaths(), Gigalomania::Image::JUSTIFY_LEFT);
		Gigalomania::Image::writeNumbers(230, ypos, game_g->numbers_yellow, game_g->players[client_player]->getFinalMen(), Gigalomania::Image::JUSTIFY_LEFT);
		if( suspend )
			Gigalomania::Image::writeNumbers(270, ypos, game_g->numbers_yellow, game_g->players[client_player]->getNSuspended(), Gigalomania::Image::JUSTIFY_LEFT);
		ypos += 10;
	}

	for(int i=0;i<n_players_c;i++) {
		if( i == client_player || game_g->players[i] == NULL )
			continue;
		PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
		/*col = SDL_MapRGB(game_g->getScreen()->getSurface()->format, r, g, b);
		rect.y = (Sint16)(ypos * game_g->getScaleHeight());
		SDL_FillRect(game_g->getScreen()->getSurface(), &rect, col);*/
		rect_y = (int)((ypos-rect_y_offset) * game_g->getScaleHeight());
		game_g->getScreen()->fillRect(rect_x, rect_y, rect_w, rect_h, r, g, b);

		//Gigalomania::Image::write(40, ypos, game_g->letters_small, "COMPUTER", Gigalomania::Image::JUSTIFY_LEFT, true);
		Gigalomania::Image::write(40, ypos, game_g->letters_small, PlayerType::getName((PlayerType::PlayerTypeID)i), Gigalomania::Image::JUSTIFY_LEFT);
		Gigalomania::Image::writeNumbers(110, ypos, game_g->numbers_yellow, game_g->players[i]->getNMenForThisIsland(), Gigalomania::Image::JUSTIFY_LEFT);
		Gigalomania::Image::writeNumbers(150, ypos, game_g->numbers_yellow, game_g->players[i]->getNBirths(), Gigalomania::Image::JUSTIFY_LEFT);
		Gigalomania::Image::writeNumbers(190, ypos, game_g->numbers_yellow, game_g->players[i]->getNDeaths(), Gigalomania::Image::JUSTIFY_LEFT);
		Gigalomania::Image::writeNumbers(230, ypos, game_g->numbers_yellow, game_g->players[i]->getFinalMen(), Gigalomania::Image::JUSTIFY_LEFT);
		if( suspend )
			Gigalomania::Image::writeNumbers(270, ypos, game_g->numbers_yellow, game_g->players[i]->getNSuspended(), Gigalomania::Image::JUSTIFY_LEFT);
		ypos += 10;
	}

	this->screen_page->draw();
	//this->screen_page->drawPopups();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void EndIslandGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);

	if( ( m_left || m_right ) && click && !game_g->isStateChanged() ) {
		this->requestQuit(false);
	}
}

void EndIslandGameState::requestQuit(bool force_quit) {
	if( force_quit ) {
		game_g->saveState();
	    game_g->getApplication()->setQuit();
	}
	else {
		game_g->setStateChanged(true);
		this->fadeScreen(true, 0, returnToChooseIsland_g);
	}
}

void GameCompleteGameState::reset() {
    //LOG("GameCompleteGameState::reset()\n");
	this->screen_page->free(true);
}

void GameCompleteGameState::draw() {
	game_g->getScreen()->clear(); // SDL on Android and OS X at least require screen to be cleared (otherwise we get corrupt regions outside of the main area)
	game_g->background_stars->draw(0, 0);

	this->screen_page->draw();
	//this->screen_page->drawPopups();

	if( !game_g->isDemo() ) {
		stringstream str;
		int l_h = game_g->letters_large[0]->getScaledHeight();
		int y = 80;

		Gigalomania::Image::writeMixedCase(160, y, game_g->letters_large, game_g->letters_small, game_g->numbers_white, "GAME COMPLETE", Gigalomania::Image::JUSTIFY_CENTRE);
		y += l_h + 2;

		if( game_g->getDifficultyLevel() == DIFFICULTY_EASY )
			str.str("Easy");
		else if( game_g->getDifficultyLevel() == DIFFICULTY_MEDIUM )
			str.str("Medium");
		else if( game_g->getDifficultyLevel() == DIFFICULTY_HARD )
			str.str("Hard");
		else if( game_g->getDifficultyLevel() == DIFFICULTY_ULTRA )
			str.str("Ultra");
		else {
			ASSERT(false);
		}
		Gigalomania::Image::writeMixedCase(160, y, game_g->letters_large, game_g->letters_small, game_g->numbers_white, str.str().c_str(), Gigalomania::Image::JUSTIFY_CENTRE);
		y += l_h + 2;

		y += l_h + 2;

		str << "Men Remaining " << game_g->getMenAvailable();
		Gigalomania::Image::writeMixedCase(160, y, game_g->letters_large, game_g->letters_small, game_g->numbers_white, str.str().c_str(), Gigalomania::Image::JUSTIFY_CENTRE);
		y += l_h + 2;

		str.str("");
		str << "Men Saved " << game_g->getNSuspended();
		Gigalomania::Image::writeMixedCase(160, y, game_g->letters_large, game_g->letters_small, game_g->numbers_white, str.str().c_str(), Gigalomania::Image::JUSTIFY_CENTRE);
		y += l_h + 2;

		int score = game_g->getMenAvailable() + game_g->getNSuspended();
		str.str("");
		str << "Total Score " << score;
		Gigalomania::Image::writeMixedCase(160, y, game_g->letters_large, game_g->letters_small, game_g->numbers_white, str.str().c_str(), Gigalomania::Image::JUSTIFY_CENTRE);
		y += l_h + 2;
	}

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void GameCompleteGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);

	if( ( m_left || m_right ) && click && !game_g->isStateChanged() ) {
		this->requestQuit(false);
	}
}

void GameCompleteGameState::requestQuit(bool force_quit) {
	if( force_quit ) {
		game_g->saveState();
	    game_g->getApplication()->setQuit();
	}
	else {
		game_g->setStateChanged(true);
		this->fadeScreen(true, 0, startNewGame_g);
	}
}
