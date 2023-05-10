#ifndef __DDZ_MOVE_H__
#define __DDZ_MOVE_H__

#include "mcts_move.h"
#include <vector>

enum ddz_enum
{
	ddz_type_no_move			= 0,			// 不出牌
	ddz_type_alone_1			= 1,			// 单张
	ddz_type_pair				= 2,			// 连对
	ddz_type_triple				= 3,			// 三张
	ddz_type_triple_1			= 4,			// 三带一
	ddz_type_triple_2			= 5,			// 三带二
	ddz_type_order				= 6,			// 顺牌
	ddz_type_order_pair			= 7,			// 连对
	ddz_type_airplane			= 8,			// 单独飞机
	ddz_type_airplane_with_pai	= 9,			// 飞机带二张
	ddz_type_bomb				= 10,			// 炸弹
	ddz_type_king_bomb			= 11,			// 王炸
	ddz_type_four_with_alone_1  = 12,			// 四带二张单牌
	ddz_type_four_with_pairs    = 13,			// 四带二张对子
};

enum pai_enum
{
	pai_type_none				= 0,
	pai_type_3					= 3,
	pai_type_4					= 4,
	pai_type_5					= 5,
	pai_type_6					= 6,
	pai_type_7					= 7,
	pai_type_8					= 8,
	pai_type_9					= 9,
	pai_type_10					= 10,
	pai_type_J					= 11,
	pai_type_Q					= 12,
	pai_type_K					= 13,
	pai_type_A					= 14,
	pai_type_2					= 15,
	pai_type_blackjack			= 16,			// 小王
	pai_type_blossom			= 17,			// 大王
	pai_type_max				= 18,

};

class ddz_move : public mcts_move
{
public:
	ddz_move(unsigned char type, unsigned char alone1 = pai_type_none, unsigned char alone2 = pai_type_none, unsigned char alone3 = pai_type_none, unsigned char alone4 = pai_type_none);
	ddz_move();
	virtual ~ddz_move();

	virtual bool			compare(const mcts_move* move);
	virtual mcts_move*		clone();
	virtual std::string		get_test_value();

	bool operator < (const ddz_move &m) const
	{
		if (_type == m._type)
			return _alone_1 > m._alone_1;
		return _type < m._type;
	}

	unsigned char				_type;
	unsigned char				_alone_1;
	unsigned char				_alone_2;
	unsigned char				_alone_3;
	unsigned char				_alone_4;
	bool						_airplane_pairs;

	unsigned char				_combo_list[20];
	int							_combo_count;

};


#endif
