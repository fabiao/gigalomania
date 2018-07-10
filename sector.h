#pragma once

/** Handles an individual sector.
*/

namespace Gigalomania {
	class Image;
	class PanelPage;
	class Button;
}

using Gigalomania::PanelPage;

class Feature;
class Sector;
class PlayingGameState;
class Invention;

using std::vector;
using std::string;
using std::stringstream;

#include "TinyXML/tinyxml.h"

#include "common.h"

const int element_multiplier_c = 2;
const int n_gatherable_rate_c = 500;
const int max_grow_population_c = 500;
const int growth_rate_c = 200; // higher is slower; beware of having a growth rate too fast, as it means a killer strategy is just to wait around letting the population grow first
const int mine_rate_c = 30; // higher is slower
const int combat_rate_c = 50; // higher is slower combat
const int bombard_rate_c = 5; // higher is slower damage
const int max_gatherables_stored_c = 22;

bool isAirUnit(int epoch);
bool defenceNeedsMan(int epoch);

class Particle {
	float xpos, ypos; // floats to allow for movement
	int birth_time;
public:
	Particle();

	float getX() const {
		return this->xpos;
	}
	float getY() const {
		return this->ypos;
	}
	void setPos(float xpos, float ypos) {
		this->xpos = xpos;
		this->ypos = ypos;
	}
	int getBirthTime() const {
		return this->birth_time;
	}
};

class ParticleSystem {
protected:
	vector<Particle> particles;
	const Gigalomania::Image *image;
	float size;

public:
	ParticleSystem(const Gigalomania::Image *image) : image(image), size(1.0f) {
	}
	virtual ~ParticleSystem() {
	}

	void setSize(float size) {
		this->size = size;
	}

	void draw(int xpos, int ypos) const;
	virtual void update()=0;
};

class SmokeParticleSystem : public ParticleSystem {
	float birth_rate;
	int life_exp;
	int last_emit_time;
	float move_x, move_y;
public:
	SmokeParticleSystem(const Gigalomania::Image *image);
	virtual ~SmokeParticleSystem() {
	}

	void setBirthRate(float birth_rate);
	void setMove(float move_x, float move_y) {
		this->move_x = move_x;
		this->move_y = move_y;
	}
	void setLifeExp(int life_exp) {
		this->life_exp = life_exp;
	}

	virtual void update();
};

class Army {
	int soldiers[n_epochs_c+1]; // unarmed men are soldiers[n_epochs_c]; // saved
	int player; // no need to save, saved by caller
	Sector *sector; // no need to save
	PlayingGameState *gamestate;

public:
	Army(PlayingGameState *gamestate, Sector *sector, int player);
	~Army() {
	}

	int getPlayer() const {
		return this->player;
	}
	Sector *getSector() const {
		return this->sector;
	}
	int getTotal() const;
	int getTotalMen() const;
	bool any(bool include_unarmed) const;
	int getStrength() const;
	int getBombardStrength() const;
	int getSoldiers(int index) const {
		return this->soldiers[index];
	}
	void add(int i,int n);
	void add(Army *army);
	void remove(int i,int n);
	void kill(int index);
	void empty() {
		for(int i=0;i<n_epochs_c+1;i++)
			soldiers[i] = 0;
	}
	bool canLeaveSafely() const;
	void retreat(bool only_air);

	int getIndividualStrength(int i) const;
	static int getIndividualStrength(int player, int i);
	static int getIndividualBombardStrength(int i);

	void saveState(stringstream &stream) const;
	void loadStateParseXMLNode(const TiXmlNode *parent);
};

class Element {
	string name;
public:
	enum Type {
		GATHERABLE = 0,
		OPENPITMINE = 1,
		DEEPMINE = 2
	};
private:
	Id id;
	Type type;

public:
	Element(const char *name,Id id,Type type);
	~Element();

	Gigalomania::Image *getImage() const;
	const char *getName() const {
		return this->name.c_str();
	}
	Type getType() const {
		return this->type;
	}
};

