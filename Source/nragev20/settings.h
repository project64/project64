/*
    N-Rage`s Dinput8 Plugin
    (C) 2002, 2006  Norbert Wladyka

    Author`s Email: norbert.wladyka@chello.at
    Website: http://go.to/nrage


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#define DIRECTINPUT_VERSION 0x0800

// hacks for GNU C compilers // No thank you....
#ifdef __GNUC__

#ifndef WINVER
#define WINVER 0x0500
#endif // #ifndef WINVER

#ifndef _WIN32_IE
#define _WIN32_IE 0x0300
#endif // #ifndef _WIN32_IE

//#ifndef _const unsigned char *_DEFINED
//#define _const unsigned char *_DEFINED
//typedef const unsigned char *const unsigned char *;
//#endif

#endif // #ifdef __GNUC__

    // Our default buffer size for TCHAR arrays (resources get loaded through here)
    // MAKE SURE localized resources do not exceed this limit, or they will be cut off.
#define DEFAULT_BUFFER      256

    // use default settings for Release and Debugbuild
#define STDCONFIG

#ifndef STDCONFIG
// ----------------------------------------------------------------------------
// custom (nonstandard) settings here
    // workaround for a Adaptoiddriver bug;
    // basically if the Adaptoid USB driver doesn't respond that there's a pak, say there is one anyway
#define ADAPTOIDPAK_RUMBLEFIX

    // remove unimplemented Elements of the GUI
// #define HIDEUNIMPLEMENTED

    // check controllercommands for valid CRC
// #define MAKEADRESSCRCCHECK
    // display Button for writing Shortcuts binary
// #define RAWPROFILEWRITE

    // enable selection of Transferpak
// #define V_TRANSFERPAK
    // enable selection of VoicePak
// #define V_VOICEPAK
// ----------------------------------------------------------------------------

#else
// Standard Settings

#ifdef _DEBUG
// ----------------------------------------------------------------------------
// Standard Debug Settings
    // workaround for a Adaptoiddriver bug
#define ADAPTOIDPAK_RUMBLEFIX

    // remove unimplemented Elements of the GUI
// #define HIDEUNIMPLEMENTED

    // check controllercommands for valid CRC
#define MAKEADRESSCRCCHECK
    // display Button for writing Shortcuts binary
#define RAWPROFILEWRITE

    // enable selection of Transferpak
#define V_TRANSFERPAK
    // enable selection of VoicePak
#define V_VOICEPAK

// spits out loads of extra info for ControllerCommand and ReadController
// #define ENABLE_RAWPAK_DEBUG

// ----------------------------------------------------------------------------

#else
// ----------------------------------------------------------------------------
// Standard Release Settings
    // workaround for a Adaptoiddriver bug
#define ADAPTOIDPAK_RUMBLEFIX

    // remove unimplemented Elements of the GUI
#define HIDEUNIMPLEMENTED

    // check controllercommands for valid CRC
// #define MAKEADRESSCRCCHECK
    // display Button for writing Shortcuts binary
// #define RAWPROFILEWRITE

    // enable selection of Transferpak
#define V_TRANSFERPAK
    // enable selection of VoicePak
// #define V_VOICEPAK

// ----------------------------------------------------------------------------

#endif // #ifdef _DEBUG
#endif // #ifndef STDCONFIG
#undef STDCONFIG

#ifdef _DEBUG
#define VERSIONINFO _T(VERSIONNUMBER) _T("-Debugbuild")
#else
#define VERSIONINFO _T(VERSIONNUMBER)
#endif // #ifdef _DEBUG

#endif // #ifndef _SETTINGS_H_
