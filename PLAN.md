# Plan: A userspace TCP/UDP stack in C++20

## Context

You want to build a networking stack from the bottom up to genuinely understand how packets become connections. Working directory is empty — this is greenfield.

Decisions locked in from the clarification round:
- **Depth**: Full stack on a Linux **TUN** device — Ethernet/ARP not strictly required because TUN delivers L3 frames (IP packets), but we'll model the L2 boundary cleanly. *(If you switch to TAP later, the same architecture lets you add an Ethernet/ARP layer without touching the rest.)*
- **Success criterion**: Real interop with the Linux kernel. `ping`, `nc -u`, and `curl` from the host must talk to your stack across a TUN interface with its own IP.
- **TCP scope**: Core TCP — RFC 9293 state machine, RFC 6298 RTO, cumulative ACKs, sliding window, MSS, FIN/RST. Defer SACK, window scaling, timestamps, congestion control.
- **Tooling**: CMake + GoogleTest, C++20, no other deps.

The canonical reference for this exact project is Saminiir's *"Let's code a TCP/IP stack"* series and the RFCs. Keep both open while building.

---

## Architecture (SOLID-aligned)

Layered pipeline, each layer behind a thin interface. Frames flow up; segments flow down. The dispatcher at each layer is **open for extension** — adding ICMP or a new transport doesn't modify existing code.

```
┌────────────────────── Application (echo server, tiny HTTP) ──────────────────────┐
│ Socket API (BSD-ish: bind/listen/accept/connect/send/recv/close)                 │
├──────────────────────────────────────────────────────────────────────────────────┤
│ Transport: UDP                  TCP (state machine + TCB + reassembly + RTO)     │
├──────────────────────────────────────────────────────────────────────────────────┤
│ Network:   IPv4 (fragmentation, checksum, routing-of-one)    ICMP (echo only)    │
├──────────────────────────────────────────────────────────────────────────────────┤
│ Link:      TunDevice (read/write raw IP packets on /dev/net/tun)                 │
└──────────────────────────────────────────────────────────────────────────────────┘
                          ▲ event loop drives ingress; timers drive egress
```

**SOLID mapping:**
- **SRP** — one class per protocol concern. `Ipv4Parser`, `Ipv4Router`, `TcpStateMachine`, `RetransmissionQueue`, `ReassemblyBuffer`, `Checksum` are separate.
- **OCP** — `INetworkProtocol` and `ITransportProtocol` interfaces. A `Demuxer` dispatches by IP proto / (proto,port) tuple. Adding ICMP or a future SCTP doesn't touch existing layers.
- **LSP** — `ISocket` is the polymorphic surface used by the app; `UdpSocket` and `TcpSocket` are substitutable.
- **ISP** — split read-side (`IPacketSink`) and write-side (`IPacketSource`) interfaces; layers depend only on the half they need.
- **DIP** — every layer takes its lower neighbor as an interface in the constructor (`Tcp` takes `INetworkLayer&`, not concrete `Ipv4`). Makes unit testing with fakes trivial.

---

## Repository layout

```
tcp-udp/
├── CMakeLists.txt
├── src/
│   ├── common/            # Buffer, ByteView, Endian, Checksum, Logger, Result<T>
│   ├── link/              # TunDevice (RAII over /dev/net/tun)
│   ├── net/               # Ipv4Header, Ipv4Layer, Icmp
│   ├── transport/
│   │   ├── udp/           # UdpHeader, UdpLayer, UdpSocket
│   │   └── tcp/           # TcpHeader, Tcb, TcpStateMachine, RetxQueue,
│   │                      # ReassemblyBuffer, TcpLayer, TcpSocket
│   ├── core/              # Demuxer, EventLoop, Timer, Scheduler, Stack (composition root)
│   └── app/               # echo_udp, echo_tcp, tiny_http (sample binaries)
├── tests/                 # gtest unit + integration
└── scripts/               # tun_setup.sh, run_with_tun.sh
```

---

## Build phases (stop-and-test after each)

