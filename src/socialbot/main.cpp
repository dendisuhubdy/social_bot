#include "twitterbot.h"
#include "githubbot.h"
#include "telegrambot.h"
#include <fstream>

#define HISTORY_FILENAME "history.json"

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
    curl_global_init(CURL_GLOBAL_ALL);

    AttributeType oldcommits, allcommits, newcommits;

    readJsonFromFile(HISTORY_FILENAME, oldcommits);
    printf("Requesting github commits:\n");
    GithubGetCommits git(cfg["github"], allcommits);

    if (!allcommits.is_list() || allcommits.size() == 0) {
        printf("error: github not available\n");
        return -1;
    }

    if (oldcommits.is_list() && oldcommits.size()) {
        newcommits.make_list(0);
    } else {
        newcommits.make_list(allcommits.size());
        for (unsigned i = 0; i < allcommits.size(); i++) {
            AttributeType &commit = allcommits[i]["commit"];
            newcommits[i] = commit["message"];
        }
    }
    printf("Total number of new commits . . . %d\n", newcommits.size());


    printf("Posting to telegram:\n");
    AttributeType x1;
    TelegramPost telegram(cfg["telegram"], x1);

    int oldest_idx = -1;
    if (newcommits.size()) {
        //oldest_idx = static_cast<int>(newcommits.size() - 1);
        oldest_idx = 0;
    }
    printf("Posting to twitter . . . %d commtis\n", oldest_idx + 1);
    for (int i = oldest_idx; i >= 0; i--) {
        TwitterPostStatus tw(cfg["twitter"], newcommits[i].to_string());
    }

    writeJsonToFile(HISTORY_FILENAME, allcommits);

    curl_global_cleanup();
    return 0;
}
