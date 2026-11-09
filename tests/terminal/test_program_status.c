/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "terminal/program_status.h"

static void expect_sequence(enum program_status_state state, enum program_status_kind kind,
                            const char *msg, int tmux_wrap, const char *expected)
{
    size_t sequence_len;
    char *sequence = program_status_sequence(state, kind, msg, tmux_wrap, &sequence_len);
    EXPECT(sequence != NULL);
    if (!sequence)
        return;

    size_t expected_len = strlen(expected);
    EXPECT(sequence_len == expected_len);
    if (sequence_len == expected_len)
        EXPECT(memcmp(sequence, expected, expected_len) == 0);
    free(sequence);
}

static void test_states(void)
{
    expect_sequence(PROGRAM_STATUS_IDLE, PROGRAM_STATUS_KIND_NONE, NULL, 0,
                    "\x1b]7501;state=idle:app=hax\x07");
    expect_sequence(PROGRAM_STATUS_WORKING, PROGRAM_STATUS_KIND_NONE, NULL, 0,
                    "\x1b]7501;state=working:app=hax\x07");
    expect_sequence(PROGRAM_STATUS_DONE, PROGRAM_STATUS_KIND_NONE, NULL, 0,
                    "\x1b]7501;state=done:app=hax\x07");
    expect_sequence(PROGRAM_STATUS_ERROR, PROGRAM_STATUS_KIND_NONE, NULL, 0,
                    "\x1b]7501;state=error:app=hax\x07");
}

static void test_blocked_with_kind_and_msg(void)
{
    /* base64("enter to continue") */
    expect_sequence(
        PROGRAM_STATUS_BLOCKED, PROGRAM_STATUS_KIND_QUESTION, "enter to continue", 0,
        "\x1b]7501;state=blocked:app=hax:kind=question:msg=ZW50ZXIgdG8gY29udGludWU=\x07");
}

static void test_kind_ignored_outside_blocked(void)
{
    expect_sequence(PROGRAM_STATUS_WORKING, PROGRAM_STATUS_KIND_PERMISSION, NULL, 0,
                    "\x1b]7501;state=working:app=hax\x07");
}

static void test_msg_ends_at_first_control_character(void)
{
    /* base64("abc") */
    expect_sequence(PROGRAM_STATUS_DONE, PROGRAM_STATUS_KIND_NONE, "abc\ndef", 0,
                    "\x1b]7501;state=done:app=hax:msg=YWJj\x07");
    /* A C1 control (U+0085, encoded C2 85) ends the line just like a C0 control. */
    expect_sequence(PROGRAM_STATUS_DONE, PROGRAM_STATUS_KIND_NONE, "abc\xc2\x85xyz", 0,
                    "\x1b]7501;state=done:app=hax:msg=YWJj\x07");
}

static void test_empty_msg_is_omitted(void)
{
    expect_sequence(PROGRAM_STATUS_DONE, PROGRAM_STATUS_KIND_NONE, "", 0,
                    "\x1b]7501;state=done:app=hax\x07");
    expect_sequence(PROGRAM_STATUS_DONE, PROGRAM_STATUS_KIND_NONE, "\n", 0,
                    "\x1b]7501;state=done:app=hax\x07");
}

static void test_msg_cap_keeps_whole_utf8_sequences(void)
{
    /* 159 single-byte characters fill the cap so the 2-byte é no longer fits whole. */
    char msg[256];
    memset(msg, 'a', 159);
    memcpy(msg + 159, "\xc3\xa9", 2);
    msg[161] = '\0';

    size_t sequence_len;
    char *sequence = program_status_sequence(PROGRAM_STATUS_WORKING, PROGRAM_STATUS_KIND_NONE, msg,
                                             0, &sequence_len);
    EXPECT(sequence != NULL);
    if (!sequence)
        return;

    /* base64 of 159 bytes is 53 complete quanta: 212 characters, no padding, no é. */
    const char *prefix = "\x1b]7501;state=working:app=hax:msg=";
    size_t prefix_len = strlen(prefix);
    EXPECT(sequence_len == prefix_len + 212 + 1);
    EXPECT(memcmp(sequence, prefix, prefix_len) == 0);
    EXPECT(sequence[sequence_len - 1] == '\x07');
    EXPECT(strchr(sequence + prefix_len, '=') == NULL);
    free(sequence);
}

static void test_tmux_passthrough(void)
{
    expect_sequence(PROGRAM_STATUS_IDLE, PROGRAM_STATUS_KIND_NONE, NULL, 1,
                    "\x1bPtmux;\x1b\x1b]7501;state=idle:app=hax\x07\x1b\\");
}

int main(void)
{
    test_states();
    test_blocked_with_kind_and_msg();
    test_kind_ignored_outside_blocked();
    test_msg_ends_at_first_control_character();
    test_empty_msg_is_omitted();
    test_msg_cap_keeps_whole_utf8_sequences();
    test_tmux_passthrough();
    T_REPORT();
}
