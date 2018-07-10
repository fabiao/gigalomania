//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <cstdlib>

#include <stdexcept> // needed for Android at least

#include <sstream>
#include <algorithm>

#include "player.h"
#include "game.h"
#include "gamestate.h"
#include "utils.h"
#include "sector.h"
#include "tutorial.h"
//---------------------------------------------------------------------------

bool Player::alliances[n_players_c][n_players_c];
int Player::alliance_last_asked[n_players_c][n_players_c];

const char *PlayerType::getName(PlayerTypeID id) {
	if( id == PLAYER_RED )
		return "RED TEAM";
	else if( id == PLAYER_GREEN )
		return "GREEN TEAM";
	else if( id == PLAYER_YELLOW )
		return "YELLOW TEAM";
	else if( id == PLAYER_BLUE )
		return "BLUE TEAM";
	ASSERT(false);
	return NULL;
}

void PlayerType::getColour(int *r,int *g,int *b,PlayerTypeID id) {
	*r = *g = *b = 0;
	if( id == PLAYER_RED ) {
		*r = 145;
		*g = 36;
		*b = 34;
	}
	else if( id == PLAYER_GREEN ) {
		*r = 67;
		*g = 160;
		*b = 71;
	}
	else if( id == PLAYER_YELLOW ) {
		*r = 209;
		*g = 184;
		*b = 0;
	}
	else if( id == PLAYER_BLUE ) {
		*r = 63;
		*g = 81;
		*b = 181;
	}
	else {
		ASSERT(false);
	}
}

//Player::Player(int index, char *name) {
//Player::Player(int index, int personality) {
Player::Player(bool is_human, int index) :
index(index), dead(false), n_births(0), n_deaths(0), n_men_for_this_island(0), n_suspended(0), is_human(is_human), alliance_last_asked_human(-1)
{
	for(int i=0;i<n_players_c;i++) {
		if( i != index && game_g->players[i] != NULL && !game_g->players[i]->isDead() ) {
			setAlliance(index, i, false);
			setAllianceLastAsked(index, i, -1);
		}
	}
}

Player::~Player() {
}

void Player::saveState(stringstream &stream) const {
	stream << "<player ";
	stream << "player_id=\"" << index << "\" ";
	stream << "dead=\"" << (dead?1:0) << "\" ";
	stream << "n_births=\"" << n_births << "\" ";
	stream << "n_deaths=\"" << n_deaths << "\" ";
	stream << "n_men_for_this_island=\"" << n_men_for_this_island << "\" ";
	stream << "n_suspended=\"" << n_suspended << "\" ";
	stream << "alliance_last_asked_human=\"" << alliance_last_asked_human << "\" ";
	stream << ">\n";
	stream << "</player>\n";
}

void Player::loadStateParseXMLNode(const TiXmlNode *parent) {
	if( parent == NULL ) {
		return;
	}
	bool read_children = true;

	switch( parent->Type() ) {
		case TiXmlNode::TINYXML_DOCUMENT:
			break;
		case TiXmlNode::TINYXML_ELEMENT:
			{
				const char *element_name = parent->Value();
				const TiXmlElement *element = parent->ToElement();
				const TiXmlAttribute *attribute = element->FirstAttribute();
				if( strcmp(element_name, "player") == 0 ) {
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "player_id") == 0 ) {
							// handled by caller
						}
						else if( strcmp(attribute_name, "dead") == 0 ) {
							dead = atoi(attribute->Value())==1;
						}
						else if( strcmp(attribute_name, "n_births") == 0 ) {
							n_births = atoi(attribute->Value());
						}
						else if( strcmp(attribute_name, "n_deaths") == 0 ) {
							n_deaths = atoi(attribute->Value());
						}
						else if( strcmp(attribute_name, "n_men_for_this_island") == 0 ) {
							n_men_for_this_island = atoi(attribute->Value());
						}
						else if( strcmp(attribute_name, "n_suspended") == 0 ) {
							n_suspended = atoi(attribute->Value());
						}
						else if( strcmp(attribute_name, "alliance_last_asked_human") == 0 ) {
							alliance_last_asked_human = atoi(attribute->Value());
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown player/player attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
				}
				else {
					// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
					LOG("unknown player tag: %s\n", element_name);
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
			}
			break;
		case TiXmlNode::TINYXML_DECLARATION:
			break;
	}

	for(const TiXmlNode *child=parent->FirstChild();child!=NULL && read_children;child=child->NextSibling())  {
		loadStateParseXMLNode(child);
	}
}

void Player::saveStateAlliances(stringstream &stream) {
	stream << "<player_alliances>\n";
	for(int i=0;i<n_players_c;i++) {
		for(int j=i+1;j<n_players_c;j++) {
			stream << "<player_alliance ";
			stream << "player_id_i=\"" << i << "\" ";
			stream << "player_id_j=\"" << j << "\" ";
			stream << "alliances=\"" << (alliances[i][j] ? 1 : 0) << "\" ";
			stream << "alliance_last_asked=\"" << alliance_last_asked[i][j] << "\" ";
			stream << "/>\n";
		}
	}
	stream << "</player_alliances>\n";
}

