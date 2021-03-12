#include <windows.h>
#include <common/IniFileClass.h>
#include <set>

int main (int argc, char *argv[])
{
	if (argc < 3)
	{
		return 0;
	}

	struct insensitive_compare
	{
		bool operator() (const std::string & a, const std::string & b) const { return _stricmp(a.c_str(),b.c_str()) < 0; }
	};

	CIniFile RomIniFile(argv[1]);
	CIniFileBase::SectionList Sections;
	RomIniFile.GetVectorOfSections(Sections);

	typedef std::map<std::string,std::vector<std::string>, insensitive_compare> strmap;
	typedef std::set<std::string, insensitive_compare> strset;
	
	strset PDNames;
	PDNames.insert("1964 Demo by Steb (PD)");
	PDNames.insert("2 Blokes'n'Armchair");
	PDNames.insert("3DS Model Conversion by Snake (PD)");
	PDNames.insert("77a by Count0 (POM '98) (PD)");
	PDNames.insert("77a Special Edition by Count0 (PD)");
	PDNames.insert("Absolute Crap Intro 1 by Kid Stardust (PD)");
	PDNames.insert("Absolute Crap Intro 2 by Lem (PD)");
	PDNames.insert("Alienstyle Intro by Renderman (PD)");
	PDNames.insert("Alienstyle Intro by Renderman (PD) [a1]");
	PDNames.insert("Alleycat 64 by Dosin (POM '99) (PD)");
	PDNames.insert("Attax64 by Pookae (POM '99) (PD)");
	PDNames.insert("BB SRAM Manager (PD)");
	PDNames.insert("Berney Must Die! by Nop_ (POM '99) (PD)");
	PDNames.insert("Bike Race '98 V1.0 by NAN (PD)");
	PDNames.insert("Bike Race '98 V1.2 by NAN (PD)");
	PDNames.insert("Birthday Demo for Steve by Nep (PD)");
	PDNames.insert("Boot Emu by Jovis (PD)");
	PDNames.insert("CD64 Memory Test (PD)");
	PDNames.insert("Chaos 89 Demo (PD)");
	PDNames.insert("Christmas Flame Demo (PD)");
	PDNames.insert("Chrome Demo - Enhanced (PD)");
	PDNames.insert("Chrome Demo - Original (PD)");
	PDNames.insert("Cliffi's Little Intro by Cliffi (POM '99) (PD)");
	PDNames.insert("Congratulations Demo for SPLiT by Widget and Immo (PD)");
	PDNames.insert("Cube Demo (PD)");
	PDNames.insert("CZN Module Player (PD)");
	PDNames.insert("Display List Ate My Mind Demo by Kid Stardust (PD)");
	PDNames.insert("DKONG Demo (PD)");
	PDNames.insert("DKONG Demo (PD)");
	PDNames.insert("DS1 Manager 1.0 by RBubba (PD)");
	PDNames.insert("DS1 Manager 1.1 by RBubba (PD)");
	PDNames.insert("Dynamix Intro (Hidden Song) by Widget and Immo (PD)");
	PDNames.insert("Dynamix Intro by Widget and Immo (PD)");
	PDNames.insert("Dynamix Readme by Widget and Immo (PD");
	PDNames.insert("Eurasia first N64 Intro by Sispeo (PD)");
	PDNames.insert("Eurasia Intro by Ste (PD)");
	PDNames.insert("Evek - V64jr Save Manager by WT_Riker (PD)");
	PDNames.insert("Explode Demo by NaN (PD)");
	PDNames.insert("Fire_Demo_by_Lac_(PD)");
	PDNames.insert("Fireworks Demo by CrowTRobo (PD)");
	PDNames.insert("Fish Demo by NaN (PD)");
	PDNames.insert("Fogworld USA Demo (PD)");
	PDNames.insert("Fractal Zoomer Demo by RedboX (PD)");
	PDNames.insert("Freekworld BBS Intro by Rene (PD)");
	PDNames.insert("Freekworld New Intro by Ste (PD)");
	PDNames.insert("Friendship Demo by Renderman (PD)");
	PDNames.insert("Frogger 2 (U) (Unreleased Alpha)");
	PDNames.insert("Game Boy Emulator (POM '98) (PD)");
	PDNames.insert("Game Boy Emulator + Super Mario 3 (PD)");
	PDNames.insert("GBlator for CD64 (PD)");
	PDNames.insert("GBlator for NTSC Dr V64 (PD)");
	PDNames.insert("GBlator for PAL Dr V64 (PD)");
	PDNames.insert("Ghemor - CD64 Xfer & Save Util (CommsLink) by CrowTRobo (PD)");
	PDNames.insert("Ghemor - CD64 Xfer & Save Util (Parallel) by CrowTRobo (PD)");
	PDNames.insert("GT Demo (PD)");
	PDNames.insert("Hard Pom '99 Demo by TS_Garp (POM '99) (PD)");
	PDNames.insert("HardCoded by Iceage");
	PDNames.insert("Heavy 64 Demo by Destop (PD)");
	PDNames.insert("HiRes CFB Demo (PD)");
	PDNames.insert("HSD Quick Intro (PD)");
	PDNames.insert("IPL4ROM (J)");
	PDNames.insert("JPEG Slideshow Viewer by Garth Elgar (PD)");
	PDNames.insert("Kid Stardust Intro with Sound by Kid Stardust (PD)");
	PDNames.insert("LaC MOD Player - The Temple Gates (PD)");
	PDNames.insert("LCARS Demo by WT Riker (PD)");
	PDNames.insert("Light Force First N64 Demo by Fractal (PD)");
	PDNames.insert("Liner V1.00 by Colin Phillipps of Memir (PD)");
	PDNames.insert("MAME 64 Emulator Beta 3 (PD)");
	PDNames.insert("MAME 64 Emulator V1.0 (PD)");
	PDNames.insert("Manic Miner - Hidden Levels by RedboX (PD)");
	PDNames.insert("Manic Miner by RedboX (PD)");
	PDNames.insert("MeeTING Demo by Renderman (PD)");
	PDNames.insert("Mempack Manager for Jr 0.9 by deas (PD)");
	PDNames.insert("Mempack Manager for Jr 0.9b by deas (PD)");
	PDNames.insert("Mempack Manager for Jr 0.9c by deas (PD)");
	PDNames.insert("Mempack to N64 Uploader by Destop V1.0 (PD)");
	PDNames.insert("Mind Present Demo 0 by Widget and Immo (POM '98) (PD)");
	PDNames.insert("Mind Present Demo Readme (POM '98) (PD)");
	PDNames.insert("Mini Racers (Unreleased)");
	PDNames.insert("MMR by Count0 (PD)");
	PDNames.insert("Namp64 - N64 MP3-Player by Obsidian (PD)");
	PDNames.insert("Money Creates Taste Demo by Count0 (POM '99) (PD)");
	PDNames.insert("Mortal Kombat SRAM Loader (PD)");
	PDNames.insert("MSFTUG Intro #1 by Lac (PD)");
	PDNames.insert("N64 Scene Gallery by CALi (PD)");
	PDNames.insert("N64 Stars Demo (PD)");
	PDNames.insert("My Angel Demo (PD)");
	PDNames.insert("NBC First Intro by CALi (PD)");
	PDNames.insert("NBC-LFC Kings of Porn Vol 01 (PD)");
	PDNames.insert("NBC-LFC Kings of Porn Vol 01 [a1} (PD)");
	PDNames.insert("NBCG Special Edition (PD)");
	PDNames.insert("NBCG's Kings of Porn Demo (PD)");
	PDNames.insert("NBCG's Tag Gallery 01 by CALi (PD)");
	PDNames.insert("NBCrew 2 Demo (PD)");
	PDNames.insert("Neon64 First Public Beta Release by HCS (PD)");
	PDNames.insert("Neon64 First Public Beta Release V2 by HCS (PD)");
	PDNames.insert("Nintendo Family by CALi (PD)");
	PDNames.insert("Nintendo On My Mind Demo by Kid Stardust (PD)");
	PDNames.insert("Nintendo WideBoy 64 by SonCrap (PD)");
	PDNames.insert("Nintro64 Demo by Lem (POM '98) (PD)");
	PDNames.insert("NuFan Demo by Kid Stardust (PD)");
	PDNames.insert("O.D.T (Or Die Trying) (E) (M5) (Unreleased Final)");
	PDNames.insert("O.D.T (Or Die Trying) (U) (M3) (Unreleased Final)");
	PDNames.insert("Oerjan Intro by Oerjan (POM '99) (PD)");
	PDNames.insert("Pamela Demo (padded) (PD)");
	PDNames.insert("Pause Demo by RedboX (PD)");
	PDNames.insert("PC-Engine 64 Emulator (POM '99) (PD)");
	PDNames.insert("Pip's Pong by Mr. Pips (PD)");
	PDNames.insert("Pip's Porn Pack 1 by Mr. Pips (PD");
	PDNames.insert("Pip's Porn Pack 2 by Mr. Pips (POM '99) (PD)");
	PDNames.insert("Pip's Porn Pack 3 by Mr. Pips (PD)");
	PDNames.insert("Pip's RPGs Beta 12 by Mr. Pips (PD)");
	PDNames.insert("Pip's RPGs Beta 14 by Mr. Pips (PD)");
	PDNames.insert("Pip's RPGs Beta 15 by Mr. Pips (PD)");
	PDNames.insert("Pip's RPGs Beta 2 by Mr. Pips (PD)");
	PDNames.insert("Pip's RPGs Beta 3 by Mr. Pips (PD)");
	PDNames.insert("Pip's RPGs Beta 6 by Mr. Pips (PD)");
	PDNames.insert("Pip's RPGs Beta 7 by Mr. Pips (PD)");
	PDNames.insert("Pip's RPGs Beta x (PD");
	PDNames.insert("Pip's RPGs Beta x (PD) [a1");
	PDNames.insert("Pip's World Game 1 by Mr. Pips (PD");
	PDNames.insert("Pip's World Game 2 by Mr. Pips (PD)");
	PDNames.insert("Pipendo by Mr. Pips (PD)");
	PDNames.insert("Planet Console Intro (PD)");
	PDNames.insert("Plasma Demo (PD)");
	PDNames.insert("POMbaer Demo by Kid Stardust (POM '99) (PD)");
	PDNames.insert("POMolizer Demo by Renderman (POM '99) (PD)");
	PDNames.insert("Pong V0.01 by Omsk (PD)");
	PDNames.insert("R.I.P. Jay Demo by Ste (PD)");
	PDNames.insert("Pom Part 1 Demo (PD");
	PDNames.insert("Pom Part 2 Demo (PD)");
	PDNames.insert("Pom Part 3 Demo (PD)");
	PDNames.insert("Pom Part 4 Demo (PD)");
	PDNames.insert("Pom Part 5 Demo (PD)");
	PDNames.insert("Pong V0.01 by Omsk (PD)");
	PDNames.insert("R.I.P. Jay Demo by Ste (PD)");
	PDNames.insert("RADWAR 2K Party Inv. Intro by Ayatolloh (PD)");
	PDNames.insert("RPA Site Intro by Lem (PD)");
	PDNames.insert("SLiDeS (PD)");
	PDNames.insert("SNES 9X Alpha (PD)");
	PDNames.insert("SPLiT's Nacho64 by SPLiT (PD)");
	PDNames.insert("SRAM Manager V1.0 Beta (32Mbit) (PD)");
	PDNames.insert("SRAM Manager V1.0 PAL Beta (PD)");
	PDNames.insert("SRAM Upload Tool (PD)");
	PDNames.insert("SRAM Upload Tool + Star Fox 64 SRAM (PD)");
	PDNames.insert("SRAM Uploader-Editor by BlackBag (PD)");
	PDNames.insert("SRAM to DS1 Tool by WT_Riker (PD)");
	PDNames.insert("Pong by Oman (PD)");
	PDNames.insert("Psychodelic Demo by Ste (POM '98) (PD)");
	PDNames.insert("Robotech - Crystal Dreams (U) (beta)");
	PDNames.insert("Rotating Demo USA by Rene (PD) [a1]");
	PDNames.insert("Sample Demo by Florian (PD)");
	PDNames.insert("Shag'a'Delic Demo by Steve and NEP (PD)");
	PDNames.insert("Shuffle Puck 64 (PD)");
	PDNames.insert("Simon for N64 V0.1a by Jean-Luc Picard (POM '99) (PD)");
	PDNames.insert("Sinus (PD)");
	PDNames.insert("Sitero Demo by Renderman (PD)");
	PDNames.insert("Sporting Clays by Charles Doty (PD) [a1]");
	PDNames.insert("Sporting Clays by Charles Doty (PD)");
	PDNames.insert("Spice Girls Rotator Demo by RedboX (PD) [a1]");
	PDNames.insert("Spacer by Memir (POM '99) (PD)");
	PDNames.insert("Soncrap Golden Eye Intro (PD) aka Rad's Bird");
	PDNames.insert("Soncrap Intro by RedboX (PD) aka Rad's Bird");
	PDNames.insert("Summer64 Demo by Lem (PD)");
	PDNames.insert("Super Bomberman 2 by Rider (POM '99) (PD)");
	PDNames.insert("Super Fighter Demo (PD)");
	PDNames.insert("T-Shirt Demo by Neptune and Steve (POM '98) (PD)");
	PDNames.insert("Tetris Beta Demo by FusionMan (POM '98) (PD)");
	PDNames.insert("Textlight Demo (PD) [a1]");
	PDNames.insert("The Corporation 1st Intro by i_savant (PD)");
	PDNames.insert("The Corporation 2nd Intro by TS_Garp (PD)");
	PDNames.insert("The Corporation XMAS Demo '99 by TS_Garp (PD)");
	PDNames.insert("Tom Demo (PD)");
	PDNames.insert("TopGun Demo (PD)");
	PDNames.insert("TR64 Demo by FIres and Icepir8 (PD)");
	PDNames.insert("TRON Demo (PD)");
	PDNames.insert("TRSI Intro by Ayatollah (POM '99) (PD)");
	PDNames.insert("Twintris by Twinsen (POM '98) (PD)");
	PDNames.insert("Ultra 1 Demo by Locke^ (PD)");
	PDNames.insert("UltraMSX2 V1.0 (PD)");
	PDNames.insert("UltraMSX2 V1.0 (ROMS Inserted) (PD)");
	PDNames.insert("UltraSMS V1.0 (PD)");
	PDNames.insert("Universal Bootemu V1.0 (PD)");
	PDNames.insert("Universal Bootemu V1.1 (PD)");
	PDNames.insert("Unix SRAM-Upload Utility 1.0 by Madman (PD)");
	PDNames.insert("UPload SRAM V1 by LaC (PD)");
	PDNames.insert("V64Jr 512M Backup Program by HKPhooey (PD)");
	PDNames.insert("V64Jr Backup Tool by WT_Riker (PD)");
	PDNames.insert("V64Jr Backup Tool V0.2b_Beta by RedboX (PD)");
	PDNames.insert("Vector Demo by Destop (POM '99) (PD)");
	PDNames.insert("View N64 Test Program (PD)");
	PDNames.insert("Virtual Springfield Site Intro by Presten (PD)");
	PDNames.insert("VNES64 + Galaga (PD)");
	PDNames.insert("VNES64 + Mario (PD)");
	PDNames.insert("VNES64 + Test Cart (PD)");
	PDNames.insert("VNES64 Emulator V0.12 by Jean-Luc Picard (PD)");
	PDNames.insert("Virtual Springfield Site Intro by Presten (PD)");
	PDNames.insert("VNES64 + Galaga (PD)");
	PDNames.insert("VNES64 + Mario (PD)");
	PDNames.insert("VNES64 + Test Cart (PD)");
	PDNames.insert("VNES64 Emulator V0.12 by Jean-Luc Picard (PD)");
	PDNames.insert("Wet Dreams Can Beta Demo by Immo (POM '99) (PD)");
	PDNames.insert("Wet Dreams Madeiragames Demo by Immo (POM '99) (PD)");
	PDNames.insert("Wet Dreams Main Demo by Immo (POM '99) (PD)");
	PDNames.insert("Wet Dreams Readme by Immo (POM '99) (PD)");
	PDNames.insert("XtraLife Dextrose Demo by RedboX (PD)");
	PDNames.insert("Y2K Demo by WT_Riker (PD)");
	PDNames.insert("Yoshi's Story BootEmu (PD)");

	strmap GoodNameSections, PDNameSections;
	for (size_t i = 0, n = Sections.size(); i < n; i++)
	{
		stdstr GoodName;
		if (!RomIniFile.GetString(Sections[i].c_str(),"Good Name","",GoodName))
		{
			continue;
		}
		std::vector<std::string> items;
		items.push_back(Sections[i]);
		if (PDNames.find(GoodName) == PDNames.end())
		{
			strmap::_Pairib res = GoodNameSections.insert(strmap::value_type(GoodName,items));
			if (!res.second)
			{
				res.first->second.push_back(Sections[i]);
			}
		} else {
			strmap::_Pairib res = PDNameSections.insert(strmap::value_type(GoodName,items));
			if (!res.second)
			{
				res.first->second.push_back(Sections[i]);
			}
		}
	}

	strlist NonSortedArray;
	NonSortedArray.push_back("Good Name");
	NonSortedArray.push_back("Internal Name");
	NonSortedArray.push_back("Status");
	NonSortedArray.push_back("Core Note");
	NonSortedArray.push_back("Plugin Note");

	FILE * fp = fopen(argv[2],"wb");
	std::string LastGoodName;
	bool NumberSectionStarted = false;
	for (strmap::iterator itr = GoodNameSections.begin(); itr != GoodNameSections.end(); itr++)
	{
		std::string GoodName = itr->first;
		for (size_t i = 0, n = itr->second.size(); i < n; i++)
		{
			std::string & SectionName = itr->second[i];
			strlist KeyList;
			RomIniFile.GetKeyList(SectionName.c_str(),KeyList);
			std::set<stdstr> SortedKeyList;
			for (strlist::iterator itr = KeyList.begin(); itr != KeyList.end(); itr++)
			{
				SortedKeyList.insert(*itr);
			}

			if (LastGoodName.length() == 0 || LastGoodName[0] != GoodName[0])
			{
				char section = (char)toupper(GoodName[0]);
				if (section >= '0' && section <= '9')
				{
					if (!NumberSectionStarted)
					{
						NumberSectionStarted = true;
						fprintf(fp,"//================  0-9  ================\n");
					}
				} else {
					fprintf(fp,"//================  %c  ================\n",section);
				}
			}
			LastGoodName = GoodName;
			fprintf(fp,"[%s]\n",SectionName.c_str());
			for (strlist::iterator itr = NonSortedArray.begin(); itr != NonSortedArray.end(); itr++)
			{
				std::set<stdstr>::iterator find_itr = SortedKeyList.find(itr->c_str());
				if (find_itr != SortedKeyList.end())
				{
					stdstr Value;
					if (RomIniFile.GetString(SectionName.c_str(),itr->c_str(),"",Value))
					{
						fprintf(fp,"%s=%s\n",itr->c_str(), Value.c_str());
					}
					SortedKeyList.erase(find_itr);
				}
			}

			for (std::set<stdstr>::iterator itr = SortedKeyList.begin(); itr != SortedKeyList.end(); itr++)
			{
				stdstr Value;
				if (RomIniFile.GetString(SectionName.c_str(),itr->c_str(),"",Value))
				{
					fprintf(fp,"%s=%s\n",itr->c_str(), Value.c_str());
				}
			}
			fprintf(fp,"\n",SectionName.c_str());
		}
	}

	if (PDNameSections.size() > 0)
	{
		fprintf(fp,"//================  PD  ================\n");
		fprintf(fp,"//\n// ROMs below are public domain, homebrew, and other non-commercial ROMs\n\n");
		for (strmap::iterator itr = PDNameSections.begin(); itr != PDNameSections.end(); itr++)
		{
			std::string GoodName = itr->first;
			for (size_t i = 0, n = itr->second.size(); i < n; i++)
			{
				std::string & SectionName = itr->second[i];
				strlist KeyList;
				RomIniFile.GetKeyList(SectionName.c_str(),KeyList);
				std::set<stdstr> SortedKeyList;
				for (strlist::iterator itr = KeyList.begin(); itr != KeyList.end(); itr++)
				{
					SortedKeyList.insert(*itr);
				}

				fprintf(fp,"[%s]\n",SectionName.c_str());
				for (strlist::iterator itr = NonSortedArray.begin(); itr != NonSortedArray.end(); itr++)
				{
					std::set<stdstr>::iterator find_itr = SortedKeyList.find(itr->c_str());
					if (find_itr != SortedKeyList.end())
					{
						stdstr Value;
						if (RomIniFile.GetString(SectionName.c_str(),itr->c_str(),"",Value))
						{
							fprintf(fp,"%s=%s\n",itr->c_str(), Value.c_str());
						}
						SortedKeyList.erase(find_itr);
					}
				}

				for (std::set<stdstr>::iterator itr = SortedKeyList.begin(); itr != SortedKeyList.end(); itr++)
				{
					stdstr Value;
					if (RomIniFile.GetString(SectionName.c_str(),itr->c_str(),"",Value))
					{
						fprintf(fp,"%s=%s\n",itr->c_str(), Value.c_str());
					}
				}
				fprintf(fp,"\n",SectionName.c_str());
			}
		}
	}
	fclose(fp);
	return 0;
}
