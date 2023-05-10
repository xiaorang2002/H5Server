#pragma once

#include "./RobotAI/ddz_state.h"
#include "./RobotAI/mcts_node.h"
#include "./RobotAI/SLua.h"


enum pai_interface_enum
{
    pai_i_type_none					= 0,
    pai_i_type_3					= 3,
    pai_i_type_4					= 4,
    pai_i_type_5					= 5,
    pai_i_type_6					= 6,
    pai_i_type_7					= 7,
    pai_i_type_8					= 8,
    pai_i_type_9					= 9,
    pai_i_type_10					= 10,
    pai_i_type_J					= 11,
    pai_i_type_Q					= 12,
    pai_i_type_K					= 13,
    pai_i_type_A					= 14,
    pai_i_type_2					= 15,
    pai_i_type_blackjack			= 16,
    pai_i_type_blossom				= 17,
    pai_i_type_max					= 18,

    pai_num_max						= 20,
};

enum move_interface_enum
{
    ddz_type_i_no_move				= 0,			// 不出牌
    ddz_type_i_alone_1				= 1,			// 单张
    ddz_type_i_pair					= 2,			// 连对
    ddz_type_i_triple				= 3,			// 三张
    ddz_type_i_triple_1				= 4,			// 三带一
    ddz_type_i_triple_2				= 5,			// 三带二
    ddz_type_i_order				= 6,			// 顺子
    ddz_type_i_order_pair			= 7,			// 连对
    ddz_type_i_airplane				= 8,			// 飞机
    ddz_type_i_airplane_with_pai	= 9,			// 飞机带牌
    ddz_type_i_bomb					= 10,			// 炸弹
    ddz_type_i_king_bomb			= 11,			// 王炸
    ddz_type_i_four_with_alone1		= 12,			// 4with2 二张单牌
    ddz_type_i_four_with_pairs		= 13,			// 4with2 二对对子
};

struct pai_interface_move
{
    uint8_t						_type;
    uint8_t						_alone_1;
    uint8_t						_alone_2;
    uint8_t						_alone_3;
    uint8_t						_alone_4;
    uint8_t						_airplane_pairs;

    uint8_t						_combo_list[20];
    uint8_t						_combo_count;
};

class CRobot
{
public:
    CRobot();
    // 设置牌信息
    void						SetDesk(uint8_t seat, uint8_t hands[3][pai_i_type_max], uint8_t grab[3], uint32_t hands_r[3][pai_num_max], uint32_t grab_r[3]);

    // 设置地主并且设置底牌
    void						SetLandlord(uint8_t seat);

    // 重置
    void						Reset();

    // 叫地主
    int							GrabLandlord();

    // 叫地主变态版
    int							GrabLandlordEX();

    // 通知出牌
    SVar						PlayCard();

    SVar						HelpPlayCard(ddz_state &state);

    // 重新设置牌
    void						FreshCards(uint8_t seat, vector<ACard> &cd, uint8_t px);

    SVar						search_cards(int seat, pai_interface_move &ptn);

    void						remove_played_cards(int seat, pai_interface_move &ptn);

private:
    uint8_t						_hands[3][pai_i_type_max];	// 牌值+牌数目
    uint32_t					_hands_real[3][pai_num_max];// 具体牌
    uint8_t						_grab[3];					// 底牌
    uint32_t					_grab_real[3];				// 真实牌
    uint8_t						_seat;						// 我的座位
    uint8_t						_landlord_seat;				// 庄家的座位
    uint8_t						_cur_desk_owner;			// 上次出牌对象
    pai_interface_move			_cur_desk_cards;			// 当前桌子上的牌
};

class RobotVM
{
public:
    RobotVM();
    void						SC_CODDZ_FAPAI(int nSeat, LONGLONG playerId, SVar &s);
    void						SC_CODDZ_SET_DIZHU(int nSeat, LONGLONG playerId, SVar &s);
    int							SC_CODDZ_JIAODIZHU_NOTIFY(int nSeat, LONGLONG playerId);
    SVar						SC_CODDZ_REQUEST_CHUPAI(int nSeat, LONGLONG playerId);
    void						SC_CODDZ_CHUPAI_END(int nSeat, LONGLONG playerId, SVar &s);
    void						SC_CODDZ_SET_STATE(LONGLONG playerId, int state);
    int							SC_CODDZ_PLAYER_ADD_MUTI_NOTIFY();

    void						UNIT_TEST();

private:
    map<LONGLONG, CRobot*>			m_mapRobot;
};
