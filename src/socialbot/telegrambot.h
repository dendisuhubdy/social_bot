#pragma once

#include "attribute.h"
#include "curl/curl.h"
#include <string>
#include <inttypes.h>


class TelegramPost {
public:
    TelegramPost(const AttributeType &cfg, const char *msg);
    ~TelegramPost();

    int writeWebResponse(char *buf, size_t sz);
private:
    void postToChannel();

    static int callbackResponse(char* data, size_t size,
                              size_t nmemb, TelegramPost *obj);
private:
    CURL *hCurl_;
    char header_[1024];
    char httpsLink_[1024];
    char strMsg_[1024];
    char strWebResponse[64*1024];
    char *bufError_;
    char *bufPostMsg_;
};
