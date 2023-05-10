#include "GlobalFunc.h"


#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>

#include <stdio.h>
#include <sstream>
#include <time.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>
#include <regex>
#include <dlfcn.h>

#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"
#include <boost/regex.hpp>

#include <curl/curl.h>
#include "cpr/cpr.h"
// 创建注单记录ID
string GlobalFunc::createRecordId()
{
    string srcStr = generateUUID();
    replaceChar(srcStr,'-');
    return srcStr;
}

// 创建UUID
string GlobalFunc::generateUUID()
{
    boost::uuids::random_generator rgen;
    boost::uuids::uuid u = rgen();
    string tmp_uuid = boost::uuids::to_string(u);
    return tmp_uuid;
}
// 创建订单号
string GlobalFunc::getBillNo(int64_t uid)
{
    return to_string(GlobalFunc::GetCurrentStamp64(true)) + GlobalFunc::getLast4No(uid);
}
// 创建订单号
// 取玩家UID后4位，不足则随机补足
string GlobalFunc::getLast4No(int64_t uid){
	stringstream ss;
	string struid = to_string(uid);
	int32_t length = struid.length();
    const int32_t fixLen = 4;
	for (int32_t i = 0; i < fixLen; i++){ //length
		char c;
		// 长度不够，则随机
		if((length < fixLen)  && ( i < (fixLen - length))){
			c = rand() % 10 + 48;
		}
		else {
            c =  struid[length - fixLen + i];
            // c =  (length >= fixLen) ? struid[length - fixLen + i] : struid[i - (fixLen - length)];
		}

		ss << c;
		// LOG_INFO << "---struid["<< uid <<"],c["<< c <<"],i["<< i <<"]"; 
	}
	
	return ss.str();
}

int64_t GlobalFunc::GetCurrentStamp64(bool ismicrosec)
{
    // muduo::Timestamp start = muduo::Timestamp::now();
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1)); //boost::gregorian::Jan
    boost::posix_time::time_duration time_from_epoch;

    if (ismicrosec)
    {
        time_from_epoch = boost::posix_time::microsec_clock::universal_time() - epoch;
        return time_from_epoch.total_microseconds();
    }
    else
    {
        time_from_epoch = boost::posix_time::second_clock::universal_time() - epoch;
        return time_from_epoch.total_seconds();
    }
}

uint64_t GlobalFunc::RandomInt64(uint64_t min, uint64_t max)
{
//    static std::random_device rd;
//    static std::mt19937 gen(rd());
//    std::uniform_int_distribution<> dis(min,  max);
//    return dis(gen);
    if(min>max)
        return max+rand()%(min-max+1);
     else
        return min+rand()%(max-min+1);

}

uint16_t GlobalFunc::GetCheckSum(uint8_t* header, int size)
{
    uint16_t sum = 0;
    uint16_t *p = (uint16_t*)header;
    for(int i = 0; i < size / 2; ++i)
        sum += *p++;
    if(size % 2)
        sum+=*(uint8_t*)p;
    return sum;
}

void GlobalFunc::SetCheckSum(internal_prev_header *header)
{
    uint16_t sum = 0;
    uint16_t *p = (uint16_t*)header;
    for(int i = 0; i < sizeof(internal_prev_header) / 2 - 1; ++i)
        sum += *p++;
    *p = sum;
}

void GlobalFunc::trimstr(string & sourceStr)
{
    if(!sourceStr.empty())
    {
        sourceStr.erase(0,sourceStr.find_first_not_of(" "));
        sourceStr.erase(sourceStr.find_last_not_of(" ") + 1);
    }
}
// void GlobalFunc::replaceChar(string &srcStr, char mark)
// {
//     std::string::size_type startpos = 0;
//     while (startpos != std::string::npos)
//     {
//         startpos = srcStr.find(mark);        //找到'\n'的位置
//         if (startpos != std::string::npos) //std::string::npos表示没有找到该字符
//         {
//             srcStr.replace(startpos, 1, ""); //实施替换，注意后面一定要用""引起来，表示字符串
//         }
//     }
// }

void GlobalFunc::replaceChar(string &srcStr, char mark)
{
    std::string::size_type startpos = srcStr.find(mark);
    while (startpos != std::string::npos)
    {
        srcStr.replace(startpos, 1, ""); //实施替换，注意后面一定要用""引起来，表示字符串
        startpos = srcStr.find(mark);        //找到'\n'的位置
    }
}


