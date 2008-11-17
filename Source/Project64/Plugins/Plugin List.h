#include "..\\Settings.h"

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
