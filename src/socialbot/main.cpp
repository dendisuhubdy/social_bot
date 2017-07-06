#include "twitterbot.h"
#include "githubbot.h"
#include "telegrambot.h"
#include "utils.h"
#include <fstream>

#ifdef _DEBUG
#ifdef WIN32
#error "Select target Release. Debug doesn't include libcurl.lib file."
#else
#pragma error "Select target Release. Debug doesn't include libcurl.lib file."
#endif
#endif

//libcurl.lib;ws2_32.lib;wldap32.lib;advapi32.lib;kernel32.lib;comdlg32.lib
//curl_easy_setopt(curl_handle, CURLOPT_STDERR, my_file_stream)

char *readFromFile(const char *filename) {
    char *buf = 0;

    std::ifstream isfile(filename);
    if (!isfile.is_open()) {
        printf("File '%s' not found\n", filename);
        return buf;
    }

    isfile.seekg(0, std::ifstream::end);
    int filesz = static_cast<int>(isfile.tellg());
    isfile.seekg(0, std::ifstream::beg);
    buf = new char[filesz + 1];
    isfile.read(buf, filesz);
    buf[filesz] = '\0';
    printf("Read %d B from file '%s'\n", filesz, filename);
    return buf;
}

void readJsonFromFile(const char *filename, AttributeType &out) {
    char *pbuf;
    out.make_nil();
    if ((pbuf = readFromFile(filename)) == 0) {
        return;
    }
    out.from_config(pbuf);
    delete pbuf;
}

void writeJsonToFile(const char *filename, AttributeType &in) {
    const char *wrbuf = in.to_config().to_string();
    std::ofstream offile(filename);
    offile.write(wrbuf, strlen(wrbuf) + 1);
}

void commits2shortmsg(AttributeType &commits, AttributeType &shortmsg) {
    int last = static_cast<int>(commits.size()) - 1;
    const char *pstart;
    int len;
    char tstr[1024];
    int short_cnt = 0;
    for (int i = last; i >= 0; i--) {
        pstart = commits[i].to_string();
        LOG_printf("Processing commit: \"%s\"", pstart);
        len = 0;
        tstr[0] = 0;
        while (pstart[len]) {
            if (pstart[len] == '\\' && pstart[len + 1] == 'n') {
                pstart = &pstart[len + 2];

                AttributeType t1;
                t1.make_string(tstr);
                shortmsg.add_to_list(&t1);
                LOG_printf("    {%d} \"%s\"", short_cnt++, tstr);
                tstr[len = 0] = '\0';
            } else {
                tstr[len] = pstart[len];
                tstr[++len] = '\0';
            }
        }
        if (len) {
            AttributeType t1;
            t1.make_string(tstr);
            shortmsg.add_to_list(&t1);
            LOG_printf("    {%d} \"%s\"", short_cnt++, tstr);
        }
    }
}

int main( int argc, char* argv[] ) {
    if (argc != 2) {
        printf("Specify confugration file:\n");
        printf("    socialbot.exe ../configs/example.json\n");
        return -1;
    }

    AttributeType cfg;
    readJsonFromFile(argv[1], cfg);
    if (!cfg.is_dict() || cfg.size() == 0) {
        printf("Wrong configuration format\n");
        return -1;
    }
    LOG_create(cfg["log_file"].to_string());
    LOG_printf("[%s]", currentDateTime().c_str());
    curl_global_init(CURL_GLOBAL_ALL);

    AttributeType oldcommits, allcommits, newcommits;

    readJsonFromFile(cfg["history"].to_string(), oldcommits);
    printf("Requesting github commits:\n");
    GithubGetCommits git(cfg["github"], allcommits);

    if (!allcommits.is_list() || allcommits.size() == 0) {
        printf("error: github not available\n");
        return -1;
    }

    if (oldcommits.is_list() && oldcommits.size()) {
        newcommits.make_list(0);
        bool found = false;
        for (unsigned i = 0; i < allcommits.size(); i++) {
            const AttributeType &sha = allcommits[i]["sha"];
            for (unsigned n = 0; n < oldcommits.size(); n++) {
                if (oldcommits[n]["sha"].is_equal(sha.to_string())) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                newcommits.add_to_list(&allcommits[i]["message"]);
            } else {
                break;
            }
        }
    } else {
        newcommits.make_list(allcommits.size());
        for (unsigned i = 0; i < allcommits.size(); i++) {
            AttributeType &commit = allcommits[i]["commit"];
            newcommits[i] = commit["message"];
        }
    }
    int oldest_idx = -1;
    AttributeType shortmsg(Attr_List);
    if (newcommits.size()) {
        commits2shortmsg(newcommits, shortmsg);
    }

    LOG_printf("Total number of new commits . . . . %d", newcommits.size());
    LOG_printf("Total number of new short message . %d", shortmsg.size());

    const char *commit_msg;
    LOG_printf("%s", "Posting to telegram:");
    for (unsigned i = 0; i < shortmsg.size(); i++) {
        commit_msg = shortmsg[i].to_string();
        TelegramPost telegram(cfg["telegram"], commit_msg);
    }

    LOG_printf("%s", "Posting to twitter:");
    char tstr1[256], tstr2[256];
    int msglen;
    int twittot;
    for (unsigned i = 0; i < shortmsg.size(); i++) {
        commit_msg = shortmsg[i].to_string();
        msglen = static_cast<int>(strlen(commit_msg));
        twittot = msglen / 110 + 1;
        if (twittot > 1) { // 140 - "riscv_vhdl: (1 of 3)..." + n reserved
            AttributeType splitlst(Attr_List), t1;
            LOG_printf("Splitting twit %s:", commit_msg);
            for (int n = 0; n < twittot; n++) {
                if (msglen > 110) {
                    memcpy(tstr1, &commit_msg[110 * n], 110);
                    tstr1[110] = '\0';
                    msglen -= 110;
                } else {
                    memcpy(tstr1, &commit_msg[110 * n], msglen);
                    tstr1[msglen] = '\0';
                    msglen = 0;
                }
                if (msglen != 0) {
                    sprintf(tstr2, "(%d of %d) %s...", n + 1, twittot, tstr1);
                } else {
                    sprintf(tstr2, "(%d of %d) %s", n + 1, twittot, tstr1);
                }
                t1.make_string(tstr2);
                splitlst.add_to_list(&t1);
            }
            for (unsigned n = splitlst.size(); n > 0; n--) {
                LOG_printf("   %s", splitlst[n - 1].to_string());
                TwitterPostStatus tw(cfg["twitter"],
                                     splitlst[n - 1].to_string());
            }
        } else {
            TwitterPostStatus tw(cfg["twitter"], commit_msg);
        }
    }

    writeJsonToFile(cfg["history"].to_string(), allcommits);

    curl_global_cleanup();
    LOG_close();
    return 0;
}
