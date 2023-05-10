
#ifndef __SMSCHANNEL_HEADER__
#define __SMSCHANNEL_HEADER__

#include <string>
struct tagSMSChannel
{
	int channel_id;			// sms channel id.
	std::string name;		// name.
	std::string address;	//
	std::string userid;		//
	std::string password;	//
	std::string account;	//
	std::string content;	//
};

class CSMSChannel
{
public:
	CSMSChannel()
	{
//		memset(&m_smsChannel,0,sizeof(m_smsChannel));
	}

	virtual ~CSMSChannel()
	{
	}

public:
	static CSMSChannel& Singleton()
	{
		static CSMSChannel* sp = 0;
		if (!sp) {
			 sp = new CSMSChannel();
		}
	//Cleanup:
		return (*sp);
	}

	tagSMSChannel& getChannel()
	{
		return m_smsChannel;
	}

protected:
	tagSMSChannel m_smsChannel;
};


#endif//__SMSCHANNEL_HEADER__
