# Microsoft Developer Studio Project File - Name="WTL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=WTL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WTL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WTL.mak" CFG="WTL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WTL - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "WTL - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WTL - Win32 Release"

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

!ELSEIF  "$(CFG)" == "WTL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../Bin/Debug"
# PROP Intermediate_Dir "../../Build/WTL/Debug"
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

!ENDIF 

# Begin Target

# Name "WTL - Win32 Release"
# Name "WTL - Win32 Debug"
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\atlapp.h
# End Source File
# Begin Source File

SOURCE=.\atlcrack.h
# End Source File
# Begin Source File

SOURCE=.\atlctrls.h
# End Source File
# Begin Source File

SOURCE=.\atlctrlw.h
# End Source File
# Begin Source File

SOURCE=.\atlctrlx.h
# End Source File
# Begin Source File

SOURCE=.\atlddx.h
# End Source File
# Begin Source File

SOURCE=.\atldlgs.h
# End Source File
# Begin Source File

SOURCE=.\atlfind.h
# End Source File
# Begin Source File

SOURCE=.\atlframe.h
# End Source File
# Begin Source File

SOURCE=.\atlgdi.h
# End Source File
# Begin Source File

SOURCE=.\atlmisc.h
# End Source File
# Begin Source File

SOURCE=.\atlprint.h
# End Source File
# Begin Source File

SOURCE=.\atlres.h
# End Source File
# Begin Source File

SOURCE=.\atlresce.h
# End Source File
# Begin Source File

SOURCE=.\atlscrl.h
# End Source File
# Begin Source File

SOURCE=.\atlsplit.h
# End Source File
# Begin Source File

SOURCE=.\atltheme.h
# End Source File
# Begin Source File

SOURCE=.\atluser.h
# End Source File
# Begin Source File

SOURCE=.\atlwince.h
# End Source File
# Begin Source File

SOURCE=.\atlwinx.h
# End Source File
# End Group
# End Target
# End Project
