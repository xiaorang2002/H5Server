#ifndef BASE64_PUBLIC_H
#define BASE64_PUBLIC_H

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);
#endif