/*
 * editor_py.h - Python support for the Editor: syntax colours and
 * completion suggestions. Internal to the Editor (apps/editor.c).
 */
#ifndef TD_EDITOR_PY_H
#define TD_EDITOR_PY_H

#include <stdint.h>

/* Kinds of text, one per byte of a line. */
enum
{
    PYED_TEXT,
    PYED_KEYWORD,
    PYED_BUILTIN,
    PYED_STRING,
    PYED_COMMENT,
    PYED_NUMBER,
    PYED_NAME,        /* the name after def or class */
    PYED_DECORATOR,
    PYED_KINDS
};

/* Classify the bytes of one line s[0..n) into cls[0..cap) (bytes past cap
 * are not stored; cls may be NULL). `state` is 0, or the open triple-quoted
 * string an earlier line left (the return value of the call for it).
 * Returns the state after this line. */
int pyed_lex_line(const char *s, int n, int state, uint8_t *cls, int cap);

/* A suggestion: n bytes at s (not NUL-terminated; may point into the
 * text, so copy it before changing the text). */
typedef struct
{
    const char *s;
    int n;
} pyed_word_t;

/* Suggestions for the word that ends at position cur of text[0..len):
 * after "module." that module's names, otherwise keywords, built-ins,
 * module names and the words already in the text that start with it.
 * `min_prefix` is the shortest word that gets suggestions (0 shows a
 * module's names right after the dot). *start is where the word starts.
 * Returns the number of suggestions (at most max), sorted. */
int pyed_complete(const char *text, int len, int cur, int min_prefix, pyed_word_t *out, int max, int *start);

#endif /* TD_EDITOR_PY_H */