class Design {
	Invention *invention;
	bool ergonomically_terrific;
	int cost[N_ID];
	int save_id;

public:
	Design(Invention *invention,bool ergonomically_terrific);

	void setCost(Id id, float cost) {
		this->cost[(int)id] = (int)(cost * element_multiplier_c);
	}
	int getCost(Id id) {
		return this->cost[(int)id];
	}
	bool isErgonomicallyTerrific() const {
		return this->ergonomically_terrific;
	}
	Invention *getInvention() const {
		return this->invention;
	}
	void setSaveId(int save_id) {
		this->save_id = save_id;
	}
	int getSaveId() const {
		return this->save_id;
	}

	static bool setupDesigns();
};

class Invention {
protected:
	string name;
public:
	enum Type {
		UNKNOWN_TYPE = -1,
		SHIELD = 0,
		DEFENCE = 1,
		WEAPON = 2,
		N_TYPES = 3
	};

protected:
	Type type;
	int epoch;
	vector<Design *> designs;

public:
	Invention(const char *name,Type type,int epoch);
	~Invention();

	//int getRelativeEpoch();
	Gigalomania::Image *getImage() const;
	const char *getName() const {
		return this->name.c_str();
	}
	Type getType() const {
		return this->type;
	}
	int getEpoch() const {
		return this->epoch;
	}
	void addDesign(Design *design);
	size_t getNDesigns() const {
		return this->designs.size();
	}
	Design *getDesign(size_t i) const {
		return this->designs.at(i);
	}
	Design *findDesign(int save_id) const;

	static Invention *getInvention(Invention::Type type,int epoch);
};

class Weapon : public Invention {
	int n_men;
public:
	Weapon(const char *name,int epoch,int n_men) : Invention(name,WEAPON,epoch) {
		this->n_men = n_men;
	}
	~Weapon() {
	}
	int getNMen() const {
		return this->n_men;
	}
};

const int max_building_turrets_c = 4;

class Building {
private:
	Type type; // saved
	Sector *sector;
	int health; // saved
	int max_health;
	int pos_x, pos_y;
	int n_turrets;
	int turret_man[max_building_turrets_c]; // saved
	int turret_man_frame[max_building_turrets_c];
	PanelPage *building_button;
	PanelPage *turret_buttons[max_building_turrets_c];
	PlayingGameState *gamestate;

public:
	Building(PlayingGameState *gamestate, Sector *sector, Type type);
	~Building();

	Type getType() const {
		return this->type;
	}
	int getX() const {
		return this->pos_x;
	}
	int getY() const {
		return this->pos_y;
	}
	int getHealth() const {
		return this->health;
	}
	int getMaxHealth() const {
		return this->max_health;
	}
	void addHealth(int v) {
		this->health += v;
		if( this->health > this->max_health )
			this->health = this->max_health;
	}
	Gigalomania::Image **getImages();
	void rotateDefenders();
	int getNDefenders() const {
		int n = 0;
		for(int i=0;i<n_turrets;i++) {
			if( turret_man[i] != -1 )
				n++;
		}
		return n;
	}
	int getNDefenders(int type) const {
		int n = 0;
		for(int i=0;i<n_turrets;i++) {
			if( turret_man[i] == type )
				n++;
		}
		return n;
	}
	int getDefenderStrength() const;
	void killIthDefender(int i);
	void killDefender(int index);
	PanelPage *getBuildingButton() const {
		return this->building_button;
	}
	int getNTurrets() const {
		return this->n_turrets;
	}
	int getTurretMan(int turret) const;
	int getTurretManFrame(int turret) const;
	PanelPage *getTurretButton(int turret) const;
	void clearTurretMan(int turret);
	void setTurretMan(int turret, int epoch);

	void saveState(stringstream &stream) const;
	void loadStateParseXMLNode(const TiXmlNode *parent);
};

class Sector {
	vector<Feature *> features;
	int xpos, ypos; // saved
	int epoch; // saved
	int player; // saved
	bool is_shutdown; // saved
	//int shutdown_player;

	bool nuked; // saved
	int nuke_by_player; // saved
	int nuke_time; // saved
	bool nuke_defence_animation; // saved
	int nuke_defence_time; // saved
	int nuke_defence_x; // saved
	int nuke_defence_y; // saved

