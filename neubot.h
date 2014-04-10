/*
 * LibNeubot interface - Public domain.
 * WARNING: Autogenerated file - do not edit!
 */

#ifndef NEUBOT_H
# define NEUBOT_H 65537

#ifdef __cplusplus
extern "C" {
#endif

/* Hooks and slots: */

typedef void (*neubot_hook_vo)(void *);
typedef void (*neubot_hook_vos)(void *, const char *);

typedef void (*neubot_slot_vo)(void *);
typedef void (*neubot_slot_vos)(void *, const char *);

/* Classes: */

struct NeubotEchoServer;
struct NeubotPollable;
struct NeubotPoller;

/* NeubotEchoServer API: */

struct NeubotEchoServer *NeubotEchoServer_construct(struct NeubotPoller *, 
    int, const char *, const char *);

/* NeubotPollable API: */

struct NeubotPollable *NeubotPollable_construct(neubot_slot_vo, 
    neubot_slot_vo, neubot_slot_vo, void *);

int NeubotPollable_attach(struct NeubotPollable *, struct NeubotPoller *, 
    long long);

void NeubotPollable_detach(struct NeubotPollable *);

long long NeubotPollable_fileno(struct NeubotPollable *);

int NeubotPollable_set_readable(struct NeubotPollable *);

int NeubotPollable_unset_readable(struct NeubotPollable *);

int NeubotPollable_set_writable(struct NeubotPollable *);

int NeubotPollable_unset_writable(struct NeubotPollable *);

void NeubotPollable_set_timeout(struct NeubotPollable *, double);

void NeubotPollable_clear_timeout(struct NeubotPollable *);

void NeubotPollable_close(struct NeubotPollable *);

/* NeubotPoller API: */

struct NeubotPoller *NeubotPoller_construct(void);

int NeubotPoller_sched(struct NeubotPoller *, double, neubot_hook_vo, void *);

int NeubotPoller_defer_read(struct NeubotPoller *, long long, neubot_hook_vo, 
    neubot_hook_vo, void *, double);

int NeubotPoller_defer_write(struct NeubotPoller *, long long, neubot_hook_vo, 
    neubot_hook_vo, void *, double);

int NeubotPoller_resolve(struct NeubotPoller *, int, const char *, 
    neubot_hook_vos, void *);

void NeubotPoller_loop(struct NeubotPoller *);

void NeubotPoller_break_loop(struct NeubotPoller *);

#ifdef __cplusplus
}
#endif

#endif  /* NEUBOT_H */
