#pragma once

/** Main game file, mainly contains various setup functions.
*/

namespace Gigalomania {
	class Screen;
	class Image;
	class PanelPage;
	class Sample;
}

using Gigalomania::Sample;

using std::vector;
using std::stringstream;
using std::string;

class Invention;
class Weapon;
class Element;
class Sector;
class GameState;
class PlayingGameState;
class Player;
class Application;
class TextEffect;
class Map;
class Tutorial;

#include "common.h"
#include "image.h"
#include "TinyXML/tinyxml.h"

enum GameStateID {
	GAMESTATEID_UNDEFINED = -1,
	GAMESTATEID_CHOOSEGAMETYPE = 0,
	GAMESTATEID_CHOOSEDIFFICULTY,
	GAMESTATEID_CHOOSEPLAYER,
	GAMESTATEID_CHOOSETUTORIAL,
	GAMESTATEID_PLACEMEN,
	GAMESTATEID_PLAYING,
	GAMESTATEID_ENDISLAND,
	GAMESTATEID_GAMECOMPLETE
};

enum GameResult {
	GAMERESULT_UNDEFINED = 0,
	GAMERESULT_WON,
	GAMERESULT_QUIT,
	GAMERESULT_LOST
};

enum DifficultyLevel {
	// don't change the numbers, as will break saved state and saved game compatibility!
	DIFFICULTY_EASY = 0,
	DIFFICULTY_MEDIUM = 1,
	DIFFICULTY_HARD = 2,
	DIFFICULTY_ULTRA = 3,
	DIFFICULTY_N_LEVELS = 4
};

enum GameMode {
	GAMEMODE_SINGLEPLAYER = 0,
	GAMEMODE_MULTIPLAYER_SERVER = 1,
	GAMEMODE_MULTIPLAYER_CLIENT = 2
};

enum GameType {
	// don't change the numbers, as will break saved state compatibility!
	GAMETYPE_SINGLEISLAND = 0,
	GAMETYPE_ALLISLANDS = 1,
	GAMETYPE_TUTORIAL = 2
};

const int default_width_c = 320;
const int default_height_c = 240;

const int infinity_c = 31;
//const int end_epoch_c = 9; // use this to have the last epoch game
const int end_epoch_c = -1; // use this to have the last epoch as being 2100AD
const int cannon_epoch_c = 5;
const int biplane_epoch_c = 6;
const int jetplane_epoch_c = 7;
const int nuclear_epoch_c = 8;
const int spaceship_epoch_c = 9;
const int laser_epoch_c = 9;
const int n_shields_c = 4;
const int n_playershields_c = 16;
const int n_flag_frames_c = 4;
const int max_defender_frames_c = 11;
const int n_attacker_directions_c = 4;
const int max_attacker_frames_c = 9;
const int n_trees_c = 4;
const int n_tree_frames_c = 4;
const int n_nuke_frames_c = 2;
const int n_saucer_frames_c = 4;
const int n_death_flashes_c = 3;
const int n_blue_flashes_c = 7;
const int n_explosions_c = 59;
const int n_coast_c = 8;
const int n_map_sq_c = 16;
const int max_islands_per_epoch_c = 3;

class Game {
	float scale_factor_w; // how much the input graphics are scaled
	float scale_factor_h;
	float scale_width; // the scale of the logical resolution or graphics size wrt the default 320x240 coordinate system
	float scale_height;
	bool onemousebutton;
	bool mobile_ui;
	bool using_old_gfx;
	bool is_testing;

	Application *application;
	Gigalomania::Screen *screen;
	bool paused;
	GameState *gamestate;
	GameState *dispose_gamestate;
	unsigned int lastmousepress_time;

	int frame_counter;
	int time_rate; // time factor
	int real_time;
	int real_loop_time;
	int game_time;
	int loop_time;
	float accumulated_time;
	int mouseTime;

	bool pref_sound_on;
	bool pref_music_on;
	bool pref_disallow_nukes;

	GameMode gameMode;
	GameType gameType;
	DifficultyLevel difficulty_level;
	int human_player;
	Tutorial *tutorial;

	GameStateID gameStateID;
	bool state_changed;
	GameResult gameResult;

	int start_epoch;
	int n_sub_epochs;
	int selected_island;
	bool completed_island[max_islands_per_epoch_c];
	Map *maps[n_epochs_c][max_islands_per_epoch_c];
	Map *map;
	int n_men_store;
	int n_player_suspended;

