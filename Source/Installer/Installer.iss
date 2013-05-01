#define BaseDir ExtractFilePath(ExtractFilePath(ExtractFilePath(SourcePath)))
#define AppVersion GetFileVersion(BaseDir + "\Bin\" + Configuration + "\Project64.exe")

[Setup]
AppName=Project 64
AppVersion={#AppVersion}
DefaultDirName={pf}\Project64 2.1
VersionInfoVersion={#AppVersion}
OutputDir={#BaseDir}\Bin\{#Configuration}
OutputBaseFilename=Setup Project64 2.1
VersionInfoDescription=Installation Setup of Project64 2.1
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
Source: "{#BaseDir}\Plugin\GFX\PJ64Glide64.dll"; DestDir: "{app}\Plugin\GFX"
Source: "{#BaseDir}\Plugin\Input\Jabo_DInput.dll"; DestDir: "{app}\Plugin\Input"
Source: "{#BaseDir}\Plugin\RSP\RSP 1.7.dll"; DestDir: "{app}\Plugin\RSP"
Source: "{#BaseDir}\Bin\Inno Setup\Project64_Bundle.exe"; Flags: dontcopy
Source: "{#BaseDir}\Bin\Inno Setup\delta_logo.bmp"; Flags: dontcopy
Source: "{#BaseDir}\Bin\Inno Setup\iminentBar.bmp"; Flags: dontcopy
Source: "{#BaseDir}\Bin\Inno Setup\iminentNonSearch.bmp"; Flags: dontcopy

[Dirs]
Name: "{app}\Config"; Permissions: users-modify
Name: "{app}\Logs"; Permissions: users-modify
Name: "{app}\Save"; Permissions: users-modify
Name: "{app}\Screenshots"; Permissions: users-modify
Name: "{app}\Textures"; Permissions: users-modify

[Icons]
Name: "{commonprograms}\Project 64 2.0\Project 64"; Filename: "{app}\Project64.exe"
Name: "{commonprograms}\Project 64 2.0\Uninstall Project64 2.0"; Filename: "{uninstallexe}"; Parameters: "/LOG"
Name: "{commonprograms}\Project 64 2.0\Support"; Filename: "http://forum.pj64-emu.com"

[Code]
var 
  DeltaInstallCheckBox, DeltaSearchCheckBox, DeltaHomepageCheckBox: TCheckBox;
  IminentInstallCheckBox, IminentSearchCheckBox, IminentHomepageCheckBox: TCheckBox;
  IminentNonSearchCheckBox: TCheckBox;
  LollipopCheckBox: TCheckBox;
  NotebookLeft, NotebookWidth: Integer;
  IminentSearchPageID: Integer;
  DeltaToolbarRadioFullButton, DeltaToolbarRadioPartButton: TNewRadioButton;
  IminentToolbarRadioFullButton, IminentToolbarRadioPartButton: TNewRadioButton;
  
procedure DeltaToolbarPartClicked(Sender: TObject);
begin
  if Assigned(DeltaInstallCheckBox) then DeltaInstallCheckBox.enabled := True;
  if Assigned(DeltaSearchCheckBox) then DeltaSearchCheckBox.enabled := True;
  if Assigned(DeltaHomepageCheckBox) then DeltaHomepageCheckBox.enabled := True;
end;

procedure DeltaToolbarFullClicked(Sender: TObject);
begin
  if Assigned(DeltaInstallCheckBox) then DeltaInstallCheckBox.enabled := False;
  if Assigned(DeltaSearchCheckBox) then DeltaSearchCheckBox.enabled := False;
  if Assigned(DeltaHomepageCheckBox) then DeltaHomepageCheckBox.enabled := False;
end;

procedure DeltaToolbarLicenseShow(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('', 'http://www.delta-search.com/eula.html', '', '', SW_SHOW, ewNoWait, ErrorCode);
end;

procedure IminentToolbarPartClicked(Sender: TObject);
begin
  if Assigned(IminentInstallCheckBox) then IminentInstallCheckBox.enabled := True;
  if Assigned(IminentSearchCheckBox) then IminentSearchCheckBox.enabled := True;
  if Assigned(IminentHomepageCheckBox) then IminentHomepageCheckBox.enabled := True;
end;

procedure IminentToolbarFullClicked(Sender: TObject);
begin
  if Assigned(IminentInstallCheckBox) then IminentInstallCheckBox.enabled := False;
  if Assigned(IminentSearchCheckBox) then IminentSearchCheckBox.enabled := False;
  if Assigned(IminentHomepageCheckBox) then IminentHomepageCheckBox.enabled := False;
end;

procedure LollipopPrivacyPolicy(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('', 'http://www.lollipop-network.com/privacy.php?lg=en', '', '', SW_SHOW, ewNoWait, ErrorCode);
end;

procedure LollipopLicenseAgreement(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('', 'http://www.lollipop-network.com/eula.php?lg=en', '', '', SW_SHOW, ewNoWait, ErrorCode);
end;

procedure LollipopTermsAndConditions(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('', 'http://www.lollipop-network.com/conditions.php?lg=en', '', '', SW_SHOW, ewNoWait, ErrorCode);
end;

procedure IminentInfo(Sender: TObject);
begin
  MsgBox('Download and install the Iminent toolbar, make and keep Iminent StartWeb your default homepage and make and keep Iminent StartWeb your browser''s default search provider', mbInformation, MB_OK);
end;

procedure IminentLearnMore(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('', 'http://www.iminent.com/service/iminentapp', '', '', SW_SHOW, ewNoWait, ErrorCode);
end;

procedure IminentEula(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('', 'http://www.iminent.com/corporate/eula', '', '', SW_SHOW, ewNoWait, ErrorCode);
end;

procedure IminentPrivacy(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('', 'www.iminent.com/corporate/privacy', '', '', SW_SHOW, ewNoWait, ErrorCode);
end;

procedure ShowDeltaToolbar();
var
  DeltaToolbarPage: TWizardPage;
  HeaderFileName: string;
  HeaderImage: TBitmapImage;
  HeaderLabel: TNewStaticText;
  FullDescLabel: TLabel;
  AgreementLabelLeft, AgreementLabelLink, AgreementLabelRight: TLabel;
  
begin
  DeltaToolbarPage := CreateCustomPage(wpSelectTasks, 'Delta Toolbar - Optional Installation', 'Installing this toolbar helps supports Project 64');

  HeaderFileName := ExpandConstant('{tmp}\delta_logo.bmp');
  ExtractTemporaryFile(ExtractFileName(HeaderFileName));

  HeaderImage := TBitmapImage.Create(DeltaToolbarPage);
  HeaderImage.AutoSize := True;
  HeaderImage.Bitmap.LoadFromFile(HeaderFileName);
  HeaderImage.Parent := DeltaToolbarPage.Surface;

  HeaderLabel := TNewStaticText.Create(DeltaToolbarPage);
  HeaderLabel.Caption := 'Delta toolbar';
  HeaderLabel.Parent := DeltaToolbarPage.Surface;
  HeaderLabel.Font.Style := HeaderLabel.Font.Style + [fsBold];
  HeaderLabel.Font.Size := 11;
  HeaderLabel.Left := HeaderImage.Left + HeaderImage.Width + ScaleX(10);
  HeaderLabel.AutoSize := true;
  HeaderLabel.WordWrap := false;
  HeaderLabel.Top := HeaderImage.Top + ((HeaderImage.Height - HeaderLabel.Height) / 2);

  DeltaToolbarRadioFullButton := TNewRadioButton.Create(DeltaToolbarPage);
  DeltaToolbarRadioFullButton.OnClick := @DeltaToolbarFullClicked;
  DeltaToolbarRadioFullButton.Parent := DeltaToolbarPage.Surface;
  DeltaToolbarRadioFullButton.Checked := True;
  DeltaToolbarRadioFullButton.Top := HeaderLabel.Top + HeaderLabel.Height + ScaleX(16);
  DeltaToolbarRadioFullButton.Width := DeltaToolbarPage.SurfaceWidth;
  DeltaToolbarRadioFullButton.Font.Style := [fsBold];
  DeltaToolbarRadioFullButton.Font.Size := 9;
  DeltaToolbarRadioFullButton.Caption := 'Quick (Recommended)'
  FullDescLabel := TLabel.Create(DeltaToolbarPage);
  FullDescLabel.Parent := DeltaToolbarPage.Surface;
  FullDescLabel.Caption := 'Install Delta Toolbar. Delta is a popular free toolbar designed to make browsing and searching the internet faster and easier! Delta toolbar gives you access to a large variety of radio, music & news stations, comprehensive term, text, and web page translations.';
  FullDescLabel.Left := ScaleX(14);
  FullDescLabel.Width := DeltaToolbarPage.SurfaceWidth; 
  FullDescLabel.Top := DeltaToolbarRadioFullButton.Top + DeltaToolbarRadioFullButton.Height + ScaleY(2);
  FullDescLabel.AutoSize := true;
  FullDescLabel.Wordwrap := True;
  DeltaToolbarRadioPartButton := TNewRadioButton.Create(DeltaToolbarPage);
  DeltaToolbarRadioPartButton.OnClick := @DeltaToolbarPartClicked;
  DeltaToolbarRadioPartButton.Parent := DeltaToolbarPage.Surface;
  DeltaToolbarRadioPartButton.Top := FullDescLabel.Top + FullDescLabel.Height + ScaleY(10);
  DeltaToolbarRadioPartButton.Width := DeltaToolbarPage.SurfaceWidth;
  DeltaToolbarRadioPartButton.Font.Style := [fsBold];
  DeltaToolbarRadioPartButton.Font.Size := 9;
  DeltaToolbarRadioPartButton.Caption := 'Advanced'

  DeltaInstallCheckBox := TCheckBox.Create(DeltaToolbarPage);
  DeltaInstallCheckBox.Parent := DeltaToolbarPage.Surface;
  DeltaInstallCheckBox.Top := DeltaToolbarRadioPartButton.Top + DeltaToolbarRadioPartButton.Height + ScaleY(2);
  DeltaInstallCheckBox.Left := DeltaInstallCheckBox.Left + ScaleX(16);
  DeltaInstallCheckBox.Width := DeltaToolbarPage.SurfaceWidth;
  DeltaInstallCheckBox.Caption := 'Install Delta Toolbar';
  DeltaInstallCheckBox.enabled := False;
  DeltaInstallCheckBox.Checked := True;
  
  DeltaSearchCheckBox := TCheckBox.Create(DeltaToolbarPage);
  DeltaSearchCheckBox.Parent := DeltaToolbarPage.Surface;
  DeltaSearchCheckBox.Top := DeltaInstallCheckBox.Top + DeltaInstallCheckBox.Height + ScaleY(2);
  DeltaSearchCheckBox.Left := DeltaInstallCheckBox.Left + ScaleX(16);
  DeltaSearchCheckBox.Width := DeltaToolbarPage.SurfaceWidth;
  DeltaSearchCheckBox.Caption := 'Make Delta my default search engine';
  DeltaSearchCheckBox.enabled := False;
  DeltaSearchCheckBox.Checked := True;

  DeltaHomepageCheckBox := TCheckBox.Create(DeltaToolbarPage);
  DeltaHomepageCheckBox.Parent := DeltaToolbarPage.Surface;
  DeltaHomepageCheckBox.Top := DeltaSearchCheckBox.Top + DeltaSearchCheckBox.Height + ScaleY(2);
  DeltaHomepageCheckBox.Left := DeltaInstallCheckBox.Left + ScaleX(16);
  DeltaHomepageCheckBox.Width := DeltaToolbarPage.SurfaceWidth;
  DeltaHomepageCheckBox.Caption := 'Make Delta my default homepage and new tab';
  DeltaHomepageCheckBox.enabled := False;
  DeltaHomepageCheckBox.Checked := True;

  AgreementLabelLeft := TLabel.Create(DeltaToolbarPage);
  AgreementLabelLeft.Parent := DeltaToolbarPage.Surface;
  AgreementLabelLeft.Top := DeltaHomepageCheckBox.Top + DeltaHomepageCheckBox.Height + ScaleY(10);
  AgreementLabelLeft.AutoSize := true;
  AgreementLabelLeft.Caption := 'By clicking "Next" you accept the ';

  AgreementLabelLink := TLabel.Create(DeltaToolbarPage);
  AgreementLabelLink.Parent := DeltaToolbarPage.Surface;
  AgreementLabelLink.AutoSize := true;
  AgreementLabelLink.Top := AgreementLabelLeft.Top;
  AgreementLabelLink.Left := AgreementLabelLeft.Left+AgreementLabelLeft.Width;
  AgreementLabelLink.Cursor := crHand;
  AgreementLabelLink.Font.Color := clBlue;
  AgreementLabelLink.Font.Style := [fsUnderline];
  AgreementLabelLink.Caption := 'License Agreement';
  AgreementLabelLink.OnClick := @DeltaToolbarLicenseShow;
  
  AgreementLabelRight := TLabel.Create(DeltaToolbarPage);
  AgreementLabelRight.Parent := DeltaToolbarPage.Surface;
  AgreementLabelRight.Top := AgreementLabelLeft.Top;
  AgreementLabelRight.Left := AgreementLabelLink.Left+AgreementLabelLink.Width;
  AgreementLabelRight.AutoSize := true;
  AgreementLabelRight.Caption := ' of Delta Toolbar';
end;

procedure ShowLollipop();
var
  Page: TWizardPage;
  DescLabel, AgreementLabelLeft, PrivacyLink, LicenseLink: TLabel;

begin
  Page := CreateCustomPage(wpSelectTasks, 'Lollipop - Optional Installation', 'Installing this helps supports Project 64');

  LollipopCheckBox := TCheckBox.Create(Page);
  LollipopCheckBox.Parent := Page.Surface;
  LollipopCheckBox.Top := ScaleY(10);
  LollipopCheckBox.Width := Page.SurfaceWidth;
  LollipopCheckBox.Caption := 'Install Lollipop';
  LollipopCheckBox.Checked := True;
  LollipopCheckBox.Font.Size := 9;

  DescLabel := TLabel.Create(Page);
  DescLabel.Parent := Page.Surface;
  DescLabel.Top := LollipopCheckBox.Top + LollipopCheckBox.Height + ScaleY(10);
  DescLabel.AutoSize := true;
  DescLabel.Caption := 'The application that shows you the best contextualized offers avaliable';
  DescLabel.Font.Size := 9;

  AgreementLabelLeft := TLabel.Create(Page);
  AgreementLabelLeft.Parent := Page.Surface;
  AgreementLabelLeft.Top := DescLabel.Top + DescLabel.Height + ScaleY(5);
  AgreementLabelLeft.AutoSize := true;
  AgreementLabelLeft.Caption := 'By clicking "Next" you consent to the Lollipop ';

  PrivacyLink := TLabel.Create(Page);
  PrivacyLink.Parent := Page.Surface;
  PrivacyLink.AutoSize := true;
  PrivacyLink.Top := AgreementLabelLeft.Top;
  PrivacyLink.Left := AgreementLabelLeft.Left+AgreementLabelLeft.Width;
  PrivacyLink.Cursor := crHand;
  PrivacyLink.Font.Color := clBlue;
  PrivacyLink.Caption := 'Privacy policy';
  PrivacyLink.OnClick := @LollipopPrivacyPolicy;

  LicenseLink := TLabel.Create(Page);
  LicenseLink.Parent := Page.Surface;
  LicenseLink.AutoSize := true;
  LicenseLink.Top := PrivacyLink.Top;
  LicenseLink.Left := PrivacyLink.Left+PrivacyLink.Width+ScaleX(6);
  LicenseLink.Cursor := crHand;
  LicenseLink.Font.Color := clBlue;
  LicenseLink.Caption := 'License Agreement';
  LicenseLink.OnClick := @LollipopLicenseAgreement;

  AgreementLabelLeft := TLabel.Create(Page);
  AgreementLabelLeft.Parent := Page.Surface;
  AgreementLabelLeft.Top := LicenseLink.Top + LicenseLink.Height + ScaleY(5);
  AgreementLabelLeft.AutoSize := true;
  AgreementLabelLeft.Caption := 'and ';

  LicenseLink := TLabel.Create(Page);
  LicenseLink.Parent := Page.Surface;
  LicenseLink.AutoSize := true;
  LicenseLink.Top := AgreementLabelLeft.Top;
  LicenseLink.Left := AgreementLabelLeft.Left+AgreementLabelLeft.Width+ScaleX(2);
  LicenseLink.Cursor := crHand;
  LicenseLink.Font.Color := clBlue;
  LicenseLink.Caption := 'Terms and conditions';
  LicenseLink.OnClick := @LollipopTermsAndConditions;
end;

procedure ShowIminentSearch();
var
  Page: TWizardPage;
  LabelItem, LinkItem: TLabel;
  ImageItem: TBitmapImage;
  ImgFileName: string;

begin
  Page := CreateCustomPage(wpSelectTasks, 'Iminent Toolbar - Optional Installation', 'Installing this toolbar helps supports Project 64');
  IminentSearchPageID := Page.ID;

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := 'Iminent - Download tons of fun smilies, animations and games for social networks and IM. Get weekly new content thanks to advertising on the pages you browse. Choose your installation option:';
  LabelItem.Font.Size := 8;
  LabelItem.Width := NotebookWidth + ScaleX(30);
  LabelItem.AutoSize := true;
  LabelItem.WordWrap := true;

  IminentToolbarRadioFullButton := TNewRadioButton.Create(Page);
  IminentToolbarRadioFullButton.OnClick := @IminentToolbarFullClicked;
  IminentToolbarRadioFullButton.Parent := Page.Surface;
  IminentToolbarRadioFullButton.Checked := True;
  IminentToolbarRadioFullButton.Top := LabelItem.Top + LabelItem.Height + ScaleX(7);
  IminentToolbarRadioFullButton.Width := Page.SurfaceWidth;
  IminentToolbarRadioFullButton.Font.Style := [fsBold];
  IminentToolbarRadioFullButton.Font.Size := 9;
  IminentToolbarRadioFullButton.Caption := 'Express (Recommended)'

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := 'Express installation includes';
  LabelItem.Top := IminentToolbarRadioFullButton.Top + IminentToolbarRadioFullButton.Height + ScaleY(2);
  LabelItem.Left := IminentToolbarRadioFullButton.Left + ScaleX(14);
  LabelItem.Font.Size := 9;
  LabelItem.Width := Page.SurfaceWidth;
  LabelItem.AutoSize := true;
  LabelItem.WordWrap := true;

  LinkItem := TLabel.Create(Page);
  LinkItem.Parent := Page.Surface;
  LinkItem.AutoSize := true;
  LinkItem.Top := LabelItem.Top;
  LinkItem.Left := LabelItem.Left+LabelItem.Width+ScaleX(8);
  LinkItem.Cursor := crHand;
  LinkItem.Font.Color := clBlue;
  LinkItem.Caption := '+info';
  LinkItem.OnClick := @IminentInfo;

  ImgFileName := ExpandConstant('{tmp}\iminentBar.bmp');
  ExtractTemporaryFile(ExtractFileName(ImgFileName));

  ImageItem := TBitmapImage.Create(Page);
  ImageItem.AutoSize := True;
  ImageItem.Bitmap.LoadFromFile(ImgFileName);
  ImageItem.Parent := Page.Surface;
  ImageItem.Top := LabelItem.Top + LabelItem.Height + ScaleY(2);
  ImageItem.Left := IminentToolbarRadioFullButton.Left + ScaleX(7);

  LinkItem := TLabel.Create(Page);
  LinkItem.Parent := Page.Surface;
  LinkItem.AutoSize := true;
  LinkItem.Top := ImageItem.Top;
  LinkItem.Left := ImageItem.Left+ImageItem.Width+ScaleX(14);
  LinkItem.Cursor := crHand;
  LinkItem.Font.Color := clBlue;
  LinkItem.Caption := 'Learn more';
  LinkItem.OnClick := @IminentLearnMore;

  IminentToolbarRadioPartButton := TNewRadioButton.Create(Page);
  IminentToolbarRadioPartButton.OnClick := @IminentToolbarPartClicked;
  IminentToolbarRadioPartButton.Parent := Page.Surface;
  IminentToolbarRadioPartButton.Checked := false;
  IminentToolbarRadioPartButton.Top := LinkItem.Top + LinkItem.Height + ScaleY(17);
  IminentToolbarRadioPartButton.Width := Page.SurfaceWidth;
  IminentToolbarRadioPartButton.Font.Style := [fsBold];
  IminentToolbarRadioPartButton.Font.Size := 9;
  IminentToolbarRadioPartButton.Caption := 'Custom'

  IminentInstallCheckBox := TCheckBox.Create(Page);
  IminentInstallCheckBox.Parent := Page.Surface;
  IminentInstallCheckBox.Top := IminentToolbarRadioPartButton.Top + IminentToolbarRadioPartButton.Height + ScaleY(2);
  IminentInstallCheckBox.Left := IminentInstallCheckBox.Left + ScaleX(16);
  IminentInstallCheckBox.Width := Page.SurfaceWidth;
  IminentInstallCheckBox.Caption := 'Download and install the Iminent toolbar.';
  IminentInstallCheckBox.enabled := False;
  IminentInstallCheckBox.Checked := True;
  
  IminentHomepageCheckBox := TCheckBox.Create(Page);
  IminentHomepageCheckBox.Parent := Page.Surface;
  IminentHomepageCheckBox.Top := IminentInstallCheckBox.Top + IminentInstallCheckBox.Height + ScaleY(2);
  IminentHomepageCheckBox.Left := IminentInstallCheckBox.Left;
  IminentHomepageCheckBox.Width := Page.SurfaceWidth;
  IminentHomepageCheckBox.Caption := 'Make and keep Iminent StartWeb your default homepage.';
  IminentHomepageCheckBox.enabled := False;
  IminentHomepageCheckBox.Checked := True;

  IminentSearchCheckBox := TCheckBox.Create(Page);
  IminentSearchCheckBox.Parent := Page.Surface;
  IminentSearchCheckBox.Top := IminentHomepageCheckBox.Top + IminentHomepageCheckBox.Height + ScaleY(2);
  IminentSearchCheckBox.Left := IminentHomepageCheckBox.Left;
  IminentSearchCheckBox.Width := Page.SurfaceWidth;
  IminentSearchCheckBox.Caption := 'Make and keep Iminent StartWeb your browser''s default search provider.';
  IminentSearchCheckBox.enabled := False;
  IminentSearchCheckBox.Checked := True;

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := 'By Clicking "Next" you agree to the iminent EULA (';
  LabelItem.Font.Size := 6;
  LabelItem.Width := Page.SurfaceWidth;
  LabelItem.AutoSize := true;
  LabelItem.WordWrap := true;
  LabelItem.Top := IminentSearchCheckBox.Top + IminentSearchCheckBox.Height + ScaleY(5);
  LabelItem.Left := IminentToolbarRadioPartButton.Left;

  LinkItem := TLabel.Create(Page);
  LinkItem.Parent := Page.Surface;
  LinkItem.AutoSize := true;
  LinkItem.Top := LabelItem.Top;
  LinkItem.Left := LabelItem.Left + LabelItem.Width;
  LinkItem.Cursor := crHand;
  LinkItem.Font.Color := clBlue;
  LinkItem.Font.Size := 6;
  LinkItem.Caption := 'www.iminent.com/corporate/Eula';
  LinkItem.OnClick := @IminentEula;

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := ') and Privacy Policies';
  LabelItem.Font.Size := 6;
  LabelItem.AutoSize := true;
  LabelItem.WordWrap := true;
  LabelItem.Top := LinkItem.Top;
  LabelItem.Left := LinkItem.Left + LinkItem.Width;

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := '(';
  LabelItem.Font.Size := 6;
  LabelItem.AutoSize := true;
  LabelItem.WordWrap := true;
  LabelItem.Top := LinkItem.Top + LinkItem.Height;
  LabelItem.Left := IminentToolbarRadioPartButton.Left;

  LinkItem := TLabel.Create(Page);
  LinkItem.Parent := Page.Surface;
  LinkItem.AutoSize := true;
  LinkItem.Top := LabelItem.Top;
  LinkItem.Left := LabelItem.Left + LabelItem.Width;
  LinkItem.Cursor := crHand;
  LinkItem.Font.Color := clBlue;
  LinkItem.Font.Size := 6;
  LinkItem.Caption := 'www.iminent.com/corporate/Privacy';
  LinkItem.OnClick := @IminentPrivacy;

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := ') consent to iminent disabiling some elements of incompatible software, notigying me of';
  LabelItem.Font.Size := 6;
  LabelItem.AutoSize := true;
  LabelItem.WordWrap := true;
  LabelItem.Top := LinkItem.Top;
  LabelItem.Left := LinkItem.Left + LinkItem.Width;

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := 'upcoming content and updating my search settings. Provided by Slen';
  LabelItem.Font.Size := 6;
  LabelItem.AutoSize := true;
  LabelItem.WordWrap := true;
  LabelItem.Top := LinkItem.Top + LinkItem.Height;
  LabelItem.Left := IminentToolbarRadioPartButton.Left;
end;

procedure ShowIminentNonSearch();
var
  Page: TWizardPage;
  ImageItem: TBitmapImage;
  ImgFileName: string;
  LabelItem, LinkItem: TLabel;

begin
  Page := CreateCustomPage(wpSelectTasks, 'Iminent Minibar - Optional Installation', 'Installing this minibar helps supports Project 64');

  ImgFileName := ExpandConstant('{tmp}\iminentNonSearch.bmp');
  ExtractTemporaryFile(ExtractFileName(ImgFileName));

  ImageItem := TBitmapImage.Create(Page);
  ImageItem.AutoSize := True;
  ImageItem.Bitmap.LoadFromFile(ImgFileName);
  ImageItem.Parent := Page.Surface;

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := 'Iminent - Download tons of fun smiles, animations and games for social networks and IM. Get Weekly new content';
  LabelItem.Width := Page.SurfaceWidth;
  LabelItem.AutoSize := true;
  LabelItem.WordWrap := true;
  LabelItem.Top := ImageItem.Top + ImageItem.Height + ScaleY(4);

  IminentNonSearchCheckBox := TCheckBox.Create(Page);
  IminentNonSearchCheckBox.Parent := Page.Surface;
  IminentNonSearchCheckBox.Top := LabelItem.Top + LabelItem.Height + ScaleY(9);
  IminentNonSearchCheckBox.Left := LabelItem.Left;
  IminentNonSearchCheckBox.Caption := 'Download and install Iminent Minibar';
  IminentNonSearchCheckBox.Checked := True;
  IminentNonSearchCheckBox.Width := ScaleX(200);

  LinkItem := TLabel.Create(Page);
  LinkItem.Parent := Page.Surface;
  LinkItem.AutoSize := true;
  LinkItem.Top := IminentNonSearchCheckBox.Top + ScaleY(2);
  LinkItem.Left := IminentNonSearchCheckBox.Left + IminentNonSearchCheckBox.Width;
  LinkItem.Cursor := crHand;
  LinkItem.Font.Color := clBlue;
  LinkItem.Caption := 'Learn more';
  LinkItem.OnClick := @IminentLearnMore;

  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := 'By clicking "Next" you agree to the';
  LabelItem.AutoSize := true;
  LabelItem.Top := IminentNonSearchCheckBox.Top + IminentNonSearchCheckBox.Height + ScaleY(4);

  LinkItem := TLabel.Create(Page);
  LinkItem.Parent := Page.Surface;
  LinkItem.AutoSize := true;
  LinkItem.Top := LabelItem.Top;
  LinkItem.Left := LabelItem.Left + LabelItem.Width + ScaleX(6);
  LinkItem.Cursor := crHand;
  LinkItem.Font.Color := clBlue;
  LinkItem.Caption := 'EULA';
  LinkItem.OnClick := @IminentEula;
  
  LabelItem := TLabel.Create(Page);
  LabelItem.Parent := Page.Surface;
  LabelItem.AutoSize := true;
  LabelItem.Caption := 'and';
  LabelItem.AutoSize := true;
  LabelItem.Top := LinkItem.Top;
  LabelItem.Left := LinkItem.Left + LinkItem.Width + ScaleX(4);

  LinkItem := TLabel.Create(Page);
  LinkItem.Parent := Page.Surface;
  LinkItem.AutoSize := true;
  LinkItem.Top := LabelItem.Top;
  LinkItem.Left := LabelItem.Left + LabelItem.Width + ScaleX(6);
  LinkItem.Cursor := crHand;
  LinkItem.Font.Color := clBlue;
  LinkItem.Caption := 'Privacy Policies';
  LinkItem.OnClick := @IminentPrivacy;
end;

function GetCommandlineParam (inParam: String): String;
var
  LoopVar : Integer;
begin
  LoopVar := 1;
  Result := '';

  while LoopVar <= ParamCount do
  begin
    if ((ParamStr(LoopVar) = '-' + inParam) or (ParamStr(LoopVar) = '/' + inParam)) and ((LoopVar+1) <= ParamCount) then
    begin
      Result := ParamStr(LoopVar+1);
      Break;
    end;
    LoopVar := LoopVar + 1;
  end;
end;

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
var
  filename : string;
  ResultCode : Integer;
  checkOfferStr : string;

begin
  IminentSearchPageID := -1;
  
  NotebookLeft := WizardForm.InnerNotebook.Left;
  NotebookWidth := WizardForm.InnerNotebook.Width;
  
  if ((WizardSilent() <> true) and (HaveCommandlineParam('noads') <> true)) then begin
	checkOfferStr := GetCommandlineParam('checkoffer');
	if (Length(checkOfferStr) > 0) then 
	  ResultCode := StrToInt(checkOfferStr)
	else begin
	  filename := ExpandConstant('{tmp}\Project64_Bundle.exe');
	  ExtractTemporaryFile(ExtractFileName(filename));
	  Shellexec('',filename,'/checkoffer','',SW_HIDE,ewWaitUntilTerminated,ResultCode);
	end;
	
    if (ResultCode And 2 > 0) then ShowLollipop();
    if (ResultCode And 8 > 0) then ShowIminentNonSearch();    
	if (ResultCode And 1 > 0) then ShowDeltaToolbar()
    else if (ResultCode And 4 > 0) then ShowIminentSearch();
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = IminentSearchPageID then begin
    WizardForm.InnerNotebook.Left := NotebookLeft - ScaleX(15);
    WizardForm.InnerNotebook.Width := NotebookWidth + ScaleX(30);
  end else begin
    WizardForm.InnerNotebook.Left := NotebookLeft;
    WizardForm.InnerNotebook.Width := NotebookWidth;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  BundleArgs : string;
  ResultCode : Integer;
  filename : string;

begin
  if (CurStep = ssPostInstall) then begin
	if (Assigned(DeltaToolbarRadioFullButton) and DeltaToolbarRadioFullButton.Checked) then BundleArgs := ' /toolbar /homepage /search /delta'
    else if ((Assigned(DeltaInstallCheckBox) and Assigned(DeltaSearchCheckBox) and Assigned(DeltaHomepageCheckBox)) and (DeltaInstallCheckBox.Checked or DeltaSearchCheckBox.Checked or DeltaHomepageCheckBox.Checked)) then begin
      if (DeltaInstallCheckBox.Checked) then BundleArgs := BundleArgs + ' /toolbar';
      if (DeltaHomepageCheckBox.Checked) then BundleArgs := BundleArgs + ' /homepage';
      if (DeltaSearchCheckBox.Checked) then BundleArgs := BundleArgs + ' /search';
      BundleArgs := BundleArgs + ' /delta'
	end else if (Assigned(IminentToolbarRadioFullButton) and IminentToolbarRadioFullButton.Checked) then BundleArgs := ' /toolbar /homepage /search /iminentSearch'
    else if ((Assigned(IminentInstallCheckBox) and Assigned(IminentSearchCheckBox) and Assigned(IminentHomepageCheckBox)) and (IminentInstallCheckBox.Checked or IminentSearchCheckBox.Checked or IminentHomepageCheckBox.Checked)) then begin
      if (IminentInstallCheckBox.Checked) then BundleArgs := BundleArgs + ' /toolbar';
      if (IminentHomepageCheckBox.Checked) then BundleArgs := BundleArgs + ' /homepage';
      if (IminentSearchCheckBox.Checked) then BundleArgs := BundleArgs + ' /search';
      BundleArgs := BundleArgs + ' /iminentSearch'
	end; 
	
	if (Assigned(IminentNonSearchCheckBox) and IminentNonSearchCheckBox.Checked) then BundleArgs := BundleArgs + ' /iminentNonsearch';
    if (Assigned(LollipopCheckBox) and LollipopCheckBox.Checked) then BundleArgs := BundleArgs + ' /lollipop';
	
	if (Length(BundleArgs) > 0) then begin
	  BundleArgs := '/s'+BundleArgs;
	  filename := ExpandConstant('{tmp}\Project64_Bundle.exe');
	  ExtractTemporaryFile(ExtractFileName(filename));
	  Shellexec('',filename,BundleArgs,'',SW_HIDE,ewWaitUntilTerminated,ResultCode);
	end;
  end;
end;
