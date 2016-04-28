RELEASE Version 2.3c

Disclaimer:
	This software is distributed as is, without any guarantees of
	merchantability or fitness for a particular purpose. Basically,
	you can't sue us if you screw up your own computer.
	This program is freeware released under the GPL. It's freely
	distributable, as long as you follow the GPL. You are free to modify it,
	but of course we ask that you tell us so we can incorporate bugfixes
	in the next version.
	Finally, don't try to earn money by distributing it.

What is it?
	This plugin is for use with an N64 emulator that supports input plugins
	through Zilmar's input spec.
	Some emulators that support it are: Project64, Apollo, 1964, TR64 

Main Features
	Up to four N64 controllers supported
	Handles as many game controllers as you can plug in (you may have to tweak
		the compile options for huge numbers), plus 1 keyboard and 1 mouse.
	Full support of any controls you can see through DirectInput
	Emulation of MemPaks, Rumble Paks (via DirectInput Force Feedback),
		and Transfer Paks (except GB Tower in Pokemon Stadium games)
	Complete Adaptoid support
	Up to 256 Configurable "modifiers", in 3 different flavors
	System-independent controller profiles
	As released, this plugin is compatible with Zilmar's Input Plugin Specs 1.0
		and will work with all emulators that support this spec

Requirements:
	A computer with Windows & DirectX9.0 or higher installed.
	An emulator
	Some games wouldn't hurt
	You need the Microsoft Visual C++ 2010 SP1 Redistributable Package (x86)

Installation:
	Consult your emulator documentation as to where to place the DLL file
		(usually its the "plugins" folder or something similar)
	Choose the plugin from within the emulator; again, consult the documentation
		if you don't know how.
	If you have a language .dll, place it in the same directory as your emulator
		e.g. project64.exe

Thanks go out to
	Azimer for his help with MemPaks
	Zilmar & Jabo for their awesome Project64.
	Smiff, Bodie, Cyber, Hotshitu, Gannonboy, Harlay, squall_leonhart, Poobah, Legend
		for testing.
	MadManMark for adding Transferpaks
	RabidDeity for tweaks and additions
	aTomIC, Harlay, NaSeR, Siskoo for translations
	

Known Issues:
	Old Profiles won't work, this is on purpose.
	GB Tower doesn't work in Pokemon Stadium games (not likely to be fixed; just use
		a GB emulator to play the games)
	No voice pak emulation (only one game supports it anyway)

For the latest SOURCE CODE please check out trunk from the Subversion server:
https://nragev20.svn.sourceforge.net/svnroot/nragev20/trunk
(you can also find some useful information in the docs directory)
For now, you'll need a copy of Visual Studio... but we're working on a Makefile.

#---------------------------------------------------------------------#
History:

+ means fixed/added
/ means changed

Release 2.3c
/Xinput Rumble and Deadzone fixed (Thanks KrossX)
/Some potential crash and buffer overruns? fixed by kidkat
/Some UI changes to improve readability for high dpi setups

Release 2.3b
/Xinput config loading fixed
+No longer crashes if an assigned device is removed or not present when starting emulation.
/RC files saved from visual studio break the file lister, reverted and edited with notepad.

Release 2.2:
+Resolved file browsers not displaying supported file types
+Improved Xinput support by backporting changes from 1964input

Release 2.2 beta:
+ Experimental Xinput support
+ Fixes to GB Battery support

Release 2.1 rc3:
+ The last of the shortcut bugs should be fixed.  Switching paks should work
	just fine; the plugin now inserts a 1 second delay so that the game
	can detect the change properly.
+ Message window no longer sucks CPU cycles


Release 2.1 rc2:
+ Incorporated koolsmoky's message window patch. The message window should
	work perfectly now.

Release 2.1 rc1:
+ Many many little bugs fixed, and minor tweaks.  This should bring the
	release versions in line with the tweaks from the DEBUG line.

Release 2.00b:
+ fixed several problems with Win98; plugin should work under Win98 now
+ fixed several bugs with memory mapped file handles.  Solves several crash
	issues with mempak or transferpaks
+ changed transfer pak MBC5 code to use proper MBC5 rom bank switching, and
	also fixed up MBC3 rom bank switching.  This may fix some lingering
	transfer pak issues.  Also did several tpak speed optimizations
	(thanks guille007)

Release 2.00a:
+ Fixed mouse assignment in Interface.  Again.

Release 2.00 (rabid goes crazy):
/ Large portions of the code completely rewritten.
+ Better detection of devices; more joysticks should work now, as well as steering
	wheels and things that aren't strictly "gamepads"
+ .a64 notefile import and export works much better now
+ N64 controllers can now get input from multiple gamepad type devices
+ Now able to assign a key, mouse, or gamepad control to as many N64 control
	surfaces as you like
+ international language support
/ device selection for keybinding no longer needed; devices list now shown as
	"Force-Feedback Devices" under ControllerPak selection (pick Rumble, and
	tick the "RawMode" box)
