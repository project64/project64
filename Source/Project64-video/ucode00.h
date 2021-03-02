// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#pragma once

void uc0_cleargeometrymode();
void uc0_culldl();
void uc0_displaylist();
void uc0_enddl();
void uc0_line3d();
void uc0_matrix();
void uc0_modifyvtx(uint8_t where, uint16_t vtx, uint32_t val);
void uc0_movemem();
void uc0_moveword();
void uc0_popmatrix();
void uc0_tri1();
void uc0_tri4();
void uc0_setgeometrymode();
void uc0_setothermode_l();
void uc0_setothermode_h();
void uc0_texture();
void uc0_vertex();

void modelview_pop(int num = 1);
void modelview_mul(float m[4][4]);
void modelview_mul_push(float m[4][4]);
void modelview_load(float m[4][4]);
void modelview_load_push(float m[4][4]);

void projection_load(float m[4][4]);
void projection_mul(float m[4][4]);

void rsp_tri1(gfxVERTEX **v, uint16_t linew = 0);
void rsp_tri2(gfxVERTEX **v);
void rsp_vertex(int v0, int n);

void load_matrix(float m[4][4], uint32_t addr);