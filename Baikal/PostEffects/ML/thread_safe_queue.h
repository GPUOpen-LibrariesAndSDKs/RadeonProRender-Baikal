#pragma once

namespace ML {
    ///< An implementation of a concurrent queue
    ///< which is providing the means to wait until 
    ///< elements are available or use non-blocking
    ///< try_pop method.
    ///<
    template <typename T> class thread_safe_queue
    {
    public:
        thread_safe_queue()
            : disabled_(false)
        {}

        ~thread_safe_queue()
        {
            disable();
        }


        // Push element: one of the threads is going to 
        // be woken up to process this if any
        void push(T const& t)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(t);
            cv_.notify_one();
        }

        // Push element: one of the threads is going to 
        // be woken up to process this if any
        void push(T&& t)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(t));
            // Notify one of the threads waiting on CV.
            cv_.notify_one();
        }

        // Wait until there are element to process.
        bool wait_and_pop(T& t)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // If queue is empty, wait on CV until either:
            // * Something is in the queue.
            // * Queue has been disabled.
            if (queue_.empty()) {
                cv_.wait(lock, [this]() { return !queue_.empty() || disabled_; });
                // Return immediately if we have been disabled.
                if (disabled_) return false;
            }

            // Here the queue is non-empty, so pop and return true.
            t = std::move(queue_.front());
            queue_.pop();
            return true;
        }

        // Try to pop element. Returns true if element has been popped, 
        // false if there are no element in the queue
        bool try_pop(T& t)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty())
                return false;
            t = std::move(queue_.front());
            queue_.pop();
            return true;
        }

        size_t size() const
        {
            std::unique_lock<std::mutex> lock(mutex_);
            return queue_.size();
        }

        // Disable the queue: all threads waiting in wait_and_pop are released.
        void disable() {
            std::lock_guard<std::mutex> lock(mutex_);
            disabled_ = true;
            cv_.notify_all();
        }

        // Enables the queue: wait_and_pop starts working again.
        void enable() {
            std::lock_guard<std::mutex> lock(mutex_);
            disabled_ = false;
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<T> queue_;
        // Flag to disable wait_and_pop
        bool disabled_;
    };
}