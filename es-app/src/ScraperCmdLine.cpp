#include "ScraperCmdLine.h"

#include "Log.h"
#include "platform.h"
#include "SystemData.h"
#include <iostream>
#include <signal.h>
#if defined(__linux__)
#include <unistd.h>
#elif defined(WIN32)
#include <Windows.h>
#endif

std::ostream& out = std::cout;

void handle_interrupt_signal(int /*p*/)
{
	sleep(50);

	LOG(LogInfo) << "Interrupt received during scrape...";

	SystemData::deleteSystems();

	exit(1);
}

int run_scraper_cmdline()
{
	out << "EmulationStation scraper\n";
	out << "========================\n";
	out << "\n";

	signal(SIGINT, handle_interrupt_signal);

	//==================================================================================
	//filter
	//==================================================================================
	enum FilterChoice
	{
		FILTER_MISSING_IMAGES,
		FILTER_ALL
	};

	int filter_choice;
	do {
		out << "Seleccione un filtro para los juegos a scrapear:\n";
		out << FILTER_MISSING_IMAGES << " - juegos sin carátula\n";
		out << FILTER_ALL << " - todos los juegos, es posible que se sustituyan los datos anteriores\n";

		std::cin >> filter_choice;
		std::cin.ignore(1, '\n'); //skip the unconsumed newline
	} while(filter_choice < FILTER_MISSING_IMAGES || filter_choice > FILTER_ALL);

	out << "\n";

	//==================================================================================
	//platforms
	//==================================================================================

	std::vector<SystemData*> systems;

	out << "Puedes scrapear plataformas específicas, o scrapear todas.\n";
	out << "¿Quieres scrapear todas las plataformas? (s/n)\n";

	std::string system_choice;
	std::getline(std::cin, system_choice);

	if(system_choice == "y" || system_choice == "Y" || system_choice == "s" || system_choice == "S")
	{
		out << "Se scrapearan todas las plataformas.\n";
		for(auto i = SystemData::sSystemVector.cbegin(); i != SystemData::sSystemVector.cend(); i++)
		{
			out << "   " << (*i)->getName() << " (" << (*i)->getGameCount() << " juego" << ((*i)->getGameCount() > 1 ? "s" : "") << ")\n";
			systems.push_back(*i);
		}

	}else{
		std::string sys_name;

		out << "Introduce los nombres de las plataformas que quieres scrapear, una por línea.\n";
		out << "No escribas nada y pulsa Intro cuando quieras continuar.\n";

		do {
			for(auto i = SystemData::sSystemVector.cbegin(); i != SystemData::sSystemVector.cend(); i++)
			{
				if(std::find(systems.cbegin(), systems.cend(), (*i)) != systems.cend())
					out << " C ";
				else
					out << "   ";

				out << "\"" << (*i)->getName() << "\" (" << (*i)->getGameCount() << " juego" << ((*i)->getGameCount() > 1 ? "s" : "") << ")\n";
			}

			std::getline(std::cin, sys_name);

			if(sys_name.empty())
				break;

			bool found = false;
			for(auto i = SystemData::sSystemVector.cbegin(); i != SystemData::sSystemVector.cend(); i++)
			{
				if((*i)->getName() == sys_name)
				{
					systems.push_back(*i);
					found = true;
					break;
				}
			}

			if(!found)
				out << "No se encontró el sistema.\n";

		} while(true);
	}

	//==================================================================================
	//manual mode
	//==================================================================================

	out << "\n";
	out << "Puedes dejar que el scraper intente seleccionar automáticamente el mejor resultado, o\n";
	out << "puedes aprobar manualmente cada resultado. Este \"modo manual\" es mucho más preciso.\n";
	out << "Se recomienda encarecidamente que uses el modo manual a no ser que tengas una colección inmensa.\n";
	out << "¿Scrapear en modo manual? (s/n)\n";

	std::string manual_mode_str;
	std::getline(std::cin, manual_mode_str);

	bool manual_mode = false;

	if(manual_mode_str == "y" || manual_mode_str == "Y" || manual_mode_str == "s" || manual_mode_str == "S")
	{
		manual_mode = true;
		out << "¡Scrapeando en modo manual!\n";
	}else{
		out << "¡Scrapeando en modo automático!\n";
	}

	//==================================================================================
	//scraping
	//==================================================================================
	out << "\n";
	out << "Venga, ¡al turrón!\n";
	out << "=============================\n";

	/*
	std::shared_ptr<Scraper> scraper = Settings::getInstance()->getScraper();
	for(auto sysIt = systems.cbegin(); sysIt != systems.cend(); sysIt++)
	{
		std::vector<FileData*> files = (*sysIt)->getRootFolder()->getFilesRecursive(GAME);

		ScraperSearchParams params;
		params.system = (*sysIt);

		for(auto gameIt = files.cbegin(); gameIt != files.cend(); gameIt++)
		{
			params.nameOverride = "";
			params.game = *gameIt;

			//print original search term
			out << getCleanFileName(params.game->getPath()) << "...\n";

			//need to take into account filter_choice
			if(filter_choice == FILTER_MISSING_IMAGES)
			{
				if(!params.game->metadata.get("image").empty()) //maybe should also check if the image file exists/is a URL
				{
					out << "   Skipping, metadata \"image\" entry is not empty.\n";
					continue;
				}
			}

			//actually get some results
			do {
				std::vector<MetaDataList> mdls = scraper->getResults(params);

				//no results
				if(mdls.size() == 0)
				{
					if(manual_mode)
					{
						//in manual mode let the user enter a custom search
						out << "   NO RESULTS FOUND! Enter a new name to search for, or nothing to skip.\n";

						std::getline(std::cin, params.nameOverride);

						if(params.nameOverride.empty())
						{
							out << "   Skipping...\n";
							break;
						}

						continue;

					}else{
						out << "   NO RESULTS FOUND! Skipping...\n";
						break;
					}
				}

				//some results
				if(manual_mode)
				{
					//print list of choices
					for(unsigned int i = 0; i < mdls.size(); i++)
					{
						out << "   " << i << " - " << mdls.at(i).get("name") << "\n";
					}

					int choice = -1;
					std::string choice_str;

					out << "Your choice: ";

					std::getline(std::cin, choice_str);
					std::stringstream choice_buff(choice_str); //convert to int
					choice_buff >> choice;

					if(choice >= 0 && choice < (int)mdls.size())
					{
						params.game->metadata = mdls.at(choice);
						break;
					}else{
						out << "Invalid choice.\n";
						continue;
					}

				}else{
					//automatic mode
					//always choose the first choice
					out << "   name -> " << mdls.at(0).get("name") << "\n";
					params.game->metadata = mdls.at(0);
					break;
				}

			} while(true);

			out << "===================\n";
		}
	}

	out << "\n\n";
	out << "Downloading boxart...\n";

	for(auto sysIt = systems.cbegin(); sysIt != systems.cend(); sysIt++)
	{
		std::vector<FileData*> files = (*sysIt)->getRootFolder()->getFilesRecursive(GAME);

		for(auto gameIt = files.cbegin(); gameIt != files.cend(); gameIt++)
		{
			FileData* game = *gameIt;
			const std::vector<MetaDataDecl>& mdd = game->metadata.getMDD();
			for(auto i = mdd.cbegin(); i != mdd.cend(); i++)
			{
				std::string key = i->key;
				std::string url = game->metadata.get(key);

				if(i->type == MD_IMAGE_PATH && HttpReq::isUrl(url))
				{
					std::string urlShort = url.substr(0, url.length() > 35 ? 35 : url.length());
					if(url.length() != urlShort.length()) urlShort += "...";

					out << "   " << game->metadata.get("name") << " [from: " << urlShort << "]...\n";

					ScraperSearchParams p;
					p.game = game;
					p.system = *sysIt;
					game->metadata.set(key, downloadImage(url, getSaveAsPath(p, key, url)));
					if(game->metadata.get(key).empty())
					{
						out << "     FAILED! Skipping.\n";
						game->metadata.set(key, url); //result URL to what it was if download failed, retry some other time
					}
				}
			}
		}
	}


	out << "\n\n";
	out << "==============================\n";
	out << "SCRAPE COMPLETE!\n";
	out << "==============================\n";
	*/

	out << "\n\n";
	out << "ES BROMA, ESTO ESTÁ AÚN EN DESARROLLO\n";

	return 0;
}
