N-Rage's Input Plugin
Release Version 1.82a

Disclaimer:
	Im not responsible for any Damage caused by this Programm!
	This Programm is Freeware, its freely distributable, but it must be distributed as whole unmodified Zip-Archive.
	U aren't allowed to earn money by distributing it.

The use of this Plugin
	This Plugin is for use with a N64-Emulator that supports InputPlugins and Zilmars Spec.
	Some Emulators that support it are: Project64, Apollo, 1964, TR64 

Main Features
	Up to four Players
	Up to four GamePads, 1 Keyboard and 1 Mouse can be handled at once
	Full support of Buttons, POVs, Sliders, Axes, Mouse, Keyboard
	Emulating Mem-Paks and Rumble-Paks(via ForceFeedBack)
	Direct Adaptoid Support
	up to 256 Configurable Modifiers, with 3 different Types
	System-independent Controller-Profiles
	This Plugin is compatible to Zilmar's Input-Plugin Specs 1.0 and will work with all Emulators that support this Spec

Requirements:
	A computer with Windows & DirectX8.0 or higher installed.
	A Emulator wouldnt be a bad thing too.

Thanks go out to
	Azimer for his help with MemPaks
	Zilmar & Jabo for their awesome Project64.
	Smiff, Bodie, Cyber, Hotshitu, Gannonboy for testing.
	MadManMark for adding Transferpaks

Known Issues:
	Old Profiles wont work, this is by purpose.

#---------------------------------------------------------------------#
History:

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

