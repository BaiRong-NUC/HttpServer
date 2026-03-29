#include <curl/curl.h>
#include <iostream>
#include <string>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main()
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "libcurl init failed\n";
        return 1;
    }

    const char* url = "http://127.0.0.1:8000/predict";
    // Replace the features array with appropriate length and values
    const char* json = R"({"features": [0.1, 0.2, 0.3, 0.4]})";

    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }
    else
    {
        std::cout << "Response: " << response << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return 0;
}