	void calculateScale(const Gigalomania::Image *image);
	void convertToHiColor(Gigalomania::Image *image) const;
	void processImage(Gigalomania::Image *image, bool old_smooth = true) const;
	bool loadAttackersWalkingImages(const string &gfx_dir, int epoch);
	bool loadOldImages();
	void getDesktopResolution(int *user_width, int *user_height) const;

	const char *getFilename(int slot) const;
	bool readMapProcessLine(int *epoch, int *index, Map **l_map, char *line, const int MAX_LINE, const char *filename);
	bool readMap(const char *filename);
	bool loadGameInfo(DifficultyLevel *difficulty, int *player, int *n_men, int suspended[n_players_c], int *epoch, bool completed[max_islands_per_epoch_c], const char *filename) const;
	bool loadGame(const char *filename);
	GameState *loadStateParseXMLNode(const TiXmlNode *parent);
	void copyFile(const char *src, const char *dst) const;

	bool testFindSoldiersBuildingNewTower(const Sector *sector, int *total, int *squares) const;

	void disposeGameState();

	int getMenPerEpoch() const;
	void updatedEpoch();
	void setEpoch(int epoch);
	void cleanupPlayers();
public:
	Gigalomania::Image *background;
	Gigalomania::Image *background_stars;
	Gigalomania::Image *player_heads_select[n_players_c];
	Gigalomania::Image *player_heads_alliance[n_players_c];
	Gigalomania::Image *grave;
	Gigalomania::Image *land[MAP_N_COLOURS];
	Gigalomania::Image *fortress[n_epochs_c];
	Gigalomania::Image *mine[n_epochs_c];
	Gigalomania::Image *factory[n_epochs_c];
	Gigalomania::Image *lab[n_epochs_c];
	Gigalomania::Image *men[n_epochs_c];
	Gigalomania::Image *unarmed_man;
	Gigalomania::Image *flags[n_players_c][n_flag_frames_c];
	Gigalomania::Image *panel_design;
	Gigalomania::Image *panel_lab;
	Gigalomania::Image *panel_factory;
	Gigalomania::Image *panel_shield;
	Gigalomania::Image *panel_defence;
	Gigalomania::Image *panel_attack;
	Gigalomania::Image *panel_bloody_attack;
	Gigalomania::Image *panel_twoattack;
	Gigalomania::Image *panel_build[N_BUILDINGS];
	Gigalomania::Image *panel_building[N_BUILDINGS];
	Gigalomania::Image *panel_knowndesigns;
	Gigalomania::Image *panel_bigdesign;
	Gigalomania::Image *panel_biglab;
	Gigalomania::Image *panel_bigfactory;
	Gigalomania::Image *panel_bigshield;
	Gigalomania::Image *panel_bigdefence;
	Gigalomania::Image *panel_bigattack;
	Gigalomania::Image *panel_bigbuild;
	Gigalomania::Image *panel_bigknowndesigns;
	Gigalomania::Image *numbers_blue[10];
	Gigalomania::Image *numbers_grey[10];
	Gigalomania::Image *numbers_white[10];
	Gigalomania::Image *numbers_orange[10];
	Gigalomania::Image *numbers_yellow[10];
	Gigalomania::Image *numbers_largegrey[10];
	Gigalomania::Image *numbers_largeshiny[10];
	Gigalomania::Image *numbers_small[n_players_c][10];
	Gigalomania::Image *numbers_half;
	Gigalomania::Image *letters_large[n_font_chars_c];
	Gigalomania::Image *letters_small[n_font_chars_c];
	Gigalomania::Image *mouse_pointers[n_players_c];
	Gigalomania::Image *playershields[n_playershields_c];
	Gigalomania::Image *building_health;
	Gigalomania::Image *dash_grey;
	Gigalomania::Image *icon_shield;
	Gigalomania::Image *icon_defence;
	Gigalomania::Image *icon_weapon;
	Gigalomania::Image *icon_shields[n_shields_c];
	Gigalomania::Image *icon_defences[n_epochs_c];
	Gigalomania::Image *icon_weapons[n_epochs_c];
	Gigalomania::Image *numbered_defences[n_epochs_c];
	Gigalomania::Image *numbered_weapons[n_epochs_c];
	Gigalomania::Image *icon_elements[N_ID];
	Gigalomania::Image *icon_clocks[13];
	Gigalomania::Image *icon_infinity;
	Gigalomania::Image *icon_bc;
	Gigalomania::Image *icon_ad;
	Gigalomania::Image *icon_ad_shiny;
	Gigalomania::Image *icon_towers[n_players_c];
	Gigalomania::Image *icon_armies[n_players_c];
	Gigalomania::Image *icon_nuke_hole;
	Gigalomania::Image *mine_gatherable_small;
	Gigalomania::Image *mine_gatherable_large;
	Gigalomania::Image *icon_ergo;
	Gigalomania::Image *icon_trash;
	Gigalomania::Image *coast_icons[n_coast_c];
	int map_sq_offset, map_sq_coast_offset;
	Gigalomania::Image *map_sq[MAP_N_COLOURS][n_map_sq_c];
	int n_defender_frames[n_epochs_c];
	Gigalomania::Image *defenders[n_players_c][n_epochs_c][max_defender_frames_c];
	Gigalomania::Image *nuke_defences[n_players_c];
	int n_attacker_frames[n_epochs_c+1][n_attacker_directions_c];
	Gigalomania::Image *attackers_walking[n_players_c][n_epochs_c+1][n_attacker_directions_c][max_attacker_frames_c]; // epochs 6-9 are special case!
	Gigalomania::Image *planes[n_players_c][n_epochs_c];
	Gigalomania::Image *nukes[n_players_c][n_nuke_frames_c];
	Gigalomania::Image *saucers[n_players_c][n_saucer_frames_c];
	Gigalomania::Image *attackers_ammo[n_epochs_c][N_ATTACKER_AMMO_DIRS];
	Gigalomania::Image *icon_openpitmine;
	Gigalomania::Image *icon_trees[n_trees_c][n_tree_frames_c];
	vector<Gigalomania::Image *> icon_clutter;
	vector<Gigalomania::Image *> icon_clutter_nuked;
	Gigalomania::Image *flashingmapsquare;
	Gigalomania::Image *mapsquare;
	Gigalomania::Image *arrow_left;
	Gigalomania::Image *arrow_right;
	Gigalomania::Image *death_flashes[n_death_flashes_c];
	Gigalomania::Image *blue_flashes[n_blue_flashes_c];
	Gigalomania::Image *explosions[n_explosions_c];
	Gigalomania::Image *icon_mice[2];
	Gigalomania::Image *icon_speeds[3];
	Gigalomania::Image *smoke_image;
	Gigalomania::Image *background_islands;

