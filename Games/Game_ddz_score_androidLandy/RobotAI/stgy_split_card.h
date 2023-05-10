#ifndef __STGY_SPLIT_CARD__
#define __STGY_SPLIT_CARD__

#include <vector>
#include "mcts_move.h"

// 拆牌权值
enum ddz_value_enum
{
    ddz_value_none			= 0,
    ddz_value_alone_1		= 1,			// 单张
    ddz_value_pairs			= 2,			// 对子
    ddz_value_three			= 3,			// 三个
    ddz_value_order			= 4,			// 顺子,每多一张+1
    ddz_value_order_pairs	= 5,			// 连对,每多一对+2
    ddz_value_order_airplane= 6,			// 飞机
    ddz_value_bomb			= 7,			// 炸弹
};



struct part_pai
{
    part_pai() : type(0),count(0) {}
    int					type;
    int					list[20];
    int					count;
    bool operator < (const part_pai &m) const
    {
        if (type == m.type)
        {
            return list[0] < m.list[0];
        }
        return type < m.type;
    }
};

struct parse_handcard
{
    parse_handcard():sum_value(0), need_round(0) {}


    int sum_value;
    int need_round;
    std::vector<part_pai> list;

    bool operator < (const parse_handcard &m) const
    {
        if (need_round == m.need_round)
            return sum_value > m.sum_value;
        return need_round < m.need_round;
    }
};

class stgy_split_card
{
public:
    stgy_split_card();
    ~stgy_split_card();

    void generate_out_pai_move(int* pai_list, std::vector<mcts_move*>& out_move_list);
    void check_whether_split_order(int* pai_list, parse_handcard& handcard);
};

#endif
