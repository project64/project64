#pragma once
#include <stddef.h>
#include <stdint.h>

/** ORs Win32 keyboard state into a 512-byte SDL scancode buffer (0xFF = down). */
void Win32MergeKeyboardOrInto(uint8_t * keyboardState, size_t byteCount);
