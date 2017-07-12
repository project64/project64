if WScript.Arguments.Count < 3 then
    ShowUsage()
ElseIf StrComp("--create",WScript.Arguments(0)) = 0 Then
    if WScript.Arguments.Count < 4 then
        ShowUsage()
    else
        CreateUploadTarget()
    end if
ElseIf StrComp("--files",WScript.Arguments(0)) = 0 Then
    if WScript.Arguments.Count < 4 then
        ShowUsage()
    else
        UploadFiles()
    end if
Else
    ShowUsage()
End if

sub ShowUsage()
    WScript.StdOut.WriteLine "incorrect parameters"
    WScript.StdOut.WriteLine "--create [password] [BuildUrl] [BuildName]"
    WScript.StdOut.WriteLine "--files [password] [dir to upload] [BuildName]"
    WScript.Quit 1
end sub

function Project64Url()
    Project64Url = "http://www.pj64-emu.com"
End Function

sub CreateUploadTarget()
    dim BuildUrl
    BuildUrl = WScript.Arguments(2)

    Dim objHTTP
    Set objHTTP = CreateObject("MSXML2.XMLHTTP")
    objHTTP.open "GET", BuildUrl & "buildTimestamp", False
    objHTTP.send
    if (objHTTP.status <> 200) then
        WScript.StdOut.WriteLine "failed to get job timestamp (" & BuildUrl & "buildTimestamp)"
        WScript.Quit 1
    end if
    dim d

    SetLocale 1033
    build_date=CDate(objHTTP.responseText)
    
    Set objHTTP = CreateObject("MSXML2.XMLHTTP")
    objHTTP.open "GET", BuildUrl & "api/xml?wrapper=changes", False
    objHTTP.send
    
    if (objHTTP.status <> 200) then
        WScript.StdOut.WriteLine "failed to get job details (" & BuildUrl & "api/xml?wrapper=changes)"
        WScript.Quit 1
    end if

    Dim xmlDoc
    Set xmlDoc = objHTTP.responseXML
    Set objLst = xmlDoc.getElementsByTagName("freeStyleBuild")

    Dim ProductDescription
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
                        ProductDescription = ProductDescription & "[*]" & comment & " (commit: [URL=""https://github.com/project64/project64/commit/" & commitId & """]"& commitId & "[/URL])" & vbCrLf
                    end if
                next
            end if
        next
    Next
    if (Len(ProductDescription) > 0) then
        ProductDescription = "Changes:"&vbCrLf&"[LIST]" & vbCrLf & ProductDescription & "[/LIST]"
    else
        ProductDescription = "No code changes"
    end if
            
    Dim url
    url = Project64Url() + "/index.php"

    dim data
    data = "option=com_betafile"
    data = data & "&task=CreateProduct"
    data = data & "&password="&WScript.Arguments(1)
    data = data & "&jform[product_name]="&WScript.Arguments(3)
    data = data & "&jform[product_desc]="&ProductDescription
    data = data & "&jform[product_date]="&Year(build_date) & "-" & Month(build_date) & "-" & Day(build_date)
    
    Set objHTTP = CreateObject("Microsoft.XMLHTTP")
    objHTTP.open "POST", url, False

    objHTTP.setRequestHeader "Content-Type", "application/x-www-form-urlencoded"
    objHTTP.send data

    if objHTTP.Status <> 200 then
        WScript.StdOut.WriteLine "Create beta file failed"
        WScript.StdOut.WriteLine "status: " & objHTTP.Status
        WScript.StdOut.WriteLine objHTTP.responseText
        WScript.Quit 1
    end if

    Set objHTTP = Nothing
end sub

sub UploadFiles()
    DirToUpload = WScript.Arguments(2)
    WScript.StdOut.WriteLine "UploadDirectory start - " & DirToUpload
    Set objFSO = CreateObject("Scripting.FileSystemObject")
    Set objFolder = objFSO.GetFolder(DirToUpload)
    Set colFiles = objFolder.Files
    For Each objFile in colFiles
        UploadFile DirToUpload & "\" & objFile.Name
    Next
    WScript.StdOut.WriteLine "UploadDirectory Finished"
end sub

sub UploadFile(FileToUpload)
    Const adTypeBinary = 1

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

    Dim url
    url = Project64Url() + "/index.php"

    dim fileContents
    fileContents = ReadBinaryFile(FileToUpload)

    dim PreFormData, PostFormData    
    PreFormData = PreFormData & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""option""" & vbCrLf & vbCrLf & "com_betafile"& vbCrLf   
    PreFormData = PreFormData & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""task""" & vbCrLf & vbCrLf & "AddFile"& vbCrLf   
    PreFormData = PreFormData & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""password""" & vbCrLf & vbCrLf & WScript.Arguments(1) & vbCrLf   
    PreFormData = PreFormData & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""jform[product_name]""" & vbCrLf & vbCrLf & WScript.Arguments(3) & vbCrLf   
    PreFormData = PreFormData & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""jform[add_file]""; filename=""" & fileName & """" & vbCrLf
    PreFormData = PreFormData & "Content-Type: application/zip" & vbCrLf & vbCrLf
    PostFormData = vbCrLf & "--AaB03x" & vbCrLf & "Content-Disposition: form-data; name=""upload""" & vbCrLf & vbCrLf & "Upload" & vbCrLf   
    PostFormData = PostFormData & vbCrLf & vbCrLf & "--AaB03x--"& vbCrLf

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

    Set objHTTP = CreateObject("Microsoft.XMLHTTP")
    objHTTP.open "POST", url, False

    objHTTP.setRequestHeader "Content-Type", "multipart/form-data; boundary=AaB03x"
    objHTTP.setRequestHeader "Content-Length", Len(fileContents)
    objHTTP.send DataToPOST
    
    if objHTTP.Status <> 200 then
        WScript.StdOut.WriteLine "Failed to upload file"
        WScript.StdOut.WriteLine "status: " & objHTTP.Status
        WScript.StdOut.WriteLine objHTTP.responseText
        WScript.Quit 1
    end if
    WScript.StdOut.WriteLine "UploadFile Finished"
end sub

Function ReadBinaryFile(path)
    Const adTypeBinary = 1
    Const adTypeText = 2

    dim inStream
    dim myByte,myByteValue,myCharacter

    set inStream=WScript.CreateObject("ADODB.Stream")

    inStream.Open
    inStream.type=1

    inStream.LoadFromFile path 

    ReadBinaryFile=inStream.Read()
    inStream.Close
End Function

Function Stream_StringToBinary(Text, CharSet)
  Const adTypeBinary = 1
  Const adTypeText = 2

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