void Player::loadStateParseXMLNodeAlliances(const TiXmlNode *parent) {
	if( parent == NULL ) {
		return;
	}
	bool read_children = true;

	switch( parent->Type() ) {
		case TiXmlNode::TINYXML_DOCUMENT:
			break;
		case TiXmlNode::TINYXML_ELEMENT:
			{
				const char *element_name = parent->Value();
				const TiXmlElement *element = parent->ToElement();
				const TiXmlAttribute *attribute = element->FirstAttribute();
				if( strcmp(element_name, "player_alliances") == 0 ) {
					// handled entirely by caller
				}
				else if( strcmp(element_name, "player_alliance") == 0 ) {
					int player_id_i = -1;
					int player_id_j = -1;
					bool alliance = false;
					int last_asked = -1;
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "player_id_i") == 0 ) {
							player_id_i = atoi(attribute->Value());
						}
						else if( strcmp(attribute_name, "player_id_j") == 0 ) {
							player_id_j = atoi(attribute->Value());
						}
						else if( strcmp(attribute_name, "alliances") == 0 ) {
							alliance = atoi(attribute->Value())==1;
						}
						else if( strcmp(attribute_name, "alliance_last_asked") == 0 ) {
							last_asked = atoi(attribute->Value());
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown player_alliances/player_alliance attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
					if( player_id_i < 0 || player_id_i >= n_players_c ) {
						throw std::runtime_error("player_alliance invalid player_id_i");
					}
					else if( player_id_j < 0 || player_id_j >= n_players_c ) {
						throw std::runtime_error("player_alliance invalid player_id_j");
					}
					alliances[player_id_i][player_id_j] = alliance;
					alliance_last_asked[player_id_i][player_id_j] = last_asked;
				}
				else {
					// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
					LOG("unknown player_alliances tag: %s\n", element_name);
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
			}
			break;
		case TiXmlNode::TINYXML_DECLARATION:
			break;
	}

	for(const TiXmlNode *child=parent->FirstChild();child!=NULL && read_children;child=child->NextSibling())  {
		loadStateParseXMLNodeAlliances(child);
	}
}

void Player::setAlliance(int a, int b, bool alliance) {
	LOG("Alliance %s between players %d and %d\n", alliance?"MADE":"BROKEN", a, b);
	ASSERT(a != b);
	ASSERT_PLAYER(a);
	ASSERT_PLAYER(b);
	if( a > b ) {
		int dummy = a;
		a = b;
		b = dummy;
	}
	alliances[a][b] = alliance;
}

bool Player::isAlliance(int a, int b) {
	ASSERT(a != b);
	ASSERT_PLAYER(a);
	ASSERT_PLAYER(b);
	if( a > b ) {
		int dummy = a;
		a = b;
		b = dummy;
	}
	return alliances[a][b];
}

void Player::resetAllAlliances() {
	for(int i=0;i<n_players_c;i++) {
		for(int j=i+1;j<n_players_c;j++) {
			setAlliance(i, j, false);
			setAllianceLastAsked(i, j, -1);
		}
	}
}

void Player::setAllianceLastAsked(int a, int b,int time) {
	ASSERT(a != b);
	ASSERT_PLAYER(a);
	ASSERT_PLAYER(b);
	if( a > b ) {
		int dummy = a;
		a = b;
		b = dummy;
	}
	alliance_last_asked[a][b] = time;
}

int Player::allianceLastAsked(int a, int b) {
	ASSERT(a != b);
	ASSERT_PLAYER(a);
	ASSERT_PLAYER(b);
	if( a > b ) {
		int dummy = a;
		a = b;
		b = dummy;
	}
	return alliance_last_asked[a][b];
}

bool Player::askHuman() {
	const int wait_time_human_c = 5000;
	//ASSERT( !isAlliance(index, human_player) );
	//ASSERT( index != human_player );
	ASSERT( !this->is_human );
	// whether to _consider_ asking the human player - we still have to go through the requestAlliance test afterwards
	if( game_g->getTutorial() != NULL && !game_g->getTutorial()->aiAllowAskAlliance() ) {
		return false;
	}
	int time = game_g->getGameTime();
	if( alliance_last_asked_human == -1 ) {
		// note - don't ask if alliance_last_asked_human==-1, to avoid being asked straight away
		alliance_last_asked_human = time;
	}
	else if( time >= alliance_last_asked_human + wait_time_human_c ) {
		alliance_last_asked_human = time;
		if( rand() % 2 == 0 )
		{
			return true;
		}
	}
	return false;
}

bool Player::requestAlliance(int player) {
	// 'player' requests alliance with 'this'
	const int wait_time_c = 5000;
	ASSERT( !isAlliance(index, player) );
	//ASSERT( index != player );
	//ASSERT(this->index != human_player);
	ASSERT( !this->is_human );
	int last_asked = Player::allianceLastAsked(index, player);
	int time = game_g->getGameTime();
	if( last_asked == -1 || time >= last_asked + wait_time_c ) {
		Player::setAllianceLastAsked(index, player, time);
		bool has_diplomatic_bonus = player == PlayerType::PLAYER_YELLOW;
		if( has_diplomatic_bonus ? (rand() % 2 == 0)  : (rand() % 3 == 0) )
		{
			return true;
		}
		//return true;
	}
	return false;
}