	// speech
	Sample *s_design_is_ready;
	Sample *s_ergo;
	Sample *s_advanced_tech;
	Sample *s_fcompleted;
	Sample *s_on_hold;
	Sample *s_running_out_of_elements;
	Sample *s_tower_critical;
	Sample *s_sector_destroyed;
	Sample *s_mine_destroyed;
	Sample *s_factory_destroyed;
	Sample *s_lab_destroyed;
	Sample *s_itis_all_over;
	Sample *s_conquered;
	Sample *s_won;
	Sample *s_weve_nuked_them;
	Sample *s_weve_been_nuked;
	Sample *s_alliance_yes[n_players_c];
	Sample *s_alliance_no[n_players_c];
	Sample *s_alliance_ask[n_players_c];
	Sample *s_quit[n_players_c];
	Sample *s_cant_nuke_ally;

	// effects
	Sample *s_explosion;
	Sample *s_scream;
	Sample *s_buildingdestroyed;
	Sample *s_guiclick;
	Sample *s_biplane;
	Sample *s_jetplane;
	Sample *s_spaceship;

	Sample *music;

	Invention *invention_shields[n_epochs_c];
	Invention *invention_defences[n_epochs_c];
	Weapon *invention_weapons[n_epochs_c];
	Element *elements[N_ID];
	Player *players[n_players_c];

	Game();
	~Game();

	bool loadImages();
	bool loadSamples();
	bool createMaps();

	float getScaleWidth() const {
		return this->scale_width;
	}
	float getScaleHeight() const {
		return this->scale_height;
	}
	void setOneMouseButton(bool onemousebutton) {
		this->onemousebutton = onemousebutton;
	}
	bool isOneMouseButton() const {
		return this->onemousebutton;
	}
	bool oneMouseButtonMode() const;
	void setMobileUI(bool mobile_ui) {
		this->mobile_ui = mobile_ui;
	}
	bool isMobileUI() const {
		return this->mobile_ui;
	}
	bool isUsingOldGfx() const {
		return this->using_old_gfx;
	}
	void setTesting(bool is_testing) {
		this->is_testing = is_testing;
	}
	bool isTesting() const {
		return this->is_testing;
	}
	
