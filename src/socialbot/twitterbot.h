#pragma once

#include "attribute.h"
#include "curl/curl.h"
#include <string>
#include <inttypes.h>


class TwitterPostStatus {
public:
    TwitterPostStatus(const AttributeType &cfg, const char *msg);
    ~TwitterPostStatus();

    int writeWebResponse(char *buf, size_t sz);
private:
    void createOAuthSignature(const AttributeType &cfg, const char *msg);
    void createOAuthHeader(const AttributeType &cfg);
    void post(const AttributeType &cfg, const char *msg);

    static int callbackResponse(char* data, size_t size,
                              size_t nmemb, TwitterPostStatus *obj);

private:
    CURL *hCurl_;
    char strMsg_[1024];
    char strTime_[1024];
    char strNonce_[1024];
    uint8_t strDigest_[1024];
    char signature_[1024];
    char oauth_header_[1024];
    char strWebResponse[64*1024];
    char *bufError_;
    char *bufPostMsg_;
};
