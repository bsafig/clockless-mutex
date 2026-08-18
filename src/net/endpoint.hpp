#pragma once

#include "../transport/transport.hpp"

#include <cstdio>
#include <thread>

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
            transport_.send(message{id_, to, payload});
        }

        void join()
        {
            if (worker_.joinable())
                worker_.join();
        }

        ProcessId id() const { return id_; }

    private:
        void run()
        {
            while (auto m = transport_.recv())
                handle(*m);
        }

        void handle(const message &m)
        {
            std::printf("[p%u] recv from p%u: \"%s\"\n",
                        id_, m.from, m.payload.c_str());
        }

        ProcessId id_;
        ITransport &transport_;
        std::thread worker_;
    };
}
