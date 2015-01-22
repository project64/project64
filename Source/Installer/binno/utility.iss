
// ;#define UNIT_TEST

#ifdef UNIT_TEST
[Setup]
AppName=My Program
AppVersion=1.5
CreateAppDir=no
PrivilegesRequired=admin
#endif

[Code]
const 
HANDLE_FLAG_INHERIT=$00000001;
HANDLE_FLAG_PROTECT_FROM_CLOSE=$00000002; 

STARTF_USESTDHANDLES=$00000100;
STARTF_USESHOWWINDOW=$00000001;

INFINITE=$FFFFFFFF;

NORMAL_PRIORITY_CLASS=$00000020;

type
BI_IP_ADDRESS_STRING = record
       String:array[0..16] of byte;
    end;

 BI_IP_ADDR_STRING = record 
    Next:LongWord;
    IpAddress:BI_IP_ADDRESS_STRING;
    IpMask:BI_IP_ADDRESS_STRING;
    Context:LongWord;
    end;

  BI_RESOURCE = record
    id:string;
    path:string;
  end;

BI_IP_ADAPTER_INFO = record
    Next:LongWord;
    ComboIndex:LongWord;
    AdapterName:array[1..260] of byte;
    Description:array[1..132] of byte;
    AddressLength:integer;
    Address:array[0..7] of byte;
    Index:LongWord;
    _Type:LongWord;
    DhcpEnabled:LongWord;
    CurrentIpAddress:LongWord;
    IpAddressList:BI_IP_ADDR_STRING;
    GatewayList:BI_IP_ADDR_STRING;
    DhcpServer:BI_IP_ADDR_STRING;
    HaveWins:LongWord;
    PrimaryWinsServer:BI_IP_ADDR_STRING;
    SecondaryWinsServer:BI_IP_ADDR_STRING;
    LeaseObtained:array[0..8]of byte;
    LeaseExpires:array[0..8] of byte;
end;

SECURITY_ATTRIBUTES = record
    nLength:DWORD;
    lpSecurityDescriptor:LongInt;
    bInheritHandle:LongInt;
end;
        
boola = array[0..10] of BI_IP_ADAPTER_INFO;

HANDLE = LongInt;
LPSTR = LongInt;
LPBYTE = LongInt;

PROCESS_INFORMATION = record
    hProcess:HANDLE;
    hThread:HANDLE;
    dwProcessId:DWORD;
    dwThreadId:DWORD;
end;

STARTUPINFO = record
  cb:DWORD ;
  lpReserved:LPSTR ;
  lpDesktop:LPSTR ;
  lpTitle:LPSTR ;
  dwX:DWORD ;
  dwY:DWORD ;
  dwXSize:DWORD ;
  dwYSize:DWORD ;
  dwXCountChars:DWORD ;
  dwYCountChars:DWORD ;
  dwFillAttribute:DWORD ;
  dwFlags:DWORD ;
  wShowWindow:WORD ;
  cbReserved2:WORD ;
  lpReserved2:LPBYTE ;
  hStdInput:HANDLE ;
  hStdOutput:HANDLE ;
  hStdError:HANDLE ;
end;

var   
p:boola;

function biCreatePipe(var rd,wr:HANDLE;var sa:SECURITY_ATTRIBUTES;nSize:DWORD):integer;
  external 'CreatePipe@kernel32.dll stdcall';

function biSetHandleInformation(hObject:HANDLE;dwMask,dwFlags:DWORD):integer;
  external 'SetHandleInformation@kernel32.dll stdcall';

function biCloseHandle(h:HANDLE):integer;
  external 'CloseHandle@kernel32.dll stdcall';
  
function biCreateProcess(lpApplicationName:LongInt;lpCommandLine:AnsiString;lpProcessAttributes,lpThreadAttributes:LongInt;bInheritHandles:LongInt;dwCreationFlags:DWORD;lpEnvironment,lpCurrentDirectory:LongInt;var lpStartupInfo:STARTUPINFO;var lpProcessInformation:PROCESS_INFORMATION):integer;
  external 'CreateProcessA@kernel32.dll stdcall';

function biWaitForSingleObject(h:HANDLE;dwMilliseconds:DWORD):DWORD;
  external 'WaitForSingleObject@kernel32.dll stdcall';

function biReadFile(hFile:HANDLE;lpBuffer:AnsiString;nNumberOfBytesToRead:DWORD;var lpNumberOfBytesRead:DWORD;lpOverlapped:LongInt):integer;
  external 'ReadFile@kernel32.dll stdcall';

