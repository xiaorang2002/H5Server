#include <stdio.h>
#include <iostream>
#include <sstream>
 
#include <cpr/cpr.h>

using namespace std;

int main(int argc, char *argv[])
{
    // 基于当前系统的当前日期/时间
    time_t now = time(0);
    char *dt = ctime(&now);
    cout << dt << "------------------1-------------------" << endl;

    string _url = "http://www.httpbin.org/get";
    // string _url = "http://192.168.2.95:9090/GameHandle?testAccount=text_97088306&agentid=10000";
    /*
    auto req = cpr::Get(cpr::Url{_url},cpr::Timeout{1000});
    if (req.status_code >= 400)
    {
        std::cerr << "Error [" << req.status_code << "] making request" << std::endl;
    }
    else
    {
        std::cout << "Request took " << req.elapsed << std::endl;
        std::cout << "Body:" << std::endl
                  << req.text;
    }
    */

    auto future_text = cpr::GetCallback([](cpr::Response r) {

        std::cout << " == " << r.text << " == " << std::endl;
        std::cout << " == " << r.elapsed << " == " << std::endl;

        return r.text;
    },cpr::Url{_url}); //,cpr::Timeout{100}

    // Sometime later
    if (future_text.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        std::cout << future_text.get() << std::endl;
    }

    cout << dt << "------------------3-------------------" << endl;

    return 0;

    auto url = cpr::Url{_url};
    cpr::AsyncResponse future = cpr::GetAsync(url); //
    auto expected_text = std::string{"Hello world!"};
    auto response = future.get();

    cout << "text:" << response.text << endl;
    cout << "url:" << response.url << endl;
    cout << "status_code:" << response.status_code << endl;
    cout << "content-type:" << response.header["content-type"] << endl;
    // EXPECT_EQ(expected_text, response.text);
    // EXPECT_EQ(url, response.url);
    // EXPECT_EQ(std::string{"text/html"}, response.header["content-type"]);
    // EXPECT_EQ(200, response.status_code);


    now = time(0);
    dt = ctime(&now);
    cout << dt << "-----------------2--------------------" << endl;

    return 0;
}



 