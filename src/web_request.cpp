#include "web_request.h"

#include <Magnum/Magnum.h>
#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/DebugStl.h>
#include <chrono>
#include <atomic>

#if defined(MAGNUM_TARGET_WEBGL)
#include <emscripten.h>

std::string get_url(const std::string& url)
{
    // proxy to allow cross-origin resource sharing
    const auto cors_url = "https://cors-anywhere.herokuapp.com/" + url;
    // Also requires "-s ASYNCIFY=1" if enabled
    char* buffer = nullptr;
    int size, error;
    emscripten_wget_data(
        cors_url.c_str(),
        reinterpret_cast<void**>(&buffer),
        &size,
        &error
    );
    if(size > 0){
        std::string response(buffer, size);
        free(buffer);
        return response;
    }
    return "Failed :(";
}

#else

#include <string_view>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

std::string get_url(const std::string& url)
{
    std::string_view url_view = url;

    url_view.remove_prefix(std::min(url_view.find("//") + 2, url_view.size()));
    auto host = url_view;
    host.remove_suffix(url_view.size() - std::min(url_view.find("/"), url_view.size()));
    auto path = url_view;
    path.remove_prefix(host.size());

    const auto host_string = std::string{ host };
    const auto path_string = std::string{ path };

    httplib::SSLClient client(host_string);

    const auto response = client.Get(path_string.c_str());

    if(response){
        if(response->status != 200){
            Corrade::Utility::Debug() << "Loading url \"" << url << "\" may have failed:" << response->status;
        }
        return response->body;
    }

    Corrade::Utility::Debug() << "Loading url \"" << url << "\" failed: no response object";

    return "";  //Todo: Better error handling
}

#endif

#if defined(MAGNUM_TARGET_WEBGL)

static void on_load_callback(void* arg, void* buffer, int length)
{
    Magnum::Debug() << "Download complete";
    Future& future = *reinterpret_cast<Future*>(arg);
    future.data = std::string(reinterpret_cast<char*>(buffer), length);
}

static void on_error_callback(void* arg)
{
    Magnum::Debug() << "Error downloading";
    Future& future = *reinterpret_cast<Future*>(arg);
    future.data = "Failed :(";  // TODO: Better error handling
}

Future get_url_async(const std::string& url)
{
    // proxy to allow cross-origin resource sharing
    const auto cors_url = "https://cors-anywhere.herokuapp.com/" + url;

    Future future;
    Magnum::Debug() << "Starting Download 1";
    emscripten_async_wget_data(cors_url.c_str(), reinterpret_cast<void*>(&future), &on_load_callback, &on_error_callback);
    return future;
}


#else

Future get_url_async(const std::string& url)
{
    return Future{ std::async(std::launch::async, get_url, url) };
}

#endif

auto last_request_time = std::chrono::system_clock::now();
constexpr const auto request_limit = 10;
constexpr const auto request_duration = std::chrono::milliseconds{ 1000 / request_limit };

std::atomic<int> current_requests = 0;
constexpr const auto max_requests = 20;

#if defined(MAGNUM_TARGET_WEBGL)

extern "C" {
    void async_wget_delete(std::string& url, void* arg)
    {
        Magnum::Debug() << "Starting Download 2";
        emscripten_async_wget_data(url.c_str(), arg, &on_load_callback, &on_error_callback);
        Magnum::Debug() << "Deleting url";
        delete &url;
    }
}

Future request_at(const std::string& url, std::chrono::system_clock::time_point request_time)
{
    const auto delay = request_time - std::chrono::system_clock::now();

    auto heap_url = new std::string(url);

    Future future;

    EM_ASM({
        function sleep (time) {
            return new Promise((resolve) => setTimeout(resolve, time));
        }

        sleep($1).then(() =>{
                Module.ccall('async_wget_delete', $0, $2, true);
        });
    }, heap_url, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(delay).count()), reinterpret_cast<void*>(&future));


    return future;
}

#else

Future request_at(const std::string& url, std::chrono::system_clock::time_point request_time)
{
    const auto fetch_function = [](const auto request_time, const std::string& url)
            {
                std::this_thread::sleep_until(request_time);

                while(current_requests >= max_requests) std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });

                ++current_requests;
                const auto data = get_url_async(url).get();
                --current_requests;

                return data;
            };
    return Future{ std::async(std::launch::async, fetch_function, request_time, url) };
}

#endif

Future get_url_async_ratelimited(const std::string& url)
{
    const auto current_time = std::chrono::system_clock::now();
    if(last_request_time + request_duration <= current_time) {
        last_request_time = current_time;
        return get_url_async(url);
    }

    last_request_time += request_duration;

    return request_at(url, last_request_time);
}
