/* SPDX-License-Identifier: MIT */
#ifndef HAX_TERMINAL_PROGRAM_STATUS_H
#define HAX_TERMINAL_PROGRAM_STATUS_H

#include <stddef.h>

/* OSC 7501 program status reports: hax tells the terminal what it is doing and the terminal
 * keeps one record per program, so it can indicate a finished turn or one waiting on the user
 * outside the focused window. Unsupported terminals ignore the sequence. */

enum program_status_state {
    PROGRAM_STATUS_IDLE,
    PROGRAM_STATUS_WORKING,
    PROGRAM_STATUS_DONE,
    PROGRAM_STATUS_BLOCKED,
    PROGRAM_STATUS_ERROR,
};

/* Why a blocked report is blocked; meaningful only with PROGRAM_STATUS_BLOCKED. */
enum program_status_kind {
    PROGRAM_STATUS_KIND_NONE,
    PROGRAM_STATUS_KIND_PERMISSION,
    PROGRAM_STATUS_KIND_QUESTION,
    PROGRAM_STATUS_KIND_AUTH,
};

/* Build one OSC 7501 report for the root record (app=hax), optionally wrapped for tmux
 * passthrough. Each report replaces the record, so app is repeated every time. msg is a
 * single line of UTF-8 context, truncated at the first control character and at a byte cap.
 * Returns a newly allocated, NUL-terminated sequence. */
char *program_status_sequence(enum program_status_state state, enum program_status_kind kind,
                              const char *msg, int tmux_wrap, size_t *out_len);

/* Emit a report on stderr when the program_status setting allows it: on always, auto only
 * when stderr is a terminal, off never. stderr keeps reports out of piped stdout and the
 * --json stream; an interactive session has both on the same terminal. */
void program_status_report(enum program_status_state state, enum program_status_kind kind,
                           const char *msg);

#endif /* HAX_TERMINAL_PROGRAM_STATUS_H */
