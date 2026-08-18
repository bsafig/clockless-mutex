// spin up N processes on a chaotic in-process network and watchthem exchange messages
#include "net/chaos.hpp"
#include "net/endpoint.hpp"
#include "transport/inproc_transport.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace clk;

int main()
{
    const std::size_t N = 3;
    Network net(N);

    ChaosConfig cfg;

    std::vector<std::unique_ptr<InprocTransport>> transports;
    std::vector<std::unique_ptr<ChaosTransport>> chaos;
    std::vector<std::unique_ptr<Endpoint>> endpoints;

    for (int i = 0; i < N; i++)
    {
        transports.push_back(std::make_unique<InprocTransport>(net, i));
        chaos.push_back(std::make_unique<ChaosTransport>(*transports[i], cfg));
        endpoints.push_back(std::make_unique<Endpoint>(i, *chaos[i]));
    }

    for (std::unique_ptr<Endpoint> &e : endpoints)
        e->start();

    for (int i = 0; i < N; i++)
        endpoints[i]->send(static_cast<ProcessId>((i + 1) % N), "hello from p" + std::to_string(i));

    std::this_thread::sleep_for(std::chrono::milliseconds(cfg.max_delay_ms + 200));

    net.close();
    for (std::unique_ptr<Endpoint> &e : endpoints)
        e->join();

    return 0;
}