	int population; // saved
	int n_designers; // saved
	int n_miners[N_ID]; // saved
	int n_builders[N_BUILDINGS]; // saved
	int n_workers; // saved
	int n_famount; // saved
	Design *current_design; // saved
	Design *current_manufacture; // saved
	int researched; // saved
	int researched_lasttime; // saved
	int manufactured; // saved
	int manufactured_lasttime; // saved
	int growth_lasttime; // saved
	int mined_lasttime; // saved
	int built_towers[n_players_c]; // for neutral sectors // saved
	int built[N_BUILDINGS]; // NB: built[BUILDING_TOWER] should never be used // saved
	int built_lasttime; // saved

	int elements[N_ID]; // elements remaining // saved
	int elementstocks[N_ID]; // elements mined // saved
	int partial_elementstocks[N_ID]; // saved

	void initTowerStuff();
	void consumeStocks(Design *design);

	int getInventionCost() const;
	int getManufactureCost() const;
	bool inventions_known[3][n_epochs_c]; // not saved, inferred fom designs
	vector<Design *> designs; // saved

	static int getBuildingCost(Type type, int building_player);
	void destroyBuilding(Type building_type,int client_player);
	void destroyBuilding(Type building_type,bool silent,int client_player);
	void updateWorkers();

	float getDefenceStrength() const;
	void doCombat(int client_player);
	void doPlayer(int client_player);

	Design *loadStateParseXMLDesign(const TiXmlAttribute *attribute);

	Building *buildings[N_BUILDINGS]; // saved
	Army *assembled_army;
	Army *stored_army; // saved
	Army *armies[n_players_c]; // saved
	int stored_defenders[n_epochs_c]; // saved
	int stored_shields[4]; // saved
	SmokeParticleSystem *smokeParticleSystem;
	SmokeParticleSystem *jetParticleSystem;
	SmokeParticleSystem *nukeParticleSystem;
	SmokeParticleSystem *nukeDefenceParticleSystem;

	PlayingGameState *gamestate;
public:

	Sector(PlayingGameState *gamestate, int epoch, int xpos, int ypos, MapColour map_colour);
	~Sector();

	void createTower(int player,int population);
	void destroyTower(bool nuked, int client_player);
	bool canShutdown() const;
	void shutdown(int client_player);
	bool isShutdown() const {
		return this->is_shutdown;
	}
	Building *getBuilding(Type type) const;
	int getNDefenders() const;
	int getNDefenders(int type) const;
	int getDefenderStrength() const;
	void killDefender(int index);
	bool canBuild(Type type) const;
	bool canMine(Id id) const;
	Design *canBuildDesign(Invention::Type type,int epoch) const;
	bool canBuildDesign(Design *design) const;
	bool canEverBuildDesign(Design *design) const;
	void autoTrashDesigns();
	bool tryMiningMore() const;
	bool usedUp() const;
	void cheat(int client_player); // for testing
	Design *knownDesign(Invention::Type type,int epoch) const;
	Design *bestDesign(Invention::Type type,int epoch) const;
	Design *canResearch(Invention::Type type,int epoch) const;
	int getNDesigns() const;
	//void consumeStocks(Design *design);
	bool assembleArmy(int epoch,int n);
	bool deployDefender(Building *building,int turret,int epoch);
	void returnDefender(Building *building,int turret);
	int getStoredDefenders(int epoch) const;
	bool useShield(Building *building,int shield);
	int getStoredShields(int shield) const;
	void update(int client_player);

