/*
	THIS FILE IS A PART OF RDR 2 SCRIPT HOOK SDK
				http://dev-c.com
			(C) Alexander Blade 2019
*/

#include "script.h"
#include <string>
#include <vector>
#include "keyboard.h"
#include <ctime>
#include <filesystem>
#include "Simpleini.h"
#include <iostream>
#include <fstream> 
#include <sstream>
#include <utility>

bool allowSave;
int HungerLastSet, ThirstLastSet, SleepLastSet, HungerDecay, ThirstDecay, SleepDecay, Hunger, Thirst, Sleeps, tAnimationSet = 0, eAnimationSet = 0;

std::vector<std::string> explode(const std::string& str, const char& ch) {
	std::string next;
	std::vector<std::string> result;

	// For each character in the string
	for (std::string::const_iterator it = str.begin(); it != str.end(); it++) 
	{
		// If we've hit the terminal character
		if (*it == ch) 
		{
			// If we have some characters accumulated
			if (!next.empty()) 
			{
				// Add them to the result vector
				result.push_back(next);
				next.clear();
			}
		}
		else 
		{
			// Accumulate the next character into the sequence
			next += *it;
		}
	}
	if (!next.empty())
		result.push_back(next);
	return result;
}

void GetConfigOptions()
{
	// Load Simpleini
	CSimpleIniA ini;
	ini.SetUnicode();

	if (std::filesystem::exists("Survivalmode.ini")) // Does our ini exists?
	{
		// Load the ini file
		ini.LoadFile("Survivalmode.ini");
		// 1day in RDR2 = IRL 48 minutes
		// All values will be in seconds
		// Declare our default values then divide by 100 so we know how often to minus 1 point - To lose 100% from hungry we must lose a point every 86.4 seconds
		const char* hd;
		const char* td;
		const char* sd;
		int ihd;
		int itd;
		int isd;
		std::stringstream hValue;
		std::stringstream tValue;
		std::stringstream sValue;
		hd = ini.GetValue("DECAY", "HungerDecay");
		td = ini.GetValue("DECAY", "ThirstDecay");
		sd = ini.GetValue("DECAY", "SleepDecay");
		// Hunger
		hValue << hd;
		hValue >> ihd;
		HungerDecay = (ihd * 2880000) / 100;
		// Thirst
		tValue << td;
		tValue >> itd;
		ThirstDecay = (itd * 2880000) / 100;
		// Sleep
		sValue << sd;
		sValue >> isd;
		SleepDecay = (isd * 2880000) / 100;
	}
	else // Oh no the file doesn't exists for some strange reason ;-;
	{
		// Create the ini file
		std::ofstream { "Survivalmode.ini" };
		// Now lets load the ini file
		ini.LoadFile("Survivalmode.ini");
		// Let's set default values here
		// All values will be in days under the section DECAY
		// 2880 seconds = 1 RDR2 day
		ini.SetValue("DECAY", "HungerDecay", "3"); // 3 in game days
		ini.SetValue("DECAY", "ThirstDecay", "2"); // 2 in game days
		ini.SetValue("DECAY", "SleepDecay", "1"); // 1 in game day
		// Save the file 
		ini.SaveFile("Survivalmode.ini");
		// Declare our default values then divide by 100 so we know how often to minus 1 point - To lose 100% from hungry we must lose a point every 86.4 seconds
		HungerDecay = 8640000 / 100;
		ThirstDecay = 5760000 / 100;
		SleepDecay = 2880000 / 100;
	}
}

void GetCoreData()
{
	if (std::filesystem::exists("Survivalmode.dat")) // Does the file exist that saves our Core values?
	{
		// Read the file
		std::ifstream cores("Survivalmode.dat");
		std::stringstream cdata;
		cdata << cores.rdbuf();
		// Explode the string into an array
		std::vector<std::string> core = explode(cdata.str(), ' ');
		Hunger = std::stoi(core[0]);
		Thirst = std::stoi(core[1]);
		Sleeps = std::stoi(core[2]);
	}
	else // Negative - Go ahead and create the file with default values
	{
		std::ofstream configfile("Survivalmode.dat");
		configfile << "50 50 50" << std::endl;
		configfile.close();
		Hunger = 50;
		Thirst = 50;
		Sleeps = 50;
	}
}

