#include "stdafx.h"

#include "tutorial.h"
#include "game.h"
#include "gamestate.h"
#include "gui.h"
#include "utils.h"

vector<TutorialInfo> TutorialManager::getTutorialInfo() {
	vector<TutorialInfo> infos;
	infos.push_back( TutorialInfo("intro", "play your first island") );
	infos.push_back( TutorialInfo("army_maneuvers", "army maneuvers") );
	infos.push_back( TutorialInfo("build_manufacture", "learn to build and manufacture") );
	return infos;
}

Tutorial *TutorialManager::setupTutorial(const string &id) {
	Tutorial *tutorial = NULL;
	if( id == "intro" )
		tutorial = new Tutorial1(id);
	else if( id == "army_maneuvers" )
		tutorial = new Tutorial2(id);
	else if( id == "build_manufacture" )
		tutorial = new Tutorial3(id);
	return tutorial;
}

void GUIHandler::resetGUI(PlayingGameState *playing_gamestate) {
	playing_gamestate->getGamePanel()->setEnabled(true);
	playing_gamestate->getScreenPage()->setEnabled(true);
}

void GUIHandlerBlockAll::setGUI(PlayingGameState *playing_gamestate) const {
	playing_gamestate->getGamePanel()->setEnabled(false);
	playing_gamestate->getScreenPage()->setEnabled(false);
	for(vector<string>::const_iterator iter = exceptions.begin(); iter != exceptions.end(); ++iter) {
		const string exception = *iter;
		PanelPage *panel = playing_gamestate->getGamePanel()->findById(exception);
		if( panel != NULL ) {
			panel->setEnabled(true);
		}
		else {
			panel = playing_gamestate->getScreenPage()->findById(exception);
			if( panel != NULL ) {
				panel->setEnabled(true);
			}
		}
	}
	// the following are always exceptions
	PanelPage *pause_button = playing_gamestate->getScreenPage()->findById("pause_button");
	T_ASSERT( pause_button != NULL );
	if( pause_button != NULL ) {
		pause_button->setEnabled(true);
	}
	PanelPage *quit_button = playing_gamestate->getScreenPage()->findById("quit_button");
	T_ASSERT( quit_button != NULL );
	if( quit_button != NULL ) {
		quit_button->setEnabled(true);
	}
	PanelPage *speed_button = playing_gamestate->getScreenPage()->findById("speed_button");
	T_ASSERT( speed_button != NULL );
	if( speed_button != NULL ) {
		speed_button->setEnabled(true);
	}
}

bool TutorialCard::hasArrow(PlayingGameState *playing_gamestate) const {
	if( this->show_arrow_on_page && playing_gamestate->getGamePanel()->getPage() != this->show_arrow_page )
		return false;
	if( this->show_arrow_on_sector && ( playing_gamestate->getCurrentSector()->getXPos() != this->show_arrow_sector_x || playing_gamestate->getCurrentSector()->getYPos() != this->show_arrow_sector_y ) )
		return false;
	return has_arrow;
}

void TutorialCard::setGUI(PlayingGameState *playing_gamestate) const {
	playing_gamestate->getGamePanel()->setEnabled(true);
	playing_gamestate->getScreenPage()->setEnabled(true);
	if( gui_handler != NULL ) {
		gui_handler->setGUI(playing_gamestate);
	}
}

bool TutorialCardWaitForPanelPage::canProceed(PlayingGameState *playing_gamestate) const {
	int c_page = playing_gamestate->getGamePanel()->getPage();
	return c_page == wait_page;
}