	int getNFeatures() const {
		return this->features.size();
	}
	const Feature *getFeature(int i) const {
		return this->features.at(i);
	}
	const ParticleSystem *getSmokeParticleSystem() const {
		return this->smokeParticleSystem;
	}
	ParticleSystem *getSmokeParticleSystem() {
		return this->smokeParticleSystem;
	}
	const ParticleSystem *getJetParticleSystem() const {
		return this->jetParticleSystem;
	}
	ParticleSystem *getJetParticleSystem() {
		return this->jetParticleSystem;
	}
	const ParticleSystem *getNukeParticleSystem() const {
		return this->nukeParticleSystem;
	}
	ParticleSystem *getNukeParticleSystem() {
		return this->nukeParticleSystem;
	}
	const ParticleSystem *getNukeDefenceParticleSystem() const {
		return this->nukeDefenceParticleSystem;
	}
	ParticleSystem *getNukeDefenceParticleSystem() {
		return this->nukeDefenceParticleSystem;
	}
	void setEpoch(int epoch);
	int getEpoch() const;
	int getBuildingEpoch() const;
	int getXPos() const {
		return xpos;
	}
	int getYPos() const {
		return ypos;
	}

	int getPlayer() const;
	int getActivePlayer() const;

	void setCurrentDesign(Design *current_design);
	const Design *getCurrentDesign() const;
	void setCurrentManufacture(Design *current_manufacture);
	const Design *getCurrentManufacture() const;
	void inventionTimeLeft(int *halfdays,int *hours) const;
	void manufactureTotalTime(int *halfdays,int *hours) const;
	void manufactureTimeLeft(int *halfdays,int *hours) const;
	bool inventionKnown(Invention::Type type,int epoch) const;
	void trashDesign(Invention *invention);
	void trashDesign(Design *design);
	bool nukeSector(Sector *source);
	int beingNuked(int *nuke_time) const {
		*nuke_time = this->nuke_time;
		return nuke_by_player;
	}
	bool isBeingNuked() const {
		return nuke_by_player != -1;
	}
	bool isNuked() const {
		return this->nuked;
	}
	void getNukePos(int *nuke_x, int *nuke_y) const;
	bool hasNuclearDefenceAnimation(int *nuke_defence_time, int *nuke_defence_x, int *nuke_defence_y) const {
		*nuke_defence_time = this->nuke_defence_time;
		*nuke_defence_x = this->nuke_defence_x;
		*nuke_defence_y = this->nuke_defence_y;
		return nuke_defence_animation;
	}

	void setElements(Id id,int n_elements);
	void getElements(int *n,int *fraction,Id id) const;
	bool anyElements(Id id) const;
	void reduceElementStocks(Id id,int reduce);
	void getElementStocks(int *n,int *fraction,Id id) const;
	void getTotalElements(int *n,int *fraction,Id id) const;

	void buildingTowerTimeLeft(int player,int *halfdays,int *hours) const;
	void buildingTimeLeft(Type type,int *halfdays,int *hours) const;

	void setPopulation(int population);
	void setDesigners(int n_designers);
	void setWorkers(int n_workers);
	void setFAmount(int n_famount);
	void setMiners(Id id,int n_miners);
	void setBuilders(Type type,int n_builders);

	int getPopulation() const;
	int getSparePopulation() const;
	int getAvailablePopulation() const;
	int getDesigners() const;
	int getWorkers() const;
	int getFAmount() const;
	int getMiners(Id id) const;
	int getBuilders(Type type) const;
	const Army *getAssembledArmy() const {
		return assembled_army;
	}
	Army *getAssembledArmy() {
		return assembled_army;
	}
	const Army *getStoredArmy() const {
		return stored_army;
	}
	Army *getStoredArmy() {
		return stored_army;
	}
	const Army *getArmy(int player) const;
	Army *getArmy(int player);
	bool enemiesPresent() const;
	bool enemiesPresentWithBombardment() const;
	bool enemiesPresent(int player) const;
	bool enemiesPresent(int player,bool include_unarmed) const;
	void returnAssembledArmy();
	bool returnArmy();
	bool returnArmy(Army *army);
	bool moveArmy(Army *army);
	void evacuate();
	void assembleAll(bool include_unarmed);
	bool mineElement(int client_player, Id i);
	void invent(int client_player);
	void buildDesign();
	void buildBuilding(Type type);
	void updateForNewBuilding(Type type);

	void saveState(stringstream &stream) const;
	void loadStateParseXMLNode(const TiXmlNode *parent);

	void printDebugInfo() const;
};