	bool createApplication();
	Application *getApplication() {
		return this->application;
	}
	const Application *getApplication() const {
		return this->application;
	}
	bool openScreen(bool fullscreen);
	Gigalomania::Screen *getScreen() {
		return this->screen;
	}
	const Gigalomania::Screen *getScreen() const {
		return this->screen;
	}
	bool isPaused() const;

	void cycleTimeRate() {
		time_rate++;
		if( time_rate > 3 )
			time_rate = 1;
	}
	void increaseTimeRate() {
		if( time_rate > 1 )
			time_rate--;
	}
	void decreaseTimeRate() {
		if( time_rate < 3 )
			time_rate++;
	}
	void setTimeRate(int time_rate);
	int getTimeRate() const {
		return this->time_rate;
	}
	void setRealTime(int real_time);
	int getRealTime() const;
	int getRealLoopTime() const;
	void setGameTime(int game_time);
	int getGameTime() const;
	int getLoopTime() const;
	int getFrameCounter() const {
		return this->frame_counter;
	}
	void updateTime(int time);
	void resetMouseClick();
	int getNClicks();

	void setGameMode(GameMode gameMode) {
		this->gameMode = gameMode;
	}
	GameMode getGameMode() const {
		return this->gameMode;
	}
	void setGameType(GameType gameType) {
		this->gameType = gameType;
	}
	GameType getGameType() const {
		return this->gameType;
	}
	void setDifficultyLevel(DifficultyLevel difficulty_level) {
		this->difficulty_level = difficulty_level;
	}
	DifficultyLevel getDifficultyLevel() const {
		return this->difficulty_level;
	}
	void setGameStateID(GameStateID state, GameState *new_gamestate = NULL);
	GameStateID getGameStateID() const {
		return this->gameStateID;
	}
	void setStateChanged(bool state_changed) {
		this->state_changed = state_changed;
	}
	bool isStateChanged() const {
		return this->state_changed;
	}
	void setGameResult(GameResult gameResult) {
		this->gameResult = gameResult;
	}
	GameResult getGameResult() const {
		return this->gameResult;
	}
	void setupTutorial(const string &id);
	const Tutorial *getTutorial() const {
		return this->tutorial;
	}
	Tutorial *getTutorial() {
		return this->tutorial;
	}
	void setCurrentMap() {
		map = maps[start_epoch][selected_island];
	}
	void setCurrentIsand(int start_epoch, int selected_island);
	const Map *getMap() const;
	Map *getMap();
	const Map *getMap(int i, int j) const {
		return this->maps[i][j];
	}
	int getStartEpoch() const {
		return this->start_epoch;
	}
	int getNSubEpochs() const {
		return this->n_sub_epochs;
	}
	void setPrefSoundOn(bool pref_sound_on) {
		this->pref_sound_on = pref_sound_on;
	}
	bool isPrefSoundOn() const {
		return this->pref_sound_on;
	}
	void setPrefMusicOn(bool pref_music_on) {
		this->pref_music_on = pref_music_on;
	}
	bool isPrefMusicOn() const {
		return this->pref_music_on;
	}
	void setPrefDisallowNukes(bool pref_disallow_nukes) {
		this->pref_disallow_nukes = pref_disallow_nukes;
	}
	bool isPrefDisallowNukes() const {
		return this->pref_disallow_nukes;
	}

	int getMapSqOffset() const {
		return map_sq_offset;
	}
	int getMapSqCoastOffset() const {
		return map_sq_coast_offset;
	}
	void loadPrefs();
	void savePrefs() const;
	bool isDemo() const;

	void deleteState() const;
	void saveState() const;
	bool loadState();