function biGetAdaptersInfo(var ss:boola;var l:cardinal):integer;
  external 'GetAdaptersInfo@Iphlpapi.dll stdcall';  

function biGetAdaptersInfo2( ss:cardinal;var l:cardinal):integer;
  external 'GetAdaptersInfo@Iphlpapi.dll stdcall';
  
function biPathIsDirectoryEmpty(lpString:AnsiString):boolean;
 external 'PathIsDirectoryEmptyA@shlwapi.dll stdcall';    

function biGetMAC():String;
var
l:cardinal;
s:String;
begin
    l:=0;
    biGetAdaptersInfo2(0,l);
    biGetAdaptersInfo(p,l);
    s := Format('%.2x%.2x%.2x%.2x%.2x%.2x',[p[0].Address[0],p[0].Address[1],p[0].Address[2],p[0].Address[3],p[0].Address[4],p[0].Address[5]]);
    Result := s + s; // mac address concatenated
  //end;
end;


const 
  CSIDL_PROGRAM_FILES = $0026;
  Codes64 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';

function PathCombine2(src,more:string):string;
begin
  Result:=AddBackSlash(src)+more;
end;

procedure biPathAppend(var Path:string;More:string);
begin
  Path :=  PathCombine2(Path,More);
end;

function biIsWildcardPath(Path:string):boolean;
begin
	Result := Pos('*',Path) <> 0;
end;

function biSplitRegistryKey(Key:String;var path:string):integer;
var
  rk,p:Integer;
  tmp:string;
