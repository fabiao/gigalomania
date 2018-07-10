#pragma once

#include "common.h"

#include "TinyXML/tinyxml.h"

/** Handles the players, including all the AI.
*/

class Sector;
class PlayingGameState;

using std::stringstream;

class PlayerType {
public:
	enum PlayerTypeID {
		PLAYER_RED = 0,
		PLAYER_GREEN = 1,
		PLAYER_YELLOW = 2,
		PLAYER_BLUE = 3
	};
	static const char *getName(PlayerTypeID id);
	static void getColour(int *r,int *g,int *b,PlayerTypeID id);
};

class Player {
	int index; // saved
	bool dead; // saved

	int n_births; // saved
	int n_deaths; // saved
	int n_men_for_this_island; // saved
	int n_suspended; // saved

	bool is_human;

	void doSectorAI(int client_player, PlayingGameState *gamestate, Sector *sector);
	static bool alliances[n_players_c][n_players_c];
	static int alliance_last_asked[n_players_c][n_players_c];

	int alliance_last_asked_human; // time we last asked human player, to avoid continually asking // aved

	static void setAllianceLastAsked(int a, int b,int time);
	static int allianceLastAsked(int a, int b);
public:
	//Player(int index, char *name);
	//Player(int index, int personality);
	Player(bool is_human, int index);
	~Player();

	bool isHuman() const {
		return this->is_human;
	}
	//int getShieldIndex();
	static int getShieldIndex(bool allied[n_players_c]);
	/*int getPersonality() {
	return personality;
	}*/
	void doAIUpdate(int client_player, PlayingGameState *gamestate);
	int getFinalMen() const;
	bool isDead() const {
		return this->dead;
	}
	void kill(PlayingGameState *gamestate);
	bool askHuman();
	bool requestAlliance(int player);

	int getNBirths() const {
		return this->n_births;
	}
	int getNDeaths() const {
		return this->n_deaths;
	}
	int getNMenForThisIsland() const {
		return this->n_men_for_this_island;
	}
	int getNSuspended() const {
		return this->n_suspended;
	}

	void registerBirths(int n) {
		this->n_births += n;
	}
	void setNDeaths(int n) {
		this->n_deaths = n;
	}
	void addNDeaths(int n) {
		this->n_deaths += n;
	}
	void setNMenForThisIsland(int n) {
		this->n_men_for_this_island = n;
	}
	void setNSuspended(int n) {
		this->n_suspended = n;
	}
	void addNSuspended(int n) {
		this->n_suspended += n;
	}

	void saveState(stringstream &stream) const;
	void loadStateParseXMLNode(const TiXmlNode *parent);
	static void saveStateAlliances(stringstream &stream);
	static void loadStateParseXMLNodeAlliances(const TiXmlNode *parent);

	static void setAlliance(int a, int b, bool alliance);
	static bool isAlliance(int a, int b);
	static void resetAllAlliances();
};
