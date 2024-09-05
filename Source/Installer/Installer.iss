 #define BaseDir ExtractFilePath(ExtractFilePath(ExtractFilePath(SourcePath)))
#define AppVersion GetFileVersion(BaseDir + "\Bin\" + Platform + "\" + Configuration + "\Project64.exe")

[Setup]
AppId={{BEB5FB69-4080-466F-96C4-F15DF271718B}
AppName=Project64
AppVersion={#AppVersion}
DefaultDirName={pf32}\Project64 Dev 4.0
VersionInfoVersion={#AppVersion}
OutputDir={#BaseDir}\Bin\{#Platform}\{#Configuration}
OutputBaseFilename=Setup Project64 Dev 4.0
VersionInfoDescription=Installation Setup of Project64 Dev 4.0
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
Source: "{#BaseDir}\Bin\{#Platform}\{#Configuration}\Project64.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BaseDir}\Config\Video.rdb"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Audio.rdb"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Cheats\*.cht"; DestDir: "{app}\Config\Cheats"
Source: "{#BaseDir}\Config\Enhancements\*.enh"; DestDir: "{app}\Config\Enhancements"
Source: "{#BaseDir}\Config\Project64.rdb"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Project64.rdx"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Lang\*.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Plugin\{#Platform}\Audio\Jabo_Dsound.dll"; DestDir: "{app}\Plugin\Audio"
Source: "{#BaseDir}\Plugin\{#Platform}\Audio\Project64-Audio.dll"; DestDir: "{app}\Plugin\Audio"
Source: "{#BaseDir}\Plugin\{#Platform}\GFX\Jabo_Direct3D8.dll"; DestDir: "{app}\Plugin\GFX"
Source: "{#BaseDir}\Plugin\{#Platform}\GFX\Project64-Video.dll"; DestDir: "{app}\Plugin\GFX"
Source: "{#BaseDir}\Plugin\{#Platform}\GFX\GLideN64\*"; DestDir: "{app}\Plugin\GFX\GLideN64"; Flags: recursesubdirs skipifsourcedoesntexist
Source: "{#BaseDir}\Plugin\{#Platform}\Input\PJ64_NRage.dll"; DestDir: "{app}\Plugin\Input"
Source: "{#BaseDir}\Plugin\{#Platform}\Input\Project64-Input.dll"; DestDir: "{app}\Plugin\Input"
Source: "{#BaseDir}\Plugin\{#Platform}\RSP\Project64-RSP.dll"; DestDir: "{app}\Plugin\RSP"
Source: "{#BaseDir}\Scripts\example.js"; DestDir: "{app}\Scripts"
Source: "{#BaseDir}\Scripts\api_documentation.js"; DestDir: "{app}\Scripts"
Source: "{#BaseDir}\JS-API-Documentation.html"; DestDir: "{app}"

[Dirs]
Name: "{app}\Config"; Permissions: everyone-full
Name: "{app}\Config\Cheats-User"; Permissions: everyone-full
Name: "{app}\Logs"; Permissions: everyone-full
Name: "{app}\Save"; Permissions: everyone-full
Name: "{app}\Screenshots"; Permissions: everyone-full
Name: "{app}\Textures"; Permissions: everyone-full
Name: "{app}\Plugin\GFX\GLideN64"; Permissions: everyone-full

[Icons]
Name: "{commondesktop}\Project64"; Filename: "{app}\Project64.exe"; Tasks: desktopicon
Name: "{commonprograms}\Project64 4.0\Project64"; Filename: "{app}\Project64.exe"
Name: "{commonprograms}\Project64 4.0\Uninstall Project64 4.0"; Filename: "{uninstallexe}"; Parameters: "/LOG"; Flags: createonlyiffileexists
Name: "{commonprograms}\Project64 4.0\Support"; Filename: "https://discord.gg/Cg3zquF"

[Tasks]
Name: desktopicon; Description: "Create a &desktop icon"
Name: portablemode; Description: "&Portable Mode"; Flags: unchecked
