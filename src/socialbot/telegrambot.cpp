#include "telegrambot.h"
#include "utils.h"
#include <fstream>

TelegramPost::TelegramPost(const AttributeType &cfg,
                           const char *msg) {
    hCurl_ = curl_easy_init();
    curl_easy_setopt(hCurl_, CURLOPT_SSL_VERIFYPEER, 0);
    // must be set for github
    curl_easy_setopt(hCurl_, CURLOPT_USERAGENT, 0);

    //standard params:
    if (cfg["verbose"].to_int()) {
        curl_easy_setopt(hCurl_, CURLOPT_VERBOSE, 1L);
    }
    curl_easy_setopt(hCurl_, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(hCurl_, CURLOPT_ENCODING, "");
    curl_easy_setopt(hCurl_, CURLOPT_INTERFACE, NULL);

    curl_easy_setopt(hCurl_, CURLOPT_PROXY, NULL );
    curl_easy_setopt(hCurl_, CURLOPT_PROXYUSERPWD, NULL );
    curl_easy_setopt(hCurl_, CURLOPT_PROXYAUTH, (long)CURLAUTH_ANY );
    if (cfg["proxy"].is_string() && cfg["proxy"].size()) {
        curl_easy_setopt(hCurl_, CURLOPT_PROXY, cfg["proxy"].to_string());
        curl_easy_setopt(hCurl_, CURLOPT_PROXYUSERPWD, NULL );
        curl_easy_setopt(hCurl_, CURLOPT_PROXYAUTH, (long)CURLAUTH_ANY );
    }

    bufPostMsg_ = new char[1024];
    memset(bufPostMsg_, 0, 1024);

    bufError_ = new char[1024];
    memset(bufError_, 0, 1024);
    curl_easy_setopt(hCurl_, CURLOPT_ERRORBUFFER, bufError_);
    curl_easy_setopt(hCurl_, CURLOPT_WRITEFUNCTION, callbackResponse);
    curl_easy_setopt(hCurl_, CURLOPT_WRITEDATA, this);

    // --data '{\"since\":\"2015-02-14T12:00:00Z\"}'",
    sprintf(header_, "Authorization: token %s",
                    cfg["oauth_token"].to_string());

#if 1
    sprintf(httpsLink_, "https://api.telegram.org/bot%s/sendMessage",
        cfg["token"].to_string());

    str2netstr(msg, strMsg_);
    sprintf(bufPostMsg_, "chat_id=@%s&text=%s",
        cfg["chat_id"].to_string(),
        strMsg_);

    postToChannel();
#else
    sprintf(httpsLink_, "https://api.telegram.org/bot%s/getme",
        cfg["token"].to_string());
    postToChannel(cfg, commit);
#endif
    curl_easy_cleanup(hCurl_);
}

TelegramPost::~TelegramPost() {
    delete [] bufError_;
    delete [] bufPostMsg_;
}

int TelegramPost::callbackResponse(char* data, size_t size,
                              size_t nmemb, TelegramPost *obj) {
    if (obj && data) {
        size_t total = size * nmemb;
        return obj->writeWebResponse(data, total);
    }
    return 0;
}

int TelegramPost::writeWebResponse(char *buf, size_t sz) {
    memcpy(strWebResponse, buf, sz);
    strWebResponse[sz] = 0;
    return static_cast<int>(sz);
}


void TelegramPost::postToChannel() {
    curl_easy_setopt(hCurl_, CURLOPT_HTTPPOST, 1 );
    curl_easy_setopt(hCurl_, CURLOPT_URL, httpsLink_);
    curl_easy_setopt(hCurl_, CURLOPT_COPYPOSTFIELDS, bufPostMsg_);

    if (curl_easy_perform(hCurl_) == CURLE_OK) {
        printf("OK: %s\n", strWebResponse);
    } else {
        printf("Error: %s\n", bufError_);
    }
}
