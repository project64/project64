' On Error Resume Next

if WScript.Arguments.Count < 3 then
    WScript.StdOut.WriteLine "Missing parameters"
    WScript.StdOut.WriteLine "[password] [file to upload] [BuildUrl]"
	WScript.Quit
end if

Set IE = WScript.CreateObject("InternetExplorer.Application", "IE_")
IE.Visible = True

Login IE
PostThread IE

Sub Wait(IE)
	Dim complete
	complete = False

	For count = 0 to 1000
		WScript.Sleep 100
		WScript.StdOut.WriteLine count & ": IE.ReadyState: " & IE.ReadyState
		if IE.ReadyState >= 4 then
			WScript.StdOut.WriteLine count & ": IE.Busy: " & IE.Busy
			if not IE.Busy then
				WScript.StdOut.WriteLine count & ": IE.document.readyState: " & IE.document.readyState
				if StrComp(IE.document.readyState, "complete", vbTextCompare) = 0 then
					complete = true
					exit for
				end if
			end if
		end if
	Next
	
	if  not complete then
		WScript.StdOut.WriteLine "Failed to wait for IE"
		WScript.Quit
	end if
End Sub

Sub Navigate(IE, url)
 	WScript.StdOut.WriteLine "Navigating to: " & url
	IE.Navigate url
	Wait IE
End Sub

Sub ValidateLoggedIn(IE)
	WScript.StdOut.WriteLine "validate login success"
	Navigate IE, "http://forum.pj64-emu.com/"
	Wait IE

	Dim LoggedIn
	LoggedIn = False
	Set NodeList = IE.document.getElementsByTagName("a") 
	For Each Elem In NodeList
		if lcase(Mid(Elem.href,1,39)) = "http://forum.pj64-emu.com/member.php?u=" then
			if lcase(Mid(Elem.parentElement.innerHTML,1,11))  = "welcome, <a" then
				if lcase(Elem.innerHTML)  = "buildbot" then	
					WScript.StdOut.WriteLine "Found welcome message"
					LoggedIn = true
					exit for
				end if
			end if
		end if
	Next

	if LoggedIn = false then
		WScript.StdOut.WriteLine "Failed to login"
		WScript.Quit
	end if
End Sub

Sub Login(IE)
	Set IE2 = WScript.CreateObject("InternetExplorer.Application", "IE_")
	IE2.Visible = True
 	WScript.StdOut.WriteLine "Login start"
	Navigate IE2, "http://forum.pj64-emu.com/"

	set navbar_username = IE2.document.getelementbyid("navbar_username")
	set navbar_password = IE2.document.getelementbyid("navbar_password")
	if navbar_username is nothing then
		ValidateLoggedIn IE	
		exit sub
	end if

	navbar_username.value = "buildbot"
	navbar_password.value = WScript.Arguments(0)
	navbar_username.form.submit
	Wait IE2
	Dim FoundIt
	FoundIt = False

	For count = 0 to 100
		WScript.StdOut.WriteLine count
		Set NodeList = IE2.document.getElementsByTagName("a") 
		For Each Elem In NodeList
			if StrComp(Elem.href, "http://forum.pj64-emu.com/", vbTextCompare) = 0 then
				if StrComp(Elem.innerHTML, "Click here if your browser does not automatically redirect you.", vbTextCompare) = 0 then
					FoundIt = True
					Exit For
				End if
			end if  
		Next
		if FoundIt = True then
			Exit For
		end if
	Next	

	if FoundIt = false then
		WScript.StdOut.WriteLine "Failed to find redirect"
		WScript.Quit
	end if
	
	WScript.StdOut.WriteLine "found redirect"
	Navigate IE, "http://forum.pj64-emu.com/"
	Wait IE
	
	WScript.StdOut.WriteLine "Quitting IE2"
	IE2.Quit
	ValidateLoggedIn IE	
	WScript.StdOut.WriteLine "Login Done"
End Sub


Sub PostThread(IE)
	WScript.StdOut.WriteLine "PostThread start"
    Navigate IE, "http://forum.pj64-emu.com/newthread.php?do=newthread&f=8"
    Wait IE
	
	Set NodeList = IE.document.getElementsByTagName("input") 
	Dim submitButton
	For Each Elem In NodeList
		if lcase(Elem.className) = "button" and lcase(Elem.value) = "submit new thread" then
			WScript.StdOut.WriteLine "found submit button"
			set submitButton = Elem
			exit for
		end if
	Next
	
	if submitButton is nothing then
        WScript.StdOut.WriteLine "failed to find submit button"
        WScript.Quit
	end if

	SetPostDetails IE, WScript.Arguments(2)
	
	set manage_attachments_button = ie.document.getelementbyid("manage_attachments_button")
	if not manage_attachments_button is nothing then
		manage_attachments_button.click
		Wait IE
		WScript.Sleep 2000
		UploadFile WScript.Arguments(1)
	end if
	
	submitButton.click
	Wait IE
End Sub