static int getGameTime() 
{
	return invoke<int>(0x4F67E8ECA7D3F667);
}

char const* convertToChar(int text)
{
	std::string tmp = std::to_string(text);
	char const *num_char = tmp.c_str();
	return num_char;
}

void PrintSubtitle(const char* text)
{
	const char* literalString = MISC::VAR_STRING(10, "LITERAL_STRING", text);
	UILOG::_UILOG_SET_CACHED_OBJECTIVE(literalString);
	UILOG::_UILOG_PRINT_CACHED_OBJECTIVE();
	UILOG::_UILOG_CLEAR_CACHED_OBJECTIVE();
}

void WriteNewCoreData()
{
	std::ofstream configfile("Survivalmode.dat", std::ofstream::trunc);
	configfile << std::to_string(Hunger) + " " + std::to_string(Thirst) + " " + std::to_string(Sleeps) << std::endl;
	configfile.close();
}

void DrainHunger()
{
	if (Hunger >= 1)
	{
		Hunger--;
	}
}

void DrainThirst()
{
	if (Thirst >= 1)
	{
		Thirst--;
	}
}

void DrainSleep()
{
	// Drain the Core
	if (Sleeps >= 1)
	{
		Sleeps--;
	}
	// Core getting too low penalties 
	(Sleeps <= 10) ? PED::SET_PED_RESET_FLAG(PLAYER::PLAYER_PED_ID(), 139, true) : PED::SET_PED_RESET_FLAG(PLAYER::PLAYER_PED_ID(), 139, false); // No stamina regen if sleep is below/equal 10%
}

void drawCoreIcons(int hunger, int thirst, int sleeps)
{
	// Load our textures
	if (!TXD::HAS_STREAMED_TEXTURE_DICT_LOADED("rpg_textures")) { TXD::REQUEST_STREAMED_TEXTURE_DICT("rpg_textures", 0); }
	if (!TXD::HAS_STREAMED_TEXTURE_DICT_LOADED("rpg_meter")) { TXD::REQUEST_STREAMED_TEXTURE_DICT("rpg_meter", 0); }
	if (!TXD::HAS_STREAMED_TEXTURE_DICT_LOADED("blips")) { TXD::REQUEST_STREAMED_TEXTURE_DICT("blips", 0); }
	// Since the meter texture only goes to 99 we need to say 100 = 99
	if (hunger >= 99) { hunger = 99; }
	if (thirst >= 99) { thirst = 99; }
	if (sleeps >= 99) { sleeps = 99; }
	// Figure out which meter uses what
	std::string sMain = "rpg_meter_";
	std::string sHunger = std::to_string(hunger);
	std::string sThirst = std::to_string(thirst);
	std::string sSleeps = std::to_string(sleeps);
	std::string fHunger = sMain + sHunger;
	std::string fThirst = sMain + sThirst;
	std::string fSleeps = sMain + sSleeps;
	// Draw our icons
	// Hunger
	GRAPHICS::DRAW_SPRITE("rpg_textures", "rpg_core_background", 0.042, 0.70, 0.0242, 0.042, 0.0, 0, 0, 0, 900, 1);
	GRAPHICS::DRAW_SPRITE("rpg_meter", "rpg_meter_99", 0.042, 0.70, 0.0252, 0.045, 0.0, 0, 0, 0, 28, 1);
	if (hunger <= 20) // Make the icon red when we are at 20%
	{
		GRAPHICS::DRAW_SPRITE("rpg_textures", "rpg_overfed", 0.042, 0.70, 0.04, 0.042, 0.0, 255, 0, 0, 900, 1);
	}
	else
	{
		GRAPHICS::DRAW_SPRITE("rpg_textures", "rpg_overfed", 0.042, 0.70, 0.04, 0.042, 0.0, 255, 255, 255, 900, 1);
	}
	GRAPHICS::DRAW_SPRITE("rpg_meter", fHunger.c_str(), 0.042, 0.70, 0.0252, 0.045, 0.0, 255, 255, 255, 900, 1);
	// Thirst
	GRAPHICS::DRAW_SPRITE("rpg_textures", "rpg_core_background", 0.065, 0.685, 0.0242, 0.042, 0.0, 0, 0, 0, 900, 1);
	GRAPHICS::DRAW_SPRITE("rpg_meter", "rpg_meter_99", 0.065, 0.685, 0.0252, 0.045, 0.0, 0, 0, 0, 28, 1);
	if (thirst <= 20) // Make the icon red when we are at 20%
	{
		GRAPHICS::DRAW_SPRITE("blips", "blip_mg_drinking", 0.065, 0.685, 0.01, 0.02, 0.0, 255, 0, 0, 900, 1);
	}
	else
	{
		GRAPHICS::DRAW_SPRITE("blips", "blip_mg_drinking", 0.065, 0.685, 0.01, 0.02, 0.0, 255, 255, 255, 900, 1);
	}
	GRAPHICS::DRAW_SPRITE("rpg_meter", fThirst.c_str(), 0.065, 0.685, 0.0252, 0.045, 0.0, 255, 255, 255, 900, 1);
	// Sleep
	GRAPHICS::DRAW_SPRITE("rpg_textures", "rpg_core_background", 0.089, 0.678, 0.0242, 0.042, 0.0, 0, 0, 0, 900, 1);
	GRAPHICS::DRAW_SPRITE("rpg_meter", "rpg_meter_99", 0.089, 0.678, 0.0252, 0.045, 0.0, 0, 0, 0, 28, 1);
	if (sleeps <= 20) // Make the icon red when we are at 20%
	{
		GRAPHICS::DRAW_SPRITE("blips", "blip_hotel_bed", 0.089, 0.678, 0.01, 0.02, 0.0, 255, 0, 0, 900, 1);
	}
	else
	{
		GRAPHICS::DRAW_SPRITE("blips", "blip_hotel_bed", 0.089, 0.678, 0.01, 0.02, 0.0, 255, 255, 255, 900, 1);
	}
	GRAPHICS::DRAW_SPRITE("rpg_meter", fSleeps.c_str(), 0.089, 0.678, 0.0252, 0.045, 0.0, 255, 255, 255, 900, 1);
	// Unload our textures
	TXD::SET_STREAMED_TEXTURE_DICT_AS_NO_LONGER_NEEDED("rpg_textures");
	TXD::SET_STREAMED_TEXTURE_DICT_AS_NO_LONGER_NEEDED("rpg_meter");
	TXD::SET_STREAMED_TEXTURE_DICT_AS_NO_LONGER_NEEDED("blips");
}

