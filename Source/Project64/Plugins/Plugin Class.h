#pragma once
#include <list>

typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned __int64    QWORD;

#ifndef PLUGIN_INFO_STRUCT
#define PLUGIN_INFO_STRUCT

typedef struct {
	WORD Version;        /* Should be set to 1 */
	WORD Type;           /* Set to PLUGIN_TYPE_GFX */
	char Name[100];      /* Name of the DLL */

	/* If DLL supports memory these memory options then set them to TRUE or FALSE
	   if it does not support it */
	BOOL NormalMemory;   /* a normal BYTE array */ 
	BOOL MemoryBswaped;  /* a normal BYTE array where the memory has been pre
	                          bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

#endif

// enum's
enum SETTING_DATA_TYPE {
	Data_DWORD_General,     // A unsigned int setting used anywhere
	Data_String_General,    // A string setting used anywhere
	Data_DWORD_Game,        // A unsigned int associated with the current game
	Data_String_Game,       // A string associated with the current game
	Data_DWORD_RDB,         // A unsigned int associated with the current game in the rom database
	Data_String_RDB,        // A string associated with the current game in the rom database
};

typedef struct {
	DWORD  dwSize;
	int    DefaultStartRange;
	int    SettingStartRange;
	int    MaximumSettings;
	int    NoDefault;
	int    DefaultLocation;
	void * handle;
	unsigned int (*GetSetting)      ( void * handle, int ID );
	const char * (*GetSettingSz)    ( void * handle, int ID, char * Buffer, int BufferLen );
    void         (*SetSetting)      ( void * handle, int ID, unsigned int Value );
    void         (*SetSettingSz)    ( void * handle, int ID, const char * Value );
	void         (*RegisterSetting) ( void * handle, int ID, int DefaultID, SettingDataType Type, 
                                      SettingType Location, const char * Category, const char * DefaultStr, DWORD Value );
	void         (*UseUnregisteredSetting) (int ID);
} PLUGIN_SETTINGS;

typedef struct {
	unsigned int (*FindSystemSettingId) ( void * handle, const char * Name );
} PLUGIN_SETTINGS2;

enum PLUGIN_TYPE {
	PLUGIN_TYPE_NONE		=	0,
	PLUGIN_TYPE_RSP			=	1,
	PLUGIN_TYPE_GFX			=	2,
	PLUGIN_TYPE_AUDIO		=	3,
	PLUGIN_TYPE_CONTROLLER	=	4,
};

class CSettings; 
class CMainGui;
class CGfxPlugin; class CAudioPlugin; class CRSP_Plugin; class CControl_Plugin;

class CPlugins :
	private CDebugSettings
{
public:
	//Functions
	CPlugins (const stdstr & PluginDir );
	~CPlugins ();

	bool Initiate           ( void );
	bool InitiateMainThread ( void );
	void SetRenderWindows   ( CMainGui * RenderWindow, CMainGui * DummyWindow );
	void ConfigPlugin       ( DWORD hParent, PLUGIN_TYPE Type );
	bool CopyPlugins        ( const stdstr & DstDir ) const;
	void Reset              ( void );
	void Reset              ( PLUGIN_TYPE Type );
	void GameReset          ( void );
	void ShutDownPlugins    ( void );

	inline CGfxPlugin      * Gfx     ( void) const { return m_Gfx;     };
	inline CAudioPlugin    * Audio   ( void) const { return m_Audio;   };
	inline CRSP_Plugin     * RSP     ( void) const { return m_RSP;     };
	inline CControl_Plugin * Control ( void) const { return m_Control; };

private:
	CPlugins(void);							// Disable default constructor
	CPlugins(const CPlugins&);				// Disable copy constructor
	CPlugins& operator=(const CPlugins&);	// Disable assignment

	void CreatePlugins    ( void );
	void CreatePluginDir  ( const stdstr & DstDir ) const;

	static void PluginChanged ( CPlugins * _this );

	//Common Classes
	CMainGui * m_RenderWindow;
	CMainGui * m_DummyWindow;

	stdstr  const m_PluginDir;

	//Plugins
	CGfxPlugin      * m_Gfx;
	CAudioPlugin    * m_Audio;
	CRSP_Plugin     * m_RSP;
	CControl_Plugin * m_Control;

	stdstr m_GfxFile;
	stdstr m_AudioFile;
	stdstr m_RSPFile;
	stdstr m_ControlFile;
};

//Dummy Functions
void DummyCheckInterrupts ( void );
void DummyFunction (void);

