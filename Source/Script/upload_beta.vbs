' On Error Resume Next

if WScript.Arguments.Count < 4 then
    WScript.StdOut.WriteLine "Missing parameters"
    WScript.StdOut.WriteLine "[password] [file to upload] [BuildUrl] [Posttitle]"
    WScript.Quit 1
end if

Set IE = CreateIeWindow()
WScript.StdOut.WriteLine IE.HWND
IE.Visible = True

Login IE
PostThread IE
IE.Quit
WScript.Quit 0

function CreateIeWindow ()
    on error resume next

    Set CreateIeWindow = nothing
    For count = 0 to 100
        WScript.StdOut.WriteLine count & ": Trying to create Internet Explorer"
        Set IE = WScript.CreateObject("InternetExplorer.Application", "IE_")
        if not IE is nothing then
            WScript.StdOut.WriteLine count & ": Created Internet Explorer"
            WScript.StdOut.WriteLine IE.HWND
            IE.Visible = True

            WScript.StdOut.WriteLine IE.HWND
            Set CreateIeWindow = IE
            exit for
        end if
        WScript.StdOut.WriteLine count & ": Not created"
        WScript.Sleep 100
        WScript.StdOut.WriteLine count & ": Should loop"
    Next
    if CreateIeWindow is nothing then
        WScript.StdOut.WriteLine "Failed to create InternetExplorer.Application"
        WScript.Quit 1
    end if
End Function

Sub Wait(IE)
    Dim complete
    complete = False

    WScript.StdOut.WriteLine "Waiting for IE"

    For count = 0 to 1000
        WScript.StdOut.WriteLine "before sleep"
        WScript.Sleep 100
        WScript.StdOut.WriteLine "after sleep"
        if IE is nothing then
            WScript.StdOut.WriteLine "after sleep"
        end if
        WScript.StdOut.WriteLine count & ": IE.readyState: " & IE.readyState
        if IE.readyState >= 4 then
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
        WScript.Quit 1
    end if
    WScript.StdOut.WriteLine "IE Done"
End Sub

Function FindIeWindow(ieID)
    on error resume next
    set IE = nothing

    For count = 0 to 100
        Set Shell = CreateObject("Shell.Application")
        For i = 0 to Shell.Windows.Count -1 
            set Win  = Shell.Windows.Item(i)
            WScript.StdOut.WriteLine i & ": " & TypeName(win.Document)

            If TypeName(win.Document) = "HTMLDocument" Then                    
                WScript.StdOut.WriteLine "uniqueID: @" & win.HWND & "@" & ieID & "@"
                if win.HWND = ieID then
                    WScript.StdOut.WriteLine "matched"
                    set IE = win
                end if
            End If
            if not IE is nothing then
                exit for
            end if
        Next
        if not IE is nothing then
            exit for
        end if
        WScript.StdOut.WriteLine count & ": failed, trying again"
        WScript.Sleep 100
    Next
    set FindIeWindow = IE
    
    if IE is nothing then
        WScript.StdOut.WriteLine "Failed to find navigating window"
    else
        WScript.StdOut.WriteLine "HWND : " & IE.HWND 
    end if
End Function

Sub Navigate(IE, url)
    dim ieId
    ieID = IE.HWND
     WScript.StdOut.WriteLine "Navigating (" & IE.HWND  & ") to: " & url
    IE.Navigate url
    WScript.Sleep 100
    ' set IE = FindIeWindow(ieID)
    Wait IE
End Sub

Sub ValidateLoggedIn(IE)
    WScript.StdOut.WriteLine "ValidateLoggedIn - 1"
    WScript.StdOut.WriteLine "ValidateLoggedIn start"
    WScript.StdOut.WriteLine "ValidateLoggedIn - 2"
    Navigate IE, "http://forum.pj64-emu.com/"
    WScript.StdOut.WriteLine "ValidateLoggedIn - 3"
    Wait IE
    WScript.StdOut.WriteLine "ValidateLoggedIn - 4"

    Dim LoggedIn
    LoggedIn = False
    WScript.StdOut.WriteLine "ValidateLoggedIn - 5"
    Set NodeList = IE.document.getElementsByTagName("a") 
    WScript.StdOut.WriteLine "ValidateLoggedIn - 6"
    WScript.StdOut.WriteLine "Got Node list"
    For Each Elem In NodeList
        WScript.StdOut.WriteLine Elem.href
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
        WScript.Quit 1
    end if
     WScript.StdOut.WriteLine "ValidateLoggedIn Done"
