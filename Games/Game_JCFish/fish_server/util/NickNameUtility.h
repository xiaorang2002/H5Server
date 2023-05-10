#pragma once

#include <string>

class NickNameUtility {
public:
        static LPCTSTR convertNickName(LPCTSTR nickName, char* name, int bufsize)
        {
            std::string temp;
                if (nickName)
                {
                    temp = nickName;
                    if (temp.length() > 0)
                    {
                        if (temp.length() <= 2) {
                            temp += "********";
                            temp = temp.substr(0,8);
                            lstrcpyn(name, temp.c_str(), bufsize);
                        }
                        else if (temp.length() <= 7)
                        {
                            std::string temp1 = temp.substr(0,1);
                            std::string temp2 = temp.substr(temp.length()-1,1);
                            temp = temp1 + "******" + temp2;
                            lstrcpyn(name, temp.c_str(), bufsize);
                        }
                        else
                        {
                            std::string temp1 = temp.substr(0,2);
                            std::string temp2 = temp.substr(temp.length()-2,2);
                            temp = temp1 + "****" + temp2;
                            lstrcpyn(name, temp.c_str(), bufsize);
                        }

                        return name;
                    }
                }

                return nickName;
	}
};
