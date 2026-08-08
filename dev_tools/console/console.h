cpp
#ifndef BMS_CONSOLE_H
#define BMS_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* ---- Log levels ---------------------------------------------------- */
typedef enum {
    BMS_LOG_TRACE = 0,
    BMS_LOG_DEBUG,
    BMS_LOG_INFO,
    BMS_LOG_WARN,
    BMS_LOG_ERROR,
    BMS_LOG_FATAL
} bms_log_level;

/* Called for every log line, after level/formatting is applied.
 * Register a sink to route output to a file, network socket, or
 * an in-browser overlay instead of stdout. */
typedef void (*bms_log_sink_fn)(bms_log_level level, const char *tag,
                                 const char *message, void *userdata);

void bms_console_init(void);
void bms_console_shutdown(void);

void bms_console_set_min_level(bms_log_level level);
void bms_console_add_sink(bms_log_sink_fn sink, void *userdata);

/* printf-style logging */
void bms_log(bms_log_level level, const char *tag, const char *fmt, ...);

#define BMS_TRACE(tag, ...) bms_log(BMS_LOG_TRACE, tag, __VA_ARGS__)
#define BMS_DEBUG(tag, ...) bms_log(BMS_LOG_DEBUG, tag, __VA_ARGS__)
#define BMS_INFO(tag, ...)  bms_log(BMS_LOG_INFO,  tag, __VA_ARGS__)
#define BMS_WARN(tag, ...)  bms_log(BMS_LOG_WARN,  tag, __VA_ARGS__)
#define BMS_ERROR(tag, ...) bms_log(BMS_LOG_ERROR, tag, __VA_ARGS__)
#define BMS_FATAL(tag, ...) bms_log(BMS_LOG_FATAL, tag, __VA_ARGS__)

/* ---- Command registry ------------------------------------------------
 * Console commands are how you drive the engine interactively:
 * e.g. "nav https://example.com", "reload", "dbg break render_frame"
 */
typedef int (*bms_cmd_fn)(int argc, char **argv, void *userdata);

typedef struct {
    const char   *name;
    const char   *help;
    bms_cmd_fn    fn;
    void         *userdata;
} bms_cmd_entry;

void bms_console_register(const char *name, const char *help,
                           bms_cmd_fn fn, void *userdata);

/* Parses a single line ("cmd arg1 arg2 ...") and dispatches it.
 * Returns the command's return code, or -1 if command not found. */
int bms_console_execute(const char *line);

/* Blocking REPL loop reading from stdin. Runs until "exit"/"quit"
 * or EOF. Intended for a dev/debug build, not shipped end-user UI. */
void bms_console_repl(void);

/* List all registered commands (used by built-in "help"). */
void bms_console_list_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* BMS_CONSOLE_H */