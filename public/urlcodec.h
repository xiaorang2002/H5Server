#ifndef URLCODEC_H
#define URLCODEC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <unistd.h>
#include <iconv.h>

namespace URL {
	std::string Encode(const std::string& str);
	int32_t Decode(const std::string& str,std::string& strResult);
	std::string MultipleDecode(const std::string& str);

	std::string url_encode(const std::string &d);
	std::string url_decode(const std::string &e); 
  
	bool hasUrlEncoded(std::string str);
}



#endif