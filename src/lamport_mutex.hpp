#pragma once

#include "clock.hpp"
#include "order.hpp"
#include "transport/transport.hpp"

#include <algorithm>
#include <condition_variable>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <utility>
#include <vector>

namespace clk
{
    // Lamport's distributed mutual-exclusion algorithm.
    class LamportMutex
    {
    public:
        LamportMutex(ProcessId self, std::vector<ProcessId> adj, ITransport &transport)
            : self_(self), adj_(std::move(adj)), transport_(transport)
        {
            for (ProcessId p : adj_)
                last_heard_[p] = 0;
        }

        LamportMutex(const LamportMutex &) = delete;
        LamportMutex &operator=(const LamportMutex &) = delete;

        ~LamportMutex() { join(); }

        void start()
        {
            worker_ = std::thread([this]
                                  { run(); });
        }
        void join()
        {
            if (worker_.joinable())
                worker_.join();
        }

        // Rule 1 and Rule 5: request the lock, then BLOCK until it is granted.
        void acquire()
        {
            std::unique_lock<std::mutex> lock(mtx_);
            LogicalTime t = clock_.tick(); // IR1
            my_request_ = Stamp{t, self_};
            requests_.insert(*my_request_);
            broadcast_locked(MsgType::Request, t);

            // Rule 5: sleep until our request is first and everyone has answered past t.
            cv_.wait(lock, [this]
                     { return can_enter_locked(); });
        }

        // Rule 3: leave the critical section and let others proceed.
        void release()
        {
            std::unique_lock<std::mutex> lock(mtx_);
            if (my_request_)
                requests_.erase(*my_request_);
            my_request_.reset();
            LogicalTime t = clock_.tick(); // IR1
            broadcast_locked(MsgType::Release, t);
        }

    private:
        void run()
        {
            while (auto m = transport_.recv())
                handle(*m);
        }

        void handle(const message &m)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            clock_.update(m.timestamp); // IR2
            last_heard_[m.from] = std::max(last_heard_[m.from], m.timestamp);

            switch (m.type)
            {
            case MsgType::Request:
                requests_.insert(Stamp{m.timestamp, m.from}); // Rule 2: enqueue
                {
                    LogicalTime t = clock_.tick();
                    send_locked(m.from, MsgType::Ack, t); // Rule 2: acknowledge
                }
                break;

            case MsgType::Release:
                erase_pid_locked(m.from); // Rule 4: remove request
                break;

            case MsgType::Ack:
                break;

            default:
                break;
            }

            cv_.notify_all();
        }

        bool can_enter_locked() const
        {
            if (!my_request_ || requests_.empty())
                return false;
            if (requests_.begin()->pid != self_)
                return false;

            for (ProcessId p : adj_)
                if (last_heard_.at(p) <= my_request_->time)
                    return false;

            return true;
        }

        void erase_pid_locked(ProcessId pid)
        {
            for (auto it = requests_.begin(); it != requests_.end(); ++it)
                if (it->pid == pid)
                {
                    requests_.erase(it);
                    return;
                }
        }

        void send_locked(ProcessId to, MsgType type, LogicalTime t)
        {
            transport_.send(message{self_, to, t, "", type});
        }

        void broadcast_locked(MsgType type, LogicalTime t)
        {
            for (ProcessId p : adj_)
                send_locked(p, type, t);
        }

        ProcessId self_;
        std::vector<ProcessId> adj_;
        ITransport &transport_;

        LamportClock clock_;
        std::mutex mtx_;
        std::condition_variable cv_;
        std::set<Stamp> requests_;
        std::map<ProcessId, LogicalTime> last_heard_;
        std::optional<Stamp> my_request_;

        std::thread worker_;
    };
}