//int Player::getShieldIndex() {
int Player::getShieldIndex(bool allied[n_players_c]) {
	int rtn = 0;
	/*if(personality == 0)
	rtn = 2;
	else if(personality == 1)
	rtn = 1;
	else if(personality == 2)
	rtn = 4;
	else if(personality == 3)
	rtn = 3;*/
	int index = 0;
	for(int i=0,c=1;i<n_players_c;i++,c*=2) {
		if( allied[i] ) {
			index += c;
		}
	}
	/*if(index == 0)
	rtn = 2;
	else if(index == 1)
	rtn = 1;
	else if(index == 2)
	rtn = 4;
	else if(index == 3)
	rtn = 3;*/
	if( index == 0 )
		rtn = 0;
	else if( index == 1 )
		rtn = 2;
	else if( index == 2 )
		rtn = 1;
	else if( index == 3 )
		rtn = 8;
	else if( index == 4 )
		rtn = 4;
	else if( index == 5 )
		rtn = 7;
	else if( index == 6 )
		rtn = 9;
	else if( index == 7 )
		rtn = 11;
	else if( index == 8 )
		rtn = 3;
	else if( index == 9 )
		rtn = 5;
	else if( index == 10 )
		rtn = 6;
	else if( index == 11 )
		rtn = 13;
	else if( index == 12 )
		rtn = 10;
	else if( index == 13 )
		rtn = 12;
	else if( index == 14 )
		rtn = 14;
	else {
		ASSERT(false);
	}
	return rtn;
}

void Player::kill(PlayingGameState *gamestate) {
	// break alliances
	for(int i=0;i<n_players_c;i++) {
		if( i != this->index )
			Player::setAlliance(i, index, false);
	}

	/*if( ((PlayingGameState *)gamestate)->getPlayerAskingAlliance() == this->index ) {
		((PlayingGameState *)gamestate)->cancelPlayerAskingAlliance();
	}*/
	if( gamestate->getPlayerAskingAlliance() == this->index ) {
		gamestate->cancelPlayerAskingAlliance();
	}

	this->dead = true;

	// check for only being one side
	bool one_side = true;
	for(int i=0;i<n_players_c && one_side;i++) {
		if( game_g->players[i] != NULL && !game_g->players[i]->isDead() ) {
			for(int j=0;j<n_players_c && one_side;j++) {
				if( game_g->players[j] != NULL && !game_g->players[j]->isDead() ) {
					if( i != j && !Player::isAlliance(i,j) ) {
						one_side = false;
					}
				}
			}
		}
	}
	if( one_side ) {
		// break all alliances
		for(int i=0;i<n_players_c;i++) {
			if( game_g->players[i] != NULL && !game_g->players[i]->isDead() ) {
				for(int j=0;j<n_players_c && one_side;j++) {
					if( game_g->players[j] != NULL && !game_g->players[j]->isDead() ) {
						if( i != j ) {
							Player::setAlliance(i, j, false);
						}
					}
				}
			}
		}
	}

	//gamestate->reset(); // needed to update player shield buttons
	//((PlayingGameState *)gamestate)->resetShieldButtons(); // needed to update player shield buttons
	gamestate->resetShieldButtons(); // needed to update player shield buttons
}

