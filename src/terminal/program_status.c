/* SPDX-License-Identifier: MIT */
#include "terminal/program_status.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buf.h"
#include "config.h"
#include "terminal/ansi.h"
#include "text/base64.h"
#include "text/utf8.h"

/* One line of context, well under the protocol's 2048-byte decoded msg limit. */
#define PROGRAM_STATUS_MSG_MAX 160

#define OSC7501_PREFIX      ANSI_ESC "]7501;state="
#define OSC7501_SUFFIX      ANSI_BEL
#define TMUX_OSC7501_PREFIX ANSI_TMUX_PASSTHROUGH_BEGIN OSC7501_PREFIX
#define TMUX_OSC7501_SUFFIX OSC7501_SUFFIX ANSI_TMUX_PASSTHROUGH_END

static const char *state_name(enum program_status_state state)
{
    switch (state) {
    case PROGRAM_STATUS_IDLE:
        return "idle";
    case PROGRAM_STATUS_WORKING:
        return "working";
    case PROGRAM_STATUS_DONE:
        return "done";
    case PROGRAM_STATUS_BLOCKED:
        return "blocked";
    case PROGRAM_STATUS_ERROR:
        return "error";
    }
    return "idle";
}

static const char *kind_name(enum program_status_kind kind)
{
    switch (kind) {
    case PROGRAM_STATUS_KIND_PERMISSION:
        return "permission";
    case PROGRAM_STATUS_KIND_QUESTION:
        return "question";
    case PROGRAM_STATUS_KIND_AUTH:
        return "auth";
    case PROGRAM_STATUS_KIND_NONE:
        break;
    }
    return NULL;
}

/* The protocol discards a report whose decoded msg holds a control character, so the line
 * ends at the first one; the byte cap never splits a multi-byte UTF-8 sequence. */
static size_t msg_length(const char *msg)
{
    size_t len = 0;
    while (msg[len] && len < PROGRAM_STATUS_MSG_MAX) {
        unsigned char byte = (unsigned char)msg[len];
        if (byte < 0x20 || byte == 0x7f)
            break;
        /* C1 controls encode as C2 80..9F. */
        if (byte == 0xc2 && (unsigned char)msg[len + 1] >= 0x80 &&
            (unsigned char)msg[len + 1] <= 0x9f)
            break;
        size_t sequence = utf8_sequence_length(byte);
        if (len + sequence > PROGRAM_STATUS_MSG_MAX)
            break;
        len += sequence;
    }
    return len;
}

char *program_status_sequence(enum program_status_state state, enum program_status_kind kind,
                              const char *msg, int tmux_wrap, size_t *out_len)
{
    struct buf sequence;
    buf_init(&sequence);
    buf_append_str(&sequence, tmux_wrap ? TMUX_OSC7501_PREFIX : OSC7501_PREFIX);
    buf_append_str(&sequence, state_name(state));
    buf_append_str(&sequence, ":app=hax");
    /* kind is defined for blocked alone; elsewhere it would be ignored anyway. */
    const char *kind_str = state == PROGRAM_STATUS_BLOCKED ? kind_name(kind) : NULL;
    if (kind_str) {
        buf_append_str(&sequence, ":kind=");
        buf_append_str(&sequence, kind_str);
    }
    if (msg) {
        size_t len = msg_length(msg);
        if (len > 0) {
            char *encoded = base64_encode(msg, len, NULL);
            buf_append_str(&sequence, ":msg=");
            buf_append_str(&sequence, encoded);
            free(encoded);
        }
    }
    buf_append_str(&sequence, tmux_wrap ? TMUX_OSC7501_SUFFIX : OSC7501_SUFFIX);

    if (out_len)
        *out_len = sequence.len;
    return buf_steal(&sequence);
}

void program_status_report(enum program_status_state state, enum program_status_kind kind,
                           const char *msg)
{
    int setting = config_tristate("program_status");
    if (setting == 0)
        return;
    if (setting < 0) {
        if (!isatty(STDERR_FILENO))
            return;
        const char *term = getenv("TERM");
        if (term && strcmp(term, "dumb") == 0)
            return;
    }

    size_t sequence_len;
    char *sequence =
        program_status_sequence(state, kind, msg, getenv("TMUX") != NULL, &sequence_len);
    fwrite(sequence, 1, sequence_len, stderr);
    fflush(stderr);
    free(sequence);
}
