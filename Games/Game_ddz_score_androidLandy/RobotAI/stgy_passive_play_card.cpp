
#include "stgy_passive_play_card.h"
#include <algorithm>
#include "ddz_state.h"

/*-------------------------------------------------------------------------------------------------------------------*/
stgy_passive_play_card::stgy_passive_play_card()
{

}

/*-------------------------------------------------------------------------------------------------------------------*/
stgy_passive_play_card::~stgy_passive_play_card()
{

}

/*-------------------------------------------------------------------------------------------------------------------*/
static bool is_max_pai(ddz_move* p_move, int* next_pai_list, int* prev_pai_list);

/*-------------------------------------------------------------------------------------------------------------------*/
static inline int move_cmp(const void *a,const void *b)
{
    return *(unsigned char *)a-*(unsigned char *)b;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void add_move_to_list(std::vector<mcts_move*>& move_list, ddz_move* move)
{
    int size = (int)move_list.size();
    for(int i = 0; i < size; ++i)
    {
        if (move_list[i]->compare(move))
        {
            delete move;
            return;
        }
    }

    move_list.push_back(move);
}



/*-------------------------------------------------------------------------------------------------------------------*/
static inline void select_4_with_pairs( std::vector<mcts_move*>& out_list,  int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    ddz_move* select_move = 0;

    int size = (int)move_list.size();
    std::vector<unsigned char> pairs;

    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_bomb && p_move->_alone_1 > limit_move->_alone_1)
        {
            if (select_move == 0)
                select_move = p_move;
            else if (p_move->_alone_1 < select_move->_alone_1)
                select_move = p_move;
        }

        else if (p_move->_type == ddz_type_pair && p_move->_alone_1 < pai_type_2)
            pairs.push_back(p_move->_alone_1);
    }

    if (select_move != 0 && pairs.size() > 1)
    {
        std::sort( pairs.begin(), pairs.end() );
        ddz_move* move = new ddz_move(ddz_type_four_with_pairs, select_move->_alone_1, pairs[0], pairs[1]);
        out_list.push_back(move);
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline void select_4_with_alone1( std::vector<mcts_move*>& out_list, int* pai_list,  std::vector<mcts_move*>& move_list, ddz_move* limit_move )
{
    ddz_move* select_move = 0;

    int size = (int)move_list.size();
    std::vector<unsigned char> alone_1;
    std::vector<unsigned char> pairs;

    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_bomb && p_move->_alone_1 > limit_move->_alone_1)
        {
            if (select_move == 0)
                select_move = p_move;
            else if (p_move->_alone_1 < select_move->_alone_1)
                select_move = p_move;
        }

        if (p_move->_type == ddz_type_alone_1 && p_move->_alone_1 < pai_type_2)
            alone_1.push_back(p_move->_alone_1);
        else if (p_move->_type == ddz_type_pair && p_move->_alone_1 < pai_type_2)
            pairs.push_back(p_move->_alone_1);
    }

    if (select_move != 0 && (pairs.size() > 0 || alone_1.size() > 1))
    {
        unsigned char with1 = 0;
        unsigned char with2 = 0;

        std::sort( alone_1.begin(), alone_1.end() );
        std::sort( pairs.begin(), pairs.end() );
        if ( pairs.size() == 0 && alone_1.size() > 1)
        {
            with1 = alone_1[0];
            with2 = alone_1[1];
        }
        else if (alone_1.size() == 0 && pairs.size() > 0)
        {
            with1 = pairs[0];
            with2 = pairs[1];
        }
        else if (alone_1.size() > 1 && alone_1[1] <= pai_type_J)
        {
            with1 = alone_1[0];
            with2 = alone_1[1];
        }
        else if (pairs.size() > 0 && pairs[0] <= pai_type_10 )
        {
            with1 = pairs[0];
            with2 = pairs[0];
        }
        else if (alone_1.size() > 1)
        {
            with1 = alone_1[0];
            with2 = alone_1[1];
        }
        else if (pairs.size() > 0)
        {
            with1 = pairs[0];
            with2 = pairs[0];
        }

        if (with1 != 0 && with2 != 0)
        {
            ddz_move* move = new ddz_move(ddz_type_four_with_alone_1, select_move->_alone_1, with1, with2);
            out_list.push_back(move);
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline void select_bomb( std::vector<mcts_move*>& out_list, int* pai_list,  std::vector<mcts_move*>& move_list, ddz_move* limit_move )
{
    ddz_move* select_move = 0;

    int size = (int)move_list.size();
    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_bomb && p_move->_alone_1 > limit_move->_alone_1)
        {
            if (select_move == 0)
                select_move = p_move;
            else if (p_move->_alone_1 < select_move->_alone_1)
                select_move = p_move;
        }
    }

    if (select_move != 0)
        out_list.push_back(select_move->clone());
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
bool stgy_passive_play_card::have_greater_than_limit_move(int* pai_list, ddz_move* limit_move)
{
    if (limit_move->_type == ddz_type_king_bomb)
        return false;

    if (limit_move->_type == ddz_type_bomb)
    {
        bool have_greater_move = false;
        for(int i = limit_move->_alone_1 + 1; i <= pai_type_2; ++i)
        {
            if (pai_list[i] > 3)
            {
                have_greater_move = true;
                break;
            }
        }

        if (!have_greater_move && pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0)
            have_greater_move = true;

        return have_greater_move;
    }

    bool have_greater_move = false;

    switch (limit_move->_type)
    {
    case ddz_type_alone_1:
    case ddz_type_pair:
    case ddz_type_triple:
        {
            int limit_count = 0;
            if (limit_move->_type == ddz_type_pair)
                limit_count = 1;
            else if (limit_move->_type == ddz_type_triple)
                limit_count = 2;

            int max_pai = pai_type_2;
            if (limit_move->_type == ddz_type_alone_1)
            {
                if (!( pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0 ))
                {
                    max_pai = pai_type_blossom;
                }
            }

            for(int i = limit_move->_alone_1 + 1; i <= max_pai; ++i)
            {
                if (pai_list[i] > limit_count)
                {
                    have_greater_move = true;
                    break;
                }
            }
        }
        break;

    case ddz_type_triple_1:
    case ddz_type_triple_2:
    case ddz_type_four_with_alone_1:
    case ddz_type_four_with_pairs:
        {
            int limit_count = 2;
            if (limit_move->_type == ddz_type_four_with_alone_1 || limit_move->_type == ddz_type_four_with_pairs)
                limit_count = 3;

            bool havePai = false;
            for(int i = limit_move->_alone_1 + 1; i <= pai_type_2; ++i)
            {
                if (pai_list[i] > limit_count)
                {
                    have_greater_move = true;
                    havePai = true;
                    break;
                }
            }

            if (havePai)
            {

            }

        }
        break;

    case ddz_type_order:
        {
            int orderLen = limit_move->_combo_count;
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            for(int i = limit_move->_combo_list[0]+1; i <= pai_type_10; ++i)
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
                        have_greater_move = true;
                        break;
                    }
                }
            }
        }
        break;

    case ddz_type_order_pair:
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            for(int i = limit_move->_combo_list[0]+1; i <= pai_type_Q; ++i)
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
                        have_greater_move = true;
                        break;
                    }
                }
            }
        }
        break;

    case ddz_type_airplane:
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            for(int i = limit_move->_combo_list[0]+1; i <= pai_type_K; ++i)
            {
                if (pai_list[i] > 2 && pai_list[i+1] > 2)
                {
                    int retainIndexCount = limit_move->_combo_count/3-2;
                    for(int j = i+2; j <= pai_type_A; ++j)
                    {
                        if (retainIndexCount == 0)
                            break;

                        if (pai_list[j] <= 2)
                            break;

                        retainIndexCount--;
                    }

                    if (retainIndexCount == 0)
                    {
                        have_greater_move = true;
                        break;
                    }
                }
            }
        }
        break;
    }

    if (!have_greater_move)
    {
        for(int i = 3; i <= pai_type_2; ++i)
        {
            if (pai_list[i] > 3)
            {
                have_greater_move = true;
                break;
            }
        }

        if (!have_greater_move && pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0)
            have_greater_move = true;
    }

    return have_greater_move;
}

