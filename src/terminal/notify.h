/* SPDX-License-Identifier: MIT */
#ifndef HAX_TERMINAL_NOTIFY_H
#define HAX_TERMINAL_NOTIFY_H

/* Emit and flush the configured terminal-attention notification on stdout. Auto mode selects OSC 9
 * only for known supporting terminals and otherwise uses BEL. No-op when output is not a terminal,
 * notifications are disabled, or the osc7501 method is selected — there the turn's done report is
 * the notification. */
void notify_attention(void);

/* Whether OSC 7501 program status reports are enabled: an explicit osc7501 selects them, auto
 * keeps them on any terminal (unsupported terminals ignore the sequence), and bel/osc9/off
 * decline. Reports go to stderr, so stderr decides what counts as a terminal. */
int notify_program_status_enabled(void);

#endif /* HAX_TERMINAL_NOTIFY_H */
