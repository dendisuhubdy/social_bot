#include "twitterbot.h"
#include "githubbot.h"
#include <fstream>

//libcurl.lib;ws2_32.lib;wldap32.lib;advapi32.lib;kernel32.lib;comdlg32.lib
//curl_easy_setopt(curl_handle, CURLOPT_DEBUGFUNCTION, my_trace)
//curl_easy_setopt(curl_handle, CURLOPT_STDERR, my_file_stream)

int main( int argc, char* argv[] ) {
    if (argc != 2) {
        printf("Specify confugration file:\n");
        printf("    socialbot.exe ../configs/example.json\n");
        return -1;
    }

    std::ifstream iscfg(argv[1]);
    if (!iscfg.is_open()) {
        printf("File '%s' not found\n");
        return -1;
    }

    iscfg.seekg(0, std::ifstream::end);
    int cfgsz = static_cast<int>(iscfg.tellg());
    iscfg.seekg(0, std::ifstream::beg);
    char *cfgbuf = new char[cfgsz + 1];
    iscfg.read(cfgbuf, cfgsz);
    cfgbuf[cfgsz] = '\0';

    AttributeType cfg(Attr_Dict);
    cfg.from_config(cfgbuf);
    delete [] cfgbuf;

    if (!cfg.is_dict()) {
        printf("Wrong configuration format\n");
        return -1;
    }


    curl_global_init(CURL_GLOBAL_ALL);

    AttributeType newcommits;

#if 1
    printf("Requesting github commits:\n");

    GithubGetCommits git(cfg["github"], newcommits);
    for (unsigned i = 0; i < newcommits.size(); i++) {
        AttributeType &commit = newcommits[i]["commit"];
        const char *author = commit["author"]["name"].to_string();
        const char *message = commit["message"].to_string();
        printf("[%d] %s\n:    %s\n", i, author, message);
    }
#endif


    printf("Post twitter status:\n");
    for (int i = 0; i < 3; i++) {
        TwitterPostStatus tw(cfg["twitter"], "GIT: test #5\n!!");
    }

    curl_global_cleanup();
    return 0;
}