/*-------------------------------------------------------------------------------------------------------------------*/
// 拆出能够管住limit_move的牌
std::vector<mcts_move*> stgy_passive_play_card::parse_pai(ddz_state* p_state, ddz_move* limit_move)
{
    // 如果管不住牌，则直接返回no move
    int next_player_index = p_state->_curr_player_index;
    int* pai_list = p_state->_player_list[next_player_index].pai_list;
    std::vector<mcts_move*>& move_list = p_state->_out_pai_move_list[next_player_index];

    if (!have_greater_than_limit_move(pai_list, limit_move))
    {
        std::vector<mcts_move*> return_list;
        return_list.push_back(new ddz_move());
        return return_list;
    }

    next_player_index++;
    if (next_player_index > 2)
        next_player_index = 0;
    int* next_pai_list = p_state->_player_list[next_player_index].pai_list;
    std::vector<mcts_move*>& next_move_list = p_state->_out_pai_move_list[next_player_index];
    bool out_is_next = (next_player_index == p_state->_curr_out_player_index);
    bool landlord_is_next = p_state->_player_list[next_player_index].is_landlord;

    next_player_index++;
    if (next_player_index > 2)
        next_player_index = 0;
    int* prev_pai_list = p_state->_player_list[next_player_index].pai_list;
    std::vector<mcts_move*>& prev_move_list = p_state->_out_pai_move_list[next_player_index];

    if (p_state->_player_list[p_state->_curr_player_index].is_landlord)
    {
        if (out_is_next)
            return landlord_passive_playcard_next_outcard(p_state, limit_move, pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list );
        else
            return landlord_passive_playcard_prev_outcard(p_state, limit_move,  pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list );
    }

    if (landlord_is_next)
    {
        if (out_is_next)
            return farmer_passive_playcard_next_landlord_and_next_outcard(p_state, limit_move, pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list );
        else
            return farmer_passive_playcard_next_landlord_and_prev_outcard(p_state, limit_move,  pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list );
    }

    if (out_is_next)
        return farmer_passive_playcard_next_farmer_and_next_outcard(p_state, limit_move, pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list );

    return farmer_passive_playcard_next_farmer_and_prev_outcard(p_state, limit_move, pai_list, move_list, next_pai_list, next_move_list, prev_pai_list, prev_move_list );
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_alone_1(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_alone_1)
        {
            if (move->_alone_1 > limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        unsigned char control_pai = pai_type_blossom;
        if (pai_list[pai_type_blossom] > 0 && pai_list[pai_type_blackjack] > 0)
            control_pai = pai_type_2;

        unsigned char can_control_pai = 0;

        for(int i = control_pai; i > limit_move->_alone_1; --i)
        {
            if (pai_list[i] < 4 && pai_list[i] > 0)
            {
                can_control_pai = i;
                break;
            }
        }

        if (can_control_pai != 0)
        {
            alone1 = can_control_pai;
        }
        else
        {
            if (can_control_pai == 0 && pai_list[pai_type_K] > 0 && pai_type_K > limit_move->_alone_1)
                can_control_pai = pai_type_K;
            if (can_control_pai == 0 && pai_list[pai_type_A] > 0 && pai_type_A > limit_move->_alone_1)
                can_control_pai = pai_type_A;
            if (can_control_pai == 0 && pai_list[pai_type_2] > 0 && pai_type_2 > limit_move->_alone_1)
                can_control_pai = pai_type_2;
            if (can_control_pai == 0 && pai_list[pai_type_blackjack] > 0 && pai_type_blackjack > limit_move->_alone_1)
                can_control_pai = pai_type_blackjack;
            if (can_control_pai == 0 && pai_list[pai_type_blossom] > 0 && pai_type_blossom > limit_move->_alone_1)
                can_control_pai = pai_type_blossom;

            if ( can_control_pai != 0 )
                alone1 = can_control_pai;
        }
    }

    if (alone1 != 0)
    {
        out_list.push_back(new ddz_move(ddz_type_alone_1, alone1));
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_pair(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_pair)
        {
            if (move->_alone_1 > limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_2; i > limit_move->_alone_1; --i)
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
static void must_control_triple(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_triple)
        {
            if (move->_alone_1 > limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_2; i > limit_move->_alone_1; --i)
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
        out_list.push_back(new ddz_move(ddz_type_triple, alone1));
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_triple_1(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    std::vector<unsigned char> al;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_triple)
        {
            if (move->_alone_1 > limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
        else if (move->_type == ddz_type_alone_1)
            al.push_back(move->_alone_1);
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_2; i > limit_move->_alone_1; --i)
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
        if (!al.empty())
        {
            std::sort(al.begin(), al.end());
            out_list.push_back(new ddz_move(ddz_type_triple_1, alone1, al[0]));
        }
        else
        {
            unsigned char with_pai = 0;
            for(int i = pai_type_3; i <= pai_type_blossom; ++i )
            {
                if (pai_list[i] <= 0)
                    continue;

                if (i == alone1)
                    continue;

                with_pai = i;
                break;
            }

            if (with_pai != 0)
            {
                out_list.push_back(new ddz_move(ddz_type_triple_1, alone1, with_pai));
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_triple_2(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    std::vector<unsigned char> pl;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_triple)
        {
            if (move->_alone_1 > limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
        else if (move->_type == ddz_type_pair)
            pl.push_back(move->_alone_1);
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_2; i > limit_move->_alone_1; --i)
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
        if (!pl.empty())
        {
            std::sort(pl.begin(), pl.end());
            out_list.push_back(new ddz_move(ddz_type_triple_2, alone1, pl[0]));
        }
        else
        {
            unsigned char with_pai = 0;
            for(int i = pai_type_3; i <= pai_type_blossom; ++i )
            {
                if (pai_list[i] <= 1)
                    continue;

                if (i == alone1)
                    continue;

                with_pai = i;
                break;
            }

            if (with_pai != 0)
            {
                out_list.push_back(new ddz_move(ddz_type_triple_2, alone1, with_pai));
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_order(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_order && move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] > limit_move->_combo_list[0])
            {
                out_list.push_back(move->clone());
                break;
            }
        }
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = limit_move->_combo_list[0] + 1; i <= pai_type_10; ++i)
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
static void must_control_order_pair(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_order_pair && move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] > limit_move->_combo_list[0])
            {
                out_list.push_back(move->clone());
                break;
            }
        }
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = limit_move->_combo_list[0] + 1; i <= pai_type_Q; ++i)
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
                    ddz_move* mp = new ddz_move(ddz_type_order_pair);
                    mp->_combo_count = 6;
                    mp->_combo_list[0] = i;mp->_combo_list[1] = i;
                    mp->_combo_list[2] = i+1;mp->_combo_list[3] = i+1;
                    mp->_combo_list[4] = i+2;mp->_combo_list[5] = i+2;
                    int retainIndexCount = limit_move->_combo_count/2-3;
                    for(int j = i+3; j <= i+2+retainIndexCount; ++j)
                    {
                        mp->_combo_list[mp->_combo_count++] = j;
                        mp->_combo_list[mp->_combo_count++] = j;
                    }
                    out_list.push_back(mp);
                    break;
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_airplane(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_airplane && move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] > limit_move->_combo_list[0])
            {
                out_list.push_back(move->clone());
                break;
            }
        }
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = limit_move->_combo_list[0] + 1; i <= pai_type_K; ++i)
        {
            if (pai_list[i] > 2 && pai_list[i+1] > 2 )
            {
                int retainIndexCount = limit_move->_combo_count/3-2;
                for(int j = i+2; j <= pai_type_A; ++j)
                {
                    if (retainIndexCount == 0)
                        break;

                    if (pai_list[j] <= 2)
                        break;

                    retainIndexCount--;
                }

                if (retainIndexCount == 0)
                {
                    ddz_move* mp = new ddz_move(ddz_type_airplane);
                    mp->_combo_count = 6;
                    mp->_combo_list[0] = i; mp->_combo_list[1] = i; mp->_combo_list[2] = i;
                    mp->_combo_list[3] = i+1; mp->_combo_list[4] = i+1; mp->_combo_list[5] = i+1;
                    int retainIndexCount = limit_move->_combo_count/3-2;
                    for(int j = i+2; j <= i+1+retainIndexCount; ++j)
                    {
                        mp->_combo_list[mp->_combo_count++] = j;
                        mp->_combo_list[mp->_combo_count++] = j;
                        mp->_combo_list[mp->_combo_count++] = j;
                    }

                    out_list.push_back(mp);
                    break;
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_airplane_with_pai(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    if (limit_move->_combo_count > 6)
        return;

    ddz_move* select_move = 0;
    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_airplane && move->_combo_count == limit_move->_combo_count && select_move == 0)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] > limit_move->_combo_list[0])
            {
                //select_move = dynamic_cast<ddz_move*>(move->clone());
            }
        }

        if (move->_type == ddz_type_alone_1)
            alone1.push_back(move->_alone_1);
        else if (move->_type == ddz_type_pair)
            pairs.push_back(move->_alone_1);
    }

    if (select_move)
    {
        //delete select_move;
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = limit_move->_combo_list[0] + 1; i <= pai_type_K; ++i)
        {
            if (pai_list[i] > 2 && pai_list[i+1] > 2 )
            {
                int retainIndexCount = limit_move->_combo_count/3-2;
                for(int j = i+2; j <= pai_type_A; ++j)
                {
                    if (retainIndexCount == 0)
                        break;

                    if (pai_list[j] <= 2)
                        break;

                    retainIndexCount--;
                }

                if (retainIndexCount == 0)
                {
                    select_move = new ddz_move(ddz_type_airplane);
                    select_move->_combo_count = 6;
                    select_move->_combo_list[0] = i; select_move->_combo_list[1] = i; select_move->_combo_list[2] = i;
                    select_move->_combo_list[3] = i+1; select_move->_combo_list[4] = i+1; select_move->_combo_list[5] = i+1;
                    int retainIndexCount = limit_move->_combo_count/3-2;
                    for(int j = i+2; j <= i+1+retainIndexCount; ++j)
                    {
                        select_move->_combo_list[select_move->_combo_count++] = j;
                        select_move->_combo_list[select_move->_combo_count++] = j;
                        select_move->_combo_list[select_move->_combo_count++] = j;
                    }
                    break;
                }
            }
        }
    }

    if (select_move)
    {
        if (limit_move->_airplane_pairs)
        {
            if (pairs.size() >= 2)
            {
                select_move->_type = ddz_type_airplane_with_pai;
                select_move->_alone_1 = pairs[0];
                select_move->_alone_2 = pairs[1];
                select_move->_airplane_pairs = true;
                out_list.push_back(select_move);
            }
            else
            {
                delete select_move;
            }
        }
        else
        {
            if (alone1.size() >= 2)
            {
                select_move->_type = ddz_type_airplane_with_pai;
                select_move->_alone_1 = alone1[0];
                select_move->_alone_2 = alone1[1];
                out_list.push_back(select_move);
            }
            else if (pairs.size() >= 1)
            {
                select_move->_type = ddz_type_airplane_with_pai;
                select_move->_alone_1 = pairs[0];
                select_move->_alone_2 = pairs[0];
                out_list.push_back(select_move);
            }
            else
            {
                delete select_move;
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_this_move_no_bomb(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    switch (limit_move->_type)
    {
    case ddz_type_alone_1:
        must_control_alone_1(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_pair:
        must_control_pair(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_triple:
        must_control_triple(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_triple_1:
        must_control_triple_1(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_triple_2:
        must_control_triple_2(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_order:
        must_control_order(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_order_pair:
        must_control_order_pair(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_airplane:
        must_control_airplane(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_airplane_with_pai:
        must_control_airplane_with_pai(out_list, pai_list, move_list, limit_move);
        break;
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_control_this_move(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    must_control_this_move_no_bomb(out_list, pai_list, move_list, limit_move);

//	if (out_list.empty())
    {
        // 炸弹
        if (limit_move->_type != ddz_type_king_bomb && limit_move->_type != ddz_type_bomb)
        {
            for(int i = pai_type_3; i <= pai_type_2; ++i)
            {
                if (pai_list[i] >= 4)
                {
                    add_move_to_list(out_list, new ddz_move(ddz_type_bomb, i));
                }
            }
        }

//		if (out_list.empty())
        {
            // 王炸
            if (pai_list[pai_type_blackjack] == 1 && pai_list[pai_type_blossom] == 1)
                add_move_to_list(out_list, new ddz_move(ddz_type_king_bomb));
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_equal_alone_1(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_alone_1)
        {
            if (move->_alone_1 >= limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        unsigned char control_pai = pai_type_blossom;
        if (pai_list[pai_type_blossom] > 0 && pai_list[pai_type_blackjack] > 0)
            control_pai = pai_type_2;

        unsigned char can_control_pai = 0;

        for(int i = control_pai; i >= limit_move->_alone_1; --i)
        {
            if (pai_list[i] < 4 && pai_list[i] > 0)
            {
                can_control_pai = i;
                break;
            }
        }

        if (can_control_pai != 0)
        {
            alone1 = can_control_pai;
        }
        else
        {
            if (can_control_pai == 0 && pai_list[pai_type_K] > 0 && pai_type_K >= limit_move->_alone_1)
                can_control_pai = pai_type_K;
            if (can_control_pai == 0 && pai_list[pai_type_A] > 0 && pai_type_A >= limit_move->_alone_1)
                can_control_pai = pai_type_A;
            if (can_control_pai == 0 && pai_list[pai_type_2] > 0 && pai_type_2 >= limit_move->_alone_1)
                can_control_pai = pai_type_2;
            if (can_control_pai == 0 && pai_list[pai_type_blackjack] > 0 && pai_type_blackjack >= limit_move->_alone_1)
                can_control_pai = pai_type_blackjack;
            if (can_control_pai == 0 && pai_list[pai_type_blossom] > 0 && pai_type_blossom >= limit_move->_alone_1)
                can_control_pai = pai_type_blossom;

            if ( can_control_pai != 0 )
                alone1 = can_control_pai;
        }
    }

    if (alone1 != 0)
    {
        out_list.push_back(new ddz_move(ddz_type_alone_1, alone1));
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_equal_pair(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_pair)
        {
            if (move->_alone_1 >= limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_2; i >= limit_move->_alone_1; --i)
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
static void must_equal_triple(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_triple)
        {
            if (move->_alone_1 >= limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_2; i >= limit_move->_alone_1; --i)
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
        out_list.push_back(new ddz_move(ddz_type_triple, alone1));
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_equal_triple_1(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    std::vector<unsigned char> al;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_triple)
        {
            if (move->_alone_1 >= limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
        else if (move->_type == ddz_type_alone_1)
            al.push_back(move->_alone_1);
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_2; i >= limit_move->_alone_1; --i)
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
        if (!al.empty())
        {
            std::sort(al.begin(), al.end());
            out_list.push_back(new ddz_move(ddz_type_triple_1, alone1, al[0]));
        }
        else
        {
            unsigned char with_pai = 0;
            for(int i = pai_type_3; i <= pai_type_blossom; ++i )
            {
                if (pai_list[i] <= 0)
                    continue;

                if (i == alone1)
                    continue;

                with_pai = i;
                break;
            }

            if (with_pai != 0)
            {
                out_list.push_back(new ddz_move(ddz_type_triple_1, alone1, with_pai));
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_equal_triple_2(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    unsigned char alone1 = 0;
    std::vector<unsigned char> pl;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_triple)
        {
            if (move->_alone_1 >= limit_move->_alone_1)
            {
                if (alone1 < move->_alone_1)
                    alone1 = move->_alone_1;
            }
        }
        else if (move->_type == ddz_type_pair)
            pl.push_back(move->_alone_1);
    }

    if (alone1 == 0)
    {
        for(int i = pai_type_2; i >= limit_move->_alone_1; --i)
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
        if (!pl.empty())
        {
            std::sort(pl.begin(), pl.end());
            out_list.push_back(new ddz_move(ddz_type_triple_2, alone1, pl[0]));
        }
        else
        {
            unsigned char with_pai = 0;
            for(int i = pai_type_3; i <= pai_type_blossom; ++i )
            {
                if (pai_list[i] <= 1)
                    continue;

                if (i == alone1)
                    continue;

                with_pai = i;
                break;
            }

            if (with_pai != 0)
            {
                out_list.push_back(new ddz_move(ddz_type_triple_2, alone1, with_pai));
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_equal_order(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_order && move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] >= limit_move->_combo_list[0])
            {
                out_list.push_back(move->clone());
                break;
            }
        }
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = limit_move->_combo_list[0]; i <= pai_type_10; ++i)
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
static void must_equal_order_pair(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_order_pair && move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] >= limit_move->_combo_list[0])
            {
                out_list.push_back(move->clone());
                break;
            }
        }
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = limit_move->_combo_list[0]; i <= pai_type_Q; ++i)
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
                    ddz_move* mp = new ddz_move(ddz_type_order_pair);
                    mp->_combo_count = 6;
                    mp->_combo_list[0] = i;mp->_combo_list[1] = i;
                    mp->_combo_list[2] = i+1;mp->_combo_list[3] = i+1;
                    mp->_combo_list[4] = i+2;mp->_combo_list[5] = i+2;
                    int retainIndexCount = limit_move->_combo_count/2-3;
                    for(int j = i+3; j <= i+2+retainIndexCount; ++j)
                    {
                        mp->_combo_list[mp->_combo_count++] = j;
                        mp->_combo_list[mp->_combo_count++] = j;
                    }
                    out_list.push_back(mp);
                    break;
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_equal_airplane(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_airplane && move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] >= limit_move->_combo_list[0])
            {
                out_list.push_back(move->clone());
                break;
            }
        }
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = limit_move->_combo_list[0]; i <= pai_type_K; ++i)
        {
            if (pai_list[i] > 2 && pai_list[i+1] > 2 )
            {
                int retainIndexCount = limit_move->_combo_count/3-2;
                for(int j = i+2; j <= pai_type_A; ++j)
                {
                    if (retainIndexCount == 0)
                        break;

                    if (pai_list[j] <= 2)
                        break;

                    retainIndexCount--;
                }

                if (retainIndexCount == 0)
                {
                    ddz_move* mp = new ddz_move(ddz_type_airplane);
                    mp->_combo_count = 6;
                    mp->_combo_list[0] = i; mp->_combo_list[1] = i; mp->_combo_list[2] = i;
                    mp->_combo_list[3] = i+1; mp->_combo_list[4] = i+1; mp->_combo_list[5] = i+1;
                    int retainIndexCount = limit_move->_combo_count/3-2;
                    for(int j = i+2; j <= i+1+retainIndexCount; ++j)
                    {
                        mp->_combo_list[mp->_combo_count++] = j;
                        mp->_combo_list[mp->_combo_count++] = j;
                        mp->_combo_list[mp->_combo_count++] = j;
                    }

                    out_list.push_back(mp);
                    break;
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void must_equal_airplane_with_pai(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    if (limit_move->_combo_count > 6)
        return;

    ddz_move* select_move = 0;
    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_airplane && move->_combo_count == limit_move->_combo_count && select_move == 0)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(move->_combo_list, move->_combo_count, sizeof(unsigned char), move_cmp);

            if (move->_combo_list[0] >= limit_move->_combo_list[0])
            {
                //select_move = dynamic_cast<ddz_move*>(move->clone());
            }
        }

        if (move->_type == ddz_type_alone_1)
            alone1.push_back(move->_alone_1);
        else if (move->_type == ddz_type_pair)
            pairs.push_back(move->_alone_1);
    }

    if (select_move)
    {
        //delete select_move;
    }

    if (out_list.empty())
    {
        qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
        for(int i = limit_move->_combo_list[0]; i <= pai_type_K; ++i)
        {
            if (pai_list[i] > 2 && pai_list[i+1] > 2 )
            {
                int retainIndexCount = limit_move->_combo_count/3-2;
                for(int j = i+2; j <= pai_type_A; ++j)
                {
                    if (retainIndexCount == 0)
                        break;

                    if (pai_list[j] <= 2)
                        break;

                    retainIndexCount--;
                }

                if (retainIndexCount == 0)
                {
                    select_move = new ddz_move(ddz_type_airplane);
                    select_move->_combo_count = 6;
                    select_move->_combo_list[0] = i; select_move->_combo_list[1] = i; select_move->_combo_list[2] = i;
                    select_move->_combo_list[3] = i+1; select_move->_combo_list[4] = i+1; select_move->_combo_list[5] = i+1;
                    int retainIndexCount = limit_move->_combo_count/3-2;
                    for(int j = i+2; j <= i+1+retainIndexCount; ++j)
                    {
                        select_move->_combo_list[select_move->_combo_count++] = j;
                        select_move->_combo_list[select_move->_combo_count++] = j;
                        select_move->_combo_list[select_move->_combo_count++] = j;
                    }
                    break;
                }
            }
        }
    }

    if (select_move)
    {
        if (limit_move->_airplane_pairs)
        {
            if (pairs.size() >= 2)
            {
                select_move->_type = ddz_type_airplane_with_pai;
                select_move->_alone_1 = pairs[0];
                select_move->_alone_2 = pairs[1];
                select_move->_airplane_pairs = true;
                out_list.push_back(select_move);
            }
            else
            {
                delete select_move;
            }
        }
        else
        {
            if (alone1.size() >= 2)
            {
                select_move->_type = ddz_type_airplane_with_pai;
                select_move->_alone_1 = alone1[0];
                select_move->_alone_2 = alone1[1];
                out_list.push_back(select_move);
            }
            else if (pairs.size() >= 1)
            {
                select_move->_type = ddz_type_airplane_with_pai;
                select_move->_alone_1 = pairs[0];
                select_move->_alone_2 = pairs[0];
                out_list.push_back(select_move);
            }
            else
            {
                delete select_move;
            }
        }
    }
}


/*-------------------------------------------------------------------------------------------------------------------*/
static void must_equal_this_move_no_bomb(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    switch (limit_move->_type)
    {
    case ddz_type_alone_1:
        must_equal_alone_1(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_pair:
        must_equal_pair(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_triple:
        must_equal_triple(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_triple_1:
        must_equal_triple_1(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_triple_2:
        must_equal_triple_2(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_order:
        must_equal_order(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_order_pair:
        must_equal_order_pair(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_airplane:
        must_equal_airplane(out_list, pai_list, move_list, limit_move);
        break;

    case ddz_type_airplane_with_pai:
        must_equal_airplane_with_pai(out_list, pai_list, move_list, limit_move);
        break;
    }
}

static void must_equal_this_move(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move)
{
    must_equal_this_move_no_bomb(out_list, pai_list, move_list, limit_move);


    if (out_list.empty())
    {
        // 炸弹
        if (limit_move->_type != ddz_type_king_bomb && limit_move->_type != ddz_type_bomb)
        {
            for(int i = pai_type_3; i <= pai_type_2; ++i)
            {
                if (pai_list[i] >= 4)
                {
                    add_move_to_list(out_list, new ddz_move(ddz_type_bomb, i));
                }
            }
        }

        if (out_list.empty())
        {
            // 王炸
            if (pai_list[pai_type_blackjack] == 1 && pai_list[pai_type_blossom] == 1)
                add_move_to_list(out_list, new ddz_move(ddz_type_king_bomb));
        }
    }
}

static void select_alone_1(std::vector<mcts_move*>& out_list, int* pai_list,  std::vector<mcts_move*>& move_list, ddz_move* limit_move, bool isFriend, bool armyHaveBomb )
{
    int size = (int)move_list.size();
    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_alone_1 && p_move->_alone_1 > limit_move->_alone_1 && p_move->_alone_1 <= pai_type_A)
        {
            out_list.push_back(p_move->clone());
        }
    }

    if (out_list.empty())
    {
        int size = (int)move_list.size();
        for( int i = 0; i < size; ++i )
        {
            ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
            if (!p_move)
                continue;

            if (p_move->_type == ddz_type_alone_1 && p_move->_alone_1 > limit_move->_alone_1 && p_move->_alone_1 <= pai_type_2)
            {
                out_list.push_back(p_move->clone());
            }
        }
    }

    if (out_list.empty())
    {
        if (limit_move->_alone_1 < pai_type_2 && pai_list[pai_type_2] > 0)
        {
            out_list.push_back(new ddz_move(ddz_type_alone_1, pai_type_2));
        }
    }

    if (out_list.empty())
    {
        int size = (int)move_list.size();
        for( int i = 0; i < size; ++i )
        {
            ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
            if (!p_move)
                continue;

            if (p_move->_type == ddz_type_alone_1 && p_move->_alone_1 > limit_move->_alone_1)
            {
                out_list.push_back(p_move->clone());
            }
        }
    }

    // 拆对子或三个
    if (out_list.empty())
    {
        int size = (int)move_list.size();
        for( int i = 0; i < size; ++i )
        {
            ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
            if (p_move->_type == ddz_type_pair && p_move->_alone_1 > limit_move->_alone_1)
            {
                out_list.push_back(new ddz_move(ddz_type_alone_1, p_move->_alone_1));
            }
            else if (p_move->_type == ddz_type_triple && p_move->_alone_1 > limit_move->_alone_1)
            {
                out_list.push_back(new ddz_move(ddz_type_alone_1, p_move->_alone_1));
            }
        }
    }

    // 拆王炸
    if (out_list.empty() && pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0 && !isFriend && !armyHaveBomb)
    {
        out_list.push_back(new ddz_move(ddz_type_alone_1, pai_type_blackjack));
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void select_pair(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move )
{
    ddz_move* select_move = 0;

    int size = (int)move_list.size();
    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_pair && p_move->_alone_1 > limit_move->_alone_1 && p_move->_alone_1 <= pai_type_K)
        {
            out_list.push_back(p_move->clone());
        }
    }

    if (out_list.empty())
    {
        int size = (int)move_list.size();
        for( int i = 0; i < size; ++i )
        {
            ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
            if (!p_move)
                continue;

            if (p_move->_type == ddz_type_pair && p_move->_alone_1 > limit_move->_alone_1 && p_move->_alone_1 <= pai_type_A)
            {
                out_list.push_back(p_move->clone());
            }
        }
    }

    if (out_list.empty())
    {
        int size = (int)move_list.size();
        int alone1_count = 0;
        int triple_count = 0;
        int airplane_count = 0;
        for( int i = 0; i < size; ++i)
        {
            ddz_move *m = dynamic_cast<ddz_move*>(move_list[i]);
            if (m->_type == ddz_type_alone_1 && m->_alone_1 < pai_type_2) {
                alone1_count++;
            } else if (m->_type == ddz_type_triple) {
                triple_count++;
            } else if (m->_type == ddz_type_airplane) {
                airplane_count ++;
            }
        }
        // 散牌太多 不要打对2
        if ((alone1_count - triple_count - 2*airplane_count) <= 2)
        {
            for( int i = 0; i < size; ++i )
            {
                ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
                if (!p_move)
                    continue;

                if (p_move->_type == ddz_type_pair && p_move->_alone_1 > limit_move->_alone_1 && p_move->_alone_1 <= pai_type_2)
                {
                    out_list.push_back(p_move->clone());
                }
            }
        }
    }

    if (out_list.empty())
    {
        int size = (int)move_list.size();
        std::vector<unsigned char> order_pairs;
        for( int i = 0; i < size; ++i )
        {
            ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
            if (!p_move)
                continue;

            if (p_move->_type == ddz_type_order_pair)
            {
                for (int j = 0; j < p_move->_combo_count; j++)
                {
                    if (p_move->_combo_list[j] > limit_move->_alone_1)
                        order_pairs.push_back(p_move->_combo_list[j]);
                }
            }
        }
        if (order_pairs.size() > 0)
        {
            std::sort(order_pairs.begin(), order_pairs.end());
            out_list.push_back(new ddz_move(ddz_type_pair, order_pairs[0]));
        }
    }

    if (out_list.empty())
    {
        int size = (int)move_list.size();
        for( int i = 0; i < size; ++i )
        {
            ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
            if (!p_move)
                continue;

            if (p_move->_type == ddz_type_triple && p_move->_alone_1 > limit_move->_alone_1 )
            {
                out_list.push_back(new ddz_move(ddz_type_pair, p_move->_alone_1));
            }

        }
    }

}

/*-------------------------------------------------------------------------------------------------------------------*/
static void select_triple(std::vector<mcts_move*>& out_list, int* pai_list,  std::vector<mcts_move*>& move_list, ddz_move* limit_move )
{
    int exact_alone1 = 0;

    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;
    std::vector<unsigned char> airplanes;

    int size = (int)move_list.size();
    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_triple && p_move->_alone_1 > limit_move->_alone_1 && p_move->_alone_1 < pai_type_2)
        {
            if (exact_alone1 == 0)
                exact_alone1 = p_move->_alone_1;
            else if (p_move->_alone_1 < exact_alone1)
                exact_alone1 = p_move->_alone_1;
        }

        if (p_move->_type == ddz_type_airplane)
        {
            for (int j = 0; j < p_move->_combo_count; j++)
            {
                if (p_move->_combo_list[j] > limit_move->_alone_1)
                {
                    airplanes.push_back(p_move->_combo_list[j]);
                }
            }
        }

        if (p_move->_type == ddz_type_alone_1)
            alone1.push_back(p_move->_alone_1);
        else if (p_move->_type == ddz_type_pair)
            pairs.push_back(p_move->_alone_1);
    }

    if (exact_alone1 == 0 && airplanes.size() > 0) {
        std::sort(airplanes.begin(), airplanes.end());
        exact_alone1 = airplanes[0];
    }

    if (exact_alone1 != 0)
    {
        unsigned char withPai = 0;
        bool have_with_pai = false;
        if (limit_move->_type == ddz_type_triple_1 )
        {
            if (alone1.size() > 0)
            {
                std::sort(alone1.begin(), alone1.end());
                if (alone1[0] < pai_type_2)
                {
                    withPai = alone1[0];
                    have_with_pai = true;
                }
            }
            if (pairs.size() > 0 && !have_with_pai)
            {
                std::sort(pairs.begin(), pairs.end());
                withPai = pairs[0];
                have_with_pai = true;
            }
            if (alone1.size() > 0 && !have_with_pai)
            {
                withPai = alone1[0];
                have_with_pai = true;
            }
        }
        else if (limit_move->_type == ddz_type_triple_2)
        {
            if (pairs.size() > 0)
            {
                std::sort(pairs.begin(), pairs.end());
                withPai = pairs[0];
                have_with_pai = true;
            }
        }

        if (limit_move->_type == ddz_type_triple)
            have_with_pai = true;

        if (have_with_pai)
            out_list.push_back(new ddz_move(limit_move->_type, exact_alone1, withPai));
        else
        {
            // 带牌没有，需要拆牌
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void select_order(std::vector<mcts_move*>& out_list, int* pai_list,  std::vector<mcts_move*>& move_list, ddz_move* limit_move )
{

    ddz_move* select_move = 0;
    std::vector<ddz_move *> orders1;
    std::vector<ddz_move *> orders2;
    int size = (int)move_list.size();
    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_order)
        {
            if (p_move->_combo_count == limit_move->_combo_count) {
                qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
                qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
                if (p_move->_combo_list[0] > limit_move->_combo_list[0])
                {
                    select_move = p_move;
                    break;
                }
                // 找到拆牌 少于两个单牌 或者 拆牌后剩下牌还能组成顺子
            } else if (p_move->_combo_count - limit_move->_combo_count >= 5) {
                qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
                qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
                if (p_move->_combo_list[p_move->_combo_count-1] > limit_move->_combo_list[limit_move->_combo_count-1])
                {
                    orders1.push_back(p_move);
                }
            } else if (p_move->_combo_count - limit_move->_combo_count <= 2) {
                qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
                qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
                if (p_move->_combo_list[p_move->_combo_count-1] > limit_move->_combo_list[limit_move->_combo_count-1])
                {
                    orders2.push_back(p_move);
                }
            }
        }
    }

    if (select_move != 0)
        out_list.push_back(select_move->clone());
    else
    {
        // 出牌列表里没有，则需要拆牌检查
        if (orders1.size() > 0) {
            // 取后面 count 个顺子
            ddz_move* move = new ddz_move(ddz_type_order);
            ddz_move* targ = orders1[0];
            int lcnt = limit_move->_combo_count;
            move->_combo_count = lcnt;
            int j = targ->_combo_count - lcnt;
            for (int i = 0; i < lcnt; i++) {
                move->_combo_list[i] = targ->_combo_list[j];
                j++;
            }
            out_list.push_back(move);
        }
        if (out_list.empty() && orders2.size() > 0)
        {
            ddz_move* move = new ddz_move(ddz_type_order);
            ddz_move* targ = orders2[0];
            int lcnt = limit_move->_combo_count;
            move->_combo_count = lcnt;
            int i = 0;
            for (; i < targ->_combo_count; i++)
            {
                if (targ->_combo_list[i] > limit_move->_combo_list[0])
                {
                    break;
                }
            }
            int k = 0;
            for (int j = i; j < targ->_combo_count; j++)
            {
                move->_combo_list[k] = targ->_combo_list[j];
                k++;
            }
            out_list.push_back(move);
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void select_order_pair(std::vector<mcts_move*>& out_list, int* pai_list,  std::vector<mcts_move*>& move_list, ddz_move* limit_move )
{

    ddz_move* select_move = 0;
    std::vector<ddz_move*> order_pairs;
    int size = (int)move_list.size();
    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_order_pair)
        {
            if (p_move->_combo_count == limit_move->_combo_count) {
                qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
                qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
                if (p_move->_combo_list[0] > limit_move->_combo_list[0])
                {
                    select_move = p_move;
                    break;
                }
            } else if (p_move->_combo_count > limit_move->_combo_count) {
                qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
                qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
                if (p_move->_combo_list[p_move->_combo_count-1] > limit_move->_combo_list[limit_move->_combo_count-1])
                {
                    order_pairs.push_back(p_move);
                }
            }
        }
    }

    if (select_move != 0)
        out_list.push_back(select_move->clone());
    else
    {
        // 出牌列表里没有，则需要拆牌检查
        if (order_pairs.size() > 0) {
            ddz_move* move = new ddz_move(ddz_type_order_pair);
            ddz_move* targ = order_pairs[0];
            int lcnt = limit_move->_combo_count;
            move->_combo_count = lcnt;
            int i = 0;
            for (; i < targ->_combo_count; i++)
            {
                if (targ->_combo_list[i] > limit_move->_combo_list[0])
                {
                    break;
                }
            }
            int k = 0;
            for (int j = i; j < targ->_combo_count; j++)
            {
                move->_combo_list[k] = targ->_combo_list[j];
                k++;
            }
            out_list.push_back(move);
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
inline static bool in_list(const std::vector<unsigned char>& list, unsigned char t)
{
    int size = (int)list.size();
    for(int i = 0; i < size; ++i)
    {
        if (list[i] == t)
            return true;
    }
    return false;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void select_airplane(std::vector<mcts_move*>& out_list, int* pai_list,  std::vector<mcts_move*>& move_list, ddz_move* limit_move )
{

    ddz_move* select_move = 0;

    int size = (int)move_list.size();
    for( int i = 0; i < size; ++i )
    {
        ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
        if (!p_move)
            continue;

        if (p_move->_type == ddz_type_airplane && p_move->_combo_count == limit_move->_combo_count)
        {
            qsort(limit_move->_combo_list, limit_move->_combo_count, sizeof(unsigned char), move_cmp);
            qsort(p_move->_combo_list, p_move->_combo_count, sizeof(unsigned char), move_cmp);
            if (p_move->_combo_list[0] > limit_move->_combo_list[0])
            {
                select_move = p_move;
                break;
            }
        }
    }

    if (select_move != 0)
    {
        std::vector<unsigned char> alone_1;
        std::vector<unsigned char> pairs;
        int with_pai_count = 0;
        if (limit_move->_alone_1 != 0 || limit_move->_alone_2 != 0 || limit_move->_alone_3 != 0 || limit_move->_alone_4 != 0)
        {
            int size = (int)move_list.size();
            for( int i = 0; i < size; ++i )
            {
                ddz_move* p_move = dynamic_cast<ddz_move*>(move_list[i]);
                if (!p_move)
                    continue;

                if (p_move->_type == ddz_type_alone_1 && p_move->_alone_1 < pai_type_2)
                    alone_1.push_back(p_move->_alone_1);
                else if (p_move->_type == ddz_type_pair && p_move->_alone_1 < pai_type_2)
                    pairs.push_back(p_move->_alone_1);
            }

            if (limit_move->_alone_1 != 0)
                with_pai_count++;
            if (limit_move->_alone_2 != 0)
                with_pai_count++;
            if (limit_move->_alone_3 != 0)
                with_pai_count++;
            if (limit_move->_alone_4 != 0)
                with_pai_count++;
        }

        bool haveWithPai = false;
        unsigned char withPai1 = 0;
        unsigned char withPai2 = 0;
        unsigned char withPai3 = 0;
        unsigned char withPai4 = 0;

        std::sort( alone_1.begin(), alone_1.end() );
        std::sort( pairs.begin(), pairs.end() );

        if (limit_move->_airplane_pairs)
        {
            if ((int)pairs.size() >= with_pai_count)
            {
                haveWithPai = true;
                if (with_pai_count >= 1)
                    withPai1 = pairs[0];
                if (with_pai_count >= 2)
                    withPai2 = pairs[1];
                if (with_pai_count >= 3)
                    withPai3 = pairs[2];
                if (with_pai_count >= 4)
                    withPai4 = pairs[3];
            }
        }
        else
        {
            if (with_pai_count == 2)
            {
                if ( alone_1.size() > 0 && alone_1[0] > pai_type_K && pairs.size() > 0 && pairs[0] < alone_1[0] )
                {
                    haveWithPai = true;
                    withPai1 = pairs[0];
                    withPai2 = pairs[0];
                }
                else if ( alone_1.size() > 1 && alone_1[1] > pai_type_A && pairs.size() > 0 && pairs[0] < pai_type_A )
                {
                    haveWithPai = true;
                    withPai1 = pairs[0];
                    withPai2 = pairs[0];
                }
                else if ((int)alone_1.size() >= 2)
                {
                    haveWithPai = true;
                    withPai1 = alone_1[0];
                    withPai2 = alone_1[1];
                }
            }
            else if (with_pai_count == 3)
            {
                if ( alone_1.size() > 1 && alone_1[1] > pai_type_A && pairs.size() > 0 && pairs[0] < alone_1[1] )
                {
                    haveWithPai = true;
                    withPai1 = alone_1[0];
                    withPai2 = pairs[0];
                    withPai3 = pairs[0];

                }
                else if ( alone_1.size() > 2 && alone_1[2] > pai_type_A && pairs.size() > 0 && pairs[0] < pai_type_A )
                {
                    haveWithPai = true;
                    withPai1 = alone_1[0];
                    withPai2 = pairs[0];
                    withPai3 = pairs[0];
                }
                else if ((int)alone_1.size() >= 3)
                {
                    haveWithPai = true;
                    withPai1 = alone_1[0];
                    withPai2 = alone_1[1];
                    withPai3 = alone_1[2];
                }
            }
            else if (with_pai_count == 4)
            {
                if ((int)alone_1.size() >= 4)
                {
                    haveWithPai = true;
                    withPai1 = alone_1[0];
                    withPai2 = alone_1[1];
                    withPai3 = alone_1[2];
                    withPai4 = alone_1[2];
                }
            }
        }

        if (haveWithPai)
        {
            ddz_move* move = new ddz_move(limit_move->_type, withPai1, withPai2, withPai3, withPai4);
            move->_airplane_pairs = limit_move->_airplane_pairs;
            memcpy(move->_combo_list, select_move->_combo_list, select_move->_combo_count);
            move->_combo_count = select_move->_combo_count;
            out_list.push_back(move);
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static void common_control_this_move(std::vector<mcts_move*>& out_list, int* pai_list, std::vector<mcts_move*>& move_list, ddz_move* limit_move, bool isFriend, bool armyHaveBomb)
{
    // 不拆牌打
    switch (limit_move->_type)
    {
    case ddz_type_alone_1:
        select_alone_1(out_list, pai_list, move_list, limit_move, isFriend, armyHaveBomb);
        break;
    case ddz_type_pair:
        select_pair(out_list, pai_list, move_list, limit_move);
        break;
    case ddz_type_triple:
    case ddz_type_triple_1:
    case ddz_type_triple_2:
        select_triple(out_list, pai_list, move_list, limit_move);
        break;
    case ddz_type_order:
        select_order(out_list, pai_list, move_list, limit_move);
        break;
    case ddz_type_order_pair:
        select_order_pair(out_list, pai_list, move_list, limit_move);
        break;
    case ddz_type_airplane:
    case ddz_type_airplane_with_pai:
        select_airplane(out_list, pai_list, move_list, limit_move);
        break;
    case ddz_type_bomb:
        select_bomb(out_list, pai_list, move_list, limit_move);
        break;
    case ddz_type_four_with_alone_1:
        select_4_with_alone1(out_list, pai_list, move_list, limit_move);
        break;
    case ddz_type_four_with_pairs:
        select_4_with_pairs(out_list, pai_list, move_list, limit_move);
        break;
    }
}

static inline bool checkHaveBomb(int* pai_list1, int* pai_list2)
{
    bool haveBomb = false;

    if (pai_list1)
    {
        for(int i = pai_type_3; i <= pai_type_2; ++i)
        {
            if (pai_list1[i] > 3)
            {
                haveBomb = true;
                break;
            }
        }
    }

    if (pai_list2 && !haveBomb)
    {
        for(int i = pai_type_3; i <= pai_type_2; ++i)
        {
            if (pai_list2[i] > 3)
            {
                haveBomb = true;
                break;
            }
        }
    }

    return haveBomb;
}

static bool is_landlord_can_control(std::vector<mcts_move*>& move_list, int* next_pai_list, int* prev_pai_list)
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
        return true;
    return false;
}


static bool is_farmer_can_control(std::vector<mcts_move*>& move_list, int *landlord_pai)
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
        return true;
    return false;
}

/*-------------------------------------------------------------------------------------------------------------------*/
// 地主压下家的牌
std::vector<mcts_move*> stgy_passive_play_card::landlord_passive_playcard_next_outcard(ddz_state* p_state, ddz_move* limit_move, int* pai_list, std::vector<mcts_move*>& move_list,
                                                                           int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* pre_pai_list, std::vector<mcts_move*>& pre_move_list)
{
    std::vector<mcts_move*>		return_list;

    if (is_landlord_can_control(move_list, next_pai_list, pre_pai_list))
    {
        if (pai_list[pai_type_blackjack] > 0  && pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
            return return_list;
        }
    }

    common_max_passive_playcard(return_list, p_state, limit_move, pai_list, move_list, next_pai_list, pre_pai_list);
    if (!return_list.empty())
        return return_list;

    if ( next_move_list.size() == 3)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(next_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(next_move_list[1]);
        ddz_move* move3 = dynamic_cast<ddz_move*>(next_move_list[2]);

        if ( move1->_type == ddz_type_bomb || move2->_type == ddz_type_bomb || move3->_type == ddz_type_bomb )
        {
            must_control_this_move_no_bomb(return_list, pai_list, move_list, limit_move);
            if (!return_list.empty())
                return return_list;
        }
    }

    bool need_out_pai = false;
    if ( next_move_list.size() == 1 )
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(next_move_list[0]);
        if (move1->_type != ddz_type_king_bomb && move1->_type != ddz_type_bomb)
        {
            need_out_pai = true;
        }
    }

    if ( !need_out_pai && next_move_list.size() == 2)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(next_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(next_move_list[1]);

        if ( (move1->_type == ddz_type_triple && (move2->_type == ddz_type_alone_1 || move2->_type == ddz_type_pair)) ||
             (move2->_type == ddz_type_triple && (move1->_type == ddz_type_alone_1 || move1->_type == ddz_type_pair)) )
             need_out_pai = true;
    }

    if (need_out_pai)
    {
        must_control_this_move(return_list, pai_list, move_list, limit_move);
        if (!return_list.empty())
            return return_list;
    }

    common_control_this_move(return_list, pai_list, move_list, limit_move, false, checkHaveBomb(next_pai_list, pre_pai_list));
    if (return_list.empty())
        return_list.push_back(new ddz_move());

    return return_list;
}

/*-------------------------------------------------------------------------------------------------------------------*/
// 地主压上家的牌
std::vector<mcts_move*> stgy_passive_play_card::landlord_passive_playcard_prev_outcard(ddz_state* p_state, ddz_move* limit_move, int* pai_list, std::vector<mcts_move*>& move_list,
                                                                                       int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* pre_pai_list, std::vector<mcts_move*>& pre_move_list)
{
    std::vector<mcts_move*>		return_list;

    if (is_landlord_can_control(move_list, next_pai_list, pre_pai_list))
    {
        if (pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
            return return_list;
        }
    }

    common_max_passive_playcard(return_list, p_state, limit_move, pai_list, move_list, next_pai_list, pre_pai_list);
    if (!return_list.empty())
        return return_list;

    if ( pre_move_list.size() == 3)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(pre_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(pre_move_list[1]);
        ddz_move* move3 = dynamic_cast<ddz_move*>(pre_move_list[2]);

        if ( move1->_type == ddz_type_bomb || move2->_type == ddz_type_bomb || move3->_type == ddz_type_bomb )
        {
            must_control_this_move_no_bomb(return_list, pai_list, move_list, limit_move);
            if (!return_list.empty())
                return return_list;
        }
    }


    bool need_out_pai = false;
    if ( pre_move_list.size() == 1 )
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(pre_move_list[0]);
        if (move1->_type != ddz_type_king_bomb && move1->_type != ddz_type_bomb)
        {
            need_out_pai = true;
        }
    }

    if ( !need_out_pai && pre_move_list.size() == 2)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(pre_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(pre_move_list[1]);

        if ( (move1->_type == ddz_type_triple && (move2->_type == ddz_type_alone_1 || move2->_type == ddz_type_pair)) ||
            (move2->_type == ddz_type_triple && (move1->_type == ddz_type_alone_1 || move1->_type == ddz_type_pair)) )
            need_out_pai = true;
    }

    if (next_move_list.size() == 1)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(next_move_list[0]);
        if (p_state->is_greater_than(move, limit_move))
            need_out_pai = true;
    }


    if (need_out_pai)
    {
        must_control_this_move(return_list, pai_list, move_list, limit_move);
        if (!return_list.empty())
            return return_list;
    }

    common_control_this_move(return_list, pai_list, move_list, limit_move, false, checkHaveBomb(next_pai_list, pre_pai_list));
    if (return_list.empty())
        return_list.push_back(new ddz_move());

    return return_list;
}

/*-------------------------------------------------------------------------------------------------------------------*/
// 农民压下家是地主的牌
std::vector<mcts_move*> stgy_passive_play_card::farmer_passive_playcard_next_landlord_and_next_outcard(ddz_state* p_state, ddz_move* limit_move, int* pai_list, std::vector<mcts_move*>& move_list,
                                                                                       int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* pre_pai_list, std::vector<mcts_move*>& pre_move_list)
{
    std::vector<mcts_move*>		return_list;

    // 如果自己能控牌 直接王炸
    if (is_farmer_can_control(move_list, next_pai_list))
    {
        if (pai_list[pai_type_blackjack] > 0  && pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
            return return_list;
        }
    }

    common_max_passive_playcard(return_list, p_state, limit_move, pai_list, move_list, next_pai_list, 0);
    if (!return_list.empty())
        return return_list;

    // 如果下家地主只剩三手牌
    if ( next_move_list.size() == 3)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(next_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(next_move_list[1]);
        ddz_move* move3 = dynamic_cast<ddz_move*>(next_move_list[2]);

        if ( move1->_type == ddz_type_bomb || move2->_type == ddz_type_bomb || move3->_type == ddz_type_bomb )
        {
            must_control_this_move_no_bomb(return_list, pai_list, move_list, limit_move);
            if (!return_list.empty())
                return return_list;
        }
    }

    // 如果下家地主只剩一手牌或二手牌
    bool need_out_pai = false;
    if ( next_move_list.size() == 1 )
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(next_move_list[0]);
        if (move1->_type != ddz_type_king_bomb && move1->_type != ddz_type_bomb)
        {
            need_out_pai = true;
        }
    }

    if ( !need_out_pai && next_move_list.size() == 2)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(next_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(next_move_list[1]);

        if ( (move1->_type == ddz_type_triple && (move2->_type == ddz_type_alone_1 || move2->_type == ddz_type_pair)) ||
            (move2->_type == ddz_type_triple && (move1->_type == ddz_type_alone_1 || move1->_type == ddz_type_pair)) )
            need_out_pai = true;
    }

    if (need_out_pai)
    {
        must_control_this_move(return_list, pai_list, move_list, limit_move);
        if (!return_list.empty())
            return return_list;
    }

    if (limit_move->_type == ddz_type_four_with_pairs || limit_move->_type == ddz_type_four_with_alone_1)
    {
        if (limit_move->_type != ddz_type_king_bomb && limit_move->_type != ddz_type_bomb)
        {
            for(int i = pai_type_3; i <= pai_type_2; ++i)
            {
                if (pai_list[i] >= 4)
                {
                    add_move_to_list(return_list, new ddz_move(ddz_type_bomb, i));
                }
            }
        }

        // 王炸
        if (pai_list[pai_type_blackjack] == 1 && pai_list[pai_type_blossom] == 1)
            add_move_to_list(return_list, new ddz_move(ddz_type_king_bomb));

    }

    if (!return_list.empty())
        return return_list;

    // 检查一下自己的主从关系，如果能打则打

    common_control_this_move(return_list, pai_list, move_list, limit_move, false, checkHaveBomb(next_pai_list, 0));
    if (return_list.empty())
        return_list.push_back(new ddz_move());

    return return_list;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_passive_play_card::is_next_friend_only_1_move(std::vector<mcts_move*>& out_list, ddz_state* p_state, ddz_move* limit_pai, int* pai_list, std::vector<mcts_move*>& move_list, std::vector<mcts_move*>& friend_move_list)
{
    if (friend_move_list.size() == 1)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(friend_move_list[0]);
        if (p_state->is_greater_than(move, limit_pai))
        {
            out_list.push_back(new ddz_move());
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
void stgy_passive_play_card::is_next_friend_can_control_pai(std::vector<mcts_move*>& out_list, ddz_state* p_state, ddz_move* limit_move, std::vector<mcts_move*>& friend_move_list, int* landlord_pai)
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
    if (total_alone_count <= 0)
    {
        // 下家能控牌，但能不能控住这手牌，不能则要管住
        int size = (int)friend_move_list.size();
        for(int i = 0; i < size; ++i)
        {
            ddz_move* move = dynamic_cast<ddz_move*>(friend_move_list[i]);
            if ( move->_type == limit_move->_type && p_state->is_greater_than(move, limit_move) && is_max_pai(move, landlord_pai, 0))
            {
                out_list.push_back(new ddz_move());
                return;
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
// 农民压上家地主的牌
std::vector<mcts_move*> stgy_passive_play_card::farmer_passive_playcard_next_farmer_and_prev_outcard(ddz_state* p_state, ddz_move* limit_move, int* pai_list, std::vector<mcts_move*>& move_list,
                                                                                                     int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* pre_pai_list, std::vector<mcts_move*>& pre_move_list)
{
    std::vector<mcts_move*>		return_list;

    if (is_farmer_can_control(move_list, pre_pai_list))
    {
        if (pai_list[pai_type_blackjack] > 0  && pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
            return return_list;
        }
    }

    common_max_passive_playcard(return_list, p_state, limit_move, pai_list, move_list, 0, pre_pai_list);
    if (!return_list.empty())
        return return_list;

    is_next_friend_only_1_move(return_list, p_state, limit_move, pai_list, move_list, next_move_list);
    if (!return_list.empty())
        return return_list;

    is_next_friend_can_control_pai(return_list, p_state, limit_move, next_move_list, pre_pai_list);
    if (!return_list.empty())
        return return_list;

    // 如果上家地主只剩三手牌
    if ( pre_move_list.size() == 3)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(pre_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(pre_move_list[1]);
        ddz_move* move3 = dynamic_cast<ddz_move*>(pre_move_list[2]);

        if ( move1->_type == ddz_type_bomb || move2->_type == ddz_type_bomb || move3->_type == ddz_type_bomb )
        {
            must_control_this_move_no_bomb(return_list, pai_list, move_list, limit_move);
            if (!return_list.empty())
                return return_list;

        }
    }

    // 如果上家地主只剩一手牌或二手牌
    bool need_out_pai = false;
    if ( pre_move_list.size() == 1 )
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(pre_move_list[0]);
        if (move1->_type != ddz_type_king_bomb && move1->_type != ddz_type_bomb)
        {
            need_out_pai = true;
        }
    }

    if ( !need_out_pai && pre_move_list.size() == 2)
    {
        ddz_move* move1 = dynamic_cast<ddz_move*>(pre_move_list[0]);
        ddz_move* move2 = dynamic_cast<ddz_move*>(pre_move_list[1]);

        if ( (move1->_type == ddz_type_triple && (move2->_type == ddz_type_alone_1 || move2->_type == ddz_type_pair)) ||
            (move2->_type == ddz_type_triple && (move1->_type == ddz_type_alone_1 || move1->_type == ddz_type_pair)) )
            need_out_pai = true;
    }

    if (need_out_pai)
    {
        must_control_this_move(return_list, pai_list, move_list, limit_move);
        if (!return_list.empty())
            return return_list;
    }

    if (limit_move->_type == ddz_type_four_with_pairs || limit_move->_type == ddz_type_four_with_alone_1)
    {
        if (limit_move->_type != ddz_type_king_bomb && limit_move->_type != ddz_type_bomb)
        {
            for(int i = pai_type_3; i <= pai_type_2; ++i)
            {
                if (pai_list[i] >= 4)
                {
                    add_move_to_list(return_list, new ddz_move(ddz_type_bomb, i));
                }
            }
        }

            // 王炸
        if (pai_list[pai_type_blackjack] == 1 && pai_list[pai_type_blossom] == 1)
            add_move_to_list(return_list, new ddz_move(ddz_type_king_bomb));

    }

    if (!return_list.empty())
        return return_list;

    common_control_this_move(return_list, pai_list, move_list, limit_move, false, checkHaveBomb(0, pre_pai_list));
    if (return_list.empty())
        return_list.push_back(new ddz_move());

    return return_list;
}

void stgy_passive_play_card::if_next_landlord_only_1_move(std::vector<mcts_move*>& out_list, ddz_state* p_state, ddz_move* limit_pai, int* pai_list, std::vector<mcts_move*>& move_list, std::vector<mcts_move*>& landlord_move_list)
{
    if (landlord_move_list.size() == 1)
    {
        ddz_move* landlord_move = dynamic_cast<ddz_move*>(landlord_move_list[0]);
        if (landlord_move->_type != limit_pai->_type)
        {
            return;
        }

        if (p_state->is_greater_than(landlord_move, limit_pai))
        {
            must_equal_this_move( out_list, pai_list, move_list, landlord_move );
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
// 农民压上家是农民的牌，下家是地主
std::vector<mcts_move*> stgy_passive_play_card::farmer_passive_playcard_next_landlord_and_prev_outcard(ddz_state* p_state, ddz_move* limit_move, int* pai_list, std::vector<mcts_move*>& move_list,
                                                                                       int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* pre_pai_list, std::vector<mcts_move*>& pre_move_list)
{
    std::vector<mcts_move*>		return_list;

    if (pre_move_list.size() == 1)
    {
        return_list.push_back(new ddz_move());
        return return_list;
    }

    if (is_farmer_can_control(move_list, next_pai_list))
    {
        if (pai_list[pai_type_blackjack] > 0  && pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
            return return_list;
        }
    }

    common_max_passive_playcard(return_list, p_state, limit_move, pai_list, move_list, next_pai_list, 0);
    if (!return_list.empty())
        return return_list;

    is_friend_can_control_pai_and_curr_friend_out_pai(return_list, p_state, limit_move, pai_list, move_list, pre_move_list, next_pai_list);
    if (!return_list.empty())
        return return_list;

    // 如果下家只有一张单牌，则必须管住
    if_next_landlord_only_1_move(return_list, p_state, limit_move, pai_list, move_list, next_move_list);
    if (!return_list.empty())
        return return_list;

    if (limit_move->_type == ddz_type_alone_1 || limit_move->_type == ddz_type_pair)
    {
        if (limit_move->_type <= pai_type_10)
        {
            int size = move_list.size();
            std::vector<unsigned char> list;
            for(int i = 0; i < size; ++i)
            {
                ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
                if (move->_type == limit_move->_type && move->_alone_1 > limit_move->_alone_1 && move->_alone_1 <= pai_type_Q)
                    list.push_back(move->_alone_1);
            }

            if (!list.empty())
            {
                std::sort(list.begin(), list.end());
                return_list.push_back(new ddz_move(limit_move->_type, list[0]));
            }

            if (return_list.empty() && limit_move->_type == ddz_type_alone_1)
            {
                if (pai_list[pai_type_K] <= 2 && pai_list[pai_type_K] > 0 && pai_type_K > limit_move->_alone_1)
                {
                    return_list.push_back(new ddz_move(limit_move->_type, pai_type_K));
                }
                if (return_list.empty() && pai_list[pai_type_A] <= 2 && pai_list[pai_type_A] > 0 && pai_type_A > limit_move->_alone_1)
                {
                    return_list.push_back(new ddz_move(limit_move->_type, pai_type_A));
                }
            }
        }
        else
        {
            int size = move_list.size();
            std::vector<unsigned char> list;
            for(int i = 0; i < size; ++i)
            {
                ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
                if (move->_type == limit_move->_type && move->_alone_1 > limit_move->_alone_1 && move->_alone_1 <= pai_type_Q)
                    list.push_back(move->_alone_1);
            }

            if (!list.empty())
            {
                std::sort(list.begin(), list.end());
                return_list.push_back(new ddz_move(limit_move->_type, list[0]));
            }

            if (return_list.empty())
            {
                for(int i = 0; i < size; ++i)
                {
                    ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
                    if (move->_type == limit_move->_type && move->_alone_1 > limit_move->_alone_1 && move->_alone_1 <= pai_type_2)
                        list.push_back(move->_alone_1);
                }

                if (!list.empty())
                {
                    std::sort(list.begin(), list.end());
                    return_list.push_back(new ddz_move(limit_move->_type, list[0]));
                }
            }

        }
    }

    if (!return_list.empty())
        return return_list;

    // 如果自己的牌有优势，则管住，然后自己出牌
    int sss = 0;


    return_list.push_back(new ddz_move());

    return return_list;
}

void stgy_passive_play_card::is_friend_can_control_pai_and_curr_friend_out_pai(std::vector<mcts_move*>& out_list, ddz_state* p_state, ddz_move* limit_pai, int* pai_list, std::vector<mcts_move*>& move_list, std::vector<mcts_move*>& friend_move_list, int* landlord_pai)
{
    int triple_count = 0;
    int airplane_count = 0;
    int bomb_count = 0;
    int other_alone_count = 0;
    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;

    std::vector<ddz_move*> maxPaiList;

    std::vector<mcts_move*> friend_move_list_and_limit_pai;
    int size = (int)friend_move_list.size();
    for(int i = 0; i < size; ++i)
    {
        friend_move_list_and_limit_pai.push_back(friend_move_list[i]);
    }
    friend_move_list_and_limit_pai.push_back(limit_pai);

    size = (int)friend_move_list_and_limit_pai.size();
    for(int i = 0; i < size; ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(friend_move_list_and_limit_pai[i]);
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
    if (total_alone_count <= 0)
    {
        out_list.push_back(new ddz_move());
    }

}
/*-------------------------------------------------------------------------------------------------------------------*/
// 如果自己能控牌，则返回出牌
void stgy_passive_play_card::is_self_can_control_pai(std::vector<mcts_move*>& out_list, ddz_state* p_state, int* pai_list, std::vector<mcts_move*>& move_list, int* landlord_pai, ddz_move* limit_move)
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
    if (total_alone_count <= 0)
    {
        int size = (int)move_list.size();
        for(int i = 0; i < size; ++i)
        {
            ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
            if (p_state->is_greater_than(move, limit_move) && is_max_pai(move, landlord_pai, 0))
            {
                out_list.push_back(move->clone());
                return;
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
// 农民压下家农民的牌
std::vector<mcts_move*> stgy_passive_play_card::farmer_passive_playcard_next_farmer_and_next_outcard(ddz_state* p_state, ddz_move* limit_move, int* pai_list, std::vector<mcts_move*>& move_list,
                                                                                       int* next_pai_list, std::vector<mcts_move*>& next_move_list, int* pre_pai_list, std::vector<mcts_move*>& pre_move_list)
{
    std::vector<mcts_move*>		return_list;

    if (next_move_list.size() == 1)
    {
        return_list.push_back(new ddz_move());
        return return_list;
    }

    if (is_farmer_can_control(move_list, pre_pai_list))
    {
        if (pai_list[pai_type_blackjack] > 0  && pai_list[pai_type_blossom] > 0)
        {
            return_list.push_back(new ddz_move(ddz_type_king_bomb));
            return return_list;
        }
    }

    common_max_passive_playcard(return_list, p_state, limit_move, pai_list, move_list, 0, pre_pai_list);
    if (!return_list.empty())
        return return_list;

    // 如果自己能控牌，则管住，并开始控牌
    is_self_can_control_pai(return_list, p_state, pai_list, move_list, pre_pai_list, limit_move);
    if (!return_list.empty())
        return return_list;

    return_list.push_back(new ddz_move());
    return return_list;
    /*
    // 判断是否传递牌，如果是，则压住，然后主动出牌
    int nextSize = next_move_list.size();
    std::vector<unsigned char> nextAlone1;
    std::vector<unsigned char> nextPairs;
    for(int i = 0; i < nextSize; ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(next_move_list[i]);
        if (move->_type == ddz_type_pair)
            nextAlone1.push_back(move->_alone_1);
        else if (move->_type == ddz_type_alone_1)
            nextPairs.push_back(move->_alone_1);
    }

    std::sort(nextAlone1.begin(), nextAlone1.end());
    std::sort(nextPairs.begin(), nextPairs.end());

    int size = move_list.size();
    std::vector<unsigned char> alone1;
    std::vector<unsigned char> pairs;
    for(int i = 0; i < nextSize; ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        if (move->_type == ddz_type_alone_1)
            alone1.push_back(move->_alone_1);
        else if (move->_type == ddz_type_pair)
            pairs.push_back(move->_alone_1);
    }


    return return_list;*/
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
static inline ddz_move* make_triple_with_pai_between_move(ddz_move* m1, ddz_move* m2)
{
    ddz_move* move = new ddz_move();
    if (m1->_type == ddz_type_triple && (m2->_type == ddz_type_alone_1 || m2->_type == ddz_type_pair))
    {
        move->_alone_1 = m1->_alone_1;
        move->_alone_2 = m1->_alone_2;
        move->_type = m2->_type == ddz_type_alone_1 ? ddz_type_triple_1 : ddz_type_triple_2;
    }

    if (m2->_type == ddz_type_triple && (m1->_type == ddz_type_alone_1 || m1->_type == ddz_type_pair))
    {
        move->_alone_1 = m1->_alone_2;
        move->_alone_2 = m1->_alone_1;
        move->_type = m1->_type == ddz_type_alone_1 ? ddz_type_triple_1 : ddz_type_triple_2;
    }

    return move;
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
static bool is_have_king_bomb(int* next_pai_list, int* prev_pai_list)
{
    bool is_have_king_bomb = false;

    if (next_pai_list)
    {
        if (next_pai_list[pai_type_blossom] > 0 && next_pai_list[pai_type_blackjack] > 0)
        {
            is_have_king_bomb = true;
        }

        if (is_have_king_bomb)
        {
            // 如果有王炸，但要拆几手出，那也认为没有王炸
            stgy_split_card split_card;
            std::vector<mcts_move*> split_move_list;
            split_card.generate_out_pai_move(next_pai_list, split_move_list);
            if (split_move_list.size() > 2)
                is_have_king_bomb = false;
            int size = (int)split_move_list.size();
            for(int i = 0; i < size; ++i)
                delete split_move_list[i];
        }
    }

    if (!is_have_king_bomb && prev_pai_list)
    {
        if (prev_pai_list[pai_type_blossom] > 0 && prev_pai_list[pai_type_blackjack] > 0)
        {
            is_have_king_bomb = true;
        }

        if (is_have_king_bomb)
        {
            // 如果有王炸，但要拆几手出，那也认为没有王炸
            stgy_split_card split_card;
            std::vector<mcts_move*> split_move_list;
            split_card.generate_out_pai_move(prev_pai_list, split_move_list);
            if (split_move_list.size() > 2)
                is_have_king_bomb = false;
            int size = (int)split_move_list.size();
            for(int i = 0; i < size; ++i)
                delete split_move_list[i];
        }
    }

    return is_have_king_bomb;
}

/*-------------------------------------------------------------------------------------------------------------------*/
// 这个是一般优先出最大牌，如果只剩二，三张牌时，优先出最大的牌
void stgy_passive_play_card::common_max_passive_playcard(std::vector<mcts_move*>& out_list, ddz_state* p_state, ddz_move* limit_pai, int* pai_list, std::vector<mcts_move*>& move_list, int* next_pai_list, int* prev_pai_list)
{
    // 如果只有一手牌，可以大过要打的牌，则直接出
    if (move_list.size() == 1)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[0]);
        if (p_state->is_greater_than(move, limit_pai))
            out_list.push_back(move->clone());
        return;
    }

    // 如果有二手牌，其中有一手是最大牌型，则优先出最大牌型
    if (move_list.size() == 2)
    {
        // 如果是三带一或三带二，则直接出牌
        ddz_move* move[2] = {dynamic_cast<ddz_move*>(move_list[0]), dynamic_cast<ddz_move*>(move_list[1])};
        if (is_triple_with_pai(move[0], move[1]))
        {
            ddz_move* m = make_triple_with_pai_between_move(move[0], move[1]);

            if (p_state->is_greater_than(m, limit_pai))
            {
                out_list.push_back(m);
                return;
            }
            else
                delete m;
        }

        for(int i = 0; i < 2; ++i)
        {
            if ( p_state->is_greater_than(move[i], limit_pai) && is_max_pai(move[i], next_pai_list, prev_pai_list))
            {
                out_list.push_back(move[i]->clone());
                return;
            }
        }

        // 如果有二手炸弹，且对方有王炸但不能一手出完，则打出
        if (move[0]->_type == ddz_type_bomb && move[1]->_type == ddz_type_bomb)
        {
            if (!is_have_king_bomb(next_pai_list, prev_pai_list))
            {
                if (move[0]->_alone_1 < move[1]->_alone_1)
                    out_list.push_back(move[0]->clone());
                else
                    out_list.push_back(move[1]->clone());
            }
        }
    }
}

bool stgy_passive_play_card::is_landlord_can_control_after_passive_playcard(int *pai_list, ddz_move *p_move, int *next_pai_list, std::vector<mcts_move*>& next_move_list, int* prev_pai_list, std::vector<mcts_move*>& prev_move_list)
{
    // 复制牌
    int	after_pai_list[pai_type_max] = {0};
    for(int i = 0; i < pai_type_max; ++i)
    {
        if (pai_list[i] != 0)
        {
            after_pai_list[i] = pai_list[i];
        }
    }

    switch (p_move->_type)
    {
    case ddz_type_alone_1:
        after_pai_list[p_move->_alone_1] -= 1;
        break;
    case ddz_type_pair:
        after_pai_list[p_move->_alone_1] -= 2;
        break;
    case ddz_type_triple:
        after_pai_list[p_move->_alone_1] -= 3;
        break;
    case ddz_type_triple_1:
        after_pai_list[p_move->_alone_1] -= 3;
        after_pai_list[p_move->_alone_2] -= 1;
        break;
    case ddz_type_triple_2:
        after_pai_list[p_move->_alone_1] -= 3;
        after_pai_list[p_move->_alone_2] -= 2;
        break;
    case ddz_type_order:
        {
            int size = p_move->_combo_count;
            qsort(p_move->_combo_list, size, sizeof(unsigned char), move_cmp);
            if (size > 0)
            {
                int start_index = p_move->_combo_list[0];
                for(int i = start_index; i < start_index + size; ++i)
                {
                    after_pai_list[i] -= 1;
                }
            }
        }
        break;
    case ddz_type_order_pair:
        {
            int size = p_move->_combo_count / 2;
            qsort(p_move->_combo_list, size, sizeof(unsigned char), move_cmp);
            if (size > 0)
            {
                int start_index = p_move->_combo_list[0];
                for(int i = start_index; i < start_index + size; ++i)
                {
                    after_pai_list[i] -= 2;
                }
            }
        }
        break;
    case ddz_type_airplane:
        {
            int size = p_move->_combo_count / 3;
            qsort(p_move->_combo_list, size, sizeof(unsigned char), move_cmp);
            if (size > 0)
            {
                int start_index = p_move->_combo_list[0];
                for(int i = start_index; i < start_index + size; ++i)
                {
                    after_pai_list[i] -= 3;
                }
            }
        }
        break;
    case ddz_type_airplane_with_pai:
        {
            int size = p_move->_combo_count / 3;
            qsort(p_move->_combo_list, size, sizeof(unsigned char), move_cmp);
            if (size > 0)
            {
                int start_index = p_move->_combo_list[0];
                for(int i = start_index; i < start_index + size; ++i)
                {
                    after_pai_list[i] -= 3;
                }
            }
            if (p_move->_alone_1 != 0)
            {
                after_pai_list[p_move->_alone_1] -= p_move->_airplane_pairs ? 2:1;
            }
            if (p_move->_alone_2 != 0)
            {
                after_pai_list[p_move->_alone_2] -= p_move->_airplane_pairs ? 2:1;
            }
            if (p_move->_alone_3 != 0)
            {
                after_pai_list[p_move->_alone_3] -= p_move->_airplane_pairs ? 2:1;
            }
            if (p_move->_alone_4 != 0)
            {
                after_pai_list[p_move->_alone_4] -= p_move->_airplane_pairs ? 2:1;
            }
        }
        break;
    case ddz_type_bomb:
        after_pai_list[p_move->_alone_1] -= 4;
        break;
    case ddz_type_king_bomb:
        after_pai_list[pai_type_blossom] = 0;
        after_pai_list[pai_type_blackjack] = 0;
        break;

    case ddz_type_four_with_alone_1:
        after_pai_list[p_move->_alone_1] -= 4;
        after_pai_list[p_move->_alone_2] -= 1;
        after_pai_list[p_move->_alone_3] -= 1;
        break;

    case ddz_type_four_with_pairs:
        after_pai_list[p_move->_alone_1] -= 4;
        after_pai_list[p_move->_alone_2] -= 2;
        after_pai_list[p_move->_alone_3] -= 2;
        break;
    }

    stgy_split_card split_card;
    std::vector<mcts_move*> move_list;
    split_card.generate_out_pai_move(after_pai_list, move_list);

    bool can = is_landlord_can_control(move_list, next_pai_list, prev_pai_list);

    for (std::vector<mcts_move*>::iterator iter = move_list.begin(); iter != move_list.end(); iter++)
    {
        delete *iter;
    }
    return can;
}

bool stgy_passive_play_card::is_farmer_can_control_after_passive_playcard(int *pai_list, ddz_move *p_move, int *landlord_pai)
{
    // 复制牌
    int	after_pai_list[pai_type_max] = {0};
    for(int i = 0; i < pai_type_max; ++i)
    {
        if (pai_list[i] != 0)
        {
            after_pai_list[i] = pai_list[i];
        }
    }
    switch (p_move->_type)
    {
    case ddz_type_alone_1:
        after_pai_list[p_move->_alone_1] -= 1;
        break;
    case ddz_type_pair:
        after_pai_list[p_move->_alone_1] -= 2;
        break;
    case ddz_type_triple:
        after_pai_list[p_move->_alone_1] -= 3;
        break;
    case ddz_type_triple_1:
        after_pai_list[p_move->_alone_1] -= 3;
        after_pai_list[p_move->_alone_2] -= 1;
        break;
    case ddz_type_triple_2:
        after_pai_list[p_move->_alone_1] -= 3;
        after_pai_list[p_move->_alone_2] -= 2;
        break;
    case ddz_type_order:
        {
            int size = p_move->_combo_count;
            qsort(p_move->_combo_list, size, sizeof(unsigned char), move_cmp);
            if (size > 0)
            {
                int start_index = p_move->_combo_list[0];
                for(int i = start_index; i < start_index + size; ++i)
                {
                    after_pai_list[i] -= 1;
                }
            }
        }
        break;
    case ddz_type_order_pair:
        {
            int size = p_move->_combo_count / 2;
            qsort(p_move->_combo_list, size, sizeof(unsigned char), move_cmp);
            if (size > 0)
            {
                int start_index = p_move->_combo_list[0];
                for(int i = start_index; i < start_index + size; ++i)
                {
                    after_pai_list[i] -= 2;
                }
            }
        }
        break;
    case ddz_type_airplane:
        {
            int size = p_move->_combo_count / 3;
            qsort(p_move->_combo_list, size, sizeof(unsigned char), move_cmp);
            if (size > 0)
            {
                int start_index = p_move->_combo_list[0];
                for(int i = start_index; i < start_index + size; ++i)
                {
                    after_pai_list[i] -= 3;
                }
            }
        }
        break;
    case ddz_type_airplane_with_pai:
        {
            int size = p_move->_combo_count / 3;
            qsort(p_move->_combo_list, size, sizeof(unsigned char), move_cmp);
            if (size > 0)
            {
                int start_index = p_move->_combo_list[0];
                for(int i = start_index; i < start_index + size; ++i)
                {
                    after_pai_list[i] -= 3;
                }
            }
            if (p_move->_alone_1 != 0)
            {
                after_pai_list[p_move->_alone_1] -= p_move->_airplane_pairs ? 2:1;
            }
            if (p_move->_alone_2 != 0)
            {
                after_pai_list[p_move->_alone_2] -= p_move->_airplane_pairs ? 2:1;
            }
            if (p_move->_alone_3 != 0)
            {
                after_pai_list[p_move->_alone_3] -= p_move->_airplane_pairs ? 2:1;
            }
            if (p_move->_alone_4 != 0)
            {
                after_pai_list[p_move->_alone_4] -= p_move->_airplane_pairs ? 2:1;
            }
        }
        break;
    case ddz_type_bomb:
        after_pai_list[p_move->_alone_1] -= 4;
        break;
    case ddz_type_king_bomb:
        after_pai_list[pai_type_blossom] = 0;
        after_pai_list[pai_type_blackjack] = 0;
        break;

    case ddz_type_four_with_alone_1:
        after_pai_list[p_move->_alone_1] -= 4;
        after_pai_list[p_move->_alone_2] -= 1;
        after_pai_list[p_move->_alone_3] -= 1;
        break;

    case ddz_type_four_with_pairs:
        after_pai_list[p_move->_alone_1] -= 4;
        after_pai_list[p_move->_alone_2] -= 2;
        after_pai_list[p_move->_alone_3] -= 2;
        break;
    }
    stgy_split_card split_card;
    std::vector<mcts_move*> move_list;
    split_card.generate_out_pai_move(after_pai_list, move_list);

    bool can = is_farmer_can_control(move_list, landlord_pai);

    for (std::vector<mcts_move*>::iterator iter = move_list.begin(); iter != move_list.end(); iter++)
    {
        delete *iter;
    }
    return can;
}
