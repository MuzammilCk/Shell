#ifndef BUILTINS_H
#define BUILTINS_H

#include "parser.h"

int is_builtin(command_t *c);

int handle_builtin(command_t *c);

void add_history(const char *line);

void free_history(void);

int get_history_length(void);
const char *get_history_item(int index);

#endif
