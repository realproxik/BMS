#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "console/console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define BMS_MAX_SINKS     8
#define BMS_MAX_COMMANDS  128
#define BMS_LINE_BUF      1024
#define BMS_MAX_ARGS      32

static bms_log_level     g_min_level = BMS_LOG_INFO;
static bms_log_sink_fn   g_sinks[BMS_MAX_SINKS];
static void             *g_sink_userdata[BMS_MAX_SINKS];
static int               g_sink_count = 0;

static bms_cmd_entry     g_commands[BMS_MAX_COMMANDS];
static int               g_command_count = 0;

static const char *level_name(bms_log_level level) {
    switch (level) {
        case BMS_LOG_TRACE: return "TRACE";
        case BMS_LOG_DEBUG: return "DEBUG";
        case BMS_LOG_INFO:  return "INFO";
        case BMS_LOG_WARN:  return "WARN";
        case BMS_LOG_ERROR: return "ERROR";
        case BMS_LOG_FATAL: return "FATAL";
        default:            return "?????";
    }
}

static void default_stdout_sink(bms_log_level level, const char *tag,
                                 const char *message, void *userdata) {
    (void)userdata;
    time_t now = time(NULL);
    struct tm tm_now;
#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    char timebuf[16];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tm_now);

    FILE *stream = (level >= BMS_LOG_WARN) ? stderr : stdout;
    fprintf(stream, "[%s] %-5s [%s] %s\n", timebuf, level_name(level), tag, message);
}

void bms_console_init(void) {
    g_sink_count = 0;
    g_command_count = 0;
    bms_console_add_sink(default_stdout_sink, NULL);
}

void bms_console_shutdown(void) {
    g_sink_count = 0;
    g_command_count = 0;
}

void bms_console_set_min_level(bms_log_level level) {
    g_min_level = level;
}

void bms_console_add_sink(bms_log_sink_fn sink, void *userdata) {
    if (g_sink_count >= BMS_MAX_SINKS) return;
    g_sinks[g_sink_count] = sink;
    g_sink_userdata[g_sink_count] = userdata;
    g_sink_count++;
}

void bms_log(bms_log_level level, const char *tag, const char *fmt, ...) {
    if (level < g_min_level) return;

    char message[BMS_LINE_BUF];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    for (int i = 0; i < g_sink_count; i++) {
        g_sinks[i](level, tag, message, g_sink_userdata[i]);
    }
}

/* ---- Commands ---------------------------------------------------- */

static int cmd_help(int argc, char **argv, void *userdata);
static int cmd_exit(int argc, char **argv, void *userdata);

void bms_console_register(const char *name, const char *help,
                           bms_cmd_fn fn, void *userdata) {
    if (g_command_count >= BMS_MAX_COMMANDS) {
        BMS_ERROR("console", "command table full, cannot register '%s'", name);
        return;
    }
    g_commands[g_command_count].name = name;
    g_commands[g_command_count].help = help;
    g_commands[g_command_count].fn = fn;
    g_commands[g_command_count].userdata = userdata;
    g_command_count++;
}

static int split_line(char *line, char **argv, int max_args) {
    int argc = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok && argc < max_args) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    return argc;
}

int bms_console_execute(const char *line_in) {
    char line[BMS_LINE_BUF];
    strncpy(line, line_in, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';

    char *argv[BMS_MAX_ARGS];
    int argc = split_line(line, argv, BMS_MAX_ARGS);
    if (argc == 0) return -1;

    for (int i = 0; i < g_command_count; i++) {
        if (strcmp(g_commands[i].name, argv[0]) == 0) {
            return g_commands[i].fn(argc, argv, g_commands[i].userdata);
        }
    }

    BMS_WARN("console", "unknown command '%s' (try 'help')", argv[0]);
    return -1;
}

void bms_console_list_commands(void) {
    for (int i = 0; i < g_command_count; i++) {
        printf("  %-16s %s\n", g_commands[i].name,
               g_commands[i].help ? g_commands[i].help : "");
    }
}

static int cmd_help(int argc, char **argv, void *userdata) {
    (void)argc; (void)argv; (void)userdata;
    printf("BMS console commands:\n");
    bms_console_list_commands();
    return 0;
}

static int cmd_exit(int argc, char **argv, void *userdata) {
    (void)argc; (void)argv; (void)userdata;
    return 1; /* sentinel: repl loop exits on return 1 from "exit"/"quit" */
}

void bms_console_repl(void) {
    static int builtins_registered = 0;
    if (!builtins_registered) {
        bms_console_register("help", "list available commands", cmd_help, NULL);
        bms_console_register("exit", "exit the console", cmd_exit, NULL);
        bms_console_register("quit", "exit the console", cmd_exit, NULL);
        builtins_registered = 1;
    }

    char line[BMS_LINE_BUF];
    printf("bms> ");
    fflush(stdout);
    while (fgets(line, sizeof(line), stdin)) {
        /* strip trailing newline for the exit check below */
        size_t len = strlen(line);
        if (len && (line[len - 1] == '\n')) line[len - 1] = '\0';

        if (line[0] != '\0') {
            char copy[BMS_LINE_BUF];
            strncpy(copy, line, sizeof(copy) - 1);
            copy[sizeof(copy) - 1] = '\0';

            char *argv[BMS_MAX_ARGS];
            char parse_buf[BMS_LINE_BUF];
            strncpy(parse_buf, copy, sizeof(parse_buf) - 1);
            parse_buf[sizeof(parse_buf) - 1] = '\0';
            int argc = split_line(parse_buf, argv, BMS_MAX_ARGS);

            if (argc > 0 && (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0)) {
                break;
            }
            bms_console_execute(copy);
        }
        printf("bms> ");
        fflush(stdout);
    }
}