	int getMenAvailable() const;
	int getNSuspended() const;
	void nextEpoch();
	void nextIsland();
	void startIsland();
	void endIsland();
	void returnToChooseIsland();
	void startNewGame();
	void placeTower();
	void newGame();
	void setClientPlayer(int set_client_player);
	bool validPlayer(int player) const;
	void requestQuit(bool force_quit);
	void keypressReturn();
	void togglePause();
	void activate();
	void deactivate();
	void mouseClick(int m_x, int m_y, bool m_left, bool m_middle, bool m_right, bool click);
	void updateGame();
	void drawGame() const;
	void addTextEffect(TextEffect *effect);
	void drawProgress(int percentage) const;

	bool readLineFromRWOps(bool &ok, SDL_RWops *file, char *buffer, char *line, int MAX_LINE, int &buffer_offset, int &newline_index, bool &reached_end);
	bool loadGameInfo(DifficultyLevel *difficulty, int *player, int *n_men, int suspended[n_players_c], int *epoch, bool completed[max_islands_per_epoch_c], int slot);
	bool loadGame(int slot);
	void saveGame(int slot) const;

	void stopMusic();
	void fadeMusic(int duration_ms) const;
	void playMusic();

	void setupPlayers();
	void setupInventions();
	void setupElements();
	bool playerAlive(int player) const;

	void runTests();
};

extern Game *game_g;

void startIsland_g();
void endIsland_g();
void returnToChooseIsland_g();
void startNewGame_g();

// DATADIR is place to look if data not present in application's folder
#define DATADIR "/games/gigalomania" // redox games dir

extern string maps_dirname;
#ifdef DATADIR
extern string alt_maps_dirname;
#endif

extern const int epoch_dates[];
extern const char *epoch_names[];

enum PlayerMode {
	PLAYER_DEMO = -2,
	PLAYER_NONE = -1
};

class Map {
	string name;
	string filename;
	MapColour colour;
	int n_opponents;
	Sector *sectors[map_width_c][map_height_c];
	bool sector_at[map_width_c][map_height_c];
	bool reserved[map_width_c][map_height_c]; // if true, don't use for starting players - used for testing

public:

	Map(MapColour colour,int n_opponents,const char *name);
	~Map();

	MapColour getColour() const {
		return this->colour;
	}
	int getNOpponents() const {
		return this->n_opponents;
	}
	const Sector *getSector(int x, int y) const;
	Sector *getSector(int x, int y);
	bool isSectorAt(int x, int y) const;

	void newSquareAt(int x,int y);
	void createSectors(PlayingGameState *gamestate, int epoch);
#if 0
	void checkSectors() const;
#endif
	void freeSectors();
	const char *getName() const {
		return name.c_str();
	}
	const char *getFilename() const {
		return filename.c_str();
	}
	void setFilename(const char *filename) {
		this->filename = filename;
	}
	int getNSquares() const;
	void draw(int offset_x, int offset_y) const;
	void findRandomSector(int *rx,int *ry) const;
	bool isReserved(int x, int y) const {
		return this->reserved[x][y];
	}
	void setReserved(int x, int y, bool r) {
		this->reserved[x][y] = r;
	}
	void canMoveTo(bool temp[map_width_c][map_height_c], int sx,int sy,int player) const;
	void calculateStats() const;

	void saveStateSectors(stringstream &stream) const;
};

void playGame(int n_args, char *args[]);

#if defined(__ANDROID__)

// JNI for Android

void launchUrl(string url);

#endif

// game constants

const int SHORT_DELAY = 4000;
const int nuke_delay_c = 250;

const int gameticks_per_hour_c = 200;
const int hours_per_day_c = 12;

const int mine_epoch_c = 3;
const int factory_epoch_c = 4;
const int lab_epoch_c = 5;
//const int air_epoch_c = 6;

const int DESIGNTIME_M3 = 20;
const int DESIGNTIME_M2 = 30;
const int DESIGNTIME_M1 = 40;
const int DESIGNTIME_0 = 50;
const int DESIGNTIME_1 = 100;
const int DESIGNTIME_2 = 200;
const int DESIGNTIME_3 = 400;

const int MANUFACTURETIME_0 = 15;
const int MANUFACTURETIME_1 = 30;
const int MANUFACTURETIME_2 = 45;
const int MANUFACTURETIME_3 = 60;

const int BUILDTIME_TOWER = 80;
const int BUILDTIME_MINE = 40;
const int BUILDTIME_FACTORY = 40;
const int BUILDTIME_LAB = 40;

const int ticks_per_frame_c = 100; // game time ticks per frame rate (used for various animated sprites)
const float time_ratio_c = 0.15f; // game time ticks per time ticks
