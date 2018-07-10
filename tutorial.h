#pragma once

/** Classes to handle the Tutorials.
*/

using std::vector;
using std::string;

class PlayingGameState;
class Tutorial;

#include "sector.h"

class TutorialInfo {
public:
	string id;
	string text;

	TutorialInfo(const string &id, const string &text) : id(id), text(text) {
	}
};

class TutorialManager {
public:
	static vector<TutorialInfo> getTutorialInfo();
	static Tutorial *setupTutorial(const string &id);
};

class GUIHandler {
public:
	virtual void setGUI(PlayingGameState *playing_gamestate) const {
	}
	virtual ~GUIHandler() {
		// need a virtual destructor even though it doesn't do anything, to ensure that subclass destructors are called (see warning on Linux GCC)
	}

	static void resetGUI(PlayingGameState *playing_gamestate);
};

class GUIHandlerBlockAll : public GUIHandler {
	vector<string> exceptions;
public:
	virtual void setGUI(PlayingGameState *playing_gamestate) const;
	void addException(const string &exception) {
		exceptions.push_back(exception);
	}
};

class TutorialCard {
	string id;
	string text;
	string next_text;
	bool has_arrow;
	int arrow_x, arrow_y;
	bool show_arrow_on_page;
	int show_arrow_page;
	bool show_arrow_on_sector;
	int show_arrow_sector_x;
	int show_arrow_sector_y;
	bool auto_proceed;

	bool player_allow_build_tower;

	GUIHandler *gui_handler;
public:
	TutorialCard(const string &id, const string &text) : id(id), text(text), next_text("Next"), has_arrow(false), arrow_x(-1), arrow_y(-1), show_arrow_on_page(false), show_arrow_page(-1), show_arrow_on_sector(false), show_arrow_sector_x(-1), show_arrow_sector_y(-1), auto_proceed(false), player_allow_build_tower(true), gui_handler(NULL) {
	}
	virtual ~TutorialCard() {
		if( gui_handler != NULL ) {
			delete gui_handler;
		}
	}

	string getId() const {
		return id;
	}
	string getText() const {
		return text;
	}

	void setNextText(const string &next_text) {
		this->next_text = next_text;
	}
	string getNextText() const {
		return next_text;
	}

	void setArrow(int arrow_x, int arrow_y) {
		this->has_arrow = true;
		this->arrow_x = arrow_x;
		this->arrow_y = arrow_y;
	}
	void setArrowShowPage(int show_arrow_page) {
		this->show_arrow_on_page = true;
		this->show_arrow_page = show_arrow_page;
	}
	void setArrowShowSector(int show_arrow_sector_x, int show_arrow_sector_y) {
		this->show_arrow_on_sector = true;
		this->show_arrow_sector_x = show_arrow_sector_x;
		this->show_arrow_sector_y = show_arrow_sector_y;
	}
	bool hasArrow(PlayingGameState *playing_gamestate) const;
	int getArrowX() const {
		return arrow_x;
	}
	int getArrowY() const {
		return arrow_y;
	}

	void setAutoProceed(bool auto_proceed) {
		this->auto_proceed = auto_proceed;
	}
	bool autoProceed() const {
		return auto_proceed;
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const {
		return true;
	}

	void setPlayerAllowBuildTower(bool player_allow_build_tower) {
		this->player_allow_build_tower = player_allow_build_tower;
	}
	bool playerAllowBuildTower() const {
		return player_allow_build_tower;
	}
	virtual void setGUI(PlayingGameState *playing_gamestate) const;
	void setGUIHandler(GUIHandler *gui_handler) {
		this->gui_handler = gui_handler;
	}
};

class TutorialCardWaitForPanelPage : public TutorialCard {
	int wait_page;
public:
	TutorialCardWaitForPanelPage(const string &id, const string &text, int wait_page) : TutorialCard(id, text), wait_page(wait_page) {
		this->setAutoProceed(true);
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const;
};

class TutorialCardWaitForDesign : public TutorialCard {
public:
	enum WaitType {
		WAITTYPE_CURRENT_DESIGN = 0,
		WAITTYPE_HAS_DESIGNED = 1,
		WAITTYPE_CURRENT_MANUFACTURE = 2,
		WAITTYPE_HAS_MANUFACTURED = 3
	};
private:
	const Sector *sector;
	WaitType wait_type;
	bool require_type;
	Invention::Type invention_type;
	bool require_epoch;
	int invention_epoch;
public:
	TutorialCardWaitForDesign(const string &id, const string &text, const Sector *sector, WaitType wait_type, bool require_type, Invention::Type invention_type, bool require_epoch, int invention_epoch) : TutorialCard(id, text), sector(sector), wait_type(wait_type), require_type(require_type), invention_type(invention_type), require_epoch(require_epoch), invention_epoch(invention_epoch) {
		this->setAutoProceed(true);
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const;
};

class TutorialCardWaitForBuilding : public TutorialCard {
	const Sector *sector;
	Type building_type;
public:
	TutorialCardWaitForBuilding(const string &id, const string &text, const Sector *sector, Type building_type) : TutorialCard(id, text), sector(sector), building_type(building_type) {
		this->setAutoProceed(true);
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const;
};

class TutorialCardWaitForDeployedArmy : public TutorialCard {
	Sector *deploy_sector;
	bool require_bombard;
	bool inverse; // if true, wait until an army is deployed anywhere other than deploy_sector (or anywhere, if deploy_sector is NULL)
	bool require_empty; // if true, check the square(s) is empty instead
	bool require_unoccupied; // if true, require that the square is not occupied by a player's tower
public:
	TutorialCardWaitForDeployedArmy(const string &id, const string &text, Sector *deploy_sector, bool require_bombard) : TutorialCard(id, text), deploy_sector(deploy_sector), require_bombard(require_bombard), inverse(false), require_empty(false), require_unoccupied(false) {
		this->setAutoProceed(true);
	}

