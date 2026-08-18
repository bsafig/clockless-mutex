#pragma once

#include "../transport/transport.hpp"

#include <chrono>
#include <cstdio>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace clk
{
    // How unreliable should the network be?
    struct ChaosConfig
    {
        double drop_prob = 0.1;
        int min_delay_ms = 25;
        int max_delay_ms = 1000;
        unsigned seed = 1;
    };

    class ChaosTransport : public ITransport
    {
    public:
        ChaosTransport(ITransport &inner, ChaosConfig cfg)
            : inner_(inner), cfg_(cfg), rng_(cfg.seed) {}

        ChaosTransport(const ChaosTransport &) = delete;
        ChaosTransport &operator=(const ChaosTransport &) = delete;

        ~ChaosTransport() { join_all(); }

        void send(const message &m) override
        {
            std::uniform_real_distribution<double> coin(0.0, 1.0);
            if (coin(rng_) < cfg_.drop_prob)
            {
                std::printf("[chaos] DROP  p%u->p%u \"%s\"\n",
                            m.from, m.to, m.payload.c_str());
                return;
            }

            std::uniform_int_distribution<int> pick(cfg_.min_delay_ms, cfg_.max_delay_ms);
            int ms = pick(rng_);

            std::lock_guard<std::mutex> lock(mtx_);
            workers_.emplace_back([this, m, ms]
                                  {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                inner_.send(m); });
        }

        std::optional<message> recv() override { return inner_.recv(); }

        void close() override
        {
            join_all();
            inner_.close();
        }

    private:
        void join_all()
        {
            std::lock_guard<std::mutex> lock(mtx_);
            for (auto &t : workers_)
                if (t.joinable())
                    t.join();
            workers_.clear();
        }

        ITransport &inner_;
        ChaosConfig cfg_;
        std::mt19937 rng_;
        std::mutex mtx_;
        std::vector<std::thread> workers_;
    };
}