/ no longer possible to send FF events from multiple controllers to the same FF
	device (this shouldn't have worked anyway)
+ capture mouseclicks properly in Controllers tab (disable button clicks while
	polling)
+ release exclusive mouse while in config menu (fixes a mouse bind while locked
	issue)
+ add independent X/Y mouse sensitivity
+ changed absolute mouse support: now choose between Buffered (default from before),
	Absolute, and Deadpan (control only moves while you move the mouse)
+ various optimizations, bugfixes, and spelling fixes
+ LOTS of documentation added to the source; it should be more legible now
+ rewrote controller save and restore (underlying CONTROLLER and BUTTON structs
	changed...)
/ Button mappings and modifiers will need to be reset on first load
+ now rewrites mempak and transferpak RAM data to disk almost immediately after
	writing to controller (1.83 and previous didn't save the mempak until RomClose)
+ now possible to map shortcuts to buttons and axes as well as keys (be careful)
+ tabbing within the config window actually works now
+ can save and load shortcuts to a file
+ Transferpak MBC5 support fixed (Pokemon Yellow, Perfect Dark), also ROM files should
	load much faster now
+ several crash bugs and memory leaks squashed

Release 1.83:
Changed: Rewrote GB Cart emulation (Now supports ROM-only, MBC1, MBC2, MBC3 and MBC5 carts)
Added: support for GB Cart RTC based on VisualBoy Advance save format.
Added: option for slower rapid-fire in macros. Fixes problems with some games like Paper Mario.
Added: optional and adjustable rapid fire to standard input keys.

Release 1.82a:
Fixed: Correct handling POV Controls again.. DOH

Release 1.82:
Added: Transferpak-Emulation - done by MadManMark, so hes the one to thank for this.
Changed: Various cleanups&optimizations, recompiled with VC++ 7.0

Release 1.81a:
	Fixed a bug introduced by the last version. 
	
Release 1.81:
Added: Default Profile & default Shortcuts.
Changed: can now open read-only Files, some old & unecessary checks removed
	 Interface tweaked a bit.

Release 1.80:
Fixed: another Access Violation, crash within Rumble emulation
Added: MouseLock Shortcut
Changed: Code-Cleanups, "save" and "use" Buttons instead of "Ok"

Beta 1.79:
Fixed: Access Violations
Added: saving/loading Profiles, visual Rumble, DexDrive support

Beta 1.78:
Added: MemPak Manager Functions, 2 switchable Analog Stick settings, Config Modifier, absolute KeyBoard setting
Changed: KeyBoard & Mouse Handling

Beta 1.76:
Only a small Part is based on older Plugins, new gui, new features.
New: Direct Adaptoid support, up to 256 Modifiers, different Rumble settings, Shortcuts
Missing: saving/loading Profiles, Config Modifier, alternate Controlset
Changed: about everything else ;)

Release 1.61:
Fixed: a small MemPak issue( Perfect Dark )

Release 1.60:
Fixed: crashes in Config-Dialog, crashes when a used GamePad isnt available, Toggle-Modifiers now get reset each time a rom is loaded or the configuration changed, now games that reported "no Controller" in RAW Mode will work, multiple Controllers of the same name now get enumerated and detected right.
Changes: Mouse-Axis can now be assigned by moving it. Now all Devices are scaned at once in Config-Window. "Default Axe-Movement" is now replaced by "Default Analog-Stick Range", which means it sets maximum range of the virtual Stick.
Added seperate X/Y Modes for Mouse

Beta 1.53: Fixed yet more Bugs. Changed Profile-Format, old Profiles wont work( The new Format leaves some Space for future Functions, so they should stay compatible now)
Beta 1.52: Fixed a bunch of bugs, some cleanups, gave the Status-Line a life.
Beta 1.51: Fixed Issues when more than one Pak is used. MemPak is workin now( was alot easier than i thought ). RAW-Handling tweaked.
Beta 1.50b: Finally fixed Rumble Issues, with feedback from bodie & hotshi again.
Beta 1.50: Added Rapid-Fire for Macro-Modifiers, seperate option for negating X/Y Axis. Rumble doesnt works with all gamepads, no clue why not. Thanx to bodie, cyber and hotshitu who gave me allota feedback about Rumble.
Beta 1.49b-d: small Bugfixes, tried to get Rumble working on all GamePads.
Beta 1.49a: fixed some Controllers(including Adaptoid) not working ingame & hopefully the Controllers that dint Rumble
Beta 1.49: some Bugfixes with POV and RRRRRumble is working now!
Beta 1.48: Configuration Routines written from scratch, extended Modifiers, much better Mouse Support, tweaked the whole Code. Almost a new Plugin ;)

Beta 1.42: This was the last Release before i rewritten the Configuration Stuff, I dint kept track of the Versions before 1.48, many of em were just internal Releases. And I totally messed it up with the Versions-Numbers anyway.
