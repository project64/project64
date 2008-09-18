#include "..\\Settings.h"

typedef struct {
	PLUGIN_INFO info;
	stdstr      FullPath;
	stdstr      FileName;
	bool        InfoFunction;
} PLUGIN;

typedef std::list<PLUGIN>   PluginList;

class CPluginList  {
	CSettings * _Settings;
	
	void AddPluginFromDir   ( const char * PluginDir, const char * Dir, PluginList * Plugins );
	bool ValidPluginVersion ( PLUGIN_INFO * PluginInfo );

public:
		       CPluginList   ( CSettings * Settings);

	PluginList GetPluginList ( void );
	void       DllAbout      ( void * hParent, const char * PluginFile );
};