void Player::doSectorAI(int client_player, PlayingGameState *gamestate, Sector *sector) {
	const int MIN_POP = 5;
	const int EVACUATE_LEVEL = 3;

	ASSERT( this->index == sector->getPlayer() );

	// reset to zero
	sector->setDesigners( 0 );
	for(int i=0;i<N_ID;i++) {
		if( game_g->elements[i]->getType() != Element::GATHERABLE )
			sector->setMiners((Id)i, 0);
	}
	sector->setWorkers( 0 );
	for(int i=0;i<N_BUILDINGS;i++) {
		sector->setBuilders((Type)i, 0);
	}

	bool enemiesPresent = sector->enemiesPresent();
	bool enemiesPresentWithBombardment = sector->enemiesPresentWithBombardment();

	int attack_pref[n_players_c];
	{
		int attack_order[n_players_c-1];
		int choose_from[n_players_c-1];
		for(int i=0,indx=0;i<n_players_c;i++) {
			if( i != index ) {
				choose_from[indx++] = i;
			}
		}
		for(int i=0;i<n_players_c-1;i++) {
			int n_choose_from = n_players_c - 1 - i;
			int c = rand() % n_choose_from;
			attack_order[i] = choose_from[c];
			choose_from[c] = choose_from[n_choose_from-1];
		}
		attack_pref[index] = 0;
		for(int i=0;i<n_players_c-1;i++) {
			attack_pref[ attack_order[i] ] = i+1;
		}
	}

	if( enemiesPresentWithBombardment && sector->getBuilding(BUILDING_TOWER)->getHealth() <= EVACUATE_LEVEL ) {
		// evacuate!
		sector->evacuate();
		/*for(int i=0;i<N_BUILDINGS;i++) {
			Building *building = sector->getBuilding((Type)i);
			if( building != NULL ) {
				for(int j=0;j<building->getNTurrets();j++) {
					if( building->getTurretMan(j) != -1 )
						sector->returnDefender(building, j);
				}
			}
		}
		int men = sector->getAvailablePopulation();
		if( men > 0 ) {
			sector->getAssembledArmy()->add(n_epochs_c, men);
			int n_pop = sector->getPopulation() - men;
			sector->setPopulation(n_pop);
		}
		sector->getArmy(this->index)->add(sector->getAssembledArmy());*/
	}

	// recall army
	if( !enemiesPresent && sector->getArmy(this->index)->getTotal() > 0 ) {
		sector->returnArmy();
	}

	if( sector->getPopulation() < MIN_POP && !enemiesPresent )
		return; // reproduce

	// defenders
	for(int i=n_epochs_c-1;i>=0 /*&& enemiesPresent*/;i--) {
		/*if( i == nuclear_epoch_c )
		continue;*/
		if( !enemiesPresentWithBombardment && i != nuclear_epoch_c && i != laser_epoch_c )
			continue; // don't put out defenders if no enemies present, except for nuclear defence or laser
		if( enemiesPresent && i == nuclear_epoch_c )
			continue; // don't put out new nuclear defences if under attack
		/*if( i == laser_epoch_c ) // test - disable lasers to test nuke defence
		continue;*/
		for(;;) {
			if( i == nuclear_epoch_c ) {
				int n_lasers = sector->getNDefenders(laser_epoch_c);
				int n_nuke_defs = sector->getNDefenders(nuclear_epoch_c);
				ASSERT(n_nuke_defs <= 1);
				if( n_lasers > 0 || n_nuke_defs > 0 )
					break; // don't need a nuclear defence
			}
			Design *design = NULL;
			if( sector->getStoredDefenders(i) == 0 ) {
				design = sector->canBuildDesign(Invention::DEFENCE, i);
				if( design == NULL )
					break;
			}
			// we can make this defence
			bool placed = false;
			for(int j=0;j<N_BUILDINGS && !placed && sector->getAvailablePopulation() > MIN_POP;j++) {
				Building *building = sector->getBuilding((Type)j);
				if( building != NULL ) {
					for(int k=0;k<building->getNTurrets() && !placed;k++) {
						if( building->getTurretMan(k) == -1 ) {
							// new defender
							placed = sector->deployDefender(building, k, i);
						}
					}
				}
			}
			// try again, replacing poorer defences this time
			for(int j=0;j<N_BUILDINGS && !placed;j++) {
				Building *building = sector->getBuilding((Type)j);
				if( building != NULL ) {
					for(int k=0;k<building->getNTurrets() && !placed;k++) {
						if( building->getTurretMan(k) != -1 && building->getTurretMan(k) < i ) {
							// replace defender
							placed = sector->deployDefender(building, k, i);
						}
					}
				}
			}
			if( !placed ) {
				// couldn't place it
				if( i >= factory_epoch_c && sector->getStoredDefenders(i) == 0 &&
					sector->getCurrentManufacture() == NULL ) {
						// manufacture one
						sector->setCurrentManufacture(design);
						sector->setFAmount(1);

				}
				break;
			}
		}
	}

	if( !enemiesPresentWithBombardment ) {
		// return defenders (except nuclear defence and lasers)
		for(int j=0;j<N_BUILDINGS;j++) {
			Building *building = sector->getBuilding((Type)j);
			if( building != NULL ) {
				//for(int k=0;k<max_building_turrets_c;k++) {
				for(int k=0;k<building->getNTurrets();k++) {
					if( building->getTurretMan(k) != -1 && building->getTurretMan(k) != nuclear_epoch_c && building->getTurretMan(k) != laser_epoch_c ) {
						/*sector->stored_defenders[ building->turret_man[k] ]++;
						building->turret_man[k] = -1;
						int n_pop = sector->getPopulation() + 1;
						sector->setPopulation(n_pop);*/
						sector->returnDefender(building, k);
					}
				}
			}
		}
	}

	if( sector->getCurrentManufacture() == NULL ) {
		// defences handled already, above
		for(int i=game_g->getNSubEpochs()-1;i>=0;i--) {
			if( game_g->getStartEpoch() + i == nuclear_epoch_c )
				continue; // nuclear weapons handled later
			if( game_g->getStartEpoch() + i >= factory_epoch_c ) {
				Design *design = sector->canBuildDesign(Invention::WEAPON, game_g->getStartEpoch() + i);
				if( design != NULL ) {
					sector->setCurrentManufacture(design);
					sector->setFAmount(1);
					break;
				}
			}
		}
	}

	// use shields?
	for(int i=0;i<N_BUILDINGS;i++) {
		Building *building = sector->getBuilding((Type)i);
		if( building == NULL )
			continue;
		if( building->getHealth() < 30 ) {
			bool healed = false;
			for(int j=3;j>=0 && !healed;j--) {
				healed = sector->useShield(building, j);
			}
			if( !healed && sector->getCurrentManufacture() != NULL ) {
				// manufacture a shield
				for(int j=game_g->getNSubEpochs()-1;j>=0;j--) {
					Design *design = sector->canBuildDesign(Invention::SHIELD, game_g->getStartEpoch() + j);
					if( design != NULL ) {
						sector->setCurrentManufacture(design);
						sector->setFAmount(1);
						break;
					}
				}
			}
		}
	}

	// nuke a sector?
    while( sector->getStoredArmy()->getSoldiers(nuclear_epoch_c) > 0 || sector->getCurrentManufacture() == NULL ) {
		// look for tower to nuke
		Sector *nuke_sector = NULL;
		bool found_tower = false;
		for(int x=0;x<map_width_c;x++) {
			for(int y=0;y<map_height_c;y++) {
				if( game_g->getMap()->isSectorAt(x, y) ) {
					Sector *c_sector = game_g->getMap()->getSector(x, y);
					int this_strength = c_sector->getArmy(sector->getPlayer())->getStrength();
					if( this_strength > 0 && ( !enemiesPresentWithBombardment || sector->getBuilding(BUILDING_TOWER)->getHealth() > EVACUATE_LEVEL ) ) {
						// only nuke our own men if this tower is under attack and nearly destroyed
						continue;
					}
					// prefer nuking towers to men
					if( c_sector->getActivePlayer() != -1 && c_sector->getPlayer() != sector->getPlayer() && !Player::isAlliance(c_sector->getPlayer(), sector->getPlayer()) ) {
						if( nuke_sector == NULL || !found_tower || attack_pref[c_sector->getPlayer()] > attack_pref[nuke_sector->getPlayer()] ) {
							nuke_sector = c_sector;
							found_tower = true;
						}
					}
					else if( !found_tower && c_sector->getActivePlayer() == -1 && c_sector->enemiesPresent(sector->getPlayer()) && !c_sector->getArmy(sector->getPlayer())->any(true) ) {
						if( nuke_sector == NULL || attack_pref[c_sector->getPlayer()] > attack_pref[nuke_sector->getPlayer()] ) {
							nuke_sector = c_sector;
						}
					}
				}
			}
		}
		if( nuke_sector == NULL ) {
			break;
		}
		if( sector->getStoredArmy()->getSoldiers(nuclear_epoch_c) > 0 && !sector->isBeingNuked() ) {
			// nuke
			nuke_sector->nukeSector(sector);
			sector->getStoredArmy()->remove(nuclear_epoch_c, 1);
		}
		else {
			// start making a nuke, only if not already making something
            Design *design = sector->canBuildDesign(Invention::WEAPON, nuclear_epoch_c);
            if( design != NULL ) {
                sector->setCurrentManufacture(design);
                sector->setFAmount(1);
            }
			break;
		}
	}

	// trash designs?
	sector->autoTrashDesigns();

	if( game_g->getTutorial() != NULL && !game_g->getTutorial()->aiAllowDesign() ) {
		// don't allow designs
	}
	else if( sector->getCurrentDesign() == NULL ) {
		// set new invention?
		int best_weapon = -1;
		int best_defence = -1;
		int best_shield = -1;
		for(int i=game_g->getNSubEpochs()-1;i>=0;i--) {
			if( best_weapon == -1 && sector->inventionKnown(Invention::WEAPON, game_g->getStartEpoch() + i) )
				best_weapon = game_g->getStartEpoch() + i;
			if( best_defence == -1 && sector->inventionKnown(Invention::DEFENCE, game_g->getStartEpoch() + i) )
				best_defence = game_g->getStartEpoch() + i;
			if( best_shield == -1 && sector->inventionKnown(Invention::SHIELD, game_g->getStartEpoch() + i) )
				best_shield = game_g->getStartEpoch() + i;
		}
		Design *design = NULL;
		Design *reserve_design = NULL;
		// prefer nuke!
		design = sector->canResearch(Invention::WEAPON, nuclear_epoch_c);

		bool try_mining_more = sector->tryMiningMore();
		for(int i=0;i<game_g->getNSubEpochs() && design == NULL;i++) {
			int eph = game_g->getStartEpoch() + i;
			Design *this_design = NULL;

			this_design = sector->canResearch(Invention::WEAPON, eph);
			if( eph > best_weapon )
				design = this_design;
			else if( reserve_design == NULL )
				reserve_design = this_design;

			if( design == NULL ) {
				this_design = sector->canResearch(Invention::DEFENCE, eph);
				if( eph > best_defence )
					design = this_design;
				else if( reserve_design == NULL )
					reserve_design = this_design;
			}
			if( design == NULL && ( best_weapon != -1 || best_defence != -1 || !try_mining_more ) ) {
				// (only design shield if we've already designed either a weapon or a defence, or there's no point waiting)
				this_design = sector->canResearch(Invention::SHIELD, eph);
				if( eph > best_shield )
					design = this_design;
				else if( reserve_design == NULL )
					reserve_design = this_design;
			}
		}

		if( design == NULL && sector->getEpoch() < game_g->getStartEpoch() + 3 )
			design = reserve_design;

		if( design != NULL ) {
			sector->setCurrentDesign(design);
		}
	}

	bool used_up = game_g->getStartEpoch() != end_epoch_c && sector->usedUp();
	bool can_design = sector->getCurrentDesign() != NULL;
	bool can_mine = false;
	if( !used_up ) {
		// even if there are elements remaining to mine, we might still consider a sector "used up" if we've already mined at least 6 of that element, and we still can't design anything
		//
		for(int i=0;i<N_ID && !can_mine;i++) {
			if( sector->canMine((Id)i) && game_g->elements[i]->getType() != Element::GATHERABLE )
				can_mine = true;
		}
	}
	bool build_mine = sector->canBuild(BUILDING_MINE);
	bool build_factory = sector->canBuild(BUILDING_FACTORY);
	bool build_lab = sector->canBuild(BUILDING_LAB);
	bool need_workers = sector->getCurrentManufacture() != NULL;

	if( enemiesPresentWithBombardment && sector->getBuilding(BUILDING_TOWER)->getHealth() <= EVACUATE_LEVEL+1 )
		can_design = false;
	if( enemiesPresentWithBombardment && sector->getBuilding(BUILDING_TOWER)->getHealth() <= EVACUATE_LEVEL+1 )
		can_mine = false;
	if( enemiesPresentWithBombardment && sector->getBuilding(BUILDING_TOWER)->getHealth() <= EVACUATE_LEVEL+1 )
		need_workers = false;

	if( enemiesPresentWithBombardment && sector->getBuilding(BUILDING_TOWER)->getHealth() <= EVACUATE_LEVEL+1 ) {
		build_mine = false;
		build_factory = false;
		build_lab = false;
	}

	int split = 1;
	int design_weight = 0;
	int work_weight = 0;
	if( can_design  ) {
		design_weight = 1;
		if( sector->getCurrentDesign()->getInvention()->getEpoch() == nuclear_epoch_c )
			design_weight = 4; // rush!
		split += design_weight;
	}
	if( can_mine )
		split++;
	if( build_mine )
		split++;
	if( build_factory )
		split++;
	if( build_lab )
		split++;
	if( need_workers  ) {
		work_weight = 1;
		if( sector->getCurrentManufacture()->getInvention()->getEpoch() == nuclear_epoch_c )
			work_weight = 4; // rush!
		split += work_weight;
	}

	int pop = sector->getAvailablePopulation();

	if( can_design ) {
		sector->setDesigners((design_weight * pop) / split);
	}
	if( can_mine ) {
		int n_miners = 0;
		while( n_miners < pop/split ) {
			for(int i=0;i<N_ID && n_miners < pop/split;i++) {
				if( sector->canMine((Id)i) && game_g->elements[i]->getType() != Element::GATHERABLE ) {
					int n = sector->getMiners((Id)i) + 1;
					sector->setMiners((Id)i, n);
					n_miners++;
				}
			}
		}
	}
	if( build_mine ) {
		sector->setBuilders(BUILDING_MINE, pop / split);
	}
	if( build_factory ) {
		sector->setBuilders(BUILDING_FACTORY, pop / split);
	}
	if( build_lab ) {
		sector->setBuilders(BUILDING_LAB, pop / split);
	}
	if( need_workers ) {
		sector->setWorkers((work_weight * pop) / split);
	}


	if( game_g->getTutorial() != NULL && !game_g->getTutorial()->aiAllowDeploy() ) {
		// don't allow deployment
	}
	else if( sector->getCurrentDesign() == NULL || enemiesPresentWithBombardment ) {
		//if( used_up || enemiesPresentWithBombardment ) {
		// think about attacking?
		Sector *target_sector = NULL;
		bool by_land = false;
		bool new_sector = false;
		int strength = 0;
		bool temp[map_width_c][map_height_c];
		game_g->getMap()->canMoveTo(temp, sector->getXPos(),sector->getYPos(),sector->getPlayer());

		// if used up, look for a new sector
		bool look_for_new_sector = used_up || ( rand() % 3 == 0 );
		if( look_for_new_sector ) {
			vector<Sector *> candidate_sectors;
			int max_n_men = 0;
			for(int x=0;x<map_width_c;x++) {
				for(int y=0;y<map_height_c;y++) {
					Sector *c_sector = game_g->getMap()->getSector(x, y);
					if( c_sector == NULL )
						continue;
					// Only worth moving to a sector that has no other players
					// If we were to move to a sector with an ally army, the army would then be immediately moved back to the tower by code in Player::doSectorAI() (this bug was fixed in 0.28)
					// No need to move to a sector with enemies - the decision to attack is done below
					if( c_sector->getActivePlayer() == -1 ) {
						if( temp[x][y] ) {
							bool has_others = false;
							for(int i=0;i<n_players_c && !has_others;i++) {
								if( i != index && c_sector->getArmy(i)->any(true) ) {
									has_others = true;
								}
							}
							if( !has_others ) {
								int n_men = c_sector->getArmy(index)->getTotal();
								// prefer sectors that already have our men - and pick the largest number
								if( n_men >= max_n_men ) {
									if( n_men > max_n_men ) {
										candidate_sectors.clear();
										max_n_men = n_men;
									}
									candidate_sectors.push_back(c_sector);
								}
							}
						}
					}
				}
			}
			if( candidate_sectors.size() > 0 ) {
				// randomly pick out of the candidate sectors
				int r = rand() % candidate_sectors.size();
				target_sector = candidate_sectors.at(r);
				by_land = true;
				new_sector = true;
			}
		}

		// look for tower to attack
		for(int x=0;x<map_width_c;x++) {
			for(int y=0;y<map_height_c;y++) {
				Sector *c_sector = game_g->getMap()->getSector(x, y);
				if( c_sector == NULL )
					continue;
				if( c_sector->getActivePlayer() != -1 && c_sector->getPlayer() != sector->getPlayer()
					&& !Player::isAlliance(c_sector->getPlayer(), sector->getPlayer())
					&& !new_sector // only consider attacking if aren't moving to a new sector
					) {
						//int this_strength = c_sector->getArmy(sector->getPlayer())->getTotal();
						int this_strength = c_sector->getArmy(sector->getPlayer())->getStrength();
						if( target_sector == NULL // we haven't chosen anywhere yet
							|| ( temp[x][y] && !by_land ) // prefer by land over by air
							|| ( temp[x][y] == by_land && this_strength > strength ) // prefer where we're already attacking
							|| ( temp[x][y] == by_land && this_strength == strength && attack_pref[c_sector->getPlayer()] > attack_pref[target_sector->getPlayer()] ) // prefer a particular player
							) {
								target_sector = c_sector;
								strength = this_strength;
								by_land = temp[x][y];
						}
				}
			}
		}
		if( target_sector == NULL ) {
			// look for men to attack
			for(int x=0;x<map_width_c;x++) {
				for(int y=0;y<map_height_c;y++) {
					Sector *c_sector = game_g->getMap()->getSector(x, y);
					if( c_sector == NULL )
						continue;
					bool enemy = false;
					for(int i=0;i<n_players_c && !enemy;i++) {
						if( i != sector->getPlayer() && c_sector->getArmy(i)->getTotal() > 0 &&
							!Player::isAlliance(i, sector->getPlayer()) )
							enemy = true;
					}
					if( enemy ) {
						//int this_strength = c_sector->getArmy(sector->getPlayer())->getTotal();
						int this_strength = c_sector->getArmy(sector->getPlayer())->getStrength();
						if( target_sector == NULL ||
							( temp[x][y] && !by_land ) ||
							( temp[x][y] == by_land && this_strength > strength ) ) {
								target_sector = c_sector;
								by_land = temp[x][y];
								strength = this_strength;
						}
					}
				}
			}
		}
		if( enemiesPresentWithBombardment && !used_up ) {
			target_sector = sector;
			by_land = true;
			new_sector = false;
		}

		if( target_sector != NULL ) {
			int min_pop = MIN_POP;
			//if( enemiesPresent && sector->getBuilding(BUILDING_TOWER)->health <= EVACUATE_LEVEL+1 )
			//if( enemiesPresent || new_sector )
			if( enemiesPresent || used_up )
				min_pop = 0;
			for(int i=n_epochs_c-1;i>=game_g->getStartEpoch();i--) {
				if( i == nuclear_epoch_c )
					continue;
				if( !by_land && !isAirUnit(i) )
					continue;
				while(sector->getAvailablePopulation() > min_pop) {
					if( !sector->assembleArmy(i, 1) )
						break;
				}
			}
			/*if( new_sector ) {
			// add unarmed men
			while(sector->getAvailablePopulation() > min_pop) {
			if( !sector->assembleArmy(n_epochs_c, 1) )
			break;
			}
			}*/

			//int assembled_strength = sector->getAssembledArmy()->getTotal();
			int assembled_strength = sector->getAssembledArmy()->getStrength();
			//int min_req = 8;
			//int min_req = 4 * (game_g->getStartEpoch()+1);
			int min_req = 8 * (game_g->getStartEpoch()+1);
			min_req = std::min(min_req, 50);
			for(int i=0;i<=game_g->getStartEpoch();i++)
				min_req *= 2;
			if( used_up ) {
				// no point waiting
				min_req = 0;
			}

			/*if( by_land && sector->getCurrentDesign() == NULL && sector->getCurrentManufacture() == NULL &&
			( enemiesPresent || assembled_strength > min_req || game_g->getStartEpoch() == end_epoch_c ) ) {
			// only use unarmed men if we aren't designing or manufacturing anything
			// and either we are under attack, or we are able to destroy enemy buildings, or moving to a new sector
			// assemble unarmed men*/
			if( by_land && sector->getCurrentDesign() == NULL && sector->getCurrentManufacture() == NULL &&
				( enemiesPresentWithBombardment || game_g->getStartEpoch() == end_epoch_c || new_sector ) ) {
					// only use unarmed men if we aren't designing or manufacturing anything
					// and either we are under attack, or we are able to destroy enemy buildings, or we are moving to a new sector
					// assemble unarmed men
					int n_unarmed = sector->getAvailablePopulation() - min_pop;
					if( n_unarmed > 0 ) {
						int n_population = sector->getPopulation();
						sector->getAssembledArmy()->add(n_epochs_c, n_unarmed);
						sector->setPopulation( n_population - n_unarmed );
					}
			}

			if( sector->getAssembledArmy()->any(true) ) {
				assembled_strength = sector->getAssembledArmy()->getStrength();

				if( new_sector && used_up && sector->getStoredArmy()->any(false) && (sector->getSparePopulation() + sector->getAssembledArmy()->getTotalMen()) < 0.75f*max_grow_population_c ) {
					// If moving to a new sector because the sector is used up, we don't want to leave weapons behind - if we didn't have enough men to use them, better to wait until the population grows.
					// However we need to have the population not too close to the max_grow_population_c (otherwise we'll no longer be growing very much)
					// Arguably this logic should apply even if sending an army to attack (when not used up), but have to be careful - we don't want to make the AI take ages to decide to attack even though it has a reasonable strength.
					sector->returnAssembledArmy();
				}
				else if( strength + assembled_strength >= min_req || enemiesPresentWithBombardment ) {
					ASSERT( !target_sector->isNuked() );
					if( target_sector->getPlayer() == client_player && !target_sector->getArmy(this->index)->any(true) ) {
						game_g->setTimeRate(1); // auto-slow if attacking a player sector (but not if already being attacked by this player)
						gamestate->refreshTimeRate();
					}
					bool moved_all = target_sector->moveArmy(sector->getAssembledArmy());
					ASSERT(moved_all);
				}
				else {
					sector->returnAssembledArmy();
				}
			}
		}
	}
}


