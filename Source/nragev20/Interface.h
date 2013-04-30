/*	
	N-Rage`s Dinput8 Plugin
    (C) 2002, 2006  Norbert Wladyka

	Author`s Email: norbert.wladyka@chello.at
	Website: http://go.to/nrage


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _NRINTERFACE_
#define _NRINTERFACE_

BOOL CALLBACK MainDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
void SetModifier( CONTROLLER *pcController );
void SetControllerDefaults( CONTROLLER *pcController );

 // application internal message
#define WM_USER_UPDATE		WM_USER + 1
 // application internal message
#define WM_USER_READVALUES	WM_USER + 2

#define CONFIGTHRESHOLD		50
#define MOUSE_THRESHOLD		10

#define SC_SCANSUCCEED		0x01
#define SC_SCANESCAPE		0x10

#define TIMER_BUTTON		1

#define INTERVAL_BUTTON		20
#define INTERVAL_RUMBLETEST	20

#define MPAK_FORMATABLE		0x01
#define MPAK_READABLE		0x02
#define MPAK_WRITEABLE		0x04

#define MPAK_OK				0x07
#define MPAK_INUSE			0x12
#define MPAK_WRONGSIZE		0x27
#define MPAK_DAMAGED		0x71
#define MPAK_ERROR			0x81
#define MPAK_NOSELECTION	0xF0


#define BSET_CONTROLS		1
#define BSET_SHORTCUTS		2


typedef struct _IFDEVICE
{
	TCHAR szProductName[MAX_PATH+1];
	BYTE bProductCounter;
} IFDEVICE, *LPIFDEVICE;

typedef struct _INTERFACEVALUES
{
	BYTE ChosenTab;
	CONTROLLER Controllers[4];
	IFDEVICE FFDevices[4];
	SHORTCUTS Shortcuts;
	LANGID Language;
	bool fDisplayShortPop;
} INTERFACEVALUES, *LPINTERFACEVALUES;

#define TAB_CONTROLLER1		0
#define TAB_CONTROLLER2		1
#define TAB_CONTROLLER3		2
#define TAB_CONTROLLER4		3
#define TAB_SHORTCUTS		4
#define TAB_FOLDERS			5

#define TAB_CONTROLS		0
#define TAB_DEVICES			1
#define TAB_MODIFIERS		2
#define TAB_PAK				3

extern INTERFACEVALUES *g_ivConfig;

#endif // _NRINTERFACE_
