
#include <types.h>
#include <stdio.h>
#include <web/Webcall.h>

int main()
{
	std::string url = "http://192.168.0.201/index.asp?name=test2&age=37";
//	std::string url = "127.0.0.1/?name=test2&age=37";
	std::string response = "test";

	CWebCall web;
	int status = web.webCall(url);
	response = web.getReponse();

	string content = "";
	int code = web.getStatusCode();
	if (WEB_OK == code) {
		content = web.getContent();
	}

	// try to 
	// printf("response:\n%s\n\n", response.c_str());
	printf("response code:[%d], content:[%s]\n", code, content.c_str());

	return 0;
}
