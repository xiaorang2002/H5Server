// Copyright (c) 2019, Landy
// All rights reserved.

#ifndef GLOBALFUNC_H
#define GLOBALFUNC_H


#include <stdio.h>
#include <sstream>
#include <time.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>
#include <random>
#include <boost/algorithm/string.hpp>

#include "Globals.h"
#include "mymd5.h"

using namespace std;


class GlobalFunc
{
public:
    GlobalFunc() = default;

public:
    static uint64_t RandomInt64(uint64_t min, uint64_t max);
    static uint16_t GetCheckSum(uint8_t* header, int size);

    static void SetCheckSum(internal_prev_header *header);
    static bool CheckCheckSum(internal_prev_header *header);
    static int getIP(string netName, string &strIP);
    static string Uint32IPToString(uint32_t ip);
    static uint8_t ToHex(uint8_t x);
    static uint8_t FromHex(uint8_t x);
    static string converToHex(const uint8_t* hex, int size,int from = 0);
    static string UrlEncode(const string& str);
    static string UrlDecode(const string& str);
    static void string_replace(string &strBig, const string &strSrc, const string &strDst);
    static int curlSendSMS(string url, string content, bool bJson);
    static int sendMessage2Tg(string content , string botToken ,int64_t botChairId,int32_t isDemo = 0);
    static int sendMessage2TgEx(string content ,string botToken ,int64_t botChairId,int32_t isDemo = 0);
    static string clearDllPrefix(string dllName);
    static void trimstr(string & sourceStr);
    static void replaceChar(string &srcStr, char mark);
    static int64_t GetCurrentStamp64(bool ismicrosec = true);
    static bool cmpValue(const pair<string, int32_t> &a, const pair<string, int32_t> &b)
    {
        return a.second > b.second;
    }
    static string generateUUID();
    static string createRecordId();

    //判断是否数字或字符组成的字符串 ///
    static bool IsDigit(std::string const &str);
    //判断是否字符组成的字符串 ///
    static bool IsLetter(std::string const &str);

    static bool readWithoutNameJsonArrayString(std::string strWithNameJson, std::string & strWithoutNameJson);
    
    // 获取两位小数点字符串
    static string getDigitalFormat(int64_t score){
        	// 1.1数字转小数点，保留小数点后两位
            int64_t tmp2 = score % 100,tmp1 = score/100;
            return to_string(tmp1) + "." + to_string(tmp2 / 10) + to_string(tmp2 % 10);
    }
    // curl回调
    static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) 
    {
        string data((const char*) ptr, (size_t) size * nmemb);
        *((stringstream*) stream) << data << endl;
        return size * nmemb;
    }
	// 创建订单号
    static string getBillNo(int64_t uid);
    static string getLast4No(int64_t uid);

    //截取double小数点后2位，直接截断并乘以100转int64_t
    static int64_t rate100(std::string const &s)
    {
        std::string::size_type npos = s.find_first_of('.');
        if (npos != std::string::npos)
        {
            std::string prefix = s.substr(0, npos);
            std::string sufix = s.substr(npos + 1, -1);
            std::string x;
            if (!sufix.empty())
            {
                if (sufix.length() >= 2)
                {
                    x = prefix + sufix.substr(0, 2);
                }
                else
                {
                    x = prefix + sufix.substr(0, 1) + "0";
                }
            }
            return atoll(x.c_str());
        }
        return atoll(s.c_str()) * 100;
    }
    // @Digit16 32位 0，16位，1;upper 0 小写，1 大写
    static string getMd5Encode(string srcStr, int32_t Digit16 = 0, int upper = 0)
    {
        std::string strMd5;
        char md5[32 + 1] = {0};
        MD5Encode32(srcStr.c_str(), srcStr.length(), md5, upper);
        strMd5 = string((char *)md5);
        if(Digit16 == 1)
            strMd5 = strMd5.substr(8, 8);//16); //取中间的第 9 位到第 24 位的部分
        else  if(Digit16 == 2)
            strMd5 = strMd5.substr(8, 6);

        return strMd5;
    }
    static string InitialConversion(int64_t timeSeconds)
    {
        time_t rawtime = (time_t)timeSeconds;
        struct tm *timeinfo;
        timeinfo = localtime(&rawtime);

        int Year = timeinfo->tm_year + 1900;
        int Mon = timeinfo->tm_mon + 1;
        int Day = timeinfo->tm_mday;
        int Hour = timeinfo->tm_hour;
        int Minuts = timeinfo->tm_min;
        int Seconds = timeinfo->tm_sec;
        string strYear;
        string strMon;
        string strDay;
        string strHour;
        string strMinuts;
        string strSeconds;

        if (Seconds < 10)
            strSeconds = "0" + to_string(Seconds);
        else
            strSeconds = to_string(Seconds);
        if (Minuts < 10)
            strMinuts = "0" + to_string(Minuts);
        else
            strMinuts = to_string(Minuts);
        if (Hour < 10)
            strHour = "0" + to_string(Hour);
        else
            strHour = to_string(Hour);
        if (Day < 10)
            strDay = "0" + to_string(Day);
        else
            strDay = to_string(Day);
        if (Mon < 10)
            strMon = "0" + to_string(Mon);
        else
            strMon = to_string(Mon);

        return to_string(Year) + "-" + strMon + "-" + strDay + " " + strHour + ":" + strMinuts + ":" + strSeconds;
    }
    //按照占位符来替换 ///
    static void replace(string& json, const string& placeholder, const string& value) {
        boost::replace_all<string>(json, "\"" + placeholder + "\"", value);
    }
};

#endif // GLOBALFUNC_H
