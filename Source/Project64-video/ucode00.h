/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2003-2009  Sergey 'Gonetz' Lipski                          *
* Copyright (C) 2002 Dave2001                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/
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

void rsp_tri1(VERTEX **v, uint16_t linew = 0);
void rsp_tri2(VERTEX **v);
void rsp_vertex(int v0, int n);

void load_matrix(float m[4][4], uint32_t addr);