Sub SetPostDetails(IE, BuildUrl)
	WScript.StdOut.WriteLine "Posting Details"
    Dim oReq
    Set oReq = CreateObject("MSXML2.XMLHTTP")
    oReq.open "GET", BuildUrl & "api/xml?wrapper=changes", False
    oReq.send

    if (oReq.status <> 200) then
        WScript.StdOut.WriteLine "failed to get job details (" & BuildUrl & "api/xml?wrapper=changes)"
        WScript.Quit
    end if

    Dim xmlDoc
    Set xmlDoc = oReq.responseXML
    Set objLst = xmlDoc.getElementsByTagName("freeStyleBuild")

    Dim PostTitle, PostContent
    For each elem in objLst
        set childNodes = elem.childNodes
        for each node in childNodes
            if lcase(node.nodeName)="fulldisplayname" then
                PostTitle = node.text
            end if
            if lcase(node.nodeName)="changeset" then
                for each item in node.childNodes
                    dim commitId, comment

                    commitId = ""
                    comment = ""

                    for each itemDetail in  item.childNodes
                        if lcase(itemDetail.nodeName)="commitid" then
                            commitId = itemDetail.text
                        end if
                        if lcase(itemDetail.nodeName)="comment" then
                            comment = Replace(Replace(itemDetail.text, vbLf, " "), vbCr, " ") 
                        end if
                    next

                    if (Len(comment) > 0 and Len(commitId) > 0) then
                        PostContent = PostContent & "[*]" & comment & " (commit: [URL=""https://github.com/project64/project64/commit/" & commitId & """]"& commitId & "[/URL])" & vbCr
                    end if
                next
            end if
        next
    Next

    if (Len(PostContent) > 0) then
        PostContent = "Changes:"&vbCr&"[LIST=1]" & vbCr & PostContent & "[/LIST]"
    else
        PostContent = "No code changes"
    end if

    WScript.StdOut.WriteLine "PostTitle = """ & PostTitle & """"
    WScript.StdOut.WriteLine "PostContent = """ & PostContent & """"
	
	Dim SetTitle
	SetTitle = False
	Set NodeList = IE.document.getElementsByTagName("input") 
	For Each Elem In NodeList
		if lcase(Elem.name) = "subject" then
			Elem.value = PostTitle
			SetTitle = true
			exit for
		end if
	Next

	if not SetTitle then
		WScript.StdOut.WriteLine "failed to set post title"
		WScript.Quit
	end if
	
	Dim SetMessage
	SetMessage = False
	Set NodeList = IE.document.getElementsByTagName("textarea") 
	For Each Elem In NodeList
		WScript.StdOut.WriteLine Elem.name
		if lcase(Elem.name) = "message" then
			Elem.value = PostContent
			SetMessage = true
			exit for
		end if
	Next

	if not SetMessage then
		WScript.StdOut.WriteLine "failed to set post message"
		WScript.Quit
	end if
	
end sub

sub UploadFile(FileToUpload)
	WScript.StdOut.WriteLine "UploadFile start"

	On Error resume next
	set IE = Nothing

	Set Shell = CreateObject("Shell.Application")
	For i = 0 to Shell.Windows.Count -1 
		set Win  = Shell.Windows.Item(i)
		If TypeName(win.Document) = "HTMLDocument" Then
			if StrComp(win.Document.title, "Manage Attachments - Project64 Forums", vbTextCompare) = 0 then
				set IE = win
			end if
		End If

		if not IE is nothing then
			exit for
		end if
	Next
	if IE is nothing then
		WScript.StdOut.WriteLine "Failed to find upload window"
		exit sub
	end if
	
	WScript.StdOut.WriteLine  "Found window"
	Set objShell = CreateObject("Wscript.Shell")
	WScript.StdOut.WriteLine  "activate: " & win.Document.title & " - " & IE.name
	Dim activated
	For count = 0 to 100
		activated = objShell.AppActivate(win.Document.title & " - " & IE.name, True)
		if activated then
		    exit for
		end if
		WScript.StdOut.WriteLine count & ": " & activated
		WScript.Sleep 100
	Next
	
	if not activated then
		WScript.StdOut.WriteLine "Failed to activate window"
		WScript.Quit
	end if
			
	Set NodeList = IE.document.getElementsByTagName("input") 
	For Each Elem In NodeList
		if StrComp(Elem.name, "attachment[]", vbTextCompare) = 0 then
			Elem.focus()
			objShell.SendKeys " "
			
			WScript.StdOut.WriteLine  "Uploading: " & FileToUpload
			Wscript.Sleep 1000
			a=Split(FileToUpload,"\")
			b=ubound(a)
			For i=0 to b
				objShell.SendKeys a(i)
				if i < b then
					objShell.SendKeys "\"
				else
					objShell.SendKeys "{ENTER}"
				end if
  				Wscript.Sleep 100
			Next
	 		exit for
		end if
	Next

	For Each Elem In NodeList
		if StrComp(Elem.name, "upload", vbTextCompare) = 0 then
			Elem.click
			Wait IE
			exit for
		end if
	Next
	
	Dim UploadDone
	UploadDone = False
	For count = 0 to 1000
		WScript.StdOut.WriteLine count & ": Waiting for upload done"
		Set NodeList = ie.document.getElementsByTagName("legend") 
		For Each Elem In NodeList
			if (len(Elem.innerHTML) > 19) and lcase(Mid(Elem.innerHTML, 1, 19)) = "current attachments" then 
				UploadDone = true
				WScript.StdOut.WriteLine "Upload done"
				exit for
			end if
		Next
		if UploadDone then
			exit for
		end if
	Next

	if not UploadDone then
		WScript.StdOut.WriteLine "Failed to upload file"
		WScript.Quit
	end if

	Set NodeList = IE.document.getElementsByTagName("input") 
	For Each Elem In NodeList
		if lcase(Elem.value) = "close this window" then
			Elem.click
			exit for
		end if
	Next
end sub

