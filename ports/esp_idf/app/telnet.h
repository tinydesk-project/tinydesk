/*
 * telnet.h - remote desktop over Telnet (port 23), with TinyDesk Shell login.
 * Everything runs in the UI task: call telnet_poll() from the main loop.
 */
#ifndef TELNET_H
#define TELNET_H

#include <stdbool.h>
#include <stdint.h>

#define TELNET_CLIENTS 2

    /* Read the on/off setting from NVS (default off; password setup required). */
void telnet_start(void);

    /* Accept a client, run its login, notice when it leaves. Non-blocking. */
void telnet_poll(void);

    /* True while a logged-in client owns the desktop. */
bool telnet_active(void);

    /* Non-blocking input from the client; -1 when nothing is waiting. */
int telnet_read_byte(void);

    /* Send to the client; returns bytes sent (short on a stalled link). */
int telnet_write(const uint8_t *buf, int len);

    /* On/off switch (saved in NVS) and the client's address while active. */
bool telnet_enabled(void);
void telnet_set_enabled(bool on);
const char *telnet_peer(void);

    /* The account the client logged in with (while active). */
const char *telnet_user(void);

    /* Client i (0 or 1) of those logged in at once: the same as the calls
 * above (which are client 0's). telnet_set_clients(n) allows n of them
 * (1 or 2; a client beyond n is disconnected). */
bool telnet_active_n(int i);
int telnet_read_byte_n(int i);
int telnet_write_n(int i, const uint8_t *buf, int len);
const char *telnet_peer_n(int i);
const char *telnet_user_n(int i);
void telnet_set_clients(int n);

    /* What a logged-in client gets (saved in NVS, "telnet_mode"): the desktop
 * moves to it (TD_TELNET_ONLY, the default), or it is a second screen
 * beside the board's own terminal showing the same desktop
 * (TD_TELNET_DUPLICATE) or extending it (TD_TELNET_EXTEND). */
int telnet_mode(void);
void telnet_set_mode(int mode);

#endif
