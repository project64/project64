#define BaseDir ExtractFilePath(ExtractFilePath(ExtractFilePath(SourcePath)))
#define AppVersion GetFileVersion(BaseDir + "\Bin\" + Configuration + "\Project64.exe")

[Setup]
AppId={{BEB5FB69-4080-466F-96C4-F15DF271718B}
AppName=Project64
AppVersion={#AppVersion}
DefaultDirName={pf32}\Project64 2.3
VersionInfoVersion={#AppVersion}
OutputDir={#BaseDir}\Bin\{#Configuration}
OutputBaseFilename=Setup Project64 2.3
VersionInfoDescription=Installation Setup of Project64 2.3
Compression=lzma2/ultra64
WizardImageFile=Installer-Sidebar.bmp
WizardSmallImageFile=Pj64LogoSmallImage.bmp
DisableProgramGroupPage=yes
DisableReadyPage=yes
Uninstallable=not IsTaskSelected('portablemode')
UninstallDisplayIcon={uninstallexe}
SetupIconFile={#BaseDir}\Source\Project64\UserInterface\Icons\pj64.ico

[Run]
Filename: "{app}\Project64.exe"; Description: "{cm:LaunchProgram,{#StringChange('Project64', '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Files]
Source: "{#BaseDir}\Bin\{#Configuration}\Project64.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BaseDir}\Config\Project64Video.rdb"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Project64.cht"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Project64.rdb"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Project64.rdx"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Lang\*.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Plugin\Audio\Jabo_Dsound.dll"; DestDir: "{app}\Plugin\Audio"
Source: "{#BaseDir}\Plugin\GFX\Jabo_Direct3D8.dll"; DestDir: "{app}\Plugin\GFX"
Source: "{#BaseDir}\Plugin\GFX\Project64-Video.dll"; DestDir: "{app}\Plugin\GFX"
Source: "{#BaseDir}\Plugin\Input\PJ64_NRage.dll"; DestDir: "{app}\Plugin\Input"
Source: "{#BaseDir}\Plugin\RSP\RSP 1.7.dll"; DestDir: "{app}\Plugin\RSP"
Source: "{#BaseDir}\Scripts\example.js"; DestDir: "{app}\Scripts"
Source: "{#BaseDir}\apidoc.htm"; DestDir: "{app}\apidoc.htm"

[Dirs]
Name: "{app}\Config"; Permissions: everyone-full
Name: "{app}\Logs"; Permissions: everyone-full
Name: "{app}\Save"; Permissions: everyone-full
Name: "{app}\Screenshots"; Permissions: everyone-full
Name: "{app}\Textures"; Permissions: everyone-full

[Icons]
Name: "{commondesktop}\Project64"; Filename: "{app}\Project64.exe"; Tasks: desktopicon
Name: "{commonprograms}\Project64 2.3\Project64"; Filename: "{app}\Project64.exe"
Name: "{commonprograms}\Project64 2.3\Uninstall Project64 2.3"; Filename: "{uninstallexe}"; Parameters: "/LOG"; Flags: createonlyiffileexists
Name: "{commonprograms}\Project64 2.3\Support"; Filename: "http://forum.pj64-emu.com"

[Tasks]
Name: desktopicon; Description: "Create a &desktop icon"
Name: portablemode; Description: "&Portable Mode"; Flags: unchecked
