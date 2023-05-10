
#include "stgy_initiative_play_card.h"
#include "ddz_move.h"
#include <algorithm>
#include "ddz_state.h"
#include <assert.h>

/*-------------------------------------------------------------------------------------------------------------------*/
stgy_initiative_play_card::stgy_initiative_play_card()
{

}

/*-------------------------------------------------------------------------------------------------------------------*/
stgy_initiative_play_card::~stgy_initiative_play_card()
{

}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline int move_cmp(const void *a,const void *b)
{
    return *(unsigned char *)a-*(unsigned char *)b;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline bool is_pai_move_max(ddz_move* p_move, std::vector<mcts_move*>* list[2])
{
    for(int i = 0; i < 2; ++i)
    {
        std::vector<mcts_move*>* p_list = list[i];
        if (p_list != 0)
        {
            for(int i = 0; i < (int)p_list->size(); ++i)
            {
                ddz_move* move = dynamic_cast<ddz_move*>(p_list->at(i));
                if (move == 0)
                    continue;

                if (move->_type == ddz_type_bomb || move->_type == ddz_type_king_bomb)
                    return false;

                if (move->_type == p_move->_type && move->_alone_1 > p_move->_alone_1)
                    return false;
            }
        }
    }

    return true;
}


/*-------------------------------------------------------------------------------------------------------------------*/
std::vector<mcts_move*> stgy_initiative_play_card::parse_pai(ddz_state* p_state)
{
    std::vector<mcts_move*>& move_list = p_state->_out_pai_move_list[p_state->_curr_player_index];
    int* pai_list = p_state->_player_list[p_state->_curr_player_index].pai_list;

    int next_player_index = p_state->_curr_player_index;
    next_player_index++;
    if (next_player_index > 2)
        next_player_index = 0;
    std::vector<mcts_move*>& next_move_list = p_state->_out_pai_move_list[next_player_index];
    int* next_pai_list = p_state->_player_list[next_player_index].pai_list;
    bool next_is_landlord = p_state->_player_list[next_player_index].is_landlord;

    next_player_index++;
    if (next_player_index > 2)
        next_player_index = 0;
    std::vector<mcts_move*>& prev_move_list = p_state->_out_pai_move_list[next_player_index];
    int* prev_pai_list = p_state->_player_list[next_player_index].pai_list;

    if (p_state->_player_list[p_state->_curr_player_index].is_landlord)
    {
        return landlord_initiative_playcard(p_state, pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list);
    }

    if (next_is_landlord)
        return farmer_initiative_playcard_next_is_landlord(p_state, pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list);

    return farmer_initiative_playcard_next_is_farmer(p_state, pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list);
}

/*-------------------------------------------------------------------------------------------------------------------*/
static bool is_pai_max_in_side(ddz_move* p_move, int* pai_list)
{
    if (pai_list[pai_type_blossom] == 1 && pai_list[pai_type_blackjack] == 1)
        return false;

    bool haveBomb = false;
    if (p_move->_type == ddz_type_bomb)
    {
        for(int i = pai_type_A; i >= pai_type_3; --i)
        {
            if (pai_list[i] == 4)
            {
                haveBomb = true;
                // 对方的炸弹大于手牌
                if (i > p_move->_alone_1)
                    return false;
            }
        }
    }

    // 对方有炸弹
    if (haveBomb)
        return false;

    switch (p_move->_type)
    {
    case ddz_type_alone_1:
        {
            for (int i = pai_type_blossom; i > p_move->_alone_1; --i)
            {
                if ( pai_list[i] > 0 )
                    return false;
            }
        }
        break;

    case ddz_type_pair:
        {
            for (int i = pai_type_2; i > p_move->_alone_1; --i)
            {
                if ( pai_list[i] > 1 )
                    return false;
            }
        }
        break;

    case ddz_type_triple:
        {
            for (int i = pai_type_2; i > p_move->_alone_1; --i)
            {
                if ( pai_list[i] > 2 )
                    return false;
            }
        }
        break;

    case ddz_type_order:
        {
            qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
            for(int i = p_move->_combo_list[0]+1; i <= pai_type_10; ++i )
            {
                if (pai_list[i] > 0 && pai_list[i+1] > 0 && pai_list[i+2] > 0 && pai_list[i+3] > 0 && pai_list[i+4] > 0 )
                {
                    int retainIndexCount = p_move->_combo_count - 5;
                    for(int j = i+5; j <= pai_type_A; ++j)
                    {
                        if (retainIndexCount == 0)
                            break;

                        if (pai_list[j] <= 0)
                            break;

                        retainIndexCount--;
                    }

                    if (retainIndexCount == 0)
                        return false;
                }
            }
        }
        break;

    case ddz_type_order_pair:
        {
            qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
            for(int i = p_move->_combo_list[0]+1; i <= pai_type_Q; ++i )
            {
                if (pai_list[i] > 1 && pai_list[i+1] > 1 && pai_list[i+2] > 1 )
                {
                    int retainIndexCount = p_move->_combo_count / 2 - 3;
                    for(int j = i+3; j <= pai_type_A; ++j)
                    {
                        if (retainIndexCount == 0)
                            break;

                        if (pai_list[j] <= 1)
                            break;

                        retainIndexCount--;
                    }
                    if (retainIndexCount == 0)
                        return false;
                }
            }
        }
        break;

    case ddz_type_airplane:
        {
            qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
            for(int i = p_move->_combo_list[0]+1; i <= pai_type_K; ++i )
            {
                if (pai_list[i] > 2 && pai_list[i+1] > 2 )
                {
                    int retainIndexCount = p_move->_combo_count / 3 - 2;
                    for(int j = i+2; j <= pai_type_A; ++j)
                    {
                        if (retainIndexCount == 0)
                            break;

                        if (pai_list[j] <= 2)
                            break;

                        retainIndexCount--;
                    }
                    if (retainIndexCount == 0)
                        return false;
                }
            }
        }
        break;
    }

    return true;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static bool is_max_pai(ddz_move* p_move, int* next_pai_list, int* prev_pai_list)
{
    if (p_move->_type == ddz_type_king_bomb)
        return true;

    bool isNextMax = false;
    if (next_pai_list)
        isNextMax = is_pai_max_in_side(p_move, next_pai_list);
    else
        isNextMax = true;

    bool isPrevMax = false;
    if (prev_pai_list)
        isPrevMax = !isNextMax ? false : is_pai_max_in_side(p_move, prev_pai_list);
    else
        isPrevMax = true;

    return isPrevMax && isNextMax;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline bool is_triple_with_pai(ddz_move* m1, ddz_move* m2)
{
    if (m1->_type == ddz_type_triple && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair))
        return true;

    if (m2->_type == ddz_type_triple && (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair))
        return true;

    return false;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static bool is_have_triple_2(ddz_move* m, int* next_pai_list, int* prev_pai_list)
{
    bool isNextMax = true;
    if (next_pai_list)
        isNextMax = is_pai_max_in_side(m, next_pai_list);

    bool haveNext_2 = false;
    if (!isNextMax)
    {
        for(int i = pai_type_3; i <= pai_type_2; ++i)
        {
            if (next_pai_list[i] > 1)
            {
                haveNext_2 = true;
                break;
            }
        }
    }

    bool isPrevMax = true;
    if (!haveNext_2 && prev_pai_list)
        isPrevMax = is_pai_max_in_side(m, prev_pai_list);

    bool havePrev_2 = false;
    if (!haveNext_2 && !isPrevMax)
    {
        for(int i = pai_type_3; i <= pai_type_2; ++i)
        {
            if (prev_pai_list[i] > 1)
            {
                havePrev_2 = true;
                break;
            }
        }
    }

    return haveNext_2 || havePrev_2;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline bool is_four_with_pai(ddz_move* m1, ddz_move* m2, ddz_move* m3)
{
    if (m1->_type == ddz_type_bomb && m2->_type == m3->_type && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair) && (m3->_type == ddz_type_alone_1 || m3->_type == ddz_type_pair))
        return true;

    if (m2->_type == ddz_type_bomb && m1->_type == m3->_type && (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair) && (m3->_type == ddz_type_alone_1 || m3->_type == ddz_type_pair))
        return true;

    if (m3->_type == ddz_type_bomb && m1->_type == m2->_type &&(m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair) && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair))
        return true;

    return false;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline ddz_move* make_triple_with_pai_between_move(ddz_move* m1, ddz_move* m2)
{
    ddz_move* move = new ddz_move();
    if (m1->_type == ddz_type_triple && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair))
    {
        move->_alone_1 = m1->_alone_1;
        move->_alone_2 = m2->_alone_1;
        move->_type = m2->_type == ddz_type_alone_1 ? ddz_type_triple_1 : ddz_type_triple_2;
    }

    if (m2->_type == ddz_type_triple && (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair))
    {
        move->_alone_1 = m2->_alone_1;
        move->_alone_2 = m1->_alone_1;
        move->_type = m1->_type == ddz_type_alone_1 ? ddz_type_triple_1 : ddz_type_triple_2;
    }

    return move;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline ddz_move* make_four_with_pai(ddz_move* m1, ddz_move* m2, ddz_move* m3)
{
    ddz_move* move = new ddz_move();

    if (m1->_type == ddz_type_bomb && m2->_type == m3->_type && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair) && (m3->_type == ddz_type_alone_1 || m3->_type == ddz_type_pair))
    {
        move->_alone_1 = m1->_alone_1;
        move->_alone_2 = m2->_alone_1;
        move->_alone_3 = m3->_alone_1;
        move->_type = m2->_type == ddz_type_alone_1 ? ddz_type_four_with_alone_1 : ddz_type_four_with_pairs;
    }

    if (m2->_type == ddz_type_bomb && m1->_type == m3->_type && (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair) && (m3->_type == ddz_type_alone_1 || m3->_type == ddz_type_pair))
    {
        move->_alone_1 = m2->_alone_1;
        move->_alone_2 = m1->_alone_1;
        move->_alone_3 = m3->_alone_1;
        move->_type = m1->_type == ddz_type_alone_1 ? ddz_type_four_with_alone_1 : ddz_type_four_with_pairs;
    }

    if (m3->_type == ddz_type_bomb && m1->_type == m2->_type &&(m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair) && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair))
    {
        move->_alone_1 = m3->_alone_1;
        move->_alone_2 = m1->_alone_1;
        move->_alone_3 = m2->_alone_1;
        move->_type = m1->_type == ddz_type_alone_1 ? ddz_type_four_with_alone_1 : ddz_type_four_with_pairs;
    }

    return move;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_provide_alone1(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_alone_1)
        {
            if (move->_alone_1 < limit_move->_alone_1)
            {
                if (alone1 == 0)
                    alone1 = move->_alone_1;
                else if (alone1 > move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_3; i < limit_move->_alone_1; ++i)
        {
            if (pai_list[i] > 0)
            {
                alone1 = i;
                break;
            }
        }
    }

    if (alone1 != 0)
    {
        out_list.push_back(new ddz_move(ddz_type_alone_1, alone1));
    }

}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_provide_pair(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_pair)
        {
            if (move->_alone_1 < limit_move->_alone_1)
            {
                if (alone1 == 0)
                    alone1 = move->_alone_1;
                else if (alone1 > move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_3; i < limit_move->_alone_1; ++i)
        {
            if (pai_list[i] > 1)
            {
                alone1 = i;
                break;
            }
        }
    }

    if (alone1 != 0)
    {
        out_list.push_back(new ddz_move(ddz_type_pair, alone1));
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_provide_triple(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_triple)
        {
            if (move->_alone_1 < limit_move->_alone_1)
            {
                if (alone1 == 0)
                    alone1 = move->_alone_1;
                else if (alone1 > move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_3; i < limit_move->_alone_1; ++i)
        {
            if (pai_list[i] > 2)
            {
                alone1 = i;
                break;
            }
        }
    }

    if (alone1 != 0)
    {
        out_list.push_back(new ddz_move(ddz_type_pair, alone1));
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_provide_order(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_order && move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] < limit_move->_combo_list[0])
            {
                out_list.push_back(move->clone());
                break;
            }
        }
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = pai_type_3; i < limit_move->_combo_list[0]; ++i)
        {
            if (pai_list[i] > 0 && pai_list[i+1] > 0 && pai_list[i+2] > 0 && pai_list[i+3] > 0 && pai_list[i+4] > 0)
            {
                int retainIndexCount = limit_move->_combo_count-5;
                for(int j = i+5; j <= pai_type_A; ++j)
                {
                    if (retainIndexCount == 0)
                        break;

                    if (pai_list[j] <= 0)
                        break;

                    retainIndexCount--;
                }

                if (retainIndexCount == 0)
                {
                    ddz_move* move = new ddz_move(ddz_type_order);
                    move->_combo_count = 5;
                    move->_combo_list[0] = i;move->_combo_list[1] = i+1;move->_combo_list[2] = i+2;move->_combo_list[3] = i+3;move->_combo_list[4] = i+4;
                    int retainIndexCount = limit_move->_combo_count-5;
                    for(int j = i+5; j <= i+4+retainIndexCount; ++j)
                        move->_combo_list[move->_combo_count++] = j;
                    out_list.push_back(move);
                    break;
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_provide_order_pair(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_order_pair && move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] < limit_move->_combo_list[0])
            {
                out_list.push_back(move->clone());
                break;
            }
        }
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = pai_type_3; i < limit_move->_combo_list[0]; ++i)
        {
            if (pai_list[i] > 1 && pai_list[i+1] > 1 && pai_list[i+2] > 1)
            {
                int retainIndexCount = limit_move->_combo_count/2-3;
                for(int j = i+3; j <= pai_type_A; ++j)
                {
                    if (retainIndexCount == 0)
                        break;

                    if (pai_list[j] <= 1)
                        break;

                    retainIndexCount--;
                }

                if (retainIndexCount == 0)
                {
                    ddz_move* move = new ddz_move(ddz_type_order_pair);
                    move->_combo_count = 6;
                    move->_combo_list[0] = i;move->_combo_list[1] = i;
                    move->_combo_list[2] = i+1;move->_combo_list[3] = i+1;
                    move->_combo_list[4] = i+2;move->_combo_list[5] = i+2;
                    int retainIndexCount = limit_move->_combo_count/2-3;
                    for(int j = i+3; j <= i+2+retainIndexCount; ++j)
                    {
                        move->_combo_list[move->_combo_count++] = j;
                        move->_combo_list[move->_combo_count++] = j;
                    }
                    out_list.push_back(move);
                    break;
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_initiative_play_card::if_friend_only_1_move(std::vector<mcts_move*>& out_list, ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, std::vector<mcts_move*>& friend_move_list)
{
    if (friend_move_list.size() == 1)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(friend_move_list[0]);
        switch (move->_type)
        {
        case ddz_type_alone_1:
            {
                must_provide_alone1(out_list, pai_list, move_list, move);
            }
            break;
        case ddz_type_pair:
            {
                must_provide_pair(out_list, pai_list, move_list, move);
            }
            break;
        case ddz_type_triple:
            {
                must_provide_triple(out_list, pai_list, move_list, move);
            }
            break;

        case ddz_type_order:
            {
                must_provide_order(out_list, pai_list, move_list, move);
            }
            break;

        case ddz_type_order_pair:
            {
                must_provide_order_pair(out_list, pai_list, move_list, move);
            }
            break;
        }
    }

    if (friend_move_list.size() == 2)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(friend_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(friend_move_list[1]);

        unsigned char m1 = 0; unsigned char m2 = 0; bool isPair = false;
        if ((move1->_type == ddz_type_triple) && (move2->_type == ddz_type_alone_1 || move2->_type == ddz_type_pair))
        {
            m1 = move1->_alone_1; m2 = move2->_alone_1;
            isPair = move2->_type == ddz_type_pair;
        }
        if ((move2->_type == ddz_type_triple) && (move1->_type == ddz_type_alone_1 || move1->_type == ddz_type_pair))
        {
            m1 = move2->_alone_1; m2 = move1->_alone_1;
            isPair = move1->_type == ddz_type_pair;
        }
        if( m1 != 0 && m2 != 0 )
        {
            unsigned char triple = 0;
            for(int i = pai_type_3; i < m1; ++i)
            {
                if (pai_list[i] > 2)
                {
                    triple = i;
                    break;
                }
            }

            if (triple != 0)
            {
                int limit_count = isPair ? 1 : 0;
                unsigned char withPai = 0;
                for(int i = pai_type_3; i < pai_type_2; ++i)
                {
                    if (i == triple)
                        continue;

                    if (pai_list[i] > limit_count)
                    {
                        withPai = i;
                        break;
                    }
                }

                if (withPai != 0)
                {
                    out_list.push_back(new ddz_move(isPair ? ddz_type_triple_2: ddz_type_triple_1, triple, withPai));
                }
            }
        }
    }
}

static bool is_not_only_1_move(std::vector<mcts_move*>* next_move_list, std::vector<mcts_move*>* prev_move_list)
{
    if (next_move_list)
    {
        if ( next_move_list->size() <= 1 )
            return false;
    }

    if (prev_move_list)
    {
        if ( prev_move_list->size() <= 1 )
            return false;
    }

    return true;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_initiative_play_card::common_max_initiative_playcard(std::vector<mcts_move*>& out_list, ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, int* next_pai_list, int* prev_pai_list, std::vector<mcts_move*>* next_move_list, std::vector<mcts_move*>* prev_move_list)
{
    // 如果只有一手牌，则直接出
    if (move_list.size() == 1)
    {
        out_list.push_back(move_list[0]->clone());
        return;
    }

    // 如果有二手牌，其中有一手是最大牌型，则优先出最大牌型
    if (move_list.size() == 2)
    {
        // 如果是三带一或三带二，则直接出牌
        ddz_move* move[2] = {dynamic_cast<ddz_move*>(move_list[0]), dynamic_cast<ddz_move*>(move_list[1])};
        if (is_triple_with_pai(move[0], move[1]))
        {
            out_list.push_back(make_triple_with_pai_between_move(move[0], move[1]));
            return;
        }

        for(int i = 0; i < 2; ++i)
        {
            if (is_max_pai(move[i], next_pai_list, prev_pai_list))
            {
                out_list.push_back(move_list[i]->clone());
                return;
            }
        }
    }

    // 如果有三手牌，有一手牌是三个，并且，另一手是单张或对子，并且三个是最大牌型，则优先出三个
    if (move_list.size() == 3)
    {
        // 如果是四带二，则直接出牌
        ddz_move* move[3] = {dynamic_cast<ddz_move*>(move_list[0]), dynamic_cast<ddz_move*>(move_list[1]), dynamic_cast<ddz_move*>(move_list[2])};

        if (is_four_with_pai(move[0], move[1], move[2]))
        {
            // 如果四张是最大，则随便出一张，然后，再炸
            ddz_move* four_pai = 0; ddz_move* m1 = 0; ddz_move* m2 = 0;
            for(int i = 0; i < 3; ++i)
            {
                four_pai = move[i];
                if (four_pai->_type == ddz_type_bomb)
                {
                    if (i == 0)
                    {
                        m1 = move[1];m2 = move[2];
                    }
                    else if (i == 1)
                    {
                        m1 = move[0];m2 = move[2];
                    }
                    else
                    {
                        m1 = move[0];m2 = move[1];
                    }
                    break;
                }
                else
                    four_pai = 0;
            }

            if (four_pai && is_max_pai(four_pai, next_pai_list, prev_pai_list) && is_not_only_1_move(next_move_list, prev_move_list) )
            {
                if ( (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair) && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair) )
                {
                    ddz_move* with_m = m1;
                    if (m1->_alone_1 < m2->_alone_1)
                        with_m = m2;

                    out_list.push_back(with_m->clone());
                }
                else if (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair)
                {
                    out_list.push_back(m1->clone());
                }
                else if (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair)
                {
                    out_list.push_back(m2->clone());
                }
                else
                {
                    out_list.push_back(m1->clone());
                }

                return;
            }

            out_list.push_back(make_four_with_pai(move[0], move[1], move[2]));
            return;
        }

        for(int i = 0; i < 3; ++i)
        {
            ddz_move* m = move[i];
            if ( m == 0 )
                continue;

            if ( m->_type == ddz_type_triple )
            {
                ddz_move* m1; ddz_move* m2;
                if (i == 0)
                {
                    m1 = move[1];m2 = move[2];
                }
                else if (i == 1)
                {
                    m1 = move[0];m2 = move[2];
                }
                else
                {
                    m1 = move[0];m2 = move[1];
                }

                if ( is_max_pai(m, next_pai_list, prev_pai_list) )
                {
                    if ( (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair) && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair) )
                    {
                        ddz_move* with_m = m1;
                        if (m1->_alone_1 > m2->_alone_1)
                            with_m = m2;

                        out_list.push_back(make_triple_with_pai_between_move(m, with_m));
                        return;
                    }

                    if (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair)
                    {
                        out_list.push_back(make_triple_with_pai_between_move(m, m1));
                        return;
                    }

                    if (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair)
                    {
                        out_list.push_back(make_triple_with_pai_between_move(m, m2));
                        return;
                    }
                }

                if (m1->_type == ddz_type_pair || m2->_type == ddz_type_pair)
                {
                    if (!is_have_triple_2(m, next_pai_list, prev_pai_list))
                    {
                        if (m1->_type == ddz_type_pair && m2->_type == ddz_type_pair)
                        {
                            ddz_move* with_m = m1;
                            if (m1->_alone_1 > m2->_alone_1)
                                with_m = m2;
                            out_list.push_back(make_triple_with_pai_between_move(m, with_m));
                        }
                        else if ( m1->_type == ddz_type_pair )
                            out_list.push_back(make_triple_with_pai_between_move(m, m1));
                        else
                            out_list.push_back(make_triple_with_pai_between_move(m, m2));
                        return;
                    }
                }
            }
        }
    }

    // 如果有四手牌，并且有四个，并且有对子，有二个是单张或对子，并且，四个是最大牌型，则优先出
    if (move_list.size() == 4)
    {
        ddz_move* move[4] = {dynamic_cast<ddz_move*>(move_list[0]), dynamic_cast<ddz_move*>(move_list[1]), dynamic_cast<ddz_move*>(move_list[2]), dynamic_cast<ddz_move*>(move_list[3])};

        for(int i = 0; i < 4; ++i)
        {
            ddz_move* m = move[i];
            if ( m == 0 )
                continue;

            if ( m->_type == ddz_type_bomb && is_max_pai(m, next_pai_list, prev_pai_list) )
            {
                ddz_move* m1; ddz_move* m2; ddz_move* m3;
                if (i == 0)
                {
                    m1 = move[1];m2 = move[2]; m3 = move[3];
                }
                else if (i == 1)
                {
                    m1 = move[0];m2 = move[2]; m3 = move[3];
                }
                else if (i == 2)
                {
                    m1 = move[0];m2 = move[1]; m3 = move[3];
                }
                else
                {
                    m1 = move[0];m2 = move[1]; m3 = move[2];
                }

                // 如果有大牌则优先出大牌
                if (is_max_pai(m1, next_pai_list, prev_pai_list))
                {
                    out_list.push_back(m1->clone());
                    return;
                }
                else if (is_max_pai(m2, next_pai_list, prev_pai_list))
                {
                    out_list.push_back(m2->clone());
                    return;
                }
                else if (is_max_pai(m3, next_pai_list, prev_pai_list))
                {
                    out_list.push_back(m3->clone());
                    return;
                }

                if (m1->_type == m2->_type && ( m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair))
                {
                    out_list.push_back(make_four_with_pai(m, m1, m2));
                    return;
                }

                if (m1->_type == m3->_type && ( m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair))
                {
                    out_list.push_back(make_four_with_pai(m, m1, m3));
                    return;
                }

                if (m2->_type == m3->_type && ( m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair))
                {
                    out_list.push_back(make_four_with_pai(m, m2, m3));
                    return;
                }
            }
        }

    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_initiative_play_card::landlord_no_maxpai_only_2_move(std::vector<mcts_move*>& out_list, std::vector<mcts_move*>& move_list)
{
    if (move_list.size() == 2)
    {
        ddz_move* move[2] = {dynamic_cast<ddz_move*>(move_list[0]), dynamic_cast<ddz_move*>(move_list[1])};

        // 外面有更大的炸弹，先出小牌
        if (move[0]->_type == ddz_type_bomb)
        {
            out_list.push_back(move[1]->clone());
            return;
        }
        else if (move[1]->_type == ddz_type_bomb)
        {
            out_list.push_back(move[0]->clone());
            return;
        }

        if (move[0]->_type == ddz_type_order)
        {
            out_list.push_back(move[0]->clone());
            return;
        }
        else if (move[1]->_type == ddz_type_order)
        {
            out_list.push_back(move[1]->clone());
            return;
        }

        if (move[0]->_type == ddz_type_order_pair)
        {
            out_list.push_back(move[0]->clone());
            return;
        }
        else if (move[1]->_type == ddz_type_order_pair)
        {
            out_list.push_back(move[1]->clone());
            return;
        }

        if (move[0]->_type == ddz_type_airplane)
        {
            if (move[1]->_type == ddz_type_pair) {
                ddz_move* p_move = new ddz_move(ddz_type_airplane_with_pai);
                memcpy(p_move->_combo_list, move[0]->_combo_list, move[0]->_combo_count);
                p_move->_combo_count = move[0]->_combo_count;
                p_move->_alone_1 = move[1]->_alone_1; p_move->_alone_2 = move[1]->_alone_1;
                out_list.push_back(p_move);
            } else {
                out_list.push_back(move[0]->clone());
            }
            return;
        }
        else if (move[1]->_type == ddz_type_airplane)
        {
            if (move[0]->_type == ddz_type_pair) {
                ddz_move* p_move = new ddz_move(ddz_type_airplane_with_pai);
                memcpy(p_move->_combo_list, move[1]->_combo_list, move[1]->_combo_count);
                p_move->_combo_count = move[1]->_combo_count;
                p_move->_alone_1 = move[0]->_alone_1; p_move->_alone_2 = move[0]->_alone_1;
                out_list.push_back(p_move);
            } else {
                out_list.push_back(move[1]->clone());
            }
            return;
        }

        // 二手牌，假如只有单牌和对牌时的策略
        if ( (move[0]->_type == ddz_type_alone_1 || move[0]->_type == ddz_type_pair) && (move[1]->_type == ddz_type_alone_1 || move[1]->_type == ddz_type_pair) )
        {
            ddz_move* m1 = 0; ddz_move* m2 = 0;
            if (move[0]->_type == ddz_type_alone_1 && move[1]->_type == ddz_type_alone_1)
            {
                m1 = move[0]; m2 = move[1];
            }
            else if (move[0]->_type == ddz_type_alone_1 && move[1]->_type == ddz_type_pair)
            {
                m1 = move[0]; m2 = move[1];
            }
            else if (move[0]->_type == ddz_type_pair && move[1]->_type == ddz_type_alone_1)
            {
                m1 = move[1]; m2 = move[0];
            }
            else
            {
                m1 = move[0]; m2 = move[1];
            }

            if (m1->_type == m2->_type && m1->_alone_1 > m2->_alone_1)
            {
                ddz_move* m = m1;
                m1 = m2; m2 = m;
            }

        }

        out_list.push_back(move[0]->clone());
        out_list.push_back(move[1]->clone());
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_initiative_play_card::landlord_no_maxpai_only_3_move(std::vector<mcts_move*>& out_list, std::vector<mcts_move*>& move_list)
{
    if (move_list.size() == 3)
    {
        ddz_move* move[3] = {dynamic_cast<ddz_move*>(move_list[0]), dynamic_cast<ddz_move*>(move_list[1]),  dynamic_cast<ddz_move*>(move_list[2])};

    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static bool comp_order_class (const ddz_move* a, const ddz_move* b)
{
    return a->_combo_list[0] < b->_combo_list[0];
}

static void make_triple_with_pai(std::vector<mcts_move*>& out_list, unsigned char triple_pai, std::vector<unsigned char>& alone1, std::vector<unsigned char>& pair)
{
    ddz_move* move = new ddz_move(ddz_type_triple, triple_pai);
    move->_combo_count = 0;
    out_list.push_back(move);


    unsigned char pai1 = 0;
    bool isWithPaiPairs = false; bool isHaveWithPai = false;

    if (alone1.size() > 0 && alone1[0] <= pai_type_10)
    {
        isHaveWithPai = true; pai1 = alone1[0];
    }

    if (!isHaveWithPai && pair.size() > 0 && pair[0] <= pai_type_10)
    {
        isHaveWithPai = true; pai1 = pair[0]; isWithPaiPairs = true;
    }

    if (!isHaveWithPai && alone1.size() > 0 && pair.size() > 0 )
    {
        if (alone1.back() > pair.back())
        {
            isHaveWithPai = true; pai1 = pair[0]; isWithPaiPairs = true;
        }
        else
        {
            isHaveWithPai = true; pai1 = alone1[0];
        }
    }

    if (!isHaveWithPai && alone1.size() > 0 && pair.size() <= 0)
    {
        isHaveWithPai = true; pai1 = alone1[0];
    }

    if (!isHaveWithPai && alone1.size() <= 0 && pair.size() > 0)
    {
        isHaveWithPai = true; pai1 = pair[0]; isWithPaiPairs = true;
    }

    if (isHaveWithPai)
    {
        move->_alone_2 = pai1;
        if (isWithPaiPairs)
        {
            move->_type = ddz_type_triple_2;
        }
        else
        {
            move->_type = ddz_type_triple_1;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void make_airplane_with_pai(std::vector<mcts_move*>& out_list, ddz_move* airplane, std::vector<unsigned char>& alone1, std::vector<unsigned char>& pair)
{
    ddz_move* move = new ddz_move(ddz_type_airplane);
    memcpy(move->_combo_list, airplane->_combo_list, airplane->_combo_count);
    move->_combo_count = airplane->_combo_count;
    out_list.push_back(move);

    int airplaneCount = airplane->_combo_count / 3;
    if (airplaneCount == 2)
    {
        unsigned char pai1 = 0; unsigned char pai2 = 0;
        bool isWithPaiPairs = false; bool isHaveWithPai = false;

        // 选择10以下的单牌
        if ( alone1.size() >= 2 && alone1[1] <= pai_type_10 )
        {
            isHaveWithPai = true;
            pai1 = alone1[0]; pai2 = alone1[1];
        }

        // 再选择10以下的对牌
        if (!isHaveWithPai && pair.size() >= 2 && pair[1] <= pai_type_10)
        {
            isHaveWithPai = true; isWithPaiPairs = true;
            pai1 = pair[0]; pai2 = pair[1];
        }

        // 再选择Q以下的单牌
        if (!isHaveWithPai && alone1.size() >= 2 && alone1[1] <= pai_type_Q)
        {
            isHaveWithPai = true;
            pai1 = alone1[0]; pai2 = alone1[1];
        }

        // 再选择Q以上的单牌
        if (!isHaveWithPai && alone1.size() >= 2 && alone1[1] > pai_type_Q )
        {
            if (!isHaveWithPai && pair.size() >= 2 && pair[1] <= pai_type_Q)
            {
                isHaveWithPai = true; isWithPaiPairs = true;
                pai1 = pair[0]; pai2 = pair[1];
            }

            if (!isHaveWithPai && alone1[1] <= pai_type_A)
            {
                isHaveWithPai = true;
                pai1 = alone1[0]; pai2 = alone1[1];
            }

            if (!isHaveWithPai && pair.size() >= 2 && pair[1] <= pai_type_A)
            {
                isHaveWithPai = true; isWithPaiPairs = true;
                pai1 = pair[0]; pai2 = pair[1];
            }

            if (!isHaveWithPai && alone1[1] <= pai_type_2)
            {
                isHaveWithPai = true;
                pai1 = alone1[0]; pai2 = alone1[1];
            }

            if (!isHaveWithPai && pair.size() >= 2 && pair[1] <= pai_type_2)
            {
                isHaveWithPai = true; isWithPaiPairs = true;
                pai1 = pair[0]; pai2 = pair[1];
            }

            if (!isHaveWithPai && pair.size() >= 1)
            {
                isHaveWithPai = true;
                pai1 = pair[0]; pai2 = pair[0];
            }

            if (!isHaveWithPai && alone1.size() >= 2)
            {
                isHaveWithPai = true;
                pai1 = alone1[0]; pai2 = alone1[1];
            }
        }

        // 如果单牌数小于1
        if (!isHaveWithPai && alone1.size() <= 1)
        {
            if (!isHaveWithPai && pair.size() >= 2 && pair[1] < pai_type_2)
            {
                isHaveWithPai = true; isWithPaiPairs = true;
                pai1 = pair[0]; pai2 = pair[1];
            }

            if (!isHaveWithPai && pair.size() >= 1 && pair[0] < pai_type_2)
            {
                isHaveWithPai = true;
                pai1 = pair[0]; pai2 = pair[0];
            }

            if (!isHaveWithPai && pair.size() >= 2 && pair[1] <= pai_type_2)
            {
                isHaveWithPai = true; isWithPaiPairs = true;
                pai1 = pair[0]; pai2 = pair[1];
            }

            if (!isHaveWithPai && pair.size() >= 1 && pair[0] <= pai_type_2)
            {
                isHaveWithPai = true;
                pai1 = pair[0]; pai2 = pair[0];
            }
        }

        if (!isHaveWithPai && alone1.size() <= 0 && pair.size() >= 1)
        {
            isHaveWithPai = true;
            pai1 = pair[0]; pai2 = pair[0];
        }

        if (isHaveWithPai)
        {
            move->_airplane_pairs = isWithPaiPairs;
            move->_type = ddz_type_airplane_with_pai;
            move->_alone_1 = pai1; move->_alone_2 = pai2;
        }
    }
    else if (airplaneCount == 3)
    {
        unsigned char pai1 = 0; unsigned char pai2 = 0; unsigned char pai3 = 0;
        bool isWithPaiPairs = false; bool isHaveWithPai = false;

        unsigned char compPoint[4] = { pai_type_10, pai_type_Q, pai_type_A, pai_type_2 };

        for(int i = 0; i < 4 && !isHaveWithPai; ++i)
        {
            if (!isHaveWithPai && alone1.size() >= 3 && alone1[2] <= compPoint[i])
            {
                isHaveWithPai = true;
                pai1 = alone1[0]; pai2 = alone1[1]; pai3 = alone1[2];
            }

            if (!isHaveWithPai && pair.size() >= 3 && pair[2] <= compPoint[i])
            {
                isHaveWithPai = true; isWithPaiPairs = true;
                pai1 = pair[0]; pai2 = pair[1]; pai3 = pair[2];
            }

            if (!isHaveWithPai && pair.size() >= 1 && pair[0] <= compPoint[i] && alone1.size() >= 1 && alone1[0] <= compPoint[i])
            {
                isHaveWithPai = true;
                pai1 = alone1[0]; pai2 = pair[0]; pai3 = pair[0];
            }
        }

        if (!isHaveWithPai && alone1.size() >= 1 && pair.size() >= 1)
        {
            isHaveWithPai = true;
            pai1 = alone1[0]; pai2 = pair[0]; pai3 = pair[0];
        }

        if (!isHaveWithPai && alone1.size() == 0 && pair.size() >= 2)
        {
            isHaveWithPai = true;
            pai1 = pair[0]; pai2 = pair[0]; pai3 = pair[1];
        }

        if (isHaveWithPai)
        {
            move->_airplane_pairs = isWithPaiPairs;
            move->_type = ddz_type_airplane_with_pai;
            move->_alone_1 = pai1; move->_alone_2 = pai2; move->_alone_3 = pai3;
        }
    }
    else if (airplaneCount == 4)
    {
        unsigned char pai1 = 0; unsigned char pai2 = 0; unsigned char pai3 = 0; unsigned char pai4 = 0;
        bool isWithPaiPairs = false; bool isHaveWithPai = false;
        unsigned char compPoint[4] = { pai_type_10, pai_type_Q, pai_type_A, pai_type_2 };

        for(int i = 0; i < 4 && !isHaveWithPai; ++i)
        {
            if (!isHaveWithPai && alone1.size() >= 4 && alone1[3] <= compPoint[i])
            {
                isHaveWithPai = true;
                pai1 = alone1[0]; pai2 = alone1[1]; pai3 = alone1[2]; pai4 = alone1[3];
            }

            if (!isHaveWithPai && pair.size() >= 4 && pair[3] <= compPoint[i])
            {
                isHaveWithPai = true; isWithPaiPairs = true;
                pai1 = pair[0]; pai2 = pair[1]; pai3 = pair[2]; pai4 = pair[3];
            }

            if (!isHaveWithPai && pair.size() >= 2 && pair[1] <= compPoint[i])
            {
                isHaveWithPai = true;
                pai1 = pair[0]; pai2 = pair[0]; pai3 = pair[1]; pai4 = pair[1];
            }

            if (!isHaveWithPai && pair.size() >= 1 && pair[0] <= compPoint[i] && alone1.size() >= 2 && alone1[1] <= compPoint[i])
            {
                isHaveWithPai = true;
                pai1 = alone1[0]; pai2 = alone1[1]; pai3 = pair[0]; pai4 = pair[0];
            }
        }

        if ( !isHaveWithPai && pair.size() >= 2)
        {
            isHaveWithPai = true;
            pai1 = pair[0]; pai2 = pair[0]; pai3 = pair[1]; pai4 = pair[1];
        }

        if (isHaveWithPai)
        {
            move->_airplane_pairs = isWithPaiPairs;
            move->_type = ddz_type_airplane_with_pai;
            move->_alone_1 = pai1; move->_alone_2 = pai2; move->_alone_3 = pai3; move->_alone_4 = pai4;
        }
    }
}

void stgy_initiative_play_card::sendout_from_max_to_min_playcard(ddz_state* p_state, std::vector<mcts_move*>& out_list, std::vector<mcts_move*>& move_list, ddz_move* landlord_move)
{
    int size = (int)move_list.size();
    for(int i = 0; i < size; ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type != landlord_move->_type)
        {
            out_list.push_back(move->clone());
            return;
        }
        else
        {
            if (p_state->is_greater_than(move, landlord_move) || !p_state->is_greater_than(landlord_move, move))
            {
                out_list.push_back(move->clone());
            }
        }
    }
}

void stgy_initiative_play_card::common_landlord_playcard(std::vector<mcts_move*>& out_list, std::vector<mcts_move*>& move_list, bool isAlone)
{
    // 通用地主出牌规则
    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;
    std::vector<unsigned char> threes;

    std::vector<ddz_move*> orders;
    std::vector<ddz_move*> order_pairs;
    std::vector<ddz_move*> airplanes;

    std::vector<ddz_move*> bombs;
    bool haveKingBomb = false;

    int size = (int)move_list.size();
    for(int i = 0; i < size; ++i)
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (p_move == 0)
            continue;

        if (p_move->_type == ddz_type_alone_1)
        {
            alone1.push_back(p_move->_alone_1);
        }
        else if (p_move->_type == ddz_type_pair)
        {
            pairs.push_back(p_move->_alone_1);
        }
        else if (p_move->_type == ddz_type_triple)
        {
            threes.push_back(p_move->_alone_1);
        }
        else if (p_move->_type == ddz_type_order)
        {
            qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
            orders.push_back(p_move);
        }
        else if (p_move->_type == ddz_type_order_pair)
        {
            qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
            order_pairs.push_back(p_move);
        }
        else if (p_move->_type == ddz_type_airplane)
        {
            qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
            airplanes.push_back(p_move);
        }
        else if (p_move->_type == ddz_type_bomb)
        {
            bombs.push_back(p_move);
        }
        else if (p_move->_type == ddz_type_king_bomb)
        {
            haveKingBomb = true;
        }
    }

    // 第一轮出牌选择
    if (orders.size() > 0)
    {
        if (orders.size() == 1)
        {
            if (orders[0]->_combo_list[orders[0]->_combo_count-1] != pai_type_A && orders[0]->_combo_list[orders[0]->_combo_count-1] != pai_type_K)
                out_list.push_back(orders[0]->clone());
        }
        else
        {
            std::sort(orders.begin(), orders.end(), comp_order_class);
            out_list.push_back(orders[0]->clone());
        }
    }

    if (order_pairs.size() > 0)
    {
        if (order_pairs.size() == 1)
        {
            if (order_pairs[0]->_combo_list[order_pairs[0]->_combo_count-1] != pai_type_A && order_pairs[0]->_combo_list[order_pairs[0]->_combo_count-1] != pai_type_K)
                out_list.push_back(order_pairs[0]->clone());
        }
        else
        {
            std::sort(order_pairs.begin(), order_pairs.end(), comp_order_class);
            out_list.push_back(order_pairs[0]->clone());
        }
    }

    if (alone1.size() > 1)
        std::sort(alone1.begin(), alone1.end());
    if (pairs.size() > 1)
        std::sort(pairs.begin(), pairs.end());

    if (airplanes.size() > 0)
    {
        if (airplanes.size() == 1)
        {
            if (airplanes[0]->_combo_list[airplanes[0]->_combo_count-1] != pai_type_A && airplanes[0]->_combo_list[airplanes[0]->_combo_count-1] != pai_type_K)
                make_airplane_with_pai(out_list, airplanes[0], alone1, pairs );

            if (out_list.empty())
            {
                int airploneCount = airplanes[0]->_combo_count / 3;
                if (order_pairs.size() == 0 && orders.size() == 0 && threes.size() == 0)
                {
                    if ( airploneCount + 1 >= (int)(alone1.size() + pairs.size()) )
                        make_airplane_with_pai(out_list, airplanes[0], alone1, pairs );
                }
            }
        }
        else
        {
            std::sort(airplanes.begin(), airplanes.end(), comp_order_class);
            make_airplane_with_pai(out_list, airplanes[0], alone1, pairs );
        }
    }

    if (!out_list.empty())
        return;

    // 第一轮出牌，出三带一或三带二
    if (threes.size() > 1)
    {
        std::sort(threes.begin(), threes.end());
        if (threes[0] <= pai_type_Q)
        {
            make_triple_with_pai(out_list, threes[0], alone1, pairs );
        }
    }

    if (!out_list.empty())
        return;

    if (threes.size() > 0 && threes[0] < pai_type_2)
    {
        make_triple_with_pai(out_list, threes[0], alone1, pairs );
    }

    // 出单牌或对牌
    if (pairs.size() > 0 )
    {
        for(int i = 0; i < (int)pairs.size(); ++i)
        {
            if (pairs[i] <= pai_type_Q)
                out_list.push_back(new ddz_move(ddz_type_pair, pairs[i]));
        }
    }

    if (!isAlone && alone1.size() > 0)
    {
        for(int i = 0; i < (int)alone1.size(); ++i)
        {
            if (alone1[i] <= pai_type_Q)
                out_list.push_back(new ddz_move(ddz_type_alone_1, alone1[i]));
        }
    }

    if (!out_list.empty())
        return;

    // 出单牌或对牌
    if (pairs.size() > 0 )
    {
        for(int i = 0; i < (int)pairs.size(); ++i)
        {
            if (pairs[i] <= pai_type_A)
                out_list.push_back(new ddz_move(ddz_type_pair, pairs[i]));
        }
    }

    if (!isAlone && alone1.size() > 0)
    {
        for(int i = 0; i < (int)alone1.size(); ++i)
        {
            if (alone1[i] <= pai_type_A)
                out_list.push_back(new ddz_move(ddz_type_alone_1, alone1[i]));
        }
    }

    if (!out_list.empty())
        return;

    // 出单牌或对牌
    if (pairs.size() > 0 )
    {
        for(int i = 0; i < (int)pairs.size(); ++i)
        {
            out_list.push_back(new ddz_move(ddz_type_pair, pairs[i]));
        }
    }

    if (!isAlone && alone1.size() > 0)
    {
        for(int i = 0; i < (int)alone1.size(); ++i)
        {
            out_list.push_back(new ddz_move(ddz_type_alone_1, alone1[i]));
        }
    }

    if (out_list.empty())
    {
        if (orders.size() > 0)
        {
            out_list.push_back(orders[0]->clone());
        }
    }

    if (out_list.empty())
    {
        if (order_pairs.size() > 0)
        {
            out_list.push_back(order_pairs[0]->clone());
        }
    }

    if (isAlone && alone1.size() > 0) {
        unsigned int index = (int)alone1.size()-1;
        out_list.push_back(new ddz_move(ddz_type_alone_1, alone1[index]));
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
std::vector<mcts_move*> stgy_initiative_play_card::landlord_initiative_playcard(ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* prev_pai_list, std::vector<mcts_move*>& prev_move_list)
{
    // 地主残局处理
    std::vector<mcts_move*>		return_list;

    common_max_initiative_playcard(return_list, p_state, pai_list, move_list, next_pai_list, prev_pai_list, &next_move_list, &prev_move_list );
    if (!return_list.empty())
        return return_list;

    is_landlord_can_control_pai(return_list, p_state, pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list);
    if (!return_list.empty())
        return return_list;

    // 地主手上二手牌，并且没有大牌占优
    landlord_no_maxpai_only_2_move(return_list, move_list);
    if (!return_list.empty())
        return return_list;

    // 地主手上有三手牌，并且没有大牌占优
    landlord_no_maxpai_only_3_move(return_list, move_list);
    if (!return_list.empty())
        return return_list;

    bool isAlone = false;
    if (next_move_list.size() == 1)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(next_move_list[0]);
        isAlone = move->_type == ddz_type_alone_1;
    }
    if (!isAlone && prev_move_list.size() == 1)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(prev_move_list[0]);
        isAlone = move->_type == ddz_type_alone_1;
    }
    common_landlord_playcard(return_list, move_list, isAlone);
    if (return_list.empty())
    {
        for(int i = pai_type_3; i <= pai_type_2; ++i)
        {
            if (pai_list[i] > 3)
            {
                return_list.push_back(new ddz_move(ddz_type_bomb, i));
            }
            else if (pai_list[i] > 2)
            {
                return_list.push_back(new ddz_move(ddz_type_triple, i));
            }
            else if (pai_list[i] > 1)
            {
                return_list.push_back(new ddz_move(ddz_type_pair, i));
            }
            else if (pai_list[i] > 0)
            {
                return_list.push_back(new ddz_move(ddz_type_alone_1, i));
            }

            if (!return_list.empty())
                break;
        }

        if (return_list.empty() && pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0 )
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
        }

        if (return_list.empty()&& pai_list[pai_type_blackjack] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_alone_1, pai_type_blackjack));
        }

        if (return_list.empty()&& pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_alone_1, pai_type_blossom));
        }
    }

    if (return_list.empty())
        return_list.push_back(new ddz_move());

    return return_list;
}

typedef std::pair<unsigned char, unsigned int> AlonePair;


    bool alone_compare_st(const  AlonePair& lhs, const  AlonePair& rhs) {
        return lhs.first > rhs.first;
    }


void stgy_initiative_play_card::farmer_initiative_playcard_landlord_only_1_alone_move(std::vector<mcts_move*>& out_list, std::vector<mcts_move*>& self_list, std::vector<mcts_move*>& next_move_list)
{
    if (next_move_list.size() == 1)
    {
        ddz_move *move = dynamic_cast<ddz_move*>(next_move_list[0]);
        if (move->_type != ddz_type_alone_1) {
            return;
        }

        std::vector<AlonePair> alone1;
        bool bAllAlone = true;
        for (unsigned int i=0; i < self_list.size(); i++) {
            ddz_move *m = dynamic_cast<ddz_move*>(self_list[i]);
            if (m->_type != ddz_type_alone_1) {
                bAllAlone = false;
                break;
            }
            alone1.push_back(std::make_pair(m->_alone_1, i));
        }
        if (bAllAlone) {
            std::sort(alone1.begin(), alone1.end(), alone_compare_st);
            int index = alone1[0].second;
            ddz_move *m = dynamic_cast<ddz_move*>(self_list[index]);
            out_list.push_back(m->clone());
        }
    }
}

void stgy_initiative_play_card::farmer_initiative_playcard_next_landlord_only_1_move(std::vector<mcts_move*>& out_list, int* pai_list,  std::vector<mcts_move*>& self_list, std::vector<mcts_move*>& next_move_list)
{
    std::vector<mcts_move*> deleteList;

    if (next_move_list.size() == 1)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(next_move_list[0]);
        switch (move->_type)
        {
        case ddz_type_alone_1:
        case ddz_type_pair:
        case ddz_type_triple:
            {
                std::vector<mcts_move*>::iterator iter = out_list.begin();
                for( ; iter != out_list.end(); )
                {
                    ddz_move* m = dynamic_cast<ddz_move*>(*iter);
                    if (m->_type == move->_type && m->_alone_1 < move->_alone_1)
                    {
                        iter = out_list.erase(iter);
                        deleteList.push_back(m);
                    }
                    else
                    {
                        ++iter;
                    }
                }
            }
            break;

        case ddz_type_order:
        case ddz_type_order_pair:
            {
                qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);
                std::vector<mcts_move*>::iterator iter = out_list.begin();
                for( ; iter != out_list.end(); )
                {
                    ddz_move* m = dynamic_cast<ddz_move*>(*iter);
                    if (m->_type == ddz_type_order || m->_type == ddz_type_order_pair)
                        qsort(m->_combo_list, m->_combo_count, sizeof(unsigned char), move_cmp);

                    if (m->_type == move->_type && m->_combo_count == move->_combo_count && m->_combo_list[0] < move->_combo_list[0])
                    {
                        iter = out_list.erase(iter);
                        deleteList.push_back(m);
                    }
                    else
                    {
                        ++iter;
                    }
                }
            }
            break;
        }

//		if (out_list.size() > 0)
        {
            int size = (int)deleteList.size();
            for(int i = 0; i < size; ++i)
            {
                mcts_move* m = deleteList[i];
                delete m;
            }
        }
    }

}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_initiative_play_card::farmer_initiative_playcard_next_farmer_only_1_move(std::vector<mcts_move*>& out_list, std::vector<mcts_move*>& move_list, int* pai_list, std::vector<mcts_move*>& next_move_list)
{
    if (next_move_list.size() == 1)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(next_move_list[0]);
        switch (move->_type)
        {
        case ddz_type_alone_1:
            {
                must_provide_alone1(out_list, pai_list, move_list, move);
            }
            break;
        case ddz_type_pair:
            {
                must_provide_pair(out_list, pai_list, move_list, move);
            }
            break;
        case ddz_type_triple:
            {
                must_provide_triple(out_list, pai_list, move_list, move);
            }
            break;

        case ddz_type_order:
            {
                must_provide_order(out_list, pai_list, move_list, move);
            }
            break;

        case ddz_type_order_pair:
            {
                must_provide_order_pair(out_list, pai_list, move_list, move);
            }
            break;
        }
    }

    if (next_move_list.size() == 2)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(next_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(next_move_list[1]);

        ddz_move* m = 0; ddz_move* m2 = 0;

        if (move1->_type == ddz_type_triple && (move2->_type == ddz_type_alone_1 || move2->_type == ddz_type_pair))
        {
            m = move1; m2 = move2;
        }
        else if (move2->_type == ddz_type_triple && (move1->_type == ddz_type_alone_1 || move1->_type == ddz_type_pair))
        {
            m = move2; m2 = move1;
        }

        if (m && m2)
        {
            unsigned type = m2->_type == ddz_type_alone_1 ? ddz_type_triple_1 : ddz_type_triple_2;
            ddz_move* move = 0;
            for(int i = pai_type_3; i < m->_alone_1; ++i)
            {
                if (pai_list[i] > 2)
                {
                    move = new ddz_move(type, i);
                    break;
                }
            }

            if (move)
            {
                unsigned char with_pai = 0;
                unsigned char comp_pai_count = move->_type == ddz_type_triple_1 ? 0 : 1;
                for(int i = pai_type_3; i <= pai_type_2; ++i)
                {
                    if (pai_list[i] > comp_pai_count)
                    {
                        with_pai = i;
                        break;
                    }
                }

                if (with_pai == 0)
                {
                    if (move->_type == ddz_type_triple_1 && pai_list[pai_type_blackjack] > 0)
                        with_pai = pai_type_blackjack;
                    if (move->_type == ddz_type_triple_1 && with_pai == 0 && pai_list[pai_type_blossom] > 0)
                        with_pai = pai_type_blossom;
                }

                if (with_pai == 0)
                    delete move;
                else
                {
                    move->_alone_2 = with_pai;
                    out_list.push_back(move);
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
bool stgy_initiative_play_card::decide_self_is_master(std::vector<mcts_move*>& self_move_list, std::vector<mcts_move*>& other_move_list)
{
    bool isSelfMaster = true;




    return isSelfMaster;
}


/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_initiative_play_card::is_landlord_can_control_pai(std::vector<mcts_move*>& out_list, ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* prev_pai_list, std::vector<mcts_move*>& prev_move_list)
{
    int triple_count = 0;
    int airplane_count = 0;
    int bomb_count = 0;
    int other_alone_count = 0;
    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;

    std::vector<ddz_move*> maxPaiList;
    int size = (int)move_list.size();
    for (int i=0; i<size; i++)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (is_max_pai(move, next_pai_list, prev_pai_list))
        {
            if (move->_type == ddz_type_triple)
            {
                triple_count++;
                maxPaiList.push_back(move);
            }
            else if (move->_type == ddz_type_alone_1 || move->_type == ddz_type_pair || move->_type == ddz_type_order || move->_type == ddz_type_order_pair)
                maxPaiList.push_back(move);
            else if (move->_type == ddz_type_airplane)
                airplane_count++;
            else if (move->_type == ddz_type_bomb)
                bomb_count++;
            continue;
        } else {
            if (move->_type == ddz_type_alone_1)
                alone1.push_back(move->_alone_1);
            else if (move->_type == ddz_type_pair)
                pairs.push_back(move->_alone_1);
            else if (move->_type == ddz_type_triple)
            {
                triple_count++;
                other_alone_count++;
            }
            else
                other_alone_count++;
        }
    }

    int total_alone_count = other_alone_count + alone1.size() + pairs.size()-1-triple_count;
    if (total_alone_count <= 0 && !maxPaiList.empty())
    {
        if ( maxPaiList[0]->_type == ddz_type_triple )
        {
            if (alone1.size() > 0)
                out_list.push_back(new ddz_move(ddz_type_triple_1, maxPaiList[0]->_alone_1, alone1[0]));
            else if (pairs.size() > 0)
                out_list.push_back(new ddz_move(ddz_type_triple_2, maxPaiList[0]->_alone_1, pairs[0]));
            else
                out_list.push_back(new ddz_move(ddz_type_triple, maxPaiList[0]->_alone_1));
        }
        else
            out_list.push_back(maxPaiList[0]->clone());
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_initiative_play_card::is_self_can_control_pai(std::vector<mcts_move*>& out_list, ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, int* landlord_pai)
{
    int triple_count = 0;
    int airplane_count = 0;
    int bomb_count = 0;
    int other_alone_count = 0;
    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;

    std::vector<ddz_move*> maxPaiList;

    int size = (int)move_list.size();
    for(int i = 0; i < size; ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (is_max_pai(move, landlord_pai, 0))
        {
            if (move->_type == ddz_type_triple)
            {
                triple_count++;
                maxPaiList.push_back(move);
            }
            else if (move->_type == ddz_type_alone_1 || move->_type == ddz_type_pair || move->_type == ddz_type_order || move->_type == ddz_type_order_pair)
                maxPaiList.push_back(move);
            else if (move->_type == ddz_type_airplane)
                airplane_count++;
            else if (move->_type == ddz_type_bomb)
                bomb_count++;
            continue;
        }
        else
        {
            if (move->_type == ddz_type_alone_1)
                alone1.push_back(move->_alone_1);
            else if (move->_type == ddz_type_pair)
                pairs.push_back(move->_alone_1);
            else if (move->_type == ddz_type_triple)
            {
                triple_count++;
                other_alone_count++;
            }
            else
                other_alone_count++;
        }
    }

    int total_alone_count = other_alone_count + alone1.size() + pairs.size()-1-triple_count;
    if (total_alone_count <= 0 && !maxPaiList.empty())
    {
        if ( maxPaiList[0]->_type == ddz_type_triple )
        {
            if (alone1.size() > 0)
                out_list.push_back(new ddz_move(ddz_type_triple_1, maxPaiList[0]->_alone_1, alone1[0]));
            else if (pairs.size() > 0)
                out_list.push_back(new ddz_move(ddz_type_triple_2, maxPaiList[0]->_alone_1, pairs[0]));
            else
                out_list.push_back(new ddz_move(ddz_type_triple, maxPaiList[0]->_alone_1));
        }
        else
            out_list.push_back(maxPaiList[0]->clone());
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
std::vector<mcts_move*> stgy_initiative_play_card::farmer_initiative_playcard_next_is_farmer(ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* prev_pai_list, std::vector<mcts_move*>& prev_move_list)
{
    std::vector<mcts_move*>		return_list;
    common_max_initiative_playcard(return_list, p_state, pai_list, move_list, 0, prev_pai_list, 0, &prev_move_list );
    if (!return_list.empty())
        return return_list;

    // 如果下家只剩一手牌，则优先出下家能打过的牌
    farmer_initiative_playcard_next_farmer_only_1_move(return_list, move_list, pai_list, next_move_list);
    if (!return_list.empty())
        return return_list;

    is_self_can_control_pai(return_list, p_state, pai_list, move_list, prev_pai_list);
    if (!return_list.empty())
        return return_list;

    // 查看庄家是否只剩一手牌了
    farmer_initiative_playcard_landlord_only_1_alone_move(return_list, move_list, prev_move_list);
    if (!return_list.empty())
        return return_list;

    // 确定主从关系
    if ( decide_self_is_master(move_list, next_move_list) )
    {
        bool isAlone = false;
        if (prev_move_list.size() == 1)
        {
            ddz_move* move = dynamic_cast<ddz_move*>(prev_move_list[0]);
            isAlone = move->_type == ddz_type_alone_1;
        }
        common_landlord_playcard(return_list, move_list, isAlone);
    }
    else
    {

    }

    if (return_list.empty())
    {
        for(int i = pai_type_3; i <= pai_type_2; ++i)
        {
            if (pai_list[i] > 3)
            {
                return_list.push_back(new ddz_move(ddz_type_bomb, i));
            }
            else if (pai_list[i] > 2)
            {
                return_list.push_back(new ddz_move(ddz_type_triple, i));
            }
            else if (pai_list[i] > 1)
            {
                return_list.push_back(new ddz_move(ddz_type_pair, i));
            }
            else if (pai_list[i] > 0)
            {
                return_list.push_back(new ddz_move(ddz_type_alone_1, i));
            }

            if (!return_list.empty())
                break;
        }

        if (return_list.empty() && pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0 )
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
        }

        if (return_list.empty()&& pai_list[pai_type_blackjack] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_alone_1, pai_type_blackjack));
        }

        if (return_list.empty()&& pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_alone_1, pai_type_blossom));
        }
    }

    if (return_list.empty())
        return_list.push_back(new ddz_move());


    return return_list;
}

/*-------------------------------------------------------------------------------------------------------------------*/
std::vector<mcts_move*> stgy_initiative_play_card::farmer_initiative_playcard_next_is_landlord(ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* prev_pai_list, std::vector<mcts_move*>& prev_move_list)
{
    std::vector<mcts_move*>		return_list;
    common_max_initiative_playcard(return_list, p_state, pai_list, move_list, next_pai_list, 0, &next_move_list, 0 );
    if (!return_list.empty())
        return return_list;

    is_self_can_control_pai(return_list, p_state, pai_list, move_list, next_pai_list);
    if (!return_list.empty())
        return return_list;

    farmer_initiative_playcard_landlord_only_1_alone_move(return_list, move_list, next_move_list);
    if (!return_list.empty())
        return return_list;

    // 如果对方农民只有一手牌，优先出对方农民低的牌
    if_friend_only_1_move(return_list, p_state, pai_list, move_list, prev_move_list);

    // 如果下家地主只剩一手牌，则出比地主大的牌，或别的其他的牌
    farmer_initiative_playcard_next_landlord_only_1_move(return_list, pai_list, move_list, next_move_list);
    if (!return_list.empty())
        return return_list;

    is_friend_can_control_pai(return_list, p_state, pai_list, move_list, prev_move_list, next_pai_list);
    // 如果下家地主只剩一手牌，则出比地主大的牌，或别的其他的牌
    farmer_initiative_playcard_next_landlord_only_1_move(return_list, pai_list, move_list, next_move_list);
    if (!return_list.empty())
        return return_list;

    // 确定主从关系
    if ( decide_self_is_master(move_list, next_move_list) )
    {
        bool isAlone = false;
        if (next_move_list.size() == 1)
        {
            ddz_move* move = dynamic_cast<ddz_move*>(next_move_list[0]);
            isAlone = move->_type == ddz_type_alone_1;
        }
        common_landlord_playcard(return_list, move_list, isAlone);
    }
    else
    {

    }

    // 如果下家地主只剩一手牌，则出比地主大的牌，或别的其他的牌
    farmer_initiative_playcard_next_landlord_only_1_move(return_list, pai_list, move_list, next_move_list);
    if (!return_list.empty())
        return return_list;

    if (next_move_list.size() == 1)
    {
        // 从最大开始出牌
        ddz_move* landlord_move = dynamic_cast<ddz_move*>(next_move_list[0]);
        sendout_from_max_to_min_playcard(p_state, return_list, move_list, landlord_move);
    }

    if (return_list.empty())
        common_landlord_playcard(return_list, move_list);

    if (return_list.empty())
    {
        for(int i = pai_type_3; i <= pai_type_2; ++i)
        {
            if (pai_list[i] > 3)
            {
                return_list.push_back(new ddz_move(ddz_type_bomb, i));
            }
            else if (pai_list[i] > 2)
            {
                return_list.push_back(new ddz_move(ddz_type_triple, i));
            }
            else if (pai_list[i] > 1)
            {
                return_list.push_back(new ddz_move(ddz_type_pair, i));
            }
            else if (pai_list[i] > 0)
            {
                return_list.push_back(new ddz_move(ddz_type_alone_1, i));
            }

            if (!return_list.empty())
                break;
        }

        if (return_list.empty() && pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0 )
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
        }

        if (return_list.empty()&& pai_list[pai_type_blackjack] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_alone_1, pai_type_blackjack));
        }

        if (return_list.empty()&& pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_alone_1, pai_type_blossom));
        }
    }

    if (return_list.empty())
        return_list.push_back(new ddz_move());

    return return_list;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_initiative_play_card::is_friend_can_control_pai(std::vector<mcts_move*>& out_list, ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, std::vector<mcts_move*>& friend_move_list, int* landlord_pai)
{
    int triple_count = 0;
    int airplane_count = 0;
    int bomb_count = 0;
    int other_alone_count = 0;
    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;

    std::vector<ddz_move*> maxPaiList;

    int size = (int)friend_move_list.size();
    for(int i = 0; i < size; ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(friend_move_list[i]);
        if (is_max_pai(move, landlord_pai, 0))
        {
            if (move->_type == ddz_type_triple)
            {
                triple_count++;
                maxPaiList.push_back(move);
            }
            else if (move->_type == ddz_type_alone_1 || move->_type == ddz_type_pair || move->_type == ddz_type_order || move->_type == ddz_type_order_pair)
                maxPaiList.push_back(move);
            else if (move->_type == ddz_type_airplane)
                airplane_count++;
            else if (move->_type == ddz_type_bomb)
                bomb_count++;
            continue;
        }
        else
        {
            if (move->_type == ddz_type_alone_1)
                alone1.push_back(move->_alone_1);
            else if (move->_type == ddz_type_pair)
                pairs.push_back(move->_alone_1);
            else if (move->_type == ddz_type_triple)
            {
                triple_count++;
                other_alone_count++;
            }
            else
                other_alone_count++;
        }
    }

    int total_alone_count = other_alone_count + alone1.size() + pairs.size()-1-triple_count;
    if (total_alone_count <= 0 && !maxPaiList.empty())
    {
        int size = (int)maxPaiList.size();
        for(int i = 0; i < size; ++i)
        {
            ddz_move* move = dynamic_cast<ddz_move*>(maxPaiList[i]);
            switch (move->_type)
            {
            case ddz_type_alone_1:
                {
                    must_provide_alone1(out_list, pai_list, move_list, move);
                }
                break;
            case ddz_type_pair:
                {
                    must_provide_pair(out_list, pai_list, move_list, move);
                }
                break;
            case ddz_type_triple:
                {
                    must_provide_triple(out_list, pai_list, move_list, move);
                }
                break;

            case ddz_type_order:
                {
                    must_provide_order(out_list, pai_list, move_list, move);
                }
                break;

            case ddz_type_order_pair:
                {
                    must_provide_order_pair(out_list, pai_list, move_list, move);
                }
                break;
            }

            if (!out_list.empty())
                return;
        }
    }

}

/*	std::vector<mcts_move*> move_list;

    fit_strategy_0( p_state, move_list );
    if (!move_list.empty())
        return move_list;

    fit_strategy_1( p_state, move_list );
    if (!move_list.empty())
        return move_list;

    fit_strategy_2( p_state, move_list );
    if (!move_list.empty())
        return move_list;

    ddz_move* select_alone1 = 0;
    ddz_move* select_pairs = 0;
    ddz_move* select_order = 0;
    ddz_move* select_order_pair = 0;
    ddz_move* select_three = 0;
    ddz_move* select_airplane = 0;

    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;

    std::vector<mcts_move*>& pai_move_list = p_state->_out_pai_move_list[p_state->_curr_player_index];

    int size = (int)pai_move_list.size();
    for(int i = 0; i < size; ++i)
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(pai_move_list[i]);
        if (p_move == 0)
            continue;

        if (p_move->_type == ddz_type_alone_1)
        {
            if (p_move->_alone_1 < pai_type_2)
                alone1.push_back(p_move->_alone_1);
            if (select_alone1 == 0 || select_alone1->_alone_1 > p_move->_alone_1)
                select_alone1 = p_move;
        }
        else if (p_move->_type == ddz_type_pair)
        {
            if (p_move->_alone_1 < pai_type_2)
                pairs.push_back(p_move->_alone_1);
            if (select_pairs == 0 || select_pairs->_alone_1 > p_move->_alone_1)
                select_pairs = p_move;
        }
        else if (p_move->_type == ddz_type_triple)
        {
            if (select_three == 0 || select_three->_alone_1 > p_move->_alone_1)
                select_three = p_move;
        }
        else if (p_move->_type == ddz_type_order)
        {
            if (select_order == 0)
                select_order = p_move;
        }
        else if (p_move->_type == ddz_type_order_pair)
        {
            if (select_order_pair == 0)
                select_order_pair = p_move;
        }
        else if (p_move->_type == ddz_type_airplane)
        {
            if (select_airplane == 0)
                select_airplane = p_move;
        }
    }

    std::sort(alone1.begin(), alone1.end());
    std::sort(pairs.begin(), pairs.end());

    // 优先出牌算法
    if (select_airplane != 0)
    {
        int with_pair_size = select_airplane->_combo_count / 3;
        if ((int)alone1.size() >= with_pair_size)
        {
            int withPai[4] = {0, 0, 0, 0};
            for(int i = 0; i < with_pair_size; ++i)
                withPai[i] = alone1[i];

            ddz_move* move = new ddz_move(ddz_type_airplane_with_pai, withPai[0], withPai[1], withPai[2], withPai[3]);
            move->_combo_count = select_airplane->_combo_count;
            memcpy(move->_combo_list, select_airplane->_combo_list, 20*sizeof(unsigned char));
            move_list.push_back(move);
        }
        else if ((int)pairs.size() >= with_pair_size)
        {
            int withPai[4] = {0, 0, 0, 0};
            for(int i = 0; i < with_pair_size; ++i)
                withPai[i] = pairs[i];

            ddz_move* move = new ddz_move(ddz_type_airplane_with_pai, withPai[0], withPai[1], withPai[2], withPai[3]);
            move->_combo_count = select_airplane->_combo_count;
            move->_airplane_pairs = true;
            memcpy(move->_combo_list, select_airplane->_combo_list, 20*sizeof(unsigned char));
            move_list.push_back(move);
        }
        else
            move_list.push_back(select_airplane->clone());
    }
    if (select_order_pair != 0 && move_list.empty())
    {
        move_list.push_back(select_order_pair->clone());
    }
    if (select_order != 0 && move_list.empty())
        move_list.push_back(select_order->clone());

    if (select_three != 0 )
    {
        if ((move_list.empty() && select_three->_alone_1 <= pai_type_J) || ((alone1.size() + pairs.size()) <= 2) )
        {
            if (alone1.size() > 0)
            {
                std::sort(alone1.begin(), alone1.end());
                move_list.push_back(new ddz_move(ddz_type_triple_1, select_three->_alone_1, alone1[0]));
            }
            else if (pairs.size() > 0)
            {
                std::sort(pairs.begin(), pairs.end());
                move_list.push_back(new ddz_move(ddz_type_triple_2, select_three->_alone_1, pairs[0]));
            }
            else
                move_list.push_back(select_three->clone());
        }
    }

    {
        if (select_alone1 != 0)
            move_list.push_back(select_alone1->clone());
        if (select_pairs != 0)
            move_list.push_back(select_pairs->clone());
    }

    if (move_list.empty())
    {
        // 无牌打时，就打王炸和炸弹
        for(int i = 0; i < size; ++i)
        {
            ddz_move* p_move = dynamic_cast<ddz_move*>(pai_move_list[i]);
            if (p_move == 0)
                continue;

            if (p_move->_type == ddz_type_bomb || p_move->_type == ddz_type_king_bomb)
                move_list.push_back(p_move->clone());
        }
    }

    if (move_list.size() == 1 )
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[0]);
        if (move->_type == ddz_type_pair && pai_move_list.size() > 1)
        {
            // 如果出牌为一对牌时，下家是对立面，可以一下出光，则把对牌改成单牌
            int next_player_index = p_state->_curr_player_index + 1;
            if (next_player_index > 2)
                next_player_index = 0;

            if (p_state->_player_list[next_player_index].is_landlord != p_state->_player_list[p_state->_curr_player_index].is_landlord)
            {
                if ( p_state->_out_pai_move_list[next_player_index].size() == 1 )
                {
                    ddz_move* next_move = dynamic_cast<ddz_move*>(p_state->_out_pai_move_list[next_player_index][0]);
                    if (next_move->_type == ddz_type_pair && next_move->_alone_1 > move->_alone_1)
                    {
                        unsigned char alone = move->_alone_1;
                        delete move;
                        move_list.clear();
                        move_list.push_back(new ddz_move(ddz_type_alone_1, alone));
                    }
                }
            }

        }
    }

    return move_list;
}
*/
