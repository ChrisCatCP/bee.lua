#pragma once

#include <bee/utility/bitmask.h>
#include <bee/utility/span.h>

#include <cstdint>

namespace bee::net {
    enum class bpoll_event : uint32_t {
        null    = 0,
        in      = (1U << 0),   // EPOLLIN
        pri     = (1U << 1),   // EPOLLPRI
        out     = (1U << 2),   // EPOLLOUT
        err     = (1U << 3),   // EPOLLERR
        hup     = (1U << 4),   // EPOLLHUP
        rdnorm  = (1U << 6),   // EPOLLRDNORM
        rdband  = (1U << 7),   // EPOLLRDBAND
        wrnorm  = (1U << 8),   // EPOLLWRNORM
        wrand   = (1U << 9),   // EPOLLWRBAND
        msg     = (1U << 10),  // EPOLLMSG
        rdhup   = (1U << 13),  // EPOLLRDHUP
        oneshot = (1U << 30),  // EPOLLONESHOT
#if !defined(_WIN32)
        et = (1U << 31),  // EPOLLET
#endif
    };
    BEE_BITMASK_OPERATORS(bpoll_event)

#if defined(_WIN32)
    using bpoll_handle = void*;
    using bpoll_socket = uintptr_t;
#else
    using bpoll_handle = int;
    using bpoll_socket = int;
#endif

    union bpoll_data_t {
        void* ptr;
        int fd;
        uint32_t u32;
        uint64_t u64;
        bpoll_socket sock;
        bpoll_handle hnd;
    };

#pragma pack(push, 1)
    struct bpoll_event_t {
        bpoll_event events;
        bpoll_data_t data;
    };
#pragma pack(pop)

    bpoll_handle bpoll_create();
    bool bpoll_close(bpoll_handle hnd);
    bool bpoll_ctl_add(bpoll_handle hnd, bpoll_socket socket, const bpoll_event_t& event);
    bool bpoll_ctl_mod(bpoll_handle hnd, bpoll_socket socket, const bpoll_event_t& event);
    bool bpoll_ctl_del(bpoll_handle hnd, bpoll_socket socket);
    int bpoll_wait(bpoll_handle hnd, const span<bpoll_event_t>& events, int timeout);
}
