================================================
          Project64, by Zilmar and Jabo
            Copyright (c) 1998 - 2001
  The Premiere Nintendo64 emulator for Windows
================================================

-------------------
Standard Disclaimer
-------------------

The N64 is a registered trademark of Nintendo, same goes for other companies mentioned above, or their products.

The authors are not affiliated with any of the companies mentioned, this software may be distributed for free, never sold in any way, as long as the original archive and software included is not modified in any way or distributed with ROM images.

You use this software at your own risk, the authors are not responsible for any loss or damage resulting from the use of this software. If you do not agree with these terms do not use this software, simple.

--------
Overview
--------

Project64 is an emulator that has been in developlment for a couple of years. We are proud to allow other people to use the product that we have made for their enjoyment. Project64 features emulation of the Reality Signal Processor, which was reverse engineered by zilmar. This information has produced an accurate interpreter that has turned in to a recompiler by jabo, setting it apart from some of the emulators in development today. Another feature in Project64 is an accurate and fast Display Processor graphics core for OpenGL and Direct3D, developed by jabo over the last few years.

--------
Features
--------

Internally Project64 features two advanced recompilers, for the R4300i and the RSP respectively, both based off of zilmar's original interpreters. Both the R4300i and RSP interpreters are available as alternatives to the recompilers via settings.

- The R4300i recompiler is written by zilmar. It features dynamic block creation and advanced optimizations due to it's register caching core. It also has self-mod protection schemes   implemented to maximize compatibility and speed.

- The RSP recompiler is written by jabo. This compiler creates dynamic blocks of code, and optimizes the signal processor code through various code analysis techniques. It makes use of MMX and SSE to provide real-time emulation of this powerful co-processor.

Project64 uses high-level emulation for graphics, and low level emulation for audio. Jabo wrote Direct3D and OpenGL plugins for graphics, they have high quality blending and texturing, with several microcodes implemented from Mario64 to Zelda64 between the plugins. High level microcode emulation is optimized using SSE, and 3DNow!, and some parts of texturing have MMX optimizations.

--------------
Known Problems
--------------

Project64 is not perfect, there is some compatibility issues in terms of CPU, Graphics, and Audio that prevents games from functioning properly. See our support web site for a compatibility listing of games that are known to run with Project64, as well as problems that are known already.

Please do not expect games to be perfect, we put a lot of effort into this emulator, but every detail may not meet the guidelines for perfect emulation. If you want to experience games as they were meant to be played purchase a nintendo 64.

-------------------
Contact the Authors
-------------------

All our plugins use the Project64 plugin specifications, see our website for details.

=> Read this file entirely, use the message boards on the website for all feedback on PJ64, we do not have time to help people individually.

- If you don't at least meet the min requirements, don't ask us for help
- Do not ask if your system will work, or if we will support your hardware
- Do not ask us about games, we will not send them to you or tell you where to get them
- Do not ask us when a specific game will work
- Do not ask us when the next version will be out, for betas, or what features it will have
- Do not ask us about plugins we didn't write, contact the proper author
- Do not report problems with using our plugins in other emulators
- Do not email us files without permission
- Do not ask us about things not on pj64.net, like the message board, we have no control

no exceptions, if you want to ask these questions try a messageboard at our website. 
http://www.pj64.net

You can reach us at the following email addresses, if it's feedback on pj64 please think about what you are asking, lots of emails get ignored because you either aren't supposed to email us these questions (read above), or it's answered in this file or through the extensive amount of information available on our support website.

jabo@emulation64.com, zilmar@emulation64.com

You can always find updated contact info on our website.

------------------
Credits and Greets
------------------

We would like to thank the following people for their support and help, in no specific order.

hWnd, Cricket, F|RES, rcp, _Demo_, Phrodide, icepir8, TNSe, gerrit, schibo, Azimer, Lemmy, LaC, Anarko, duddie, Bpoint, StrmnNrmn, slacka, smiff

As well as the people we have forgotten.

[EOF]