#ifndef OSU_TOP_WEB_REQUEST_H
#define OSU_TOP_WEB_REQUEST_H

#include <string>
#include <future>

std::string get_url(const std::string& url);
std::future<std::string> get_url_async(const std::string& url);
std::future<std::string> get_url_async_ratelimited(const std::string& url);

#endif //OSU_TOP_WEB_REQUEST_H
