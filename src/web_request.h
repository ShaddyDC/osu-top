#ifndef OSU_TOP_WEB_REQUEST_H
#define OSU_TOP_WEB_REQUEST_H

#include <string>
#include <Magnum/Magnum.h>

#if defined(MAGNUM_TARGET_WEBGL)

#include <chrono>
#include <thread>

struct Future{
    std::string data = "";
    bool validity = true;

    bool ready() const
    {
        return !data.empty();
    }

    bool valid() const
    {
        return validity;
    }

    std::string get()
    {
        while(!ready()) std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
        return data;
    }

    Future() = default;
    Future(Future&& other) : data{ std::move(other.data) }
    {
        other.validity = false;
    }

    Future& operator=(Future&& other)
    {
        data = std::move(other.data);
        validity = other.validity;
        other.validity = false;
        return *this;
    }
};

#else
#include <future>

struct Future {
    bool ready() const
    {
        return future.wait_for(std::chrono::microseconds::zero()) == std::future_status::ready;
    }
    bool valid() const
    {
        return future.valid();
    }

    std::string get()
    {
        return future.get();
    }
    std::future<std::string> future;
};

#endif


std::string get_url(const std::string& url);
Future get_url_async(const std::string& url);
Future get_url_async_ratelimited(const std::string& url);

#endif //OSU_TOP_WEB_REQUEST_H
