#ifndef REPL_H
#define REPL_H

int run_repl(void);
int process_input_line(const char *line);
int is_exit_command(const char *line);

#endif
