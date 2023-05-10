
#include <iostream>
#include <stdlib.h>
//#include "GameLogic.h"
//#include "GameTable.h"

#ifdef WIN32
#include <windows.h>
#define xsleep(t) Sleep(t*1000)
#define clscr() system("cls")
#else
#include <unistd.h>
#define xsleep(t) sleep(t)
#define clscr() system("reset")
#endif

#include "proto/Game.Common.pb.h"
#include "proto/GameServer.Message.pb.h"
#include "proto/s13s.Message.pb.h"
#include "public/gameDefine.h"
#include "s13s.h"

int main()
{
	S13S::CGameLogic::TestEnumCards("s13s_cardList.ini");
	//S13S::CGameLogic::TestPlayerCards();
	//S13S::CGameLogic::TestProtoCards();
	//S13S::CGameLogic::TestManualCards();



// 	::GameServer::MSG_S2C_PlayInOtherRoom otherroom;
// 	::Game::Common::Header* header = otherroom.mutable_header();
// 	header->set_sign(HEADER_SIGN);
// 
// 	//otherroom.set_gameid(0);
// 	//otherroom.set_roomid(0);
// 
// 	size_t bytesize = otherroom.ByteSizeLong();
// 	std::vector<unsigned char> data(bytesize);
// 	if (otherroom.SerializeToArray(&data[0], bytesize)) {
// 		
// 	}

	//S13S::CGameLogic::TestCompareCards();


	return 0;
}
