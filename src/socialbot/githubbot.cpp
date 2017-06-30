#include "githubbot.h"
#include "sha1/HMAC_SHA1.h"
#include <fstream>
char jsonparse[1024*1024];
int json_cnt;

GithubGetCommits::GithubGetCommits(const AttributeType &cfg,
                                   AttributeType &commits) {
    json_cnt = 0;
    hCurl_ = curl_easy_init();
    curl_easy_setopt(hCurl_, CURLOPT_SSL_VERIFYPEER, 0);
    // must be set for github
    curl_easy_setopt(hCurl_, CURLOPT_USERAGENT, "socialbot");

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

    sprintf(httpsLink_, "https://api.github.com/repos/%s/%s/commits",
        cfg["author"].to_string(),
        cfg["repo_name"].to_string());
    get(cfg, commits);
    curl_easy_cleanup(hCurl_);
}

GithubGetCommits::~GithubGetCommits() {
    delete [] bufError_;
    delete [] bufPostMsg_;
}

int GithubGetCommits::callbackResponse(char* data, size_t size,
                              size_t nmemb, GithubGetCommits *obj) {
    if (obj && data) {
        size_t total = size * nmemb;
        return obj->writeWebResponse(data, total);
    }
    return 0;
}

int GithubGetCommits::writeWebResponse(char *buf, size_t sz) {
    memcpy(strWebResponse, buf, sz);
    strWebResponse[sz] = 0;
    memcpy(&jsonparse[json_cnt], buf, sz);
    json_cnt += sz;
    jsonparse[json_cnt] = 0;
    return static_cast<int>(sz);
}


void GithubGetCommits::get(const AttributeType &cfg, AttributeType &commits) {
    struct curl_slist* pOAuthHeaderList = NULL;

    pOAuthHeaderList = curl_slist_append( pOAuthHeaderList, header_);
    if (pOAuthHeaderList) {
        curl_easy_setopt(hCurl_, CURLOPT_HTTPHEADER, pOAuthHeaderList);
    }

    curl_easy_setopt(hCurl_, CURLOPT_HTTPGET, 1 );
    curl_easy_setopt(hCurl_, CURLOPT_URL, httpsLink_);

    if (curl_easy_perform(hCurl_) == CURLE_OK) {
        printf("OK: %s\n", strWebResponse);
        commits.from_config(jsonparse);
    } else {
        printf("Error: %s\n", bufError_);
    }
    if (pOAuthHeaderList) {
        curl_slist_free_all(pOAuthHeaderList);
    }
}
