#include <excpt.h>

#define __except_try() __try
#define __except_catch() __except (g_MMU->MemoryFilter(_exception_code(), _exception_info()))
