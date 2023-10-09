
#include "logging.h"

static int log_cursor = 0;

void log_to_screen(char text[])
{
	pre_log();
    tte_write(text);
}

void pre_log()
{
	tte_printf("#{P:0,%i}", (log_cursor * CURSOR_HEIGHT) % CURSOR_MAX_HEIGHT);
    tte_erase_line();
	tte_printf("% 3i ", log_cursor);
    log_cursor++;
}
