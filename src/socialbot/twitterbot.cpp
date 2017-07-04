#include "twitterbot.h"
#include "sha1/HMAC_SHA1.h"
#include <fstream>
std::ofstream ostw("twitterbot.log");

static const char *httpsTwitterStatus = "https://api.twitter.com/1.1/statuses/update.json";

extern int str2netstr(const char *inStr, char *outStr);
extern std::string base64_encode(unsigned char const* bytes_to_encode,
                                  unsigned int in_len);

static const char *type_info_names[] = {
  "TEXT",
  "HEADER_IN",    /* 1 */
  "HEADER_OUT",   /* 2 */
  "DATA_IN",      /* 3 */
  "DATA_OUT",     /* 4 */
  "SSL_DATA_IN",  /* 5 */
  "SSL_DATA_OUT", /* 6 */
};

static void verbose_callback(CURL *handle,
                           curl_infotype type,
                           char *data,
                           size_t size,
                           void *userptr) {
    char tstr[1024];
    int tstr_sz = sprintf(tstr, "\n!!%d=%s:\n", type, type_info_names[type]);
    ostw.write(tstr, tstr_sz);
    ostw.write(data, size);
}

TwitterPostStatus::TwitterPostStatus(const AttributeType &cfg,
                                     const char *msg) {
    
    memset(this, 0, sizeof(this));

    srand( (unsigned int)time( NULL ) ); // seed
    time_t tm1 = time(NULL);
    int tm2 = rand() % 1000;
    //tm1 = 1499072346ll;
    //tm2 = 0x2d2;

    sprintf_s(strTime_, 1024, "%I64d", tm1);
    sprintf_s(strNonce_, 1024, "%I64d%x", tm1, tm2);

    char tmp_msg[1024];
    sprintf(tmp_msg, "%s %s", cfg["message_marker"].to_string(), msg);
    str2netstr(tmp_msg, strMsg_);

    hCurl_ = curl_easy_init();
    curl_easy_setopt(hCurl_, CURLOPT_USERAGENT, 0);

    //standard params:
    if (cfg["verbose"].to_int()) {
        curl_easy_setopt(hCurl_, CURLOPT_VERBOSE, 1L);
        //curl_easy_setopt(hCurl_, CURLOPT_DEBUGFUNCTION, verbose_callback);
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

    char tstr[1024];
    sprintf(tstr, "%s:%s", cfg["username"].to_string(), cfg["password"].to_string());
    curl_easy_setopt(hCurl_, CURLOPT_USERPWD, tstr);

    bufPostMsg_ = new char[1024];
    memset(bufPostMsg_, 0, 1024);

    bufError_ = new char[1024];
    memset(bufError_, 0, 1024);
    curl_easy_setopt(hCurl_, CURLOPT_ERRORBUFFER, bufError_);
    curl_easy_setopt(hCurl_, CURLOPT_WRITEFUNCTION, callbackResponse);
    curl_easy_setopt(hCurl_, CURLOPT_WRITEDATA, this);


    createOAuthSignature(cfg, strMsg_);
    createOAuthHeader(cfg);
    post(cfg, strMsg_);

    curl_easy_cleanup(hCurl_);
}

TwitterPostStatus::~TwitterPostStatus() {
    delete [] bufError_;
    delete [] bufPostMsg_;
}

int TwitterPostStatus::callbackResponse(char* data, size_t size,
                              size_t nmemb, TwitterPostStatus *obj) {
    if (obj && data) {
        size_t total = size * nmemb;
        return obj->writeWebResponse(data, total);
    }
    return 0;
}

int TwitterPostStatus::writeWebResponse(char *buf, size_t sz) {
    memcpy(strWebResponse, buf, sz);
    strWebResponse[sz] = 0;
    return static_cast<int>(sz);
}

void TwitterPostStatus::createOAuthSignature(const AttributeType &cfg,
                                       const char *msg) {
    char params[1024];
    char neturl[1024];
    char netparams[1024];
    char base[2*1024];
    char secret[1024];
    memset(params, 0, sizeof(params));
    memset(neturl, 0, sizeof(neturl));
    memset(netparams, 0, sizeof(netparams));
    memset(base, 0, sizeof(base));
    memset(secret, 0, sizeof(secret));

    sprintf(params, "oauth_consumer_key=%s&oauth_nonce=%s"
		            "&oauth_signature_method=HMAC-SHA1&oauth_timestamp=%s"
		            "&oauth_token=%s&oauth_version=1.0&status=%s",
        cfg["oauth_consumer_key"].to_string(),
        strNonce_,
        strTime_,
        cfg["oauth_token"].to_string(),
        msg);

    str2netstr(httpsTwitterStatus , neturl);
    str2netstr(params, netparams);

    int base_sz = sprintf(base, "POST" "&" "%s" "&" "%s", neturl, netparams);
    int secret_sz = sprintf(secret, "%s" "&" "%s",
                            cfg["consumer_secret"].to_string(),
                            cfg["token_secret"].to_string());

    CHMAC_SHA1 objHMACSHA1;
    memset(strDigest_, 0, sizeof(strDigest_));
    memset(signature_, 0, sizeof(signature_));

    objHMACSHA1.HMAC_SHA1(reinterpret_cast<uint8_t *>(base),
                          base_sz,
                          reinterpret_cast<uint8_t *>(secret),
                          secret_sz,
                          strDigest_);

    std::string base64Str = base64_encode(strDigest_, 20 /* SHA 1 digest is 160 bits */ );
    str2netstr(base64Str.c_str(), signature_);
}

void TwitterPostStatus::createOAuthHeader(const AttributeType &cfg) {
    sprintf(oauth_header_, "Authorization: OAuth "
        "oauth_consumer_key=\"%s\",oauth_nonce=\"%s\""
        ",oauth_signature=\"%s\""
		",oauth_signature_method=\"HMAC-SHA1\",oauth_timestamp=\"%s\""
		",oauth_token=\"%s\",oauth_version=\"1.0\"",
        cfg["oauth_consumer_key"].to_string(),
        strNonce_,
        signature_,
        strTime_,
        cfg["oauth_token"].to_string());
}

void TwitterPostStatus::post(const AttributeType &cfg, const char *msg) {
    struct curl_slist* pOAuthHeaderList = NULL;
    pOAuthHeaderList = curl_slist_append( pOAuthHeaderList, oauth_header_);
    if (pOAuthHeaderList) {
        curl_easy_setopt(hCurl_, CURLOPT_HTTPHEADER, pOAuthHeaderList);
    }

    sprintf(bufPostMsg_, "status=%s\0", msg);

    curl_easy_setopt(hCurl_, CURLOPT_POST, 1 );
    curl_easy_setopt(hCurl_, CURLOPT_URL, httpsTwitterStatus);
    curl_easy_setopt(hCurl_, CURLOPT_COPYPOSTFIELDS, bufPostMsg_);
    curl_easy_setopt(hCurl_, CURLOPT_SSL_VERIFYPEER, 0);
    //curl_easy_setopt(hCurl_, CURLOPT_SSL_VERIFYHOST, 0);
    if (curl_easy_perform(hCurl_) == CURLE_OK) {
        printf("OK: %s\n", strWebResponse);
    } else {
        printf("Error: %s\n", bufError_);
    }
    if (pOAuthHeaderList) {
        curl_slist_free_all(pOAuthHeaderList);
    }
}