### Phase 0 — Scaffolding (½ day)
- `CMakeLists.txt` with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined` for debug.
- `common/Buffer.h` — owning byte buffer + `ByteView` (non-owning span). All parsers consume `ByteView`.
- `common/Checksum.h` — RFC 1071 one's-complement sum (single function reused by IP/UDP/TCP/ICMP).
- `common/Endian.h` — `hton16/32`, `ntoh16/32` constexpr helpers.
- GoogleTest wired up; one trivial test green.

### Phase 1 — TUN device + IPv4 echo loopback (1 day)
- `scripts/tun_setup.sh` — creates `tun0`, assigns `10.0.0.1/24` to host, marks the *peer* IP `10.0.0.2` as ours. Needs `CAP_NET_ADMIN` (run with `sudo` or set caps on the binary).
- `link/TunDevice` — RAII wrapper around `open("/dev/net/tun")` + `ioctl(TUNSETIFF, IFF_TUN | IFF_NO_PI)`. Blocking `read`/`write` of raw IP packets.
- `net/Ipv4Header` — parse + serialize, validate version/IHL/total length/header checksum.
- **Milestone**: `ping 10.0.0.2` from host — your stack parses ICMP Echo Request and writes a hand-crafted Echo Reply back. Verify with `tcpdump -i tun0 -n`.

### Phase 2 — Event loop + demuxer (½ day)
- `core/EventLoop` — `epoll`-based; owns the TUN fd and a `timerfd` for the timer wheel.
- `core/Demuxer` — `register(protocol_id, INetworkProtocol*)`. IPv4 layer calls into it on each inbound packet.
- `core/Stack` is the composition root: builds TUN, IPv4, ICMP, UDP, TCP, wires them together. **Nothing else news up dependencies** — DIP enforced by construction.
- ICMP becomes a real `INetworkProtocol` instead of a hardcoded reply.

### Phase 3 — UDP (½ day)
- `transport/udp/UdpHeader` — parse/serialize, **pseudo-header checksum** (this is where most people get the checksum wrong; write the test first).
- `UdpLayer` registered with the demuxer for IP proto 17.
- `UdpSocket` with a fixed-size receive queue keyed on `(local_port, remote_addr, remote_port)`.
- **Milestone**: `nc -u 10.0.0.2 7777` reaches your echo_udp app and echoes back.

### Phase 4 — TCP, in slices (the bulk of the work — ~1–2 weeks)

Build TCP in **strictly ordered slices**. Each slice ends with a passing integration test before the next begins.

1. **Header + checksum**. `TcpHeader`, options parser (just MSS — others ignored on read, never sent). Pseudo-header checksum.
2. **TCB + state enum**. `Tcb` owns `snd.una/nxt/wnd/iss`, `rcv.nxt/wnd/irs`, peer addr/port, MSS, current `TcpState`. State enum exactly matches RFC 9293 §3.3.2.
3. **Passive open / 3-way handshake (server side)**. `LISTEN → SYN_RECEIVED → ESTABLISHED`. Generate ISS from a clock-keyed hash (RFC 6528-ish — doesn't have to be cryptographic for a learning project, but don't use 0). Test: `nc 10.0.0.2 8080` connects.
4. **Receive path**: in-order data → app, generate ACK. `ReassemblyBuffer` keyed by sequence number for out-of-order segments. Drop segments outside the receive window per RFC 9293 §3.10.7.4.
5. **Send path + sliding window**. Respect peer's advertised window; segment outgoing data at MSS. `RetransmissionQueue` holds unacked segments.
6. **RTO (RFC 6298)**. Karn's algorithm; SRTT/RTTVAR; exponential backoff; min 1s (you may lower to 200ms locally for faster tests, but document it). Single retransmission timer per TCB driven by the timer wheel.
7. **Active open (client side)**. `SYN_SENT → ESTABLISHED`. Test: your stack `connect()`s to a host nginx.
8. **Teardown**. `FIN_WAIT_1/2`, `CLOSE_WAIT`, `LAST_ACK`, `TIME_WAIT` (with 2×MSL timer; shorten for tests). RST handling on every state per RFC.
9. **Edge cases**: zero-window probes, simultaneous open/close, ACK of unsent data → RST, retransmission of SYN and FIN.

**Explicitly out of scope** (write these as `// not implemented` with a one-line reason): SACK, window scaling, timestamps, Nagle, delayed ACK tuning beyond a simple 200ms timer, PMTUD, congestion control. Leave clean extension points (e.g. an `ICongestionControl` interface that currently only has a `NoCongestionControl` impl).