bool TutorialCardWaitForDesign::canProceed(PlayingGameState *playing_gamestate) const {
	if( wait_type == WAITTYPE_CURRENT_DESIGN || wait_type == WAITTYPE_CURRENT_MANUFACTURE ) {
		const Design *current_design = wait_type == WAITTYPE_CURRENT_DESIGN ? sector->getCurrentDesign() : sector->getCurrentManufacture();
		if( current_design == NULL )
			return false;
		const Invention *invention = current_design->getInvention();
		if( require_type && invention->getType() != invention_type )
			return false;
		if( require_epoch && invention->getEpoch() != invention_epoch )
			return false;
		return true;
	}
	else if( wait_type == WAITTYPE_HAS_DESIGNED || wait_type == WAITTYPE_HAS_MANUFACTURED ) {
		for(int i=0;i<Invention::N_TYPES;i++) {
			if( require_type && i != invention_type )
				continue;
			int n_j = i == Invention::SHIELD ? n_shields_c : n_epochs_c;
			for(int j=0;j<n_j;j++) {
				if( require_epoch && j != invention_epoch )
					continue;
				if( wait_type == WAITTYPE_HAS_DESIGNED && sector->inventionKnown(static_cast<Invention::Type>(i), j) ) {
					return true;
				}
				else if( wait_type == WAITTYPE_HAS_MANUFACTURED && i == Invention::SHIELD && sector->getStoredShields(j) > 0 ) {
					return true;
				}
				else if( wait_type == WAITTYPE_HAS_MANUFACTURED && i == Invention::DEFENCE && sector->getStoredDefenders(j) > 0 ) {
					return true;
				}
				else if( wait_type == WAITTYPE_HAS_MANUFACTURED && i == Invention::WEAPON && sector->getStoredArmy()->getSoldiers(j) > 0 ) {
					return true;
				}
			}
		}
		return false;
	}
	ASSERT(false);
	return true;
}

bool TutorialCardWaitForBuilding::canProceed(PlayingGameState *playing_gamestate) const {
	if( sector->getBuilding(building_type) != NULL ) {
		return true;
	}
	return false;
}

bool TutorialCardWaitForDeployedArmy::canProceed(PlayingGameState *playing_gamestate) const {
	if( !inverse ) {
		int str = require_bombard ? deploy_sector->getArmy(playing_gamestate->getClientPlayer())->getBombardStrength() : deploy_sector->getArmy(playing_gamestate->getClientPlayer())->getTotal();
		if( require_unoccupied && deploy_sector->getPlayer() != PLAYER_NONE ) {
			return false;
		}
		if( require_empty && str == 0 )
			return true;
		else if( !require_empty && str > 0 )
			return true;
	}
	else {
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				if( game_g->getMap()->isSectorAt(x, y) ) {
					const Sector *sector = game_g->getMap()->getSector(x, y);
					if( sector == deploy_sector )
						continue;
					if( require_unoccupied && sector->getPlayer() != PLAYER_NONE ) {
						continue;
					}
					int str = require_bombard ? sector->getArmy(playing_gamestate->getClientPlayer())->getBombardStrength() : sector->getArmy(playing_gamestate->getClientPlayer())->getTotal();
					if( require_empty && str > 0 )
						return false;
					else if( !require_empty && str > 0 )
						return true;
				}
			}
		}
		if( require_empty )
			return true;
	}
	return false;
}

bool TutorialCardWaitForNewTower::canProceed(PlayingGameState *playing_gamestate) const {
	if( !inverse ) {
		bool has_tower = tower_sector->getPlayer() == playing_gamestate->getClientPlayer();
		if( has_tower )
			return true;
	}
	else {
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				if( game_g->getMap()->isSectorAt(x, y) ) {
					const Sector *sector = game_g->getMap()->getSector(x, y);
					if( sector == tower_sector )
						continue;
					bool has_tower = sector->getPlayer() == playing_gamestate->getClientPlayer();
					if( has_tower )
						return true;
				}
			}
		}
	}
	return false;
}

Tutorial::~Tutorial() {
	for(vector<TutorialCard *>::const_iterator iter = cards.begin(); iter != cards.end(); ++iter) {
		TutorialCard *card = *iter;
		delete card;
	}
}

bool Tutorial::jumpTo(const string &id) {
	int index = 0;
	for(vector<TutorialCard *>::const_iterator iter = cards.begin(); iter != cards.end(); ++iter, index++) {
		TutorialCard *card = *iter;
		if( card->getId() == id ) {
			card_index = index;
			return true;
		}
	}
	return false;
}

Tutorial1::Tutorial1(const string &id) : Tutorial(id) {
	start_epoch = 0;
	island = 0;
	start_map_x = 1;
	start_map_y = 2;
	n_men = 20;
	ai_allow_growth = false;
	ai_allow_design = false;
	ai_allow_ask_alliance = false;
	ai_allow_deploy = false;
}

