#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "types.h"

ExecStatus execute_command(const Command *command);
ExecStatus execute_insert(const InsertCommand *insert_cmd);
ExecStatus execute_select(const SelectCommand *select_cmd);

#endif
