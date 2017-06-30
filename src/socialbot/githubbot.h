#pragma once

#include "attribute.h"
#include "curl/curl.h"
#include <string>
#include <inttypes.h>


class GithubGetCommits {
public:
    GithubGetCommits(const AttributeType &cfg, AttributeType &commits);
    ~GithubGetCommits();

    int writeWebResponse(char *buf, size_t sz);
private:
    void get(const AttributeType &cfg, AttributeType &commits);

    static int callbackResponse(char* data, size_t size,
                              size_t nmemb, GithubGetCommits *obj);
private:
    CURL *hCurl_;
    char header_[1024];
    char httpsLink_[1024];
    char strWebResponse[64*1024];
    char *bufError_;
    char *bufPostMsg_;
};
