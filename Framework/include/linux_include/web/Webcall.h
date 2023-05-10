#ifndef __WEBCALL_HEADER__
#define __WEBCALL_HEADER__

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#ifndef WEB_OK
#define WEB_OK	(200)
#endif//WEB_OK

#ifdef  _WIN32
#pragma comment(lib,"ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define _strdup		strdup
#define closesocket 	close
#define INVALID_SOCKET	(-1)
#endif//_WIN32

using namespace std;
class CWebCall
{
public:
	CWebCall()
	{
#ifdef  _WIN32
		WSADATA wsaData = { 0 };
		WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif//_WIN32
	}

	~CWebCall()
	{
#ifdef  _WIN32
		WSACleanup();
#endif//_WIN32
	}

	CWebCall(string host, string queryString)
	{
#ifdef  _WIN32
		WSADATA wsaData = { 0 };
		WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif//_WIN32
		webCall(host, queryString);
	}

public:
	int webCall(std::string url)
	{
		int status = -1;
		do
		{
			// try to format the special url content now.
			size_t pos = url.find("http://");
			if (pos != string::npos) {
				url = url.substr(7, url.length() - 1);
			}

			string host = "", file="/", queryString = "";
			// try to split the special url with host and queryString.
			std::vector<string> tokens;
			splits(tokens, url, "?");
			if (tokens.size() >= 1)
			{
				host = tokens[0];
				pos = host.rfind('/');
				if ((pos != 0) &&
					(pos != host.length()-1) &&
					(pos != string::npos)) {
					file = host.substr(pos, host.length() - 1);
					host = host.substr(0, pos);
				}
			}

			// try to clear the special '\r' char from the special response data now.
			host.erase(remove_if(host.begin(), host.end(), hostisspace), host.end());
			// try to get query string.
			if (tokens.size() >= 2) {
				queryString = tokens[1];
			}

			// try to call the special web host value.
			status = webCall(host, queryString, file);

		} while (0);
	//Cleanup:
		return (status);
	}

	// try to call the special remote host content value for later user content item data value now.
	int webCall(const std::string host, const std::string queryString, const std::string file="/")
	{
		int status = -1;
		int sock = INVALID_SOCKET;
		do
		{
			struct sockaddr_in addr = {0};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(80);
			addr.sin_addr.s_addr = inet_addr(host.c_str());
			// try to create the special socket now.
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (INVALID_SOCKET == sock) {
				status = -1001;
				break;
			}

			// try to connect the special remote host item data now.
			int err = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
			if (err != S_OK) {
				status = err;
				break;
			}

			// build the full content.
			request = "GET " + file;
			if (queryString.length()) {
				request += "?";
				request += queryString;
			}

			// build the full header info.
			request += " HTTP/1.1\r\nHost:";
			request += host;
			request += "\r\nConnection:Close\r\n\r\n";

			response.clear();
			printf("request:\n%s\n\n", request.c_str());
			int ret = send(sock, request.c_str(), request.length(), 0);
			// try to receive the special content item data now.
			char buf[4096] = { 0 };
			int len = recv(sock, buf, sizeof(buf) - 1, 0);
			while (len > 0) {
				response += buf;
				len = recv(sock, buf, sizeof(buf) - 1, 0);
			}

			// try to clear the special '\r' char from the special response data content value now.
			response.erase(remove_if(response.begin(), response.end(), myisspace), response.end());
			// all data ok.
			status = S_OK;
		} while (0);
	//Cleanup:
		if (INVALID_SOCKET != sock) closesocket(sock);
		return (status);
	}

public:// get response.
	string getReponse()
	{
		// get response.
		return response;
	}

	// get the content.
	string getContent()
	{
		string content;
		do
		{
			// try to get the special response content now.
			size_t pos = response.find("Content-Length");
			if (pos != string::npos) {
				string temp = response.substr(pos, response.length() - 1);
				pos = temp.find("\n\n");
				if (pos != string::npos)
				{
					content = temp.substr(pos+2, temp.length() - 1);
				}
			}

		} while(0);
	//Cleanup:
		return (content);
	}

	// try to get the special response code.
	int getStatusCode()
	{
		int status = -1;
		do
		{
			size_t npos = response.find('\n');
			if (npos != string::npos) {
				std::string first = response.substr(0, npos);
				std::vector<string> vec;
				splits(vec,first, " ");
				if (vec.size() >= 2) {
					status = (int)strtod(vec[1].c_str(),NULL);
				}
			}
		} while (0);
	//Cleanup:
		return (status);
	}

protected:
	

	// check is space char item.
	static int myisspace(int _C)
	{
		if (_C == '\r')
			return 1;
		return 0;
	}

	// try to check the host char.
	static int hostisspace(int _C)
	{
		if (_C == '/')
			return 1;
		return 0;
	}

	// try to split the special content value for data content data value item now.
	int splits(std::vector<std::string>& vec, std::string instr, const char* limit)
	{
		int status = -1;
		char* str = _strdup(instr.c_str());
		do
		{
			vec.clear();
			char *token = strtok(str, limit);
			while (token) {
				// printf("token:[%s]\n",token);
				vec.push_back(token);
				token = strtok(NULL, limit);
			}

		} while (0);
	//Cleanup:
		if (str) free(str);
		return (status);
	}

protected:
	std::string request;
	std::string response;
};


#endif//__WEBCALL_HEADER__
