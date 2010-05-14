# Microsoft Developer Studio Project File - Name="7zip" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=7zip - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "7zip.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "7zip.mak" CFG="7zip - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "7zip - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "7zip - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "7zip - Win32 External Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "7zip - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "7zip - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../Build/7zip/Debug"
# PROP Intermediate_Dir "../../Build/7zip/Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "7zip - Win32 External Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "7zip___Win32_External_Release"
# PROP BASE Intermediate_Dir "7zip___Win32_External_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../Bin/External"
# PROP Intermediate_Dir "../../Build/7zip/External"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "7zip - Win32 Release"
# Name "7zip - Win32 Debug"
# Name "7zip - Win32 External Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\7zAlloc.c
# End Source File
# Begin Source File

SOURCE=.\7zBuffer.c
# End Source File
# Begin Source File

SOURCE=.\7zCrc.c
# End Source File
# Begin Source File

SOURCE=.\7zDecode.c
# End Source File
# Begin Source File

SOURCE=.\7zExtract.c
# End Source File
# Begin Source File

SOURCE=.\7zHeader.c
# End Source File
# Begin Source File

SOURCE=.\7zIn.c
# End Source File
# Begin Source File

SOURCE=.\7zItem.c
# End Source File
# Begin Source File

SOURCE=.\7zMethodID.c
# End Source File
# Begin Source File

SOURCE=.\Compress\LzmaDecode.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\7zAlloc.h
# End Source File
# Begin Source File

SOURCE=.\7zBuffer.h
# End Source File
# Begin Source File

SOURCE=.\7zCrc.h
# End Source File
# Begin Source File

SOURCE=.\7zDecode.h
# End Source File
# Begin Source File

SOURCE=.\7zExtract.h
# End Source File
# Begin Source File

SOURCE=.\7zHeader.h
# End Source File
# Begin Source File

SOURCE=.\7zIn.h
# End Source File
# Begin Source File

SOURCE=.\7zItem.h
# End Source File
# Begin Source File

SOURCE=.\7zMethodID.h
# End Source File
# Begin Source File

SOURCE=.\7zTypes.h
# End Source File
# Begin Source File

SOURCE=.\Compress\LzmaDecode.h
# End Source File
# Begin Source File

SOURCE=.\status.h
# End Source File
# End Group
# End Target
# End Project