### Phase 5 — Sample apps + interop tests (1–2 days)
- `app/echo_tcp` — accept loop, echoes lines.
- `app/tiny_http` — single-threaded HTTP/1.0, `Content-Length` only, serves a static string. **Milestone**: `curl http://10.0.0.2:8080/` returns 200 OK.
- Integration tests in `tests/integration/` that spawn the stack binary, wait for TUN up, then drive it with `curl`/`nc` and assert.

---

## Critical files (to be created)

- `src/link/TunDevice.{h,cpp}` — TUN open + ioctl. Phase 1.
- `src/net/Ipv4Layer.{h,cpp}` + `Ipv4Header.{h,cpp}` — Phase 1.
- `src/net/Icmp.{h,cpp}` — Phase 1/2.
- `src/core/EventLoop.{h,cpp}`, `Demuxer.{h,cpp}`, `Stack.{h,cpp}` — Phase 2.
- `src/transport/udp/UdpLayer.{h,cpp}`, `UdpSocket.{h,cpp}` — Phase 3.
- `src/transport/tcp/TcpStateMachine.{h,cpp}`, `Tcb.h`, `RetransmissionQueue.{h,cpp}`, `ReassemblyBuffer.{h,cpp}`, `TcpLayer.{h,cpp}`, `TcpSocket.{h,cpp}` — Phase 4.
- `src/common/Checksum.h` — reused by every protocol layer; **write its tests first**.
- `scripts/tun_setup.sh` — interface bring-up; needed before any binary runs.

---

## Things that will bite you (pre-warned)

1. **Pseudo-header checksum**. UDP and TCP checksums cover a synthetic IPv4 pseudo-header (src, dst, zero, proto, length). Get this wrong and the kernel silently drops every packet. Test with known vectors from RFC 1071 / Wireshark captures.
2. **Endianness**. Every multi-byte header field is network byte order. Centralize in `Endian.h`; never write `htons` ad hoc.
3. **TUN vs TAP framing**. `IFF_NO_PI` removes the 4-byte protocol prefix — use it; otherwise your IPv4 parser sees garbage.
4. **Capabilities**. Either `sudo` the binary or `setcap cap_net_admin+ep ./stack` once.
5. **Sequence number arithmetic is modular.** Use `int32_t` differences for comparisons (`(int32_t)(a - b) < 0`) — RFC 9293 §3.4.
6. **Timer correctness**. One timer wheel driven by `timerfd`, not `sleep` and not per-connection threads. The whole stack is single-threaded by design — far easier to reason about; revisit only after it works.
7. **Don't skip ASan/UBSan**. Buffer parsing in C++ is exactly where they earn their keep.

---

## Verification

End-to-end checks, run after the phase that introduces each:

| Phase | Command (host side)                       | Expected                              |
|-------|-------------------------------------------|---------------------------------------|
| 1     | `ping -c 3 10.0.0.2`                      | 0% loss, ~0.x ms RTT                  |
| 2     | `tcpdump -i tun0 icmp` while pinging      | Echo request + your reply visible     |
| 3     | `echo hi \| nc -u -w1 10.0.0.2 7777`      | `hi` echoed back                      |
| 4.3   | `nc -v 10.0.0.2 8080` (just connect)      | 3-way handshake completes in tcpdump  |
| 4.6   | `nc 10.0.0.2 8080` then drop packets via `iptables -A OUTPUT -p tcp --dport 8080 -j DROP` briefly | retransmissions visible, recovers |
| 5     | `curl -v http://10.0.0.2:8080/`           | 200 OK with body                      |

Unit tests (gtest) per layer:
- `ChecksumTest` — RFC 1071 vectors, pseudo-header vectors.
- `Ipv4HeaderTest` — round-trip serialize/parse, bad-IHL rejection, fragmentation flags.
- `TcpStateMachineTest` — drive every RFC 9293 transition with synthetic segments, no real socket. This is the highest-ROI test file in the project.
- `RetransmissionQueueTest`, `ReassemblyBufferTest` — pure unit, no I/O.

Run the whole suite with `ctest --output-on-failure` and the binaries under `valgrind` / ASan once per phase.

---

## Estimated effort

Roughly **2–4 weeks of focused work**, dominated by Phase 4. The phases before TCP are short and high-feedback — you'll have working ICMP and UDP within a couple of days, which is motivating before the TCP grind.

