/*
 * td_proto_cmds.h - the `mqtt`, `modbus` and `bt` shell commands.
 */
#ifndef TD_PROTO_CMDS_H
#define TD_PROTO_CMDS_H

/* Register the commands with TinyDesk Shell (after its init, before sessions
 * start). Returns 0 on success. */
int td_proto_register_shell_commands(void);

/* Optional: how a repeating command (modbus read -i) notices Ctrl+C. The
 * function returns true once per Ctrl+C typed. Without it such commands
 * only stop after their count. */
#include <stdbool.h>
void td_proto_set_break_check(bool (*fn)(void));

#endif
