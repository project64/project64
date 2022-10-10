#pragma once

#include <Project64-core/Notification.h>

bool AppInit(CNotification * Notify, const char * BaseDirectory, int argc, char ** argv);
void AppCleanup(void);