bool GlobalFunc::CheckCheckSum(internal_prev_header *header)
{
    uint16_t sum = 0;
    uint16_t *p = (uint16_t*)header;
    for(int i = 0; i < sizeof(internal_prev_header) / 2 - 1; ++i)
        sum += *p++;
    return *p == sum;
}

int GlobalFunc::getIP(string netName, string &strIP)
{
#define BUF_SIZE 1024
    int sock_fd;
    struct ifconf conf;
    struct ifreq *ifr;
    char buff[BUF_SIZE] = {0};
    int num;
    int i;

    sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if ( sock_fd < 0 )
        return -1;

    conf.ifc_len = BUF_SIZE;
    conf.ifc_buf = buff;

    if ( ioctl(sock_fd, SIOCGIFCONF, &conf) < 0 )
    {
        close(sock_fd);
        return -1;
    }

    num = conf.ifc_len / sizeof(struct ifreq);
    ifr = conf.ifc_req;

    for(i = 0; i < num; i++)
    {
        struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);

        if ( ioctl(sock_fd, SIOCGIFFLAGS, ifr) < 0 )
        {
                close(sock_fd);
                return -1;
        }

        if ( (ifr->ifr_flags & IFF_UP) && strcmp(netName.c_str(),ifr->ifr_name) == 0 )
        {
                strIP = inet_ntoa(sin->sin_addr);
                close(sock_fd);

                return 0;
        }

        ifr++;
    }

    close(sock_fd);

    return -1;
}

string GlobalFunc::Uint32IPToString(uint32_t ip)
{
    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;

    string strIP = inet_ntoa(addr.sin_addr);
    return strIP;
}

//URLEncode  URLDecode
uint8_t GlobalFunc::ToHex(uint8_t x)
{
    return  x > 9 ? x + 55 : x + 48;
}

uint8_t GlobalFunc::FromHex(uint8_t x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    //else
    //    assert(0);
    return y;
}

string GlobalFunc::converToHex(const uint8_t* hex, int size,int from)
{
    string strValue;
    char buf[32]={0};
    for(int i=from;i<size+from;i++)
    {
        snprintf(buf,sizeof(buf),"%02X", (unsigned char)hex[i]);
        strValue += buf;
    }
//Cleanup:
    return strValue;
}

string GlobalFunc::UrlEncode(const string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~') ||
            (str[i] == '=') ||
            (str[i] == '!') ||
            (str[i] == '*') ||
            (str[i] == '\'') ||
            (str[i] == '(') ||
            (str[i] == ')') ||
            (str[i] == '&')  )
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

string GlobalFunc::UrlDecode(const string& str)
{
    string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
            //assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

void GlobalFunc::string_replace(string &strBig, const string &strSrc, const string &strDst)
{
    std::string::size_type pos = 0;
    std::string::size_type srcLen = strSrc.size();
    std::string::size_type dstLen = strDst.size();

    while( (pos=strBig.find(strSrc, pos)) != std::string::npos )
    {
        strBig.replace( pos, srcLen, strDst );
        pos += dstLen;
    }
}

int GlobalFunc::curlSendSMS(string url, string content, bool bJson)
{
    CURL *curl;
    CURLcode code, res;

//    content = UrlEncode(content);
//    LOG_INFO << "CONTENT Encode:"<<content;

    struct curl_slist* headers = NULL;
    if(bJson)
        headers = curl_slist_append(headers, "Content-Type:application/json");
    headers = curl_slist_append(headers, "charset=utf-8");

    code = curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 6);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return res;
}

