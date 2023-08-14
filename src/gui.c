/* Chat GUI
   Copyright (c) 2023 bellrise */

#include "gui.h"

#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct window
{
    pthread_mutex_t lock;
    int width;
    int height;
    int cursor_x;
    int cursor_y;
    char **linebuf;
    int n_lines;
    char *cmdbuf;
    int cmdlen;
};

static struct window window;

static void sigint_handler(int __attribute__((unused)) _)
{
    gui_close();
    exit(0);
}

void setpos(int x, int y)
{
    printf("\033[%d;%dH", y, x);
    fflush(stdout);
}

void draw_command_box()
{
    setpos(1, window.height - 2);
    fputc('+', stdout);
    for (int i = 0; i < window.width - 2; i++) {
        fputc('-', stdout);
    }

    fputs("+\n|\n+", stdout);

    for (int i = 0; i < window.width - 2; i++) {
        fputc('-', stdout);
    }
    fputc('+', stdout);

    setpos(window.width, window.height - 1);
    fputc('|', stdout);
}

void redraw()
{
    printf("\033[2J\033[1;1H");
    draw_command_box();

    setpos(1, 1);
    for (int i = window.n_lines - 1; i >= 0; i--) {
        if (window.linebuf[i] == NULL)
            fputc('\n', stdout);
        else
            printf("%s\n", window.linebuf[i]);
    }

    setpos(3, window.height - 1);
    if (window.cmdlen) {
        printf("%.*s", window.cmdlen, window.cmdbuf);
    }

    fflush(stdout);
}

void gui_open()
{
    memset(&window, 0, sizeof(window));
    printf("\033[2J\033[1;1H");
    fflush(stdout);

    struct winsize winsize;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);

    window.height = winsize.ws_row;
    window.width = winsize.ws_col;

    window.n_lines = window.height - 3;
    window.linebuf = malloc(sizeof(char *) * window.n_lines);
    for (int i = 0; i < window.n_lines; i++)
        window.linebuf[i] = NULL;

    window.cmdbuf = calloc(1, 512);
    window.cmdlen = 0;

    signal(SIGINT, sigint_handler);

    struct termios tios;
    tcgetattr(STDIN_FILENO, &tios);
    tios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tios);

    redraw();
}

void gui_close()
{
    struct termios tios;
    tcgetattr(STDIN_FILENO, &tios);
    tios.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tios);

    printf("\033[2J\033[1;1H");
    fflush(stdout);
}

void gui_write_line(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    /* To add a line, we need to move all existing lines 1 up. */

    free(window.linebuf[window.n_lines - 1]);
    memmove((void *) ((size_t) window.linebuf) + sizeof(char *), window.linebuf,
            (window.n_lines - 1) * sizeof(char *));

    char *line = malloc(window.width + 64);
    vsnprintf(line, window.width + 64, fmt, args);
    window.linebuf[0] = line;

    pthread_mutex_lock(&window.lock);
    redraw();
    pthread_mutex_unlock(&window.lock);

    va_end(args);
}

char *gui_input()
{
    char *buf = calloc(1, 512);
    int c;

    while (1) {
        c = fgetc(stdin);
        if (c == '\n' || c == '\r') {
            window.cmdlen = 0;
            return buf;
        }

        /* Backspace */
        if (c == '\b' || c == 0x7f) {
            if (window.cmdlen == 0)
                continue;

            buf[--window.cmdlen] = 0;
            window.cmdbuf[window.cmdlen] = 0;
            fputs("\b \b", stdout);
            fflush(stdout);
            continue;
        }

        buf[window.cmdlen] = c;
        window.cmdbuf[window.cmdlen++] = c;
        fputc(c, stdout);
        fflush(stdout);
    }

    return buf;
}
