#define BaseDir ExtractFilePath(ExtractFilePath(ExtractFilePath(SourcePath)))
#define AppVersion GetFileVersion(BaseDir + "\Bin\" + Configuration + "\Project64.exe")

#include BaseDir+"\Source\Installer\binno\binno.iss"

[Setup]
AppId={{BEB5FB69-4080-466F-96C4-F15DF271718B}
AppName=Project 64
AppVersion={#AppVersion}
DefaultDirName={pf}\Project64 2.2
VersionInfoVersion={#AppVersion}
OutputDir={#BaseDir}\Bin\{#Configuration}
OutputBaseFilename=Setup Project64 2.2
VersionInfoDescription=Installation Setup of Project64 2.2
Compression=lzma2/ultra64
WizardImageFile=Installer-Sidebar.bmp
WizardSmallImageFile=Pj64LogoSmallImage.bmp
DisableProgramGroupPage=yes
DisableReadyPage=yes
UninstallDisplayIcon={uninstallexe}
SetupIconFile={#BaseDir}\Source\Project64\User Interface\Icons\pj64.ico

[Run]
Filename: "{app}\Project64.exe"; Description: "{cm:LaunchProgram,{#StringChange('Project 64', '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Files]
Source: "{#BaseDir}\Bin\{#Configuration}\Project64.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BaseDir}\Config\Project64.cht"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Project64.rdb"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Project64.rdx"; DestDir: "{app}\Config"
Source: "{#BaseDir}\Config\Glide64.rdb"; DestDir: "{app}\Config"
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
Source: "{#BaseDir}\Lang\Spanish.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Swedish.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\T-Chinese.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Lang\Ukrainian.pj.Lang"; DestDir: "{app}\Lang"
Source: "{#BaseDir}\Plugin\Audio\Jabo_Dsound.dll"; DestDir: "{app}\Plugin\Audio"
Source: "{#BaseDir}\Plugin\GFX\Jabo_Direct3D8.dll"; DestDir: "{app}\Plugin\GFX"
Source: "{#BaseDir}\Plugin\GFX\PJ64Glide64.dll"; DestDir: "{app}\Plugin\GFX"
Source: "{#BaseDir}\Plugin\Input\Jabo_DInput.dll"; DestDir: "{app}\Plugin\Input"
Source: "{#BaseDir}\Plugin\Input\PJ64_NRage.dll"; DestDir: "{app}\Plugin\Input"
Source: "{#BaseDir}\Plugin\RSP\RSP 1.7.dll"; DestDir: "{app}\Plugin\RSP"

[Dirs]
Name: "{app}\Config"; Permissions: users-modify
Name: "{app}\Logs"; Permissions: users-modify
Name: "{app}\Save"; Permissions: users-modify
Name: "{app}\Screenshots"; Permissions: users-modify
Name: "{app}\Textures"; Permissions: users-modify

[Icons]
Name: "{commonprograms}\Project 64 2.2\Project 64"; Filename: "{app}\Project64.exe"
Name: "{commonprograms}\Project 64 2.2\Uninstall Project64 2.2"; Filename: "{uninstallexe}"; Parameters: "/LOG"
Name: "{commonprograms}\Project 64 2.2\Support"; Filename: "http://forum.pj64-emu.com"

[Code]
function HaveCommandlineParam (inParam: String): Boolean;
var
  LoopVar : Integer;
begin
  LoopVar := 1;
  Result := false;

  while LoopVar <= ParamCount do
  begin
    if ((ParamStr(LoopVar) = '-' + inParam) or (ParamStr(LoopVar) = '/' + inParam)) then
    begin
      Result := true;
      Break;
    end;
    LoopVar := LoopVar + 1;
  end;
end;

procedure InitializeWizard();
begin  
  if ((WizardSilent() <> true) and (HaveCommandlineParam('noads') <> true)) then begin
	  CreateBINNOPage(wpSelectTasks,'pj64emu','pj64emu');
  end;
end;