void Initialize()
{
	// Load all the config stuff
	GetConfigOptions();
	GetCoreData();
	// Set all of our LastSet values to the current game time
	HungerLastSet = getGameTime();
	ThirstLastSet = getGameTime();
	SleepLastSet = getGameTime();
	//PrintSubtitle("Welcome to Survival Mode!");

}

bool isGamePaused()
{
	return invoke<bool>(0x535384D6067BA42E);
}

bool refillHungerCoreScenario() 
{
	const char* animations[] = {
	"face_human@gen_male@scenario@eating"
	};
	for (const char* animation : animations)
	{
		if (STREAMING::HAS_ANIM_DICT_LOADED(animation))
		{
			return true;
			break;
		}
	}
	return false;
}

bool refillThirstCoreScenario()
{
	const char* animations[] = {
		"face_human@gen_male@scenario@drinkbottle",
		"face_human@gen_male@scenario@drinkshot",
		"face_human@gen_male@scenario@drinkmug",
		"face_human@gen_male@scenario@drinkhand"
	};
	for (const char* animation : animations)
	{
		if (STREAMING::HAS_ANIM_DICT_LOADED(animation))
		{
			return true;
			break;
		}
	}
	return false;
}

bool refillSleepCoreScenario()
{
	const char* scenarios[] = {
		"WORLD_PLAYER_SLEEP_BEDROLL",
		"WORLD_PLAYER_SLEEP_BEDROLL_ARTHUR",
		"WORLD_PLAYER_SLEEP_GROUND",
		"PROP_PLAYER_SLEEP_BED",
		"PROP_PLAYER_SLEEP_BED_ARTHUR",
		"PROP_PLAYER_SLEEP_TENT_A_FRAME",
		"PROP_PLAYER_SLEEP_TENT_A_FRAME_ARTHUR",
		"PROP_PLAYER_SLEEP_TENT_MALE_A",
		"PROP_PLAYER_SLEEP_TENT_MALE_A_ARTHUR",
		"PROP_PLAYER_SLEEP_A_FRAME_TENT_PLAYER_CAMPS",
		"PROP_PLAYER_SLEEP_A_FRAME_TENT_PLAYER_CAMPS_ARTHUR",
		"WORLD_PLAYER_CAMP_FIRE_KNEEL1",
		"WORLD_PLAYER_CAMP_FIRE_KNEEL2",
		"WORLD_PLAYER_CAMP_FIRE_KNEEL3",
		"WORLD_PLAYER_CAMP_FIRE_KNEEL4",
		"WORLD_PLAYER_CAMP_FIRE_SIT",
		"WORLD_PLAYER_CAMP_FIRE_SQUAT",
		"WORLD_PLAYER_DYNAMIC_KNEEL_KNIFE",
		"WORLD_PLAYER_CAMP_FIRE_SQUAT_MALE_A",
		"WORLD_PLAYER_CAMP_FIRE_SIT_MALE_A",
		"WORLD_PLAYER_DYNAMIC_CAMP_FIRE_KNEEL_ARTHUR",
		"PROP_PLAYER_SLEEP_TENT_A_FRAME",
		"PROP_PLAYER_SEAT_CHAIR_PLAYER_CAMP",
		"PROP_PLAYER_SEAT_CHAIR_DYNAMIC",
		"PROP_PLAYER_SEAT_CHAIR_GENERIC",
		"PROP_PLAYER_SEAT_CHAIR_GENERIC_CA"
	};
	for (const char* scenario : scenarios)
	{
		if (PED::IS_PED_USING_SCENARIO_HASH(PLAYER::PLAYER_PED_ID(), MISC::GET_HASH_KEY(scenario)) && TASK::_GET_SCENARIO_POINT_PED_IS_USING(PLAYER::PLAYER_PED_ID(), 1) == -1)
		{
			return true;
			break;
		}
	}
	return false;
}