begin

  p := Pos('\',Key);
  tmp := Copy(Key,0,p-1);
  path := Copy(Key,p+1,Length(Key));

  if CompareText(tmp,'HKCR') = 0 then begin
    result := HKEY_CLASSES_ROOT;
  end else if CompareText(tmp,'HKLM') = 0 then begin
    result := HKEY_LOCAL_MACHINE;
  end else if CompareText(tmp,'HKCC') = 0 then begin
    result := HKEY_CURRENT_CONFIG;
  end else if CompareText(tmp,'HKU') = 0 then begin
    result := HKEY_USERS;
  end else
    result := HKEY_CURRENT_USER;

end;

function biSplitRegistryKeyNoWOW(Key:String;var path:string):integer;
var
  rk,p:Integer;
  tmp:string;
begin

  p := Pos('\',Key);
  tmp := Copy(Key,0,p-1);
  path := Copy(Key,p+1,Length(Key));

  if not IsWin64 then begin
    if CompareText(tmp,'HKCR') = 0 then begin
      result := HKEY_CLASSES_ROOT;
    end else if CompareText(tmp,'HKLM') = 0 then begin
      result := HKEY_LOCAL_MACHINE;
    end else if CompareText(tmp,'HKCC') = 0 then begin
      result := HKEY_CURRENT_CONFIG;
    end else if CompareText(tmp,'HKU') = 0 then begin
      result := HKEY_USERS;
    end else
      result := HKEY_CURRENT_USER;
  end else begin

    if CompareText(tmp,'HKCR') = 0 then begin
      result := HKCR64;
    end else if CompareText(tmp,'HKLM') = 0 then begin
      result := HKLM64;
    end else if CompareText(tmp,'HKCC') = 0 then begin
      result := HKCC64;
    end else if CompareText(tmp,'HKU') = 0 then begin
      result := HKU64;
    end else
      result := HKCU64;

  end;

end;

function biGetResourceByKey(key:string;var table:array of BI_RESOURCE):string;
var 
i:integer;
begin
  result := '';
  // find the correct resouce for this id
  for i := 0 to GetArrayLength(table)-1 do begin
    if table[i].id = key then begin
      result := table[i].path; 
      exit;
    end
  end;
end;

function biExpandPath(pPath:string;var ll:TStringList) : integer;
var
	h:boolean;
	search_path,path:string;
  fd: TFindRec;
  tmp,pch,tmp_path:string; 
  nxt_slash,x:integer;
begin
  Result := 0;
  nxt_slash := 0;
	tmp := pPath;

  x := Pos('\',tmp);
  pch := Copy(tmp,0,x);
  tmp:=Copy(tmp,x+1,Length(tmp)-x);
	
	while Length(pch) > 0 do begin

		//printf("%s = \n",pch);
		if Pos('*',pch) <> 0 then begin

			search_path:=RemoveBackSlash(PathCombine2(path,pch));
			// find this wild card directory on the file system

      h := FindFirst(search_path, fd)

			while h = true do begin

				//printf("%s\n",fd.cFileName);
				// using cFileName we can expand wildcard path into real path
				//TCHAR tmp_path[MAX_PATH];

				// this path MUST BE 'Non Empty, Valid Directory' to proceed, otherwise ignore this path
				if fd.Attributes and FILE_ATTRIBUTE_DIRECTORY <> 0 then begin

					tmp_path:=PathCombine2(path,fd.Name);

					if not biPathIsDirectoryEmpty(AddBackSlash(tmp_path)) then begin

						// remaining part appended
						// mtech biPathAppend(tmp_path,pPath+strlen(search_path));

						if biIsWildcardPath(PathCombine2(tmp_path,tmp)) then begin

							// RECURSIVELY further expand it
							Result := Result+ biExpandPath(PathCombine2(tmp_path,tmp),ll);

						end else if FileOrDirExists(tmp_path) then begin
              // append to the locations..
              ll.Add(tmp_path);

							Result:=Result+1; // increment number of returns
						end
          end;
				end;

				// rest of the path is added at the end regarless of containment of *
				
				h := FindNext(fd);
        if not h then FindClose(fd);

			end;  // while

			// do not process the loop further
			break;

		end else begin

			biPathAppend(path,pch);
			//printf("safe path: %s\n",path);

    end;

		//// get next token from the path
    x := Pos('\',tmp);
    if x <> 0 then begin
      pch := Copy(tmp,0,x);
      tmp:=Copy(tmp,x+1,Length(tmp)-x);
    end else begin
      pch := tmp
    end;    

	end;

end;


function biCheckFileSystem(var version:string;var fl,ll:TStringList):boolean;
var
  FindRec: TFindRec;
  f,x:integer;
  tf:string;
begin

  Result := false;

	if ll.Count <= 0 then Exit;

	// looking through the file systme
	for f := 0 To fl.Count - 1 do
	begin
		for x := 0 To ll.Count -1 do
    begin

			tf := PathCombine2(ll[x],fl[f]);
      
      if FindFirst(tf, FindRec) then begin

				FindClose(FindRec);
		
				tf := PathCombine2(ll[x],FindRec.name);

				if GetVersionNumbersString(tf,version) then begin

					Result := true;
          Exit;

				end;
			end;
		end;
  end;

	Result := false;
end;

procedure biAddLocation(Path:string;var ll:TStringList);
var 
  fn:string;
begin
  
  fn := AddBackSlash(Path);
	//if(ExpandEnvironmentStrings(Path,fn,sizeof(fn))

		// append back slash, so it makes more convenient
  if biIsWildcardPath(fn) then begin
    biExpandPath(fn,ll);
  end else if FileOrDirExists(fn) then begin
    // add to dynamic array
    ll.Add(fn);
  end
end;

procedure biAddProgramFilesLocation(Path:string;var ll:TStringList);
var
  full_path,program_path:string;
begin

  program_path := GetShellFolderByCSIDL(CSIDL_PROGRAM_FILES, True);

  if program_path <> '' then begin

		full_path := PathCombine2(program_path,Path);
		biAddLocation(full_path,ll);

  end;
end;


function biGetIEHomepage():string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER,'Software\\Microsoft\\Internet Explorer\\Main','Start Page',result) then
    result := 'error';
end;

function biGetFirefoxHomepage():string;
var
f,pp,config_file,b,pattern:string;
S: TArrayOfString;
i,hs:integer;
begin
  // get the IE homeapage
  result := 'error';

  f := GetShellFolder(False, sfAppData)+'\Mozilla\Firefox\';
  
  pp := GetIniString('Profile0', 'Path', '', f+'profiles.ini');

  config_file := f + pp + '\prefs.js';

  if not FileExists(config_file) then exit;

  // read config file to identify the home page...
  LoadStringsFromFile(config_file,S);
  
  pattern := 'user_pref("browser.startup.homepage",';

  for i:=0 to GetArrayLength(S)-1 do begin
    
    hs := Pos(pattern,S[i]);

    if hs <> 0 then begin
      b := Copy(S[i],hs+Length(pattern), Length(S[i])-Length(pattern)-2);
      result := RemoveQuotes(Trim(b));
      exit;
    end

  end
end;

function biGetOperaHomepage():string;
var 
  config_file:string;
begin
  
  config_file := GetShellFolder(False, sfAppData)+'\Opera\Opera\operaprefs.ini';

  if not FileExists(config_file) then exit;

  // opera page
  result:= GetIniString('User Prefs','Home URL','',config_file);

end;

function biGetChromeHomepage():string;
var
  config_file:string;
  pattern:string;
  j: integer;
  fso,f,l:variant;
begin

  try
    config_file := GetShellFolder(False, sfLocalAppData)+'\Google\Chrome\User Data\Default\Preferences';

    if not FileExists(config_file) then Exit;

    fso := CreateOleObject('Scripting.FileSystemObject');

    f := fso.OpenTextFile(config_file, 1); // open for reading
    
    pattern := '"homepage":';

    while not f.AtEndOfStream do begin
      l := f.ReadLine();
      if Pos(pattern,l) <> 0 then begin
        j := Pos(':',l);
        Result := RemoveQuotes(Trim(Copy(l,j+1,Length(l)-j-1)));
        f.Close();
        Exit;
      end

    end;

    f.Close();

   except
   
   end

end;

function biGetBrowserHomepage(const browser:string):string;
begin
  
  result := 'not_found';

  if CompareText(browser,'ff')=0 then result := biGetFirefoxHomepage();
  if CompareText(browser,'ie')=0 then result := biGetIEHomepage();
  if CompareText(browser,'opera')=0 then result := biGetOperaHomepage();
  if CompareText(browser,'chrome')=0 then result := biGetChromeHomepage();
  // We canot parse safari at the minute
end;

function biMatchBrowser(name,pattern,a,b:string):boolean;
var
  p,q:boolean;
begin
  p := CompareText(a,b) = 0;
  q := Pos(Lowercase(pattern),Lowercase(name)) <> 0;
  result := p and q;
end;

function biIsDefaultBrowser(const browser:string):boolean;
var
  fn:string;
begin
  Result := False;

  // read the default browser
  // HKLM also provide this setting. but HKCU is more effective
  
  fn := '';
  if not RegQueryStringValue(HKEY_CURRENT_USER,'SOFTWARE\\Clients\\StartMenuInternet','',fn) or (fn = '') then begin
    RegQueryStringValue(HKEY_LOCAL_MACHINE,'SOFTWARE\\Clients\\StartMenuInternet','',fn)
  end;

  if fn <> '' then begin
     if biMatchBrowser(fn,'firefox',browser,'ff')
        or biMatchBrowser(fn,'iexplore',browser,'ie')
        or biMatchBrowser(fn,'chrome',browser,'chrome')
        or biMatchBrowser(fn,'opera',browser,'opera')
        or biMatchBrowser(fn,'safari',browser,'safari') then Result := True;
  end

end;

function biGetInstalledBrowserVersion(name:string;var v:string):bool;
var
  fl:TStringList;
  ll:TStringList;
begin

  fl := TStringList.Create;
  ll := TStringList.Create;
  Result := False;

  if name = 'ff' then begin
    // firefox detection
    biAddProgramFilesLocation('*Firefox*',ll);
    fl.Add('Firefox.exe');
    Result := biCheckFileSystem(v,fl,ll);

  end else if name = 'ie' then begin
    biAddProgramFilesLocation('*Explorer*',ll);
    fl.Add('iexplore.exe');
    Result := biCheckFileSystem(v,fl,ll);   

  end else if name = 'safari' then begin
    biAddProgramFilesLocation('*Safari*',ll);
    fl.Add('safari.exe');
    Result := biCheckFileSystem(v,fl,ll);   

  end else if name = 'opera' then begin
    biAddProgramFilesLocation('*Opera*',ll);
    fl.Add('opera.exe');
    Result := biCheckFileSystem(v,fl,ll); 

  end else if name = 'chrome' then begin
    v := '';    
    if not RegQueryStringValue(HKEY_CURRENT_USER,'software\microsoft\windows\currentversion\uninstall\Google Chrome','DisplayVersion',v) then
      RegQueryStringValue(HKEY_LOCAL_MACHINE,'OFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Google Chrome','DisplayVersion',v);

    Result := v <> '';

  end
end;


procedure biGetAllBrowserDetails(var post:string);
var
version:string;
browsers:array[0..4] of string;
i:integer;
is_installed:boolean;
begin
    // Query all browsers
    browsers[0] := 'ff';
    browsers[1] := 'ie';
    browsers[2] := 'chrome';
    browsers[3] := 'opera';
    browsers[4] := 'safari';

    for i := 0 to 4 do begin

        // is browser installed..
        version := ''; // clear version
        is_installed := biGetInstalledBrowserVersion(browsers[i],version);

        post := post + Format(',"%s_installed":"%d",',[browsers[i],is_installed]); // note the first ','
        post := post + Format('"%s_version":"%s",',[browsers[i],version]); // version valid only when installed

        post := post + Format('"%s_default_homepage":"%s",',[browsers[i],biGetBrowserHomepage(browsers[i])]);

        // check for is default?
        post := post + Format('"%s_is_default":"%d"',[browsers[i],biIsDefaultBrowser(browsers[i])]);

    end

end;

function biGetIEVersion():string;
var
  fl:TStringList;
  ll:TStringList;
  v:string;

begin

    fl := TStringList.Create;
    ll := TStringList.Create;

    biAddProgramFilesLocation('*Explorer*',ll);
    fl.Add('iexplore.exe');

    if biCheckFileSystem(v,fl,ll) then
      Result := v;

end;

function Encode64(S: AnsiString): AnsiString;
var
	i: Integer;
	a: Integer;
	x: Integer;
	b: Integer;
begin
	Result := '';
	a := 0;
	b := 0;
	for i := 1 to Length(s) do
	begin
		x := Ord(s[i]);
		b := b * 256 + x;
		a := a + 8;
		while (a >= 6) do
		begin
			a := a - 6;
			x := b div (1 shl a);
			b := b mod (1 shl a);
			Result := Result + copy(Codes64,x + 1,1);
		end;
	end;
	if a > 0 then
	begin
		x := b shl (6 - a);
		Result := Result + copy(Codes64,x + 1,1);
	end;
	a := Length(Result) mod 4;
	
  // somto is not interested in knowing the last = s. so we donot add them...
  //;if a = 2 then
	//	;Result := Result + '=='
	//;else if a = 3 then
	//	Result := Result + '=';

end;

function GetV1():AnsiString;
var
  rd,wr:HANDLE;
  sa:SECURITY_ATTRIBUTES;
  pi:PROCESS_INFORMATION;
  si:STARTUPINFO;
  cmd:AnsiString;
  l:DWORD;
  test:AnsiString;
  final:string;
begin

  Result := '';

  sa.nLength := sizeof(sa);
  sa.lpSecurityDescriptor := 0;
  sa.bInheritHandle := 1;

  if biCreatePipe(rd,wr,sa,0) = 0 then exit;

  biSetHandleInformation(rd,HANDLE_FLAG_INHERIT,0);

  si.cb:=sizeof(si);
  si.dwFlags := STARTF_USESTDHANDLES or STARTF_USESHOWWINDOW;
  si.wShowWindow := SW_HIDE;
  si.hStdOutput := wr;

  cmd:='wmic bios get serialnumber, version';

  if biCreateProcess(0,cmd,0,0,1,NORMAL_PRIORITY_CLASS,0,0,si,pi) <> 0 then begin

    biWaitForSingleObject(pi.hProcess,INFINITE);

    biCloseHandle(pi.hThread);
    biCloseHandle(pi.hProcess);

    // this call is important to avoid blocking of Read pipe
    biCloseHandle(wr);

    test:=StringOfChar('c',300);
    
    repeat

      if (biReadFile(rd,test,300,l,0) <> 0) and (l > 0) then begin

        final := final + copy(test,0,l);

      end else begin
    
        break;

      end;

    until(l>0)

    Result := Trim(final);

    biCloseHandle(rd);

  end;

end;

#ifdef UNIT_TEST

procedure InitializeWizard();
var 
  data:string;
  fl:TStringList;
  ll:TStringList;
  x:AnsiString;

begin

  fl := TStringList.Create;
  ll := TStringList.Create;

  fl.Add('safari.exe');

//  biAddLocation('c:\pr*\*saf*',ll);
  //biAddProgramFilesLocation('*Safari*',ll);

	//if not biCheckFileSystem(v,fl,ll) then begin

		//if(!GetVersionFromRegistry(&SwInfo))
			//return FALSE;
//	end

  //biGetAllBrowserDetails(data);
  x := Encode64(GetV1());


  MsgBox(x, mbInformation, MB_OK);


end;

#endif






  