End Sub

Sub Login(IE)
    On Error resume next

    Set IE2 = WScript.CreateObject("InternetExplorer.Application", "IE_")
    IE2.Visible = True
     WScript.StdOut.WriteLine "Login start"
    Navigate IE2, "http://forum.pj64-emu.com/"

    set navbar_username = IE2.document.getelementbyid("navbar_username")
    set navbar_password = IE2.document.getelementbyid("navbar_password")
    if navbar_username is nothing then
        WScript.StdOut.WriteLine "Failed to find login form, checking if already logged in"
        ValidateLoggedIn IE    
        exit sub
    end if

     WScript.StdOut.WriteLine "Found login form"
    navbar_username.value = "buildbot"
    navbar_password.value = WScript.Arguments(0)
    navbar_username.form.submit
    Wait IE2
    Dim FoundIt
    FoundIt = False

    WScript.StdOut.WriteLine "Looking for redirect"
    For count = 0 to 100
        Set NodeList = IE2.document.getElementsByTagName("a") 
        WScript.StdOut.WriteLine count & ": Found " & NodeList.length & " a tags"
        For Each Elem In NodeList
            if StrComp(Elem.href, "http://forum.pj64-emu.com/", vbTextCompare) = 0 then
                if StrComp(Elem.innerHTML, "Click here if your browser does not automatically redirect you.", vbTextCompare) = 0 then
                    FoundIt = True
                    Exit For
                End if
            end if
            if StrComp(Mid(Elem.href,1,57), "http://forum.pj64-emu.com/login.php?do=logout&logouthash=", vbTextCompare) = 0 then
                if StrComp(Elem.innerHTML, "Log Out buildbot", vbTextCompare) = 0 then
                    WScript.StdOut.WriteLine "missed redirect link, found logout link"
                    FoundIt = True
                    Exit For
                End if
            end if
        Next
        if FoundIt = True then
            Exit For
        end if
        WScript.Sleep 100
    Next    

    if FoundIt = false then
        WScript.StdOut.WriteLine "Failed to find redirect"
        WScript.Quit 1
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
    dim ieId
    ieID = IE.HWND
    
    WScript.StdOut.WriteLine "PostThread start"
    Navigate IE, "http://forum.pj64-emu.com/newthread.php?do=newthread&f=10"
    Wait IE
    set IE = FindIeWindow(ieID)

    Dim submitButton
    For count = 0 to 100
        WScript.StdOut.WriteLine count & ": looking for submit button"
        set submitButton = nothing
        Set NodeList = IE.document.getElementsByTagName("input") 
        For Each Elem In NodeList
            WScript.StdOut.WriteLine count & ": " & lcase(Elem.className)
            if lcase(Elem.className) = "button" and lcase(Elem.value) = "submit new thread" then
                WScript.StdOut.WriteLine count & ": found submit button"
                set submitButton = Elem
                exit for
            end if
        Next
        if not submitButton is nothing then
            exit for
        end if
        WScript.Sleep 300
    Next
    
    if submitButton is nothing then
        WScript.StdOut.WriteLine "failed to find submit button"
        WScript.Quit 1
    end if

    SetPostDetails IE, WScript.Arguments(2), WScript.Arguments(3)
    UploadDirectory ieID, WScript.Arguments(1)    

    WScript.StdOut.WriteLine "submitting"
    submitButton.click
    WScript.Sleep 100
    set IE = FindIeWindow(ieID)
    Wait IE
    WScript.StdOut.WriteLine "PostThread Finished"
