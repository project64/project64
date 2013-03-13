#define BaseDir ExtractFilePath(ExtractFilePath(ExtractFilePath(SourcePath)))
#define AppVersion GetFileVersion(BaseDir + "\Bin\" + Configuration + "\Project64.exe")

[Setup]
AppName=Project64
AppVersion={#AppVersion}
DefaultDirName={pf}\Project64 2.0
VersionInfoVersion={#AppVersion}
OutputDir={#BaseDir}\Bin\{#Configuration}
OutputBaseFilename=Setup Project64 2.0
VersionInfoDescription=Installation Setup of Project64 2.0
Compression=lzma2/ultra64

[Files]
Source: "{#BaseDir}\Bin\{#Configuration}\Project64.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BaseDir}\Docs\Release Docs\PJgameFAQ.chm"; DestDir: "{app}"
Source: "{#BaseDir}\Docs\Release Docs\Project64.chm"; DestDir: "{app}"
Source: "{#BaseDir}\Docs\Release Docs\Cheat - Changes.txt"; DestDir: "{app}\Docs"
Source: "{#BaseDir}\Docs\Release Docs\Cheat - Readme.txt"; DestDir: "{app}\Docs"
Source: "{#BaseDir}\Docs\Release Docs\GameFAQ - WhatsNew.txt"; DestDir: "{app}\Docs"
Source: "{#BaseDir}\Docs\Release Docs\RDB - WhatsNew.txt"; DestDir: "{app}\Docs"
Source: "{#BaseDir}\Docs\Release Docs\RDX - ReadMe (Unofficial).txt"; DestDir: "{app}\Docs"
Source: "{#BaseDir}\Docs\Release Docs\Readme.txt"; DestDir: "{app}\Docs"
Source: "{#BaseDir}\Docs\Release Docs\Whatsnew.txt"; DestDir: "{app}\Docs"
Source: "{#BaseDir}\Docs\Release Docs\3rd Party Plugins\Adaptoid.txt"; DestDir: "{app}\Docs\3rd Party Plugins"
Source: "{#BaseDir}\Docs\Release Docs\3rd Party Plugins\N-Rage - Readme.txt"; DestDir: "{app}\Docs\3rd Party Plugins"
Source: "{#BaseDir}\Docs\Release Docs\Plugin Specs\Audio #1.1.h"; DestDir: "{app}\Docs\Plugin Specs"
Source: "{#BaseDir}\Docs\Release Docs\Plugin Specs\Controller #1.1.h"; DestDir: "{app}\Docs\Plugin Specs"
Source: "{#BaseDir}\Docs\Release Docs\Plugin Specs\Gfx #1.3.h"; DestDir: "{app}\Docs\Plugin Specs"
Source: "{#BaseDir}\Docs\Release Docs\Plugin Specs\Plugin Spec history.txt"; DestDir: "{app}\Docs\Plugin Specs"
Source: "{#BaseDir}\Docs\Release Docs\Plugin Specs\Rsp #1.1.h"; DestDir: "{app}\Docs\Plugin Specs"
Source: "{#BaseDir}\Config\Blank Project64.rdb"; DestDir: "{app}"; DestName: "Project64.rdb"
Source: "{#BaseDir}\Config\Project64.cht"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Project64.rdb"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Project64.rdx"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Lang\Brazilian Portuguese.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Bulgarian.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Chinese (Simplified).pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Chinese (Taiwan).pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Chinese (Traditional).pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\ChineseB5.pj.lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\ChineseGB.pj.lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Czech.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Danish.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Dutch.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\English.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\English_alternative.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Finnish.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\French.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\German.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\German_int.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\German_localised.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Greek.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Hungarian.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Italian.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Italian_alternative.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Japanese.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Lithuanian.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Norwegian.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Polish.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Russian.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Spanish (South America).pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Spanish (Spain).pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Spanish.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Swedish.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\T-Chinese.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Ukrainian.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Plugin\1.6 Plugins\Adaptoid_v1_0.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\1.6 Plugins\Jabo_DInput.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\1.6 Plugins\Jabo_Direct3D6.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\1.6 Plugins\Jabo_Direct3D8.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\1.6 Plugins\Jabo_Dsound.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\1.6 Plugins\No Sound.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\1.6 Plugins\NRage_DInput8_V2.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\1.6 Plugins\RSP.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\1.6 Plugins\Zilmar_Audio.dll"; DestDir: "{app}\Plugin\1.6 Plugins"
Source: "{#BaseDir}\Plugin\Audio\Jabo_Dsound.dll"; DestDir: "{app}\Plugin\Audio"
Source: "{#BaseDir}\Plugin\GFX\Jabo_Direct3D8.dll"; DestDir: "{app}\Plugin\GFX"
Source: "{#BaseDir}\Plugin\Input\Jabo_DInput.dll"; DestDir: "{app}\Plugin\Input"
Source: "{#BaseDir}\Plugin\RSP\RSP 1.7.dll"; DestDir: "{app}\Plugin\RSP"

[Dirs]
Name: "{app}\Config"; Permissions: users-modify
Name: "{app}\Logs"; Permissions: users-modify
Name: "{app}\Save"; Permissions: users-modify
Name: "{app}\Screenshots"; Permissions: users-modify

[Icons]
Name: "{group}\Project64"; Filename: "{app}\Project64.exe"