void Tutorial1::initCards() {
	Sector *start_sector = game_g->getMap()->getSector(start_map_x, start_map_y);
	ASSERT(start_sector != NULL);
	Sector *enemy_sector = game_g->getMap()->getSector(2, 2);
	ASSERT(enemy_sector != NULL);

	TutorialCard *card = NULL;

#if defined(__ANDROID__)
	card = new TutorialCard("0", "Welcome to Gigalomania!\nThis tutorial will introduce you to the game,\nand show you how to win your first island.\nUse the volume keys on your device to control the music volume.\nClick 'next' to continue when you're ready.");
#else
	card = new TutorialCard("0", "Welcome to Gigalomania!\nThis tutorial will introduce you to the game,\nand show you how to win your first island.\nClick 'next' to continue when you're ready.");
#endif
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("1", "At the top left you can see a map of the current island.\nThe island is split up into sectors.\nThe coloured squares represent sectors controlled by a player.");
	card->setArrow(40, 56);
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("2", "The age of this sector is 10,000 bc.\nThis represents the state of technology. \nIt's possible for different sectors to be in different ages.");
	card->setArrow(80, 110);
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("3", "Next we'll go over the control panel, which\nallows you to control your sector.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("4", "The number below the person icon shows the number of people in your\nsector that are available.\nThe population will grow gradually with time.");
	card->setArrow(16, 136);
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("5", "Now let's design a weapon!\nClick on the lightbulb icon to start researching.", (int)GamePanel::STATE_DESIGN);
	card->setArrow(36, 156);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_design");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("6", "This page allows you to design new inventions:\nThe left hand column shows shields, which are used to repair buildings.\nThe middle shows defences for your sector.\nThe right shows weapons to attack the enemy!");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCardWaitForDesign("7", "For this tutorial, we're going to design a weapon.\nClick on one of the weapons.\nI recommend the Rock weapon, but any will do.", start_sector, TutorialCardWaitForDesign::WAITTYPE_CURRENT_DESIGN, true, Invention::WEAPON, false, -1);
	card->setArrow(90, 168);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	if( game_g->isOneMouseButton() ) {
		card = new TutorialCard("8", "Now put some of your people to work designing the weapon.\nClick on the number, then use the arrows to increase or decrease\nthe number of designers.");
	}
	else {
		card = new TutorialCard("8", "Now put some of your people to work designing the weapon.\nUsing the right mouse button, click on the number to increase\nthe number of designers, left mouse button to decrease.");
	}
	card->setArrow(50, 130);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("9", "While your designers are working, the clock shows the remaining time\nuntil the invention is ready.");
	card->setArrow(86, 130);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	if( game_g->isOneMouseButton() ) {
		card = new TutorialCardWaitForDesign("10", "To hurry things up, you can make time go faster.\nClick the 1x icon to cycle through different time rates.", start_sector, TutorialCardWaitForDesign::WAITTYPE_HAS_DESIGNED, true, Invention::WEAPON, false, -1);
	}
	else {
		card = new TutorialCardWaitForDesign("10", "To hurry things up, you can make time go faster.\nRight click on the 1x icon to speed time up.\nLeft click slows time back down.", start_sector, TutorialCardWaitForDesign::WAITTYPE_HAS_DESIGNED, true, Invention::WEAPON, false, -1);
	}
	card->setArrow(100, 10);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("11", "Great! Now click on the lightbulb to go back to the main interface.", (int)GamePanel::STATE_SECTORCONTROL);
	card->setArrow(50, 110);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		gui_handler->addException("button_bigdesign");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("11", "Time to attack our enemy. The shield and sword icon allows you to\nassemble your army.", (int)GamePanel::STATE_ATTACK);
	card->setArrow(80, 125);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_attack");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("12", "This page shows you the available soldiers to deploy.\nThey can be unarmed or, preferably, armed with weapons\nyou have invented.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("13", "Not only are armed soldiers stronger, but they are required to destroy\nan enemy's buildings. Unarmed soldiers can fight other soldiers,\nbut won't knock down the enemy tower.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("14", "Click on the weapon icon to assemble soldiers with the weapon\nyou've just invented.\nAssemble as many soldiers as we have people available!");
	card->setArrow(15, 170);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForDeployedArmy("15", "Then click on the enemy's sector in the map - that's the\nright hand square - to send your army to attack.", enemy_sector, true);
	card->setArrow(48, 56);
	card->setArrowShowPage((int)GamePanel::STATE_ATTACK);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		gui_handler->addException("map_1_2"); // need to allow the user to return to the player sector if necessary (as user can switch to the other sector without sending men)
		gui_handler->addException("map_2_2");
		gui_handler->addException("land_panel"); // needed in case the user sends the army to the wrong sector, allows user another way to select the army
		gui_handler->addException("button_attack"); // allow the user to get back to the attack screen (a GUI resets to main if user switches current sector)
		gui_handler->addException("button_bigattack"); // ...and for consistency, allow the user to go back to the main screen
		// and in case the user wants to design some more:
		gui_handler->addException("button_design");
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		gui_handler->addException("button_bigdesign");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("16", "Now wait until the battle is won!\nRemember to speed up the time rate if you prefer.");
	card->setNextText("Done");
	cards.push_back(card);

	// for debugging
	//this->card_index = 13;
}

Tutorial2::Tutorial2(const string &id) : Tutorial(id) {
	start_epoch = 0;
	island = 2;
	start_map_x = 2;
	start_map_y = 2;
	n_men = 25;
	auto_end = true;
	ai_allow_growth = false;
	ai_allow_design = false;
	ai_allow_ask_alliance = false;
	ai_allow_deploy = false;
	allow_retreat_loss = false;
}

void Tutorial2::initCards() {
	Sector *start_sector = game_g->getMap()->getSector(start_map_x, start_map_y);
	ASSERT(start_sector != NULL);

	TutorialCard *card = NULL;

	card = new TutorialCardWaitForPanelPage("0", "In this tutorial we'll learn some army maneuvers.\nSelect the shield and sword icon to deploy some soldiers.", (int)GamePanel::STATE_ATTACK);
	card->setPlayerAllowBuildTower(false);
	card->setArrow(80, 125);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_attack");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("1", "In the first tutorial you learnt how to design weapons.\nHere you'll be learning how to deploy and move your soldiers,\nso unarmed men will do.");
	card->setPlayerAllowBuildTower(false);
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("2", "Click to assemble some unarmed men.");
	card->setPlayerAllowBuildTower(false);
	card->setArrow(15, 145);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_deploy_unarmedmen");
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		gui_handler->addException("button_return_attackers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("3", "Note that if you make a mistake assembling your army,\nyou can cancel by clicking the shield and sword icon at the bottom.");
	card->setPlayerAllowBuildTower(false);
	card->setArrow(87, 195);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_deploy_unarmedmen");
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		gui_handler->addException("button_return_attackers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForDeployedArmy("4", "Assemble some unarmed men, and send them to a\nsector by clicking on a new map square of your choice.", start_sector, false);
	card->setPlayerAllowBuildTower(false);
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setInverse(true);
	card->setArrow(60, 70);
	card->setArrowShowPage((int)GamePanel::STATE_ATTACK);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_deploy_unarmedmen");
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		gui_handler->addException("button_return_attackers");
		gui_handler->addException("button_attack");
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				// enable the current square too, as we need to allow getting back if the user clicks another square without an assembled army!
				if( game_g->getMap()->isSectorAt(x, y) ) {
					char buffer[256] = "";
					sprintf(buffer, "map_%d_%d", x, y);
					gui_handler->addException(buffer);
				}
			}
		}
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	if( game_g->isOneMouseButton() ) {
		card = new TutorialCard("5", "Now see if you can return them to the home sector.\nYou can select an army by clicking\non the main area to the right of the map.");
	}
	else {
		card = new TutorialCard("5", "Now see if you can return them to the home sector.\nYou can select an army by right clicking,\neither on the current map square,\nor right clicking on the main area to the right.");
	}
	card->setPlayerAllowBuildTower(false);
	cards.push_back(card);

	card = new TutorialCardWaitForDeployedArmy("6", "When the army is selected, the shield icon will show,\nor a bloody sword icon if retreating from an enemy sector.\nMove the army back home by clicking on the square of your sector", start_sector, false);
	card->setPlayerAllowBuildTower(false);
	card->setArrow(47, 54);
	cards.push_back(card);

	// return back home

	if( game_g->isOneMouseButton() ) {
		card = new TutorialCardWaitForDeployedArmy("7", "Now let's return our men back to the tower.\nClick to select the army as before, then\nclick on your tower.", NULL, false);
	}
	else {
		card = new TutorialCardWaitForDeployedArmy("7", "Now let's return our men back to the tower.\nRight click to select the army as before, then\nleft click on your tower.", NULL, false);
	}
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setInverse(true);
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setRequireEmpty(true);
	card->setPlayerAllowBuildTower(false);
	card->setArrow(260, 80);
	card->setArrowShowSector(start_map_x, start_map_y);
	cards.push_back(card);

	// build new tower

	card = new TutorialCard("8", "So far you've only had a single tower,\nbut you can build new towers.");
	card->setPlayerAllowBuildTower(false);
	cards.push_back(card);

	card = new TutorialCard("9", "Each tower can act independently, researching and\nconstructing different weapons.\nEach tower needs to invent its own weapons.");
	card->setPlayerAllowBuildTower(false);
	cards.push_back(card);

	card = new TutorialCardWaitForDeployedArmy("10", "Assemble some unarmed men again, and this time\nsend them to a square that isn't occupied by the enemy", start_sector, false);
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setInverse(true);
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setRequireUnoccupied(true);
	cards.push_back(card);

	card = new TutorialCardWaitForNewTower("11", "Now sit back and wait until your new tower is constructed.\nRemember to speed up the rate of time if you want.", start_sector);
	static_cast<TutorialCardWaitForNewTower *>(card)->setInverse(true);
	cards.push_back(card);

	card = new TutorialCard("12", "You have completed this tutorial!");
	cards.push_back(card);

	// for debugging
	//this->card_index = 5;
}

Tutorial3::Tutorial3(const string &id) : Tutorial(id) {
	start_epoch = 3;
	island = 1;
	start_map_x = 3;
	start_map_y = 3;
	n_men = 50;
	//auto_end = true;
	ai_allow_growth = false;
	ai_allow_design = false;
	ai_allow_ask_alliance = false;
	ai_allow_deploy = false;
	//allow_retreat_loss = false;
}

void Tutorial3::initCards() {
	Sector *start_sector = game_g->getMap()->getSector(start_map_x, start_map_y);
	ASSERT(start_sector != NULL);

	TutorialCard *card = NULL;

	card = new TutorialCard("0", "We're now thousands of years into the future since the last tutorial.\nIn this tutorial, you'll learn how to make use of newer technology\nto build weapons to destroy your enemies!");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("1", "From the fourth age onwards, you can construct an additional building\nin your sector - a mine.\nSelect the pickaxe icon at the bottom left to build a mine.", (int)GamePanel::STATE_BUILD);
	card->setArrow(25, 210);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_build_mine");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForBuilding("2", "Assign some men to start building a mine, and then\nwait for it to be constructed.", start_sector, BUILDING_MINE);
	card->setArrow(60, 130);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_nbuilders2_mine");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("3", "Now click the bricks icon to go back to the main interface.", (int)GamePanel::STATE_SECTORCONTROL);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_bigbuild");
		card->setGUIHandler(gui_handler);
	}
	card->setArrow(50, 110);
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("4", "Your mine can extract elements from the ground, in order to\ncreate new inventions. To do this, click on any of the elements.", (int)GamePanel::STATE_ELEMENTSTOCKS);
	card->setArrow(45, 185);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("5", "This page shows you the sector's current element stocks.\nSo far you've worked with gatherables, that your men automatically\npick up, without the need for a mine.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	card->setArrow(80, 150);
	cards.push_back(card);

	card = new TutorialCard("6", "But more advanced elements require you to assign miners to them.\nClick on the 0 number to add some miners now, and wait\nuntil we have at least 1 unit.\nOnly use a few of your men for this.");
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		card->setGUIHandler(gui_handler);
	}
	card->setArrow(52, 183);
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("7", "Now click the pickaxe icon to go back to the main screen.", (int)GamePanel::STATE_SECTORCONTROL);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		card->setGUIHandler(gui_handler);
	}
	card->setArrow(50, 110);
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("8", "Now let's design a weapon!\nClick on the lightbulb icon to start researching.", (int)GamePanel::STATE_DESIGN);
	card->setArrow(36, 156);
	card->setArrowShowPage((int)GamePanel::STATE_SECTORCONTROL);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_design");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForDesign("9", "First, design a longbow", start_sector, TutorialCardWaitForDesign::WAITTYPE_HAS_DESIGNED, true, Invention::WEAPON, true, 3);
	card->setArrow(90, 168);
	card->setArrowShowPage((int)GamePanel::STATE_DESIGN);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_design");
		gui_handler->addException("button_bigdesign");
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_designers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForDesign("10", "Now design a trebuchet", start_sector, TutorialCardWaitForDesign::WAITTYPE_HAS_DESIGNED, true, Invention::WEAPON, true, 4);
	card->setArrow(90, 188);
	card->setArrowShowPage((int)GamePanel::STATE_DESIGN);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_design");
		gui_handler->addException("button_bigdesign");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_designers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("11", "Now click the lightbulb icon to go back to the main screen.", (int)GamePanel::STATE_SECTORCONTROL);
	card->setArrow(50, 110);
	card->setArrowShowPage((int)GamePanel::STATE_DESIGN);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_bigdesign");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("12", "As a result of your two inventions, we've moved forward to a new age.\nThis means we can build a factory, which is necessary to construct\ntrebuchets. Click the factory icon at the bottom to build a factory.",  (int)GamePanel::STATE_BUILD);
	card->setArrow(45, 210);
	card->setArrowShowPage((int)GamePanel::STATE_SECTORCONTROL);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_build_factory");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForBuilding("13", "Now assign some men to build a factory, as you did with the mine,\nand wait for it to be constructed.", start_sector, BUILDING_FACTORY);
	card->setArrow(60, 160);
	card->setArrowShowPage((int)GamePanel::STATE_BUILD);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_build_factory");
		gui_handler->addException("button_bigbuild");
		gui_handler->addException("button_nbuilders2_factory");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("14", "Now click the bricks icon to go back to the main interface.", (int)GamePanel::STATE_SECTORCONTROL);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_bigbuild");
		card->setGUIHandler(gui_handler);
	}
	card->setArrow(50, 110);
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("15", "Click on the factory icon to start manufacturing.", (int)GamePanel::STATE_FACTORY);
	card->setArrow(65, 156);
	card->setArrowShowPage((int)GamePanel::STATE_SECTORCONTROL);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_factory");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForDesign("16", "Select the trebuchet to start manufacturing", start_sector, TutorialCardWaitForDesign::WAITTYPE_CURRENT_MANUFACTURE, true, Invention::WEAPON, false, -1);
	card->setArrow(90, 205);
	card->setArrowShowPage((int)GamePanel::STATE_FACTORY);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_factory");
		gui_handler->addException("button_bigfactory");
		gui_handler->addException("button_fweapons_1");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForDesign("17", "The top number shows the assigned number of workers,\nthe bottom shows the number of trebuchets to be manufactured.\nAssign workers by increasing the top number, then wait!", start_sector, TutorialCardWaitForDesign::WAITTYPE_HAS_MANUFACTURED, true, Invention::WEAPON, false, -1);
	card->setArrow(90, 135);
	card->setArrowShowPage((int)GamePanel::STATE_FACTORY);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_elements_0");
		gui_handler->addException("button_elements_1");
		gui_handler->addException("button_elements_2");
		gui_handler->addException("button_elements_3");
		gui_handler->addException("button_nminers2_0");
		gui_handler->addException("button_nminers2_1");
		gui_handler->addException("button_nminers2_2");
		gui_handler->addException("button_nminers2_3");
		gui_handler->addException("button_bigelementstocks");
		gui_handler->addException("button_factory");
		gui_handler->addException("button_bigfactory");
		gui_handler->addException("button_fweapons_1");
		gui_handler->addException("button_workers");
		gui_handler->addException("button_famount");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("18", "You now have your first trebchet! Now manufacture some more,\nand use them to defeat your enemies. Good luck!");
	card->setNextText("Done");
	cards.push_back(card);
}
