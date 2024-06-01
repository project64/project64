// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#pragma once

void uc1_branch_z();
void uc1_rdphalf_1();
void uc1_quad3d();
void uc1_tri1();
void uc1_tri2();
void uc1_vertex();

extern uint32_t branch_dl;
