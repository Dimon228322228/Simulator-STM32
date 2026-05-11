#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cube_script.h"

#define MAX_LINE_LENGTH 256

static char *trim(char *line) {
    while (*line && isspace((unsigned char)*line)) line++;
    char *end = line + strlen(line);
    while (end > line && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';
    return line;
}

static CubeScriptCommand make_reg_cmd(uint32_t addr, uint32_t value) {
    CubeScriptCommand cmd;
    cmd.type = CUBE_CMD_WRITE_REG;
    cmd.payload.write_reg.addr = addr;
    cmd.payload.write_reg.value = value;
    cmd.repeat_count = 1;
    return cmd;
}

static CubeScriptCommand make_delay_cmd(uint32_t cycles) {
    CubeScriptCommand cmd;
    cmd.type = CUBE_CMD_DELAY;
    cmd.payload.delay.cycles = cycles;
    cmd.repeat_count = 1;
    return cmd;
}

static CubeScriptCommand make_log_cmd(const char *msg) {
    CubeScriptCommand cmd;
    cmd.type = CUBE_CMD_LOG;
    memset(cmd.payload.log.message, 0, sizeof(cmd.payload.log.message));
    strncpy(cmd.payload.log.message, msg, sizeof(cmd.payload.log.message) - 1);
    cmd.repeat_count = 1;
    return cmd;
}

static CubeScriptCommand make_irq_cmd(const char *vector_label) {
    CubeScriptCommand cmd;
    cmd.type = CUBE_CMD_DEFINE_IRQ;
    memset(cmd.payload.define_irq.label, 0, sizeof(cmd.payload.define_irq.label));
    strncpy(cmd.payload.define_irq.label, vector_label,
            sizeof(cmd.payload.define_irq.label) - 1);
    cmd.repeat_count = 1;
    return cmd;
}

static CubeScriptCommand make_run_cmd(const char *entry_label) {
    CubeScriptCommand cmd;
    cmd.type = CUBE_CMD_RUN;
    memset(cmd.payload.run.entry_label, 0, sizeof(cmd.payload.run.entry_label));
    strncpy(cmd.payload.run.entry_label, entry_label,
            sizeof(cmd.payload.run.entry_label) - 1);
    cmd.repeat_count = 1;
    return cmd;
}

static void add_command(CubeScript *script, CubeScriptCommand cmd) {
    if (script->count >= CUBE_SCRIPT_MAX_COMMANDS) {
        script->error = CUBE_SCRIPT_ERR_TOO_MANY_COMMANDS;
        return;
    }
    script->commands[script->count++] = cmd;
}

static int parse_hex32(const char *text, uint32_t *value) {
    char *endptr = NULL;
    *value = (uint32_t)strtoul(text, &endptr, 0);
    if (endptr == text || *endptr != '\0') {
        return 0;
    }
    return 1;
}

static void parse_write(CubeScript *script, char **tokens, int count) {
    if (count < 3) {
        script->error = CUBE_SCRIPT_ERR_SYNTAX;
        return;
    }
    uint32_t addr = 0;
    uint32_t value = 0;
    uint32_t repeat = 1;
    if (!parse_hex32(tokens[1], &addr) || !parse_hex32(tokens[2], &value)) {
        script->error = CUBE_SCRIPT_ERR_SYNTAX;
        return;
    }
    if (count >= 4 && !parse_hex32(tokens[3], &repeat)) {
        script->error = CUBE_SCRIPT_ERR_SYNTAX;
        return;
    }
    CubeScriptCommand cmd = make_reg_cmd(addr, value);
    cmd.repeat_count = repeat;
    add_command(script, cmd);
}

static void parse_delay(CubeScript *script, char **tokens, int count) {
    if (count < 2) {
        script->error = CUBE_SCRIPT_ERR_SYNTAX;
        return;
    }
    uint32_t cycles = 0;
    uint32_t repeat = 1;
    if (!parse_hex32(tokens[1], &cycles)) {
        script->error = CUBE_SCRIPT_ERR_SYNTAX;
        return;
    }
    if (count >= 3 && !parse_hex32(tokens[2], &repeat)) {
        script->error = CUBE_SCRIPT_ERR_SYNTAX;
        return;
    }
    CubeScriptCommand cmd = make_delay_cmd(cycles);
    cmd.repeat_count = repeat;
    add_command(script, cmd);
}

static void parse_log(CubeScript *script, char *line) {
    char *msg = line;
    if (line[0] == 'L' && line[1] == 'O' && line[2] == 'G' && line[3] == ' ') {
        msg += 4;
    }
    msg = trim(msg);
    CubeScriptCommand cmd = make_log_cmd(msg);
    add_command(script, cmd);
}

static void parse_irq(CubeScript *script, char **tokens, int count) {
    if (count < 2) {
        script->error = CUBE_SCRIPT_ERR_SYNTAX;
        return;
    }
    CubeScriptCommand cmd = make_irq_cmd(tokens[1]);
    add_command(script, cmd);
}

static void parse_run(CubeScript *script, char **tokens, int count) {
    if (count < 2) {
        script->error = CUBE_SCRIPT_ERR_SYNTAX;
        return;
    }
    CubeScriptCommand cmd = make_run_cmd(tokens[1]);
    add_command(script, cmd);
}

void cube_script_init(CubeScript *script) {
    memset(script, 0, sizeof(*script));
}

int cube_script_parse_file(const char *path, CubeScript *script) {
    cube_script_init(script);
    FILE *file = fopen(path, "r");
    if (!file) {
        script->error = CUBE_SCRIPT_ERR_IO;
        return 0;
    }

    char linebuf[MAX_LINE_LENGTH];
    int line_num = 0;
    while (fgets(linebuf, sizeof(linebuf), file)) {
        line_num++;
        char *line = trim(linebuf);
        if (*line == '\0' || *line == '#') {
            continue;
        }

        char *tokens[10];
        int token_count = 0;
        char *token = strtok(line, " \t");
        while (token && token_count < 10) {
            tokens[token_count++] = token;
            token = strtok(NULL, " \t");
        }

        if (token_count == 0) {
            continue;
        }

        const char *keyword = tokens[0];
        if (strcmp(keyword, "WRITE") == 0) {
            parse_write(script, tokens, token_count);
        } else if (strcmp(keyword, "DELAY") == 0) {
            parse_delay(script, tokens, token_count);
        } else if (strcmp(keyword, "LOG") == 0) {
            parse_log(script, line);
        } else if (strcmp(keyword, "IRQ") == 0) {
            parse_irq(script, tokens, token_count);
        } else if (strcmp(keyword, "RUN") == 0) {
            parse_run(script, tokens, token_count);
        } else {
            script->error = CUBE_SCRIPT_ERR_UNKNOWN_CMD;
        }

        if (script->error != CUBE_SCRIPT_ERR_OK) {
            fprintf(stderr, "[CUBE] Syntax error at line %d\n", line_num);
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    return 1;
}

int cube_script_execute(const CubeScript *script) {
    (void)script;
    return 1;
}