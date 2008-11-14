#include "..\\Settings.h"

/*typedef struct {
	PLUGIN_INFO info;
	stdstr      FullPath;
	stdstr      FileName;
	bool        InfoFunction;
} PLUGIN;

typedef std::list<PLUGIN>   PluginList;

class CPluginList  {	
	void AddPluginFromDir   ( const char * PluginDir, const char * Dir, PluginList * Plugins );
	bool ValidPluginVersion ( PLUGIN_INFO * PluginInfo );

public:
		       CPluginList   ();

	PluginList GetPluginList ( void );
	void       DllAbout      ( void * hParent, const char * PluginFile );
};
*/

class CPluginList  
{
public:
	typedef struct {
		PLUGIN_INFO Info;
		bool        AboutFunction;
		CPath       FullPath;
		stdstr      FileName;
	} PLUGIN;

private:	
	typedef std::vector<PLUGIN>   PluginList;

	PluginList m_PluginList;
	CPath      m_PluginDir;

	void AddPluginFromDir   ( CPath Dir);
	bool ValidPluginVersion ( PLUGIN_INFO & PluginInfo );

public:
	   CPluginList(bool bAutoFill = true);
	  ~CPluginList();

	bool     LoadList       ( void );
	int      GetPluginCount ( void ) const;
	const PLUGIN * GetPluginInfo  ( int indx ) const;
};