bool hideCoreIcons()
{
	return !CAM::IS_GAMEPLAY_CAM_RENDERING()
		|| HUD::IS_RADAR_HIDDEN()
		|| HUD::IS_RADAR_HIDDEN_BY_SCRIPT()
		|| GRAPHICS::ANIMPOSTFX_IS_RUNNING("WheelHUDIn")
		|| CAM::IS_CINEMATIC_CAM_RENDERING()
		|| GRAPHICS::ANIMPOSTFX_IS_RUNNING("killCam")
		;
}

bool saveGame(const char* type)
{
	return invoke<bool>(0x62C9EB51656D68CE, MISC::GET_HASH_KEY(type));
}

void update()
{
	if (!isGamePaused()) // Make sure our game isn't paused
	{
		if (IsKeyJustUp(VK_F1))
		{
			
		}
		if (SAVE::SAVEGAME_IS_SAVE_PENDING())
		{
			PrintSubtitle("Save game pending");
		}
		// Set our Health regen to 0
		PLAYER::SET_PLAYER_HEALTH_RECHARGE_MULTIPLIER(PLAYER::PLAYER_ID(), 0.0f);
		// Disable fast traveling
		if (!MISC::GET_MISSION_FLAG())
		{
			PLAYER::STOP_PLAYER_TELEPORT();
		}
		// Stop saves from occuring unless the player sleeps
		if (allowSave)
		{
			if (SAVE::SAVEGAME_SAVE_SP(MISC::GET_HASH_KEY("SAVEGAMETYPE_SP_AUTOSAVE")))
			{
				allowSave = false;
			}
		}
		// Reload game if player dies
		if (GRAPHICS::ANIMPOSTFX_IS_RUNNING("killCam"))
		{
			do {
				WAIT(0);
			}
			while (GRAPHICS::ANIMPOSTFX_IS_RUNNING("killCam"));
		}
		// Core Draining stuff
		if (getGameTime() - HungerLastSet >= HungerDecay)
		{
			DrainHunger();
			HungerLastSet = getGameTime();
		}
		if (getGameTime() - ThirstLastSet >= ThirstDecay)
		{
			DrainThirst();
			ThirstLastSet = getGameTime();
		}
		if (getGameTime() - SleepLastSet >= SleepDecay)
		{
			DrainSleep();
			SleepLastSet = getGameTime();
		}
		// Draw our icons
		if (!hideCoreIcons())
		{
			drawCoreIcons(Hunger, Thirst, Sleeps);
		}
		// Core refilling actions 
		if (TASK::GET_IS_TASK_ACTIVE(PLAYER::PLAYER_PED_ID(), 471)) // Test if the core refill TASK is active
		{
			if (refillThirstCoreScenario() && refillHungerCoreScenario()) // Did both load?
			{
				if (eAnimationSet > tAnimationSet) // If the hunger core is older than the thirst core we can assume that they wanted to drink
				{
					// Unload the hunger animation dict so the next time it happens both will not return true
					STREAMING::REMOVE_ANIM_DICT("face_human@gen_male@scenario@eating");
					// Run our normal thirst stuff
					if (getGameTime() - tAnimationSet >= 8000)
					{
						if (Thirst <= 80)
						{
							tAnimationSet = getGameTime();
							Thirst = Thirst + 20;
							DrainThirst();
						}
						else
						{
							// Get the increase we need to hit 101
							tAnimationSet = getGameTime();
							int nThirst = 101 - Thirst;
							Thirst = Thirst + nThirst;
							DrainThirst();
						}
					}
				}
				else // They clearly want to eat
				{
					// Unload the thirst animation dicts so the next time it happens both will not return true
					const char* animations[] = {
						"face_human@gen_male@scenario@drinkbottle",
						"face_human@gen_male@scenario@drinkshot",
						"face_human@gen_male@scenario@drinkmug",
						"face_human@gen_male@scenario@drinkhand"
					};
					for (const char* animation : animations)
					{
						STREAMING::REMOVE_ANIM_DICT(animation);
					}
					// Run our normal hunger stuff
					if (getGameTime() - eAnimationSet >= 5000)
					{
						if (Hunger <= 80)
						{
							eAnimationSet = getGameTime();
							Hunger = Hunger + 20;
							DrainHunger();
						}
						else
						{
							// Get the increase we need to hit 101
							eAnimationSet = getGameTime();
							int nHunger = 101 - Hunger;
							Hunger = Hunger + nHunger;
							DrainHunger();
						}
					}
				}
			}
			// Hunger
			if (refillHungerCoreScenario() && !refillThirstCoreScenario())
			{
				if (getGameTime() - eAnimationSet >= 5000)
				{
					if (Hunger <= 80)
					{
						eAnimationSet = getGameTime();
						Hunger = Hunger + 20;
						DrainHunger();
					}
					else
					{
						// Get the increase we need to hit 101
						eAnimationSet = getGameTime();
						int nHunger = 101 - Hunger;
						Hunger = Hunger + nHunger;
						DrainHunger();
					}
				}
			}
			// Thirst
			if (refillThirstCoreScenario() && !refillHungerCoreScenario())
			{
				if (getGameTime() - tAnimationSet >= 8000)
				{
					if (Thirst <= 80)
					{
						tAnimationSet = getGameTime();
						Thirst = Thirst + 20;
						DrainThirst();
					}
					else
					{
						// Get the increase we need to hit 101
						tAnimationSet = getGameTime();
						int nThirst = 101 - Thirst;
						Thirst = Thirst + nThirst;
						DrainThirst();
					}
				}
			}
		}
		// Sleep
		int refillSleep = 0;
		if (refillSleepCoreScenario() || refillSleep == 1) // Sleep scenario has started
		{
			// Make sure we are still
			if (TASK::IS_PED_STILL(PLAYER::PLAYER_PED_ID()))
			{
				Sleeps = 101;
				DrainSleep();
				refillSleep = 0;
				allowSave = true; // Auto save whenever the person sleeps
			}
			else // Wait until we are then set refillSleep -> 1 so we can run the core refill
			{
				while (!TASK::IS_PED_STILL(PLAYER::PLAYER_PED_ID()))
				{
					refillSleep = 1;
				}
			}
		}
	}
}

void main()
{
	Initialize();
	while (true)
	{
		update();
		WAIT(0);
	}
}

void ScriptMain()
{
	srand(GetTickCount());
	main();
}
