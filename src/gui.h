/* User interface.
   Copyright (c) 2023 bellrise */

#pragma once

void gui_open();
void gui_close();

void gui_write_line(const char *fmt, ...);
char *gui_input();
