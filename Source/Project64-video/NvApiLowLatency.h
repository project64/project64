// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
//
// Best-effort NVIDIA low-latency hint for the OpenGL renderer.
//
// True NVIDIA Reflex is not available here: the Reflex SDK targets Direct3D
// 11/12 and Vulkan only, and this plugin renders with OpenGL. The
// OpenGL-applicable NVIDIA lever is the driver's "Maximum pre-rendered frames"
// (prerender limit), which caps the GL render-ahead queue - the same intent as
// NVIDIA "Low Latency Mode". This module sets that to 1 for this application's
// own NVIDIA driver profile via NvAPI's DRS interface.
//
// It is entirely best-effort and fail-safe: on non-Windows builds and on any
// machine without the NVIDIA driver (nvapi DLL absent) it is a no-op, and every
// NvAPI call is return-checked so a failure simply does nothing.

#pragma once

// Applies the NVIDIA low-latency (prerender-limit = 1) hint to this application's
// driver profile. Safe to call once after the GL context is created. No-op on
// non-NVIDIA / non-Windows systems.
void NvApiApplyLowLatency();
