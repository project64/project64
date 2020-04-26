if WScript.Arguments.Count < 3 then
    ShowUsage()
Else
    CopyArtificat()
End if

sub ShowUsage()
    WScript.StdOut.WriteLine "incorrect parameters"
    WScript.StdOut.WriteLine "[GlideN64ProjectUrl] [GlideN64 Workspacedir] [Project64 plugindir]"
    WScript.Quit 1
end sub

sub CopyArtificat()
	Set fso = CreateObject("Scripting.FileSystemObject")

    dim BuildUrl
    BuildUrl = WScript.Arguments(0)

    dim Workspacedir
	Workspacedir = WScript.Arguments(1)

    dim plugindir
	plugindir = WScript.Arguments(2)

	if (fso.FolderExists(plugindir) = false) then
		WScript.StdOut.WriteLine "plugindir does not exist ("&plugindir&")"
		WScript.Quit 1
	end if

	if (fso.FolderExists(plugindir&"\GFX") = false) then
		fso.CreateFolder plugindir&"\GFX"
	end if
	
	if (fso.FolderExists(plugindir&"\GFX\GLideN64") = false) then
		fso.CreateFolder plugindir&"\GFX\GLideN64"
	end if

	if (fso.FolderExists(plugindir&"\GFX\GLideN64\translations") = false) then
		fso.CreateFolder plugindir&"\GFX\GLideN64\translations"
	end if
	
    Dim objHTTP
    Set objHTTP = CreateObject("MSXML2.XMLHTTP")
    objHTTP.open "GET", BuildUrl & "/lastSuccessfulBuild/api/xml", False
    objHTTP.send
    if (objHTTP.status <> 200) then
        WScript.StdOut.WriteLine "failed to get job api (" & BuildUrl & "/lastSuccessfulBuild/api/xml)"
        WScript.Quit 1
    end if

    Dim xmlDoc
    Set xmlDoc = objHTTP.responseXML
    Set objLst = xmlDoc.getElementsByTagName("artifact")
    For each elem in objLst
        set childNodes = elem.childNodes
		dim filename
		dim relativepath

        for each node in childNodes
			if lcase(node.nodeName)="filename" then
				filename = node.text
			end if
			if lcase(node.nodeName)="relativepath" then
				relativepath = node.text
			end if
        next
		
		dim fullpath
		fullpath = Workspacedir &"\"&relativepath
		
		if Right(lcase(filename),5)=".lang" then
			if fso.FileExists(fullpath) then
				fso.CopyFile fullpath, plugindir&"\GFX\GLideN64\translations\"&filename
			end if
		end if

		if lcase(filename)="gliden64.dll" then
			if fso.FileExists(fullpath) then
				fso.CopyFile fullpath, plugindir&"\GFX\GLideN64\"&filename
			end if
		end if
    Next
end sub


