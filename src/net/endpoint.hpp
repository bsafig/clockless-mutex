#pragma once

#include "../transport/transport.hpp"
#include "../clock.hpp"

#include <cstdio>
#include <thread>
#include <vector>

namespace clk
{
    class Endpoint
    {
    public:
        Endpoint(ProcessId id, ITransport &transport)
            : id_(id), transport_(transport) {}

        Endpoint(const Endpoint &) = delete;
        Endpoint &operator=(const Endpoint &) = delete;

        ~Endpoint()
        {
            join();
        }

        void start()
        {
            worker_ = std::thread([this]
                                  { run(); });
        }

        void send(ProcessId to, const Payload &payload)
        {
            LogicalTime t = clk_.tick();
            transport_.send(message{id_, to, t, payload});
        }

        void join()
        {
            if (worker_.joinable())
                worker_.join();
        }

        ProcessId id() const { return id_; }

        const std::vector<message> &received() const { return rec_; }

    private:
        void run()
        {
            while (auto m = transport_.recv())
                handle(*m);
        }

        void handle(const message &m)
        {
            LogicalTime t = clk_.update(m.timestamp);
            rec_.push_back(m);
            std::printf("[p%u @%llu] recv from p%u (sent @%llu): \"%s\"\n",
                        id_,
                        (unsigned long long)t,
                        m.from,
                        (unsigned long long)m.timestamp,
                        m.payload.c_str());
        }

        ProcessId id_;
        ITransport &transport_;
        std::thread worker_;
        LamportClock clk_;
        std::vector<message> rec_;
    };
}
