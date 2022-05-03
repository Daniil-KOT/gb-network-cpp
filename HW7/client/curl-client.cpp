#include <iomanip>
#include <iostream>
#include <string>

#include <curl/curl.h>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <host:port>" << std::endl;
    }

    CURL*    curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();

    if (curl)
    {
        std::string host = "https://" + std::string(argv[1]);
        std::string command, page;

        std::cout << "HTTP command (in capital letters): " << std::flush;
        std::getline(std::cin, command);

        std::cout << "Page starting with /: " << std::flush;
        std::getline(std::cin, page);
        std::cout << std::endl;

        host += page;

        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());

        // needed because we're using self-signed certs for a server
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        if (command == "POST")
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        }

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr,
                    "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        std::cout << std::endl;

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}