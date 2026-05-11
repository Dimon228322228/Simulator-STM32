#ifndef CUBE_SCRIPT_H
#define CUBE_SCRIPT_H

#include <stdint.h>

#define CUBE_SCRIPT_MAX_COMMANDS 512
#define CUBE_SCRIPT_MAX_LOG_MSG 128
#define CUBE_SCRIPT_MAX_LABEL 64

typedef enum {
    CUBE_CMD_WRITE_REG,
    CUBE_CMD_DELAY,
    CUBE_CMD_LOG,
    CUBE_CMD_DEFINE_IRQ,
    CUBE_CMD_RUN
} CubeCommandType;

typedef struct {
    CubeCommandType type;
    union {
        struct {
            uint32_t addr;
            uint32_t value;
        } write_reg;
        struct {
            uint32_t cycles;
        } delay;
        struct {
            char message[CUBE_SCRIPT_MAX_LOG_MSG];
        } log;
        struct {
            char label[CUBE_SCRIPT_MAX_LABEL];
        } define_irq;
        struct {
            char entry_label[CUBE_SCRIPT_MAX_LABEL];
        } run;
    } payload;
    uint32_t repeat_count;
} CubeScriptCommand;

typedef enum {
    CUBE_SCRIPT_ERR_OK = 0,
    CUBE_SCRIPT_ERR_IO,
    CUBE_SCRIPT_ERR_SYNTAX,
    CUBE_SCRIPT_ERR_UNKNOWN_CMD,
    CUBE_SCRIPT_ERR_TOO_MANY_COMMANDS
} CubeScriptError;

typedef struct {
    CubeScriptCommand commands[CUBE_SCRIPT_MAX_COMMANDS];
    int count;
    CubeScriptError error;
} CubeScript;

void cube_script_init(CubeScript *script);
int cube_script_parse_file(const char *path, CubeScript *script);
int cube_script_execute(const CubeScript *script);

#endif // CUBE_SCRIPT_H
