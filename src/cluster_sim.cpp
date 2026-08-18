// spin up N processes on a chaotic in-process network and watchthem exchange messages
#include "net/chaos.hpp"
#include "net/endpoint.hpp"
#include "transport/inproc_transport.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace clk;

int main()
{
    const std::size_t N = 2;
    Network net(N);

    ChaosConfig cfg;

    std::vector<std::unique_ptr<InprocTransport>> transports;
    std::vector<std::unique_ptr<ChaosTransport>> chaos;
    std::vector<std::unique_ptr<Endpoint>> endpoints;

    for (ProcessId i = 0; i < N; i++)
    {
        transports.push_back(std::make_unique<InprocTransport>(net, i));
        chaos.push_back(std::make_unique<ChaosTransport>(*transports[i], cfg));
        endpoints.push_back(std::make_unique<Endpoint>(i, *chaos[i]));
    }

    for (std::unique_ptr<Endpoint> &e : endpoints)
        e->start();

    // p0 fires 5 messages at p1, stamped @1..@5 in order. Chaos scrambles arrival.
    const char *labels[] = {"A", "B", "C", "D", "E"};
    for (int k = 0; k < 5; ++k)
        endpoints[0]->send(1, labels[k]);

    std::this_thread::sleep_for(std::chrono::milliseconds(cfg.max_delay_ms + 200));

    net.close();
    for (std::unique_ptr<Endpoint> &e : endpoints)
        e->join();

    // Recovery: sort p1's received messages by Lamport timestamp to reconstruct
    std::vector<message> log = endpoints[1]->received();

    std::sort(log.begin(), log.end(),
              [](const message &a, const message &b)
              { return a.timestamp < b.timestamp; });

    std::printf("\n-- p1 recovers send order by Lamport timestamp --\n");
    for (const message &m : log)
        std::printf("  %s @%llu\n", m.payload.c_str(), (unsigned long long)m.timestamp);

    return 0;
}
