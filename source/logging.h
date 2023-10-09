
#ifndef __LOGGING__
#define __LOGGING__

#include <tonc_tte.h>

#define CURSOR_HEIGHT 8
#define CURSOR_MAX_HEIGHT 160

#define LOG_TO_SCREENF  \
    pre_log();          \
    tte_printf          \

void log_to_screen(char text[]);
void pre_log();



#endif