End Sub

Sub SetPostDetails(IE, BuildUrl, PostTitle)
    WScript.StdOut.WriteLine "Posting Details"
    Dim oReq
    Set oReq = CreateObject("MSXML2.XMLHTTP")
    oReq.open "GET", BuildUrl & "api/xml?wrapper=changes", False
    oReq.send

    if (oReq.status <> 200) then
        WScript.StdOut.WriteLine "failed to get job details (" & BuildUrl & "api/xml?wrapper=changes)"
        WScript.Quit 1
    end if

    Dim xmlDoc
    Set xmlDoc = oReq.responseXML
    Set objLst = xmlDoc.getElementsByTagName("freeStyleBuild")

    Dim PostContent
    For each elem in objLst
        set childNodes = elem.childNodes
        for each node in childNodes
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
        WScript.Quit 1
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
        WScript.Quit 1
    end if

end sub

sub UploadDirectory(ieID, DirToUpload)
    WScript.StdOut.WriteLine "UploadDirectory start - " & DirToUpload
    Set objFSO = CreateObject("Scripting.FileSystemObject")
    Set objFolder = objFSO.GetFolder(DirToUpload)
    Set colFiles = objFolder.Files
    For Each objFile in colFiles
        UploadFile ieID, DirToUpload & "\" & objFile.Name
    Next
    WScript.StdOut.WriteLine "UploadDirectory Finished"
end sub

