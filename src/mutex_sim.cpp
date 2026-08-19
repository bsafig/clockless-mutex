// N processes contend for a single lock using Lamport's distributed
// mutual-exclusion algorithm. A shared counter checks how many processes are in
// the critical section at once
#include "lamport_mutex.hpp"
#include "net/chaos.hpp"
#include "transport/inproc_transport.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

using namespace clk;

static std::atomic<int> g_in_cs{0};
static std::atomic<int> g_violations{0};
static std::atomic<int> g_entries{0};

static std::vector<ProcessId> peers_of(ProcessId self, std::size_t n)
{
    std::vector<ProcessId> peers;
    for (ProcessId i = 0; i < n; ++i)
        if (i != self)
            peers.push_back(i);
    return peers;
}

static void critical_section(ProcessId i)
{
    int concurrent = g_in_cs.fetch_add(1) + 1;
    g_entries.fetch_add(1);
    if (concurrent != 1)
    {
        g_violations.fetch_add(1);
        std::printf("  VIOLATION: p%u entered the CS while %d were already inside\n",
                    i, concurrent - 1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    g_in_cs.fetch_sub(1);
}

int main()
{
    const std::size_t N = 5;
    const int ROUNDS = 10;

    std::printf("== trial 1: reliable FIFO transport ==\n");
    {
        Network net(N);
        std::vector<std::unique_ptr<InprocTransport>> tps;
        std::vector<std::unique_ptr<LamportMutex>> nodes;
        for (ProcessId i = 0; i < N; ++i)
        {
            tps.push_back(std::make_unique<InprocTransport>(net, i));
            nodes.push_back(std::make_unique<LamportMutex>(i, peers_of(i, N), *tps[i]));
        }
        for (auto &n : nodes)
            n->start();

        std::vector<std::thread> drivers;
        for (ProcessId i = 0; i < N; ++i)
            drivers.emplace_back([i, ROUNDS, &nodes]
                                 {
                for (int r = 0; r < ROUNDS; ++r)
                {
                    nodes[i]->acquire();
                    critical_section(i);
                    nodes[i]->release();
                } });
        for (auto &d : drivers)
            d.join();
        net.close();
        for (auto &n : nodes)
            n->join();
    }
    std::printf("[reliable] %d critical-section entries, %d violations\n\n",
                g_entries.load(), g_violations.load());

    g_in_cs.store(0);
    g_violations.store(0);
    g_entries.store(0);
    std::printf("== trial 2: chaos transport (drops + reorders messages) ==\n");
    {
        Network net(N);
        ChaosConfig cfg;
        cfg.drop_prob = 0.10;
        cfg.min_delay_ms = 0;
        cfg.max_delay_ms = 200;
        cfg.seed = 1;

        std::vector<std::unique_ptr<InprocTransport>> tps;
        std::vector<std::unique_ptr<ChaosTransport>> chs;
        std::vector<std::unique_ptr<LamportMutex>> nodes;
        for (ProcessId i = 0; i < N; ++i)
        {
            tps.push_back(std::make_unique<InprocTransport>(net, i));
            chs.push_back(std::make_unique<ChaosTransport>(*tps[i], cfg));
            nodes.push_back(std::make_unique<LamportMutex>(i, peers_of(i, N), *chs[i]));
        }
        for (auto &n : nodes)
            n->start();

        std::atomic<int> finished{0};
        std::vector<std::thread> drivers;
        for (ProcessId i = 0; i < N; ++i)
            drivers.emplace_back([i, ROUNDS, &nodes, &finished]
                                 {
                for (int r = 0; r < ROUNDS; ++r)
                {
                    nodes[i]->acquire();
                    critical_section(i);
                    nodes[i]->release();
                }
                finished.fetch_add(1); });

        int prev = 0, stagnant_ms = 0;
        while (finished.load() < static_cast<int>(N))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            int cur = g_entries.load();
            stagnant_ms = (cur == prev) ? stagnant_ms + 150 : 0;
            prev = cur;
            if (stagnant_ms >= 1500)
            {
                std::printf("\n[chaos] STALLED after %d/%d CS entries: a process is stuck "
                            "waiting forever.\n",
                            cur, static_cast<int>(N) * ROUNDS);
                std::printf("[chaos] a message the algorithm needed was dropped or reordered, "
                            "so its grant condition can never be satisfied.\n");
                std::printf("[chaos] Lamport's algorithm assumes reliable, in-order delivery -- "
                            "chaos breaks that. violations so far: %d\n",
                            g_violations.load());
                std::fflush(stdout);
                std::_Exit(0);
            }
        }

        for (auto &d : drivers)
            d.join();
        net.close();
        for (auto &n : nodes)
            n->join();
    }
    std::printf("[chaos] %d critical-section entries, %d violations\n",
                g_entries.load(), g_violations.load());

    return 0;
}