	void setInverse(bool inverse) {
		this->inverse = inverse;
	}
	void setRequireEmpty(bool require_empty) {
		this->require_empty = require_empty;
	}
	void setRequireUnoccupied(bool require_unoccupied) {
		this->require_unoccupied = require_unoccupied;
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const;
};

class TutorialCardWaitForNewTower : public TutorialCard {
	Sector *tower_sector;
	bool inverse; // if true, wait until a tower is built anywhere other than tower_sector (or anywhere, if tower_sector is NULL)
public:
	TutorialCardWaitForNewTower(const string &id, const string &text, Sector *tower_sector) : TutorialCard(id, text), tower_sector(tower_sector), inverse(false)  {
		this->setAutoProceed(true);
	}

	void setInverse(bool inverse) {
		this->inverse = inverse;
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const;
};

class Tutorial {
protected:
	string id;
	int start_epoch;
	int island;
	int start_map_x, start_map_y;
	int n_men;
	bool auto_end;
	size_t card_index;
	vector<TutorialCard *> cards;
	bool ai_allow_growth;
	bool ai_allow_design;
	bool ai_allow_ask_alliance;
	bool ai_allow_deploy;
	bool allow_retreat_loss;

public:
	Tutorial(const string &id) : id(id), start_epoch(0), island(0), start_map_x(0), start_map_y(0), n_men(0), auto_end(false), card_index(0), ai_allow_growth(true), ai_allow_design(true), ai_allow_ask_alliance(true), ai_allow_deploy(true), allow_retreat_loss(true) {
	}
	virtual ~Tutorial();

	string getId() const {
		return id;
	}
	int getStartEpoch() const {
		return start_epoch;
	}
	int getIsland() const {
		return island;
	}
	int getStartMapX() const {
		return start_map_x;
	}
	int getStartMapY() const {
		return start_map_y;
	}
	int getNMen() const {
		return n_men;
	}
	bool aiAllowGrowth() const {
		return ai_allow_growth;
	}
	bool aiAllowDesign() const {
		return ai_allow_design;
	}
	bool aiAllowAskAlliance() const {
		return ai_allow_ask_alliance;
	}
	bool aiAllowDeploy() const {
		return ai_allow_deploy;
	}
	bool allowRetreatLoss() const {
		return allow_retreat_loss;
	}
	const TutorialCard *getCard() const {
		if( card_index >= cards.size() )
			return NULL;
		return cards.at(card_index);
	}
	void proceed() {
		card_index++;
	}
	bool jumpTo(const string &id);
	void jumpToEnd() {
		card_index = cards.size();
	}
	bool autoEnd() const {
		return auto_end;
	}

	virtual void initCards()=0;
};

class Tutorial1 : public Tutorial {
public:
	Tutorial1(const string &id);

	virtual void initCards();
};

class Tutorial2 : public Tutorial {
public:
	Tutorial2(const string &id);

	virtual void initCards();
};

class Tutorial3 : public Tutorial {
public:
	Tutorial3(const string &id);

	virtual void initCards();
};