void Player::doAIUpdate(int client_player, PlayingGameState *gamestate) {
	if( game_g->players[index]->isDead() ) {
		return;
	}
	//LOG("Player::doAIUpdate()\n");

	int loop_time = game_g->getLoopTime();

	// TODO: currently breaking/making alliances is entirely random, should improve this...

	// break alliances
	int p_break_alliance = poisson(20000, loop_time);
	bool break_alliance = false;
	if( (rand() % RAND_MAX) <= p_break_alliance ) {
		for(int i=0;i<n_players_c;i++) {
			if( i != index && Player::isAlliance(i, index) ) {
				Player::setAlliance(i, index, false);
				break_alliance = true;
			}
		}
	}
	if( break_alliance ) {
		//gamestate->reset(); // reset shield buttons
		//((PlayingGameState *)gamestate)->resetShieldButtons(); // needed to update player shield buttons
		gamestate->resetShieldButtons(); // needed to update player shield buttons
	}

	// make alliances
	for(int i=0;i<n_players_c;i++) {
		if( i != this->index && game_g->players[i] != NULL && !game_g->players[i]->isDead() /*&& i != human_player*/ ) {
			//if( this->index == ((PlayingGameState *)gamestate)->getPlayerAskingAlliance() || i == ((PlayingGameState *)gamestate)->getPlayerAskingAlliance() ) {
			if( this->index == gamestate->getPlayerAskingAlliance() || i == gamestate->getPlayerAskingAlliance() ) {
				// one of these AIs is asking the player, so don't request
				continue;
			}
			// request alliance
			/*if( ((PlayingGameState *)gamestate)->canRequestAlliance(index, i) ) {
				((PlayingGameState *)gamestate)->requestAlliance(index, i, false);
			}*/
			if( gamestate->canRequestAlliance(index, i) ) {
				gamestate->requestAlliance(index, i, false);
			}
		}
	}

	// update sectors
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			Sector *sector = game_g->getMap()->getSector(x, y);
			if( sector == NULL )
				continue;
			if( sector->getActivePlayer() == this->index ) {
				//game_g->getMap()->sectors[x][y]->doAIUpdate();
				doSectorAI(client_player, gamestate, sector);
			}
			else {
				Army *army = sector->getArmy(index);
				if( !army->any(true) ) {
					// no army to move
				}
				else if( !army->canLeaveSafely() ) {
					// can't retreat safely
				}
				else {
					// at this point, we have an army, with no enemies present, so check if we can build here
					bool move = false;
					if( sector->isShutdown() )
						move = true;
					else if( sector->getPlayer() != -1 ) {
						ASSERT( Player::isAlliance(sector->getPlayer(), index) ); // must be true, otherwise we should not be able to retreat safely
						move = true;
					}
					for(int i=0;i<n_players_c && !move;i++) {
						if( i != index && sector->getArmy(i)->any(true) ) {
							ASSERT( Player::isAlliance(i, index) ); // must be true, otherwise we should not be able to retreat safely
							move = true;
						}
					}
					if( move ) {
						// find somewhere to move
						// TODO: move to attack players, if can't return to a tower
						bool done = false;
						for(int cx=0;cx<map_width_c && !done;cx++) {
							for(int cy=0;cy<map_height_c && !done;cy++) {
								Sector *c_sector = game_g->getMap()->getSector(cx, cy);
								if( c_sector != NULL && c_sector->getActivePlayer() == this->index ) {
									ASSERT( c_sector != sector );
									done = c_sector->moveArmy(army);
								}
							}
						}
					}
				}
			}
		}
	}
	//LOG("EXIT Player::doAIUpdate()\n");
}

int Player::getFinalMen() const {
	int final_men = n_men_for_this_island + n_births - n_deaths;
	ASSERT(final_men >= 0);
	return final_men;
}
