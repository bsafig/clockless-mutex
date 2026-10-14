#pragma once

#include "transport.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

namespace clk
{
    class Inbox
    {
    public:
        void push(const message &m)
        {
            {
                std::lock_guard<std::mutex> lock(mtx_);
                q_.push(m);
            }
            cv_.notify_one();
        }

        std::optional<message> pop()
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this]
                     { return !q_.empty() || closed_; });

            if (q_.empty())
                return std::nullopt;
            message m = q_.front();
            q_.pop();
            return m;
        }

        void close()
        {
            {
                std::lock_guard<std::mutex> lock(mtx_);
                closed_ = true;
            }
            cv_.notify_all();
        }

    private:
        std::queue<message> q_;
        std::mutex mtx_;
        std::condition_variable cv_;
        bool closed_ = false;
    };

    // ---------------------------------------------------------------------
    // Network: the shared medium. Owns one Inbox per process and routes
    // each message into the recipient's inbox.
    //
    // TODO(you): implement the three method bodies below.
    // ---------------------------------------------------------------------
    class Network
    {
    public:
        explicit Network(std::size_t n) : inboxes_(n) {}

        void deliver(const message &m)
        {
            inboxes_[m.to].push(m);
        }

        std::optional<message> receive(ProcessId id)
        {
            return inboxes_[id].pop();
        }

        void close()
        {
            for (Inbox &i : inboxes_)
            {
                i.close();
            }
        }

        std::size_t size() const { return inboxes_.size(); }

    private:
        std::vector<Inbox> inboxes_;
    };

    class InprocTransport : public ITransport
    {
    public:
        InprocTransport(Network &net, ProcessId self) : _net(net), _self(self) {}
        void send(const message &m) override { _net.deliver(m); }
        std::optional<message> recv() override { return _net.receive(_self); }
        void close() override { _net.close(); }

    private:
        Network &_net;
        ProcessId _self;
    };
}
