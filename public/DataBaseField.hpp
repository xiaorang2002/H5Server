
#ifndef DBFIELD_H
#define DBFIELD_H

#include <string> 

using namespace std;
 
#define GAMEMAIN    "gamemain"
#define GAMECONFIG  "gameconfig"
#define GAMELOG     "gamelog"

/***************************************************************/
// gamemain库
/***************************************************************/
namespace gamemain
{

class user_login_check_list
{ 
public:
    static string self;
    static string loginip;
    static string CONST;
};

string user_login_check_list::self    = "user_login_check_list";
string user_login_check_list::loginip = "loginip";

std::string user_login_check_list::CONST("str");
}
/***************************************************************/
// gameconfig库
/***************************************************************/
namespace gameconfig
{
//game_kind 集合
class game_kind
{
public:
    static string self;
    static string gameid;
};

string game_kind::self      = "game_kind";
string game_kind::gameid    = "gameid";

}

/***************************************************************/
// gamelog库
/***************************************************************/
namespace gamelog
{
//game_replay 集合
class game_replay
{
public:
    static string self;
    static string roomname;
};

string game_replay::self        = "game_replay";
string game_replay::roomname    = "roomname";



//match_game_replay 集合
class match_game_replay
{ 
public:
    static string self;
    static string roomname;
};

string match_game_replay::self        = "game_replay";
string match_game_replay::roomname    = "roomname";

}
#endif // DBFIELD_H