sub UploadFile(ieID, FileToUpload)
    set IE = FindIeWindow(ieID)
    WScript.StdOut.WriteLine "UploadFile start - " & FileToUpload

    dim filePos
    filePos = InStrRev(FileToUpload, "\")
    if filePos = 0 then 
        WScript.StdOut.WriteLine "failed to find directory seperator in " & FileToUpload
        WScript.Quit 1
    end if

    dim fileName
    fileName = Mid(FileToUpload, filePos + 1, len(FileToUpload))
    WScript.StdOut.WriteLine "fileName: " & fileName

    extPos = InStrRev(fileName, ".")
    if extPos = 0 then 
        WScript.StdOut.WriteLine "failed to find file extension in " & fileName
        WScript.Quit 1
    end if
    extension = Mid(fileName, extPos, len(fileName))
    if lcase(extension) <> ".zip" And lcase(extension) <> ".exe" And lcase(extension) <> ".apk" then 
        WScript.StdOut.WriteLine "not a valid extension: " & fileName
        WScript.Quit 1
    end if    

    set manage_attachments_button = IE.document.getelementbyid("manage_attachments_button")
    if manage_attachments_button is nothing then
        WScript.StdOut.WriteLine "failed to find manage_attachments_button"
        WScript.Quit 1
    end if

    WScript.StdOut.WriteLine "InStr(1, lcase(manage_attachments_button.onclick), ""vb_attachments"") = " & InStr(1, lcase(manage_attachments_button.onclick), "vb_attachments")
    dim startPos, endPos
    startPos = InStr(1, lcase(manage_attachments_button.onclick), "vb_attachments")
    if startPos = 0 then 
        WScript.StdOut.WriteLine "failed to find vb_attachments in " & manage_attachments_button.onclick
        WScript.Quit 1
    end if
    startPos = InStr(startPos, lcase(manage_attachments_button.onclick), "'")
    if startPos = 0 then 
        WScript.StdOut.WriteLine "failed to find first quote in " & manage_attachments_button.onclick
        WScript.Quit 1
    end if
    startPos = startPos + 1
    endPos = InStr(startPos,manage_attachments_button.onclick, "'")
    if endPos = 0 then 
        WScript.StdOut.WriteLine "failed to find second quote in " & manage_attachments_button.onclick
        WScript.Quit 1
    end if

    Set IE2 = WScript.CreateObject("InternetExplorer.Application", "IE_")
    IE2.Visible = True
    Navigate IE2, "http://forum.pj64-emu.com/" & Mid(manage_attachments_button.onclick, startPos, endPos - startPos)
    Wait IE2

    Set FormList = IE2.document.getElementsByTagName("form") 
    if FormList.length <> 1 or FormList(0).name <> "newattachment" then 
        WScript.StdOut.WriteLine "failed to find attachement form"
        WScript.Quit 1
    end if

    Set InputList = FormList(0).getElementsByTagName("input")
    WScript.StdOut.WriteLine "InputList.length = " & InputList.length

    dim PreFormData, PostFormData

    For Each Input In InputList
        if lcase(Input.type) = "hidden" then
            PreFormData = PreFormData & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""" & Input.name & """" & vbCrLf & vbCrLf & Input.value& vbCrLf   
        end if
        WScript.StdOut.WriteLine "Input.type: " & Input.type & " Input.name: " & Input.name & " Input.value: " & Input.value
    next
    PreFormData = PreFormData & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""attachment[]""; filename=""" & fileName & """" & vbCrLf
    PreFormData = PreFormData & "Content-Type: application/zip" & vbCrLf & vbCrLf
    PostFormData = vbCrLf & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""upload""" & vbCrLf & vbCrLf & "Upload" & vbCrLf   
    PostFormData = PostFormData & vbCrLf & vbCrLf & "--AaB03x--"& vbCrLf

    WScript.StdOut.WriteLine  PreFormData & PostFormData

    dim fileContents
    fileContents = ReadBinaryFile(FileToUpload)

    dim UploadUrl
    UploadUrl = "http://forum.pj64-emu.com/" & FormList(0).action
    IE2.Quit

    Header = "Content-Type: multipart/form-data; boundary=AaB03x" & vbCrLf

    Const adTypeBinary = 1

    Dim DataToPOSTStream
    Set DataToPOSTStream = CreateObject("ADODB.Stream")  

    DataToPOSTStream.type=adTypeBinary
    DataToPOSTStream.Open

    DataToPOSTStream.Write = Stream_StringToBinary(PreFormData,"us-ascii")
    DataToPOSTStream.Write = fileContents
    DataToPOSTStream.Write = Stream_StringToBinary(PostFormData,"us-ascii")
    DataToPOSTStream.Position = 0
    DataToPOSTStream.Type = adTypeBinary
    Dim DataToPOST
    DataToPOST = DataToPOSTStream.Read

    Set IE3 = CreateIeWindow()
    IE3.Visible = 1
    dim ie3Id
    ie3Id = IE3.HWND
    WScript.StdOut.WriteLine "Uploading form to: " & UploadUrl    
    IE3.Navigate UploadUrl, Nothing, Nothing, DataToPOST, Header
    WScript.Sleep 100
    'set IE3 = FindIeWindow(ie3Id)
    Wait IE3

    Dim UploadDone
    UploadDone = False
    For count = 0 to 1000
        WScript.StdOut.WriteLine count & ": Waiting for upload done"
        Set NodeList = IE3.document.getElementsByTagName("legend") 
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
        WScript.Quit 1
    end if
    IE3.Quit

    WScript.StdOut.WriteLine "UploadFile Finished"
end sub

Function Stream_StringToBinary(Text, CharSet)
  Const adTypeText = 2
  Const adTypeBinary = 1

  Dim BinaryStream
  Set BinaryStream = CreateObject("ADODB.Stream")

  BinaryStream.Type = adTypeText
  BinaryStream.CharSet = CharSet

  BinaryStream.Open
  BinaryStream.WriteText Text

  BinaryStream.Position = 0
  BinaryStream.Type = adTypeBinary
  Stream_StringToBinary = BinaryStream.Read

  Set BinaryStream = Nothing
End Function

Function ReadBinaryFile(path)
    Const adTypeText = 2
    Const adTypeBinary = 1

    dim inStream
    dim myByte,myByteValue,myCharacter

    set inStream=WScript.CreateObject("ADODB.Stream")

    inStream.Open
    inStream.type=1

    inStream.LoadFromFile path 

    ReadBinaryFile=inStream.Read()
    inStream.Close
End Function