// 发送机器人消息2
int GlobalFunc::sendMessage2TgEx(string content ,string  botToken ,int64_t botChairId,int32_t isDemo)
{
    try
    {
        int64_t timestart = time(nullptr);

        stringstream ss_url;
        ss_url << "https://api.telegram.org/bot" << botToken << "/sendMessage";
        string url = ss_url.str();

        std::cout << "================================================" << std::endl;
        std::cout << url << std::endl;
        std::cout << content << std::endl;
        std::cout << "size:" << content.length() << std::endl;
        std::cout << "------------------------------------------------" << std::endl;

        std::vector<cpr::Pair> payloadData;
        // payloadData.push_back({"chat_id", to_string(botChairId)});
        // payloadData.push_back({"text", content});

        //  auto payload = Payload{{"x", "1"}};
        // payloadData.push_back(cpr::Payload{{"chat_id", to_string(botChairId)}});
        // payloadData.push_back(cpr::Payload{{"text", content}});
        // auto response = cpr::Post(cpr::Url{url}, cpr::Payload(payloadData.begin(), payloadData.end()));

        // std::cout << "elapsed:" << response.elapsed << endl;
        // std::cout << "status_code:" << response.status_code << endl;
        // std::cout << "elapsed time:" << time(nullptr) - timestart << std::endl;
        // std::cout << "text:" << response.text << endl;
        std::cout << "================================================" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "---sendMessage2TgEx Error," << e.what() << " " << __FUNCTION__ << '\n';
    }
    
    return 0;
}

// 发送机器人消息
int GlobalFunc::sendMessage2Tg(string content ,string  botToken ,int64_t botChairId,int32_t isDemo)
{ 
    int ret = 0;
    int64_t timestart = time(nullptr);
    content = "chat_id="+ to_string(botChairId) +"&text=" + content;

    stringstream ss_url;
	ss_url << "https://api.telegram.org/bot" << botToken <<"/sendMessage";
    string url = ss_url.str();

    std::cout << "================================================" << std::endl;
    std::cout << url << std::endl;
    std::cout << content << std::endl;
    std::cout << "size:" << content.length() << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type:application/x-www-form-urlencoded;charset=utf-8"); 

    std::stringstream out;
    void *curl = curl_easy_init();
    // 设置URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
    // 设置接收数据的处理函数和存放变量
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    //是否显示头
    // curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    // 版本
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    // 获取当前版本
    // curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);
    // fprintf(stderr, "    curl V%s loaded\n", data->version); //curl V7.29.0 loaded

    // 执行HTTP GET操作
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        ret = -1;
    }
    curl_easy_cleanup(curl);

    // 接受数据存放在out中，输出之
    std::cout << "elapsed time:" << time(nullptr) - timestart << std::endl;
    std::cout << "response text:" << out.str() << std::endl;
    std::cout << "================================================" << std::endl;
    return ret;
}

string GlobalFunc::clearDllPrefix(string dllname)
{
    // drop the special ./ prefix path.
    size_t nprefix = dllname.find("./");
    if (0 == nprefix)
    {
        dllname.replace(nprefix,2,"");
    }

    // drop the lib prefix name now.
    nprefix  = dllname.find("lib");
    if (0 == nprefix)
    {
        dllname.replace(nprefix, 3, "");
    }

    // drop the .so extension name.
    nprefix = dllname.find(".so");
    if (std::string::npos != nprefix)
    {
        dllname.replace(nprefix,3,"");
    }

    return (dllname);
}

bool GlobalFunc::IsDigit(std::string const &str)
{
    boost::regex reg("^[0-9]+$");
    return boost::regex_match(str, reg);
}

//判断是否字符组成的字符串 ///
bool GlobalFunc::IsLetter(std::string const &str)
{
    boost::regex reg("^[a-zA-Z]+$");
    return boost::regex_match(str, reg);
}



//带名字数组jons字符串转换成空数组json字符串
bool GlobalFunc::readWithoutNameJsonArrayString(std::string strWithNameJson, std::string & strWithoutNameJson)
{
	if (strWithNameJson.size() == 0)
	{
		return false;
	}
	strWithoutNameJson = strWithNameJson;
	boost::iterator_range<std::string::iterator> result = boost::algorithm::find_first(strWithoutNameJson, "[");
	if (!result.empty())
	{
		std::string strTemp = strWithoutNameJson.substr(result.begin() - strWithoutNameJson.begin());
		strWithoutNameJson = strTemp;
		result = boost::algorithm::find_last(strWithoutNameJson, "]");
		//result = boost::algorithm::ifind_first(strWithoutNameJson, "]");
		if (!result.empty())
		{
			strTemp = strWithoutNameJson.substr(0, (result.begin() - strWithoutNameJson.begin() + 1));
			strWithoutNameJson = strTemp;
		}
	}
	return true;
}

// boost::regex reg("^[0-9]+([.]{1}[0-9]+){0,1}$");
// 		return boost::regex_match(str, reg);
