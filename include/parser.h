#ifndef PARSER_H
#define PARSER_H

#include "types.h"

ParseStatus parse_command(const char *input, Command *command);
ParseStatus parse_insert(const char *input, Command *command);
ParseStatus parse_select(const char *input, Command *command);
ParseStatus parse_condition(const char *input, Condition *condition);
void init_command(Command *command);
void free_command(Command *command);

#endif
