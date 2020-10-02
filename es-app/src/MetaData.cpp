#include "MetaData.h"

#include "utils/FileSystemUtil.h"
#include "Log.h"
#include <pugixml/src/pugixml.hpp>

MetaDataDecl gameDecls[] = {
	// key,         type,                   default,            statistic,  name in GuiMetaDataEd,  prompt in GuiMetaDataEd
	{"name",        MD_STRING,              "",                 false,      "nombre",                 "nombre del juego"},
	{"sortname",    MD_STRING,              "",                 false,      "orden alfabetico",             "introducir nombre corto"},
	{"desc",        MD_MULTILINE_STRING,    "",                 false,      "descripcion",          "descripcion del juego"},
	{"image",       MD_PATH,                "",                 false,      "imagen",                "ruta de la imagen"},
	{"video",       MD_PATH     ,           "",                 false,      "video",                "ruta del video"},
	{"marquee",     MD_PATH,                "",                 false,      "marquesina",              "ruta de la marquesina"},
	{"thumbnail",   MD_PATH,                "",                 false,      "miniatura",            "ruta a miniatura"},
	{"rating",      MD_RATING,              "0.000000",         false,      "clasificacion",               "introducir clasificación"},
	{"releasedate", MD_DATE,                "not-a-date-time",  false,      "fecha de lanzamiento",         "fecha de lanzamiento"},
	{"developer",   MD_STRING,              "unknown",          false,      "desarolladora",            "desarolladora del juego"},
	{"publisher",   MD_STRING,              "unknown",          false,      "publicadora",            "publicadora o editor"},
	{"genre",       MD_STRING,              "unknown",          false,      "genero",                "genero del juego"},
	{"players",     MD_INT,                 "1",                false,      "jugadores",              "numero de jugadores"},
	{"favorite",    MD_BOOL,                "false",            false,      "favoritos",             "favorito off/on"},
	{"hidden",      MD_BOOL,                "false",            false,      "oculto",               "oculto off/on"},
	{"kidgame",     MD_BOOL,                "false",            false,      "infantil kidgame",              "juego infantil off/on"},
	{"playcount",   MD_INT,                 "0",                true,       "veces jugado",           "numero de veces jugado"},
	{"lastplayed",  MD_TIME,                "0",                true,       "jugado por ultima vez",          "fecha de ultima partida"}
};
const std::vector<MetaDataDecl> gameMDD(gameDecls, gameDecls + sizeof(gameDecls) / sizeof(gameDecls[0]));

MetaDataDecl folderDecls[] = {
	{"name",        MD_STRING,              "",                 false,      "nombre",                 "nombre del juego"},
	{"sortname",    MD_STRING,              "",                 false,      "orden alfabetico",             "introducir nombre corto"},
	{"desc",        MD_MULTILINE_STRING,    "",                 false,      "descripcion",          "descripcion del juego"},
	{"image",       MD_PATH,                "",                 false,      "imagen",                "ruta de la imagen"},
	{"thumbnail",   MD_PATH,                "",                 false,      "miniatura",            "ruta a miniatura"},
	{"video",       MD_PATH,                "",                 false,      "video",                "ruta del video"},
	{"marquee",     MD_PATH,                "",                 false,      "marquesina",              "ruta de la marquesina"},
	{"rating",      MD_RATING,              "0.000000",         false,      "clasificacion",               "introducir clasificación"},
	{"releasedate", MD_DATE,                "not-a-date-time",  false,      "fecha de lanzamiento",         "fecha de lanzamiento"},
	{"developer",   MD_STRING,              "unknown",          false,      "desarolladora",            "desarolladora del juego"},
	{"publisher",   MD_STRING,              "unknown",          false,      "publicadora",            "publicadora o editor"},
	{"genre",       MD_STRING,              "unknown",          false,      "genero",                "genero del juego"},
	{"players",     MD_INT,                 "1",                false,      "jugadores",              "numero de jugadores"}
};
const std::vector<MetaDataDecl> folderMDD(folderDecls, folderDecls + sizeof(folderDecls) / sizeof(folderDecls[0]));

const std::vector<MetaDataDecl>& getMDDByType(MetaDataListType type)
{
	switch(type)
	{
	case GAME_METADATA:
		return gameMDD;
	case FOLDER_METADATA:
		return folderMDD;
	}

	LOG(LogError) << "Invalid MDD type";
	return gameMDD;
}



MetaDataList::MetaDataList(MetaDataListType type)
	: mType(type), mWasChanged(false)
{
	const std::vector<MetaDataDecl>& mdd = getMDD();
	for(auto iter = mdd.cbegin(); iter != mdd.cend(); iter++)
		set(iter->key, iter->defaultValue);
}


MetaDataList MetaDataList::createFromXML(MetaDataListType type, pugi::xml_node& node, const std::string& relativeTo)
{
	MetaDataList mdl(type);

	const std::vector<MetaDataDecl>& mdd = mdl.getMDD();

	for(auto iter = mdd.cbegin(); iter != mdd.cend(); iter++)
	{
		pugi::xml_node md = node.child(iter->key.c_str());
		if(md)
		{
			// if it's a path, resolve relative paths
			std::string value = md.text().get();
			if (iter->type == MD_PATH)
			{
				value = Utils::FileSystem::resolveRelativePath(value, relativeTo, true);
			}
			mdl.set(iter->key, value);
		}else{
			mdl.set(iter->key, iter->defaultValue);
		}
	}

	return mdl;
}

void MetaDataList::appendToXML(pugi::xml_node& parent, bool ignoreDefaults, const std::string& relativeTo) const
{
	const std::vector<MetaDataDecl>& mdd = getMDD();

	for(auto mddIter = mdd.cbegin(); mddIter != mdd.cend(); mddIter++)
	{
		auto mapIter = mMap.find(mddIter->key);
		if(mapIter != mMap.cend())
		{
			// we have this value!
			// if it's just the default (and we ignore defaults), don't write it
			if(ignoreDefaults && mapIter->second == mddIter->defaultValue)
				continue;

			// try and make paths relative if we can
			std::string value = mapIter->second;
			if (mddIter->type == MD_PATH)
				value = Utils::FileSystem::createRelativePath(value, relativeTo, true);

			parent.append_child(mapIter->first.c_str()).text().set(value.c_str());
		}
	}
}

void MetaDataList::set(const std::string& key, const std::string& value)
{
	mMap[key] = value;
	mWasChanged = true;
}

const std::string& MetaDataList::get(const std::string& key) const
{
	return mMap.at(key);
}

int MetaDataList::getInt(const std::string& key) const
{
	return atoi(get(key).c_str());
}

float MetaDataList::getFloat(const std::string& key) const
{
	return (float)atof(get(key).c_str());
}

bool MetaDataList::wasChanged() const
{
	return mWasChanged;
}

void MetaDataList::resetChangedFlag()
{
	mWasChanged = false;
}
