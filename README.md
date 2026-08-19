# clockless-mutex

A small C++ exploration of Leslie Lamport's "Time, Clocks, and the Ordering of
Events in a Distributed System" (1978). Processes run on independent threads and
communicate only by message passing over a deliberately unreliable in-process
network. Each process keeps a Lamport logical clock, so the causal order of
events can be reconstructed even when messages are delayed, dropped, or
reordered in transit.

## Build

Requires a C++17 compiler (tested with g++ 13) and pthreads.

    bash compile.sh

This builds and runs the wire-format tests and the cluster simulation.

## Layout

    src/
      core.hpp                        shared type aliases
      proto/message.hpp               the message type
      proto/wire.hpp                  big-endian (de)serialization
      transport/transport.hpp         ITransport interface
      transport/inproc_transport.hpp  in-memory network, thread-safe queues
      net/endpoint.hpp                a process: worker thread + send/recv
      net/chaos.hpp                   drop / delay / reorder decorator
      clock.hpp                       Lamport logical clock (IR1/IR2)
      cluster_sim.cpp                 demo
    tests/
      wireTest.cpp                    round-trip tests for the wire format

## Status

- Phase 1 - message-passing network with fault injection. Done.
- Phase 2 - Lamport logical clocks; the demo recovers send order from reordered
  delivery. Done.
- Phase 3 - total ordering and distributed mutual exclusion. Planned.

## Reference

Lamport, L. (1978). Time, Clocks, and the Ordering of Events in a Distributed
System. Communications of the ACM, 21(7), 558-565.
