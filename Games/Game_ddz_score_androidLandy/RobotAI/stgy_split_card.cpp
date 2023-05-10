
#include "stgy_split_card.h"
#include "ddz_move.h"
#include <algorithm>
#include "memory.h"
#ifdef _WIN32
#include "windows.h"
#endif


/*-------------------------------------------------------------------------------------------------------------------*/
static void real_parse_pai(int* pai_list, std::vector<parse_handcard>& parse_list);

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
stgy_split_card::stgy_split_card()
{
}

/*-------------------------------------------------------------------------------------------------------------------*/
stgy_split_card::~stgy_split_card()
{
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline void parse_select_king(int* pai_list, parse_handcard& handcard)
{
    if (pai_list[pai_type_blackjack] == 1 && pai_list[pai_type_blossom] == 1)
    {
        part_pai part;
        part.type = ddz_value_bomb;
        part.list[part.count++] =pai_type_blackjack;
        part.list[part.count++] = pai_type_blossom;
        handcard.list.push_back(part);
    }
    else if (pai_list[pai_type_blackjack] == 1)
    {
        part_pai part;
        part.type = ddz_value_alone_1;
        part.list[part.count++] = pai_type_blackjack;
        handcard.list.push_back(part);
    }
    else if (pai_list[pai_type_blossom] == 1)
    {
        part_pai part;
        part.type = ddz_value_alone_1;
        part.list[part.count++] = pai_type_blossom;
        handcard.list.push_back(part);
    }
    pai_list[pai_type_blackjack] = 0;
    pai_list[pai_type_blossom] = 0;

    if (pai_list[pai_type_2] >= 4)
    {
        part_pai part;
        part.type = ddz_value_bomb;
        part.list[part.count++] = pai_type_2;
        part.list[part.count++] = pai_type_2;
        part.list[part.count++] = pai_type_2;
        part.list[part.count++] = pai_type_2;
        handcard.list.push_back(part);
    }
    else if (pai_list[pai_type_2] >= 3)
    {
        part_pai part;
        part.type = ddz_value_three;
        part.list[part.count++] = pai_type_2;
        part.list[part.count++] = pai_type_2;
        part.list[part.count++] = pai_type_2;
        handcard.list.push_back(part);
    }
    else if (pai_list[pai_type_2] >= 2)
    {
        part_pai part;
        part.type = ddz_value_pairs;
        part.list[part.count++] = pai_type_2;
        part.list[part.count++] = pai_type_2;
        handcard.list.push_back(part);
    }
    else if (pai_list[pai_type_2] >= 1)
    {
        part_pai part;
        part.type = ddz_value_alone_1;
        part.list[part.count++] = pai_type_2;
        handcard.list.push_back(part);
    }
    pai_list[pai_type_2] = 0;

    for(int i = pai_type_3; i <= pai_type_A; ++i )
    {
        if (pai_list[i] >= 4)
        {
            part_pai part;
            part.type = ddz_value_bomb;
            part.list[part.count++] = i;
            part.list[part.count++] = i;
            part.list[part.count++] = i;
            part.list[part.count++] = i;
            handcard.list.push_back(part);
            pai_list[i] = 0;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline void help_parse_select_airplane_forward_link_alone(int* pai_list, part_pai& part)
{
    while (true)
    {
        int last_value = part.list[part.count-1];
        if ( (last_value + 1) > pai_type_A )
            break;
        if (pai_list[last_value+1] > 2)
        {
            part.list[part.count++] = last_value+1;
            part.list[part.count++] = last_value+1;
            part.list[part.count++] = last_value+1;

            pai_list[last_value+1] -= 3;
        }
        else
            break;
    }
}


/*-------------------------------------------------------------------------------------------------------------------*/
static inline void parse_select_airplane(int* pai_list, parse_handcard& handcard)
{
    for(int i = pai_type_3; i <= pai_type_K; )
    {
        if (pai_list[i] > 2 && pai_list[i+1] > 2)
        {
            part_pai part;
            part.type = ddz_value_order_airplane;
            part.list[0] = i;part.list[1] = i;part.list[2] = i;
            part.list[3] = i + 1;part.list[4] = i + 1;part.list[5] = i + 1;
            part.count = 6;

            pai_list[i] -= 3;
            pai_list[i+1] -= 3;

            help_parse_select_airplane_forward_link_alone(pai_list, part);
            i = part.list[part.count-1]+1;

            handcard.list.push_back(part);
        }
        else
            ++i;
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline void parse_select_alone(int* pai_list, parse_handcard& handcard)
{
    for(int i = pai_type_3; i <= pai_type_A; ++i)
    {
        if (pai_list[i] > 2)
        {
            part_pai part;
            part.type = ddz_value_three;
            part.list[part.count++] = i;
            part.list[part.count++] = i;
            part.list[part.count++] = i;
            handcard.list.push_back(part);
        }
        else if (pai_list[i] > 1)
        {
            part_pai part;
            part.type = ddz_value_pairs;
            part.list[part.count++] = i;
            part.list[part.count++] = i;
            handcard.list.push_back(part);
        }
        else if (pai_list[i] > 0)
        {
            part_pai part;
            part.type = ddz_value_alone_1;
            part.list[part.count++] = i;
            handcard.list.push_back(part);
        }
        pai_list[i] = 0;
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline void help_parse_order_forward_link_alone(int* pai_list, part_pai& part)
{
    while (true)
    {
        int last_value = part.list[part.count-1];
        if ( (last_value + 1) > pai_type_A )
            break;
        if (pai_list[last_value+1] > 0)
        {
            part.list[part.count++] = last_value+1;
        }
        else
            break;
    }
}

static inline void help_parse_order_forward_link_alone_2(int* pai_list, part_pai& part)
{
    while (true)
    {
        int last_value = part.list[part.count-1];
        if ( (last_value + 1) > pai_type_A )
            break;
        if (pai_list[last_value+1] > 1)
        {
            part.list[part.count++] = last_value+1;
        }
        else
            break;
    }
}

static inline void help_parse_order_forward_link_alone_3(int* pai_list, part_pai& part)
{
    while (true)
    {
        int last_value = part.list[part.count-1];
        if ( (last_value + 1) > pai_type_A )
            break;
        if (pai_list[last_value+1] > 1)
        {
            part.list[part.count++] = last_value+1;
            part.list[part.count++] = last_value+1;

            pai_list[last_value+1] -= 2;
        }
        else
            break;
    }

}

static inline void help_generate_order(int* pai_list, int min, int max, parse_handcard& handcard)
{
    if (min >= max || max > pai_type_A || min < pai_type_3 || (max-min+1) < 5)
        return;

    part_pai part;
    part.count = max-min+1;
    part.type = ddz_value_order;
    for(int i = min; i <= max; ++ i)
    {
        if (pai_list[i] <= 0)
            return;

        pai_list[i] -= 1;
        part.list[i-min] = i;
    }

    handcard.list.push_back(part);
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline bool is_delete_split_order(int* pai_list, part_pai& part, part_pai& order, bool& is_not_add_order_pairs)
{
    int min_order_pairs = part.list[0];
    int max_order_pairs = part.list[part.count-1];

    if (max_order_pairs < order.list[0] || min_order_pairs > order.list[order.count-1] )
        return false;

    if (min_order_pairs >= order.list[0] && max_order_pairs <= order.list[order.count-1])
    {
        if ( min_order_pairs == order.list[0] )
        {
            if ( (order.count - part.count) >= 5 )
            {
                order.count -= part.count;
                memcpy(&order.list[0], &order.list[part.count], sizeof(int)*order.count);

                for(int i = min_order_pairs; i <= max_order_pairs; ++i)
                    pai_list[i] += 1;
                return false;
            }

            if (max_order_pairs == order.list[order.count-1])
            {
                for(int i = min_order_pairs; i <= max_order_pairs; ++i)
                    pai_list[i] += 1;
                return true;
            }

            if (part.count >= (order.count - part.count))
            {
                int max = order.list[order.count-1];
                for(int i = order.list[0]; i <= max; ++i)
                    pai_list[i] += 1;
                return true;
            }
            else
            {
                // 删除连队
                is_not_add_order_pairs = true;
                return false;
            }
        }
        else if (max_order_pairs == order.list[order.count-1])
        {
            if ( (order.count - part.count) >= 5 )
            {
                for(int i = 0; i < part.count; ++i)
                    order.list[order.count - 1 - i] = 0;
                order.count -= part.count;

                for(int i = min_order_pairs; i <= max_order_pairs; ++i)
                    pai_list[i] += 1;
                return false;
            }

            // 连队大于拆顺子后的单牌
            if (part.count >= (order.count - part.count))
            {
                int max = order.list[order.count-1];
                for(int i = order.list[0]; i <= max; ++i)
                    pai_list[i] += 1;
                return true;
            }
            else
            {
                // 删除连队
                is_not_add_order_pairs = true;
                return false;
            }

        }
        else
        {
            if ( (order.count - part.count - (min_order_pairs-order.list[0]) ) >= 5 )
            {
                int old_count = order.count;
                order.count -= part.count + min_order_pairs-order.list[0];
                int old_zero = order.list[0];
                memcpy(&order.list[0], &order.list[old_count-order.count], sizeof(int)*order.count);

                for(int i = old_zero; i <= max_order_pairs; ++i)
                    pai_list[i] += 1;
                return false;
            }

            if (part.count >= (order.count - part.count))
            {
                int max = order.list[order.count-1];
                for(int i = order.list[0]; i <= max; ++i)
                    pai_list[i] += 1;
                return true;
            }
            else
            {
                // 删除连队
                is_not_add_order_pairs = true;
                return false;
            }
        }
    }

    return false;
}


static inline void  process_order_and_order_pairs(int* pai_list, part_pai& part, parse_handcard& handcard)
{
    bool is_not_add_order_pairs = false;
    std::vector<part_pai>::iterator iter = handcard.list.begin();
    for( ; iter != handcard.list.end(); ++iter)
    {
        if (iter->type == ddz_value_order)
        {
            if (is_delete_split_order(pai_list, part, *iter, is_not_add_order_pairs))
            {
                iter = handcard.list.erase(iter);
                break;
            }
            if (is_not_add_order_pairs)
                break;
        }
    }

    if (!is_not_add_order_pairs)
    {
        // 加连对到handcard去
        part_pai part_order_pair;
        part_order_pair.type = ddz_value_order_pairs;

        int max = part.list[part.count-1];
        for(int i = part.list[0]; i <= max; ++i)
        {
            part_order_pair.list[part_order_pair.count++] = i;
            part_order_pair.list[part_order_pair.count++] = i;
            pai_list[i] -= 2;
        }

        handcard.list.push_back(part_order_pair);
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline void process_order_and_three(int* pai_list, part_pai& part)
{
    while (true)
    {
        if (part.count <= 5)
            return;

        if (part.count > 5)
        {
            int pai = part.list[0];
            if (pai_list[pai] > 1)
            {
                pai_list[pai] += 1;

                part.count -= 1;
                memcpy(&part.list[0], &part.list[1], part.count*sizeof(int));
                continue;
            }

            pai = part.list[part.count-1];
            if (pai_list[pai] > 1)
            {
                pai_list[pai] += 1;

                part.list[part.count-1] = 0;
                part.count -= 1;
                continue;
            }
        }

        if (part.count > 6)
        {
            int pai = part.list[1];
            if (pai_list[pai] > 1)
            {
                pai_list[pai] += 1;
                pai_list[part.list[0]] += 1;

                part.count -= 2;
                memcpy(&part.list[0], &part.list[2], part.count*sizeof(int));
                continue;
            }

            pai = part.list[part.count-2];
            if (pai_list[pai] > 1)
            {
                pai_list[pai] += 1;
                pai_list[part.list[part.count-1]] += 1;

                part.list[part.count - 1] = 0;
                part.list[part.count - 2] = 0;
                part.count -= 2;
                continue;
            }
        }

        return;
    }

}

static inline void process_order_and_pairs(int* pai_list, part_pai& part)
{
    while (true)
    {
        if (part.count <= 5)
            return;

        if (part.count > 5)
        {
            int pai = part.list[part.count-1];
            if (pai_list[pai] > 0)
            {
                pai_list[pai] += 1;

                part.list[part.count-1] = 0;
                part.count -= 1;
                continue;
            }
        }

        return;
    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline void n_parse_order(int* pai_list, parse_handcard& handcard)
{
    // 检查链条顺子
    part_pai order_pairs[10];
    int order_pairs_count = 0;
    for(int i = pai_type_3; i <= pai_type_K;)
    {
        if (pai_list[i] > 1 && pai_list[i+1] > 1 )
        {
            order_pairs[order_pairs_count].type = ddz_value_order_pairs;
            order_pairs[order_pairs_count].count = 2;
            order_pairs[order_pairs_count].list[0] = i;
            order_pairs[order_pairs_count].list[1] = i+1;

            help_parse_order_forward_link_alone_2(pai_list, order_pairs[order_pairs_count]);
            i = order_pairs[order_pairs_count].list[order_pairs[order_pairs_count].count-1]+1;

            order_pairs_count++;
        }
        else
            ++i;
    }

    part_pai real_order_pairs[10];
    int real_order_pairs_count = 0;
    for(int i = pai_type_3; i <= pai_type_Q;)
    {
        if (pai_list[i] > 1 && pai_list[i+1] > 1 && pai_list[i+2] > 1)
        {
            real_order_pairs[real_order_pairs_count].type = ddz_value_order_pairs;
            real_order_pairs[real_order_pairs_count].count = 3;
            real_order_pairs[real_order_pairs_count].list[0] = i;
            real_order_pairs[real_order_pairs_count].list[1] = i+1;
            real_order_pairs[real_order_pairs_count].list[2] = i+2;

            help_parse_order_forward_link_alone_2(pai_list, real_order_pairs[real_order_pairs_count]);
            i = real_order_pairs[real_order_pairs_count].list[real_order_pairs[real_order_pairs_count].count-1]+1;

            real_order_pairs_count++;
        }
        else
            ++i;
    }

    part_pai part_list[10];
    int part_count = 0;

    for(int i = pai_type_3; i <= pai_type_10;)
    {
        if (pai_list[i] > 0 && pai_list[i+1] > 0 && pai_list[i+2] > 0 && pai_list[i+3] > 0 && pai_list[i+4] > 0)
        {
            // 创建顺子
            part_list[part_count].type = ddz_type_order;
            part_list[part_count].count = 5;
            part_list[part_count].list[0] = i;
            part_list[part_count].list[1] = i+1;
            part_list[part_count].list[2] = i+2;
            part_list[part_count].list[3] = i+3;
            part_list[part_count].list[4] = i+4;

            help_parse_order_forward_link_alone(pai_list, part_list[part_count]);
            i = part_list[part_count].list[part_list[part_count].count-1]+1;

            part_count++;
        }
        else
            ++i;
    }

    // 形成顺子
    for(int i = 0; i < part_count; ++i)
    {
        unsigned char min_part = part_list[i].list[0];
        unsigned char max_part = part_list[i].list[part_list[i].count - 1];

        bool generate_order = false;

        for(int j = 0; j < order_pairs_count; ++j)
        {
            unsigned char min_pair = order_pairs[j].list[0];
            unsigned char max_pair = order_pairs[j].list[order_pairs[j].count -1];
            if (min_pair > min_part && max_pair < max_part)
            {
                // 生成链条顺子
                if ((max_pair - min_part + 1) >= 5 && (max_part - min_pair + 1) >= 5)
                {
                    generate_order = true;
                    help_generate_order(pai_list, min_part, max_pair, handcard);
                    help_generate_order(pai_list, min_pair, max_part, handcard);
                }
            }
        }

        if (!generate_order)
        {
            // 没有形成链条顺子，则简单生成顺子
            help_generate_order(pai_list, min_part, max_part, handcard);
        }
    }

    // 解析连对
    for(int i = 0; i < real_order_pairs_count; ++i)
    {
        bool is_have_order_pair = true;
        for(int j = 0; j < real_order_pairs[i].count; ++j)
        {
            int pai_index = real_order_pairs[i].list[j];
            if (pai_index < pai_type_3 || pai_index > pai_type_A)
            {
                is_have_order_pair = false;
                break;
            }

            if (pai_list[pai_index] <= 0)
            {
                // 连对用于二个顺子中，则不形成连对
                is_have_order_pair = false;
                break;
            }
        }

        if (is_have_order_pair)
        {
            // 判断拆顺子后的单牌数，如果过大，则不形成连对
            process_order_and_order_pairs(pai_list, real_order_pairs[i], handcard);
        }
    }

    // 处理顺子和三条的关系 和对子的关系
    int size = (int)handcard.list.size();
    for(int i = 0; i < size; ++i)
    {
        part_pai& part = handcard.list[i];
        if (part.type == ddz_value_order)
        {
            process_order_and_three(pai_list, part);
            process_order_and_pairs(pai_list, part);
        }
    }

    // 再次检查，看能不能形成新的连对
    for(int i = pai_type_3; i <= pai_type_Q; ++i)
    {
        if (pai_list[i] > 1 && pai_list[i+1] > 1 && pai_list[i+2] > 1)
        {
            part_pai part;

            part.type = ddz_value_order_pairs;
            part.count = 6;
            part.list[0] = i;part.list[1] = i;
            part.list[2] = i+1;part.list[3] = i+1;
            part.list[4] = i+2;part.list[5] = i+2;

            pai_list[i] -= 2;
            pai_list[i+1] -= 2;
            pai_list[i+2] -= 2;

            help_parse_order_forward_link_alone_3(pai_list, part);
            i = real_order_pairs[real_order_pairs_count].list[real_order_pairs[real_order_pairs_count].count-1]+1;

            handcard.list.push_back(part);

        }
        else
            ++i;

    }
}

/*-------------------------------------------------------------------------------------------------------------------*/
struct helpcalcvalue
{
    int count;
    int value;
    int type;
};
static void calc_handcard_value(parse_handcard& handcard)
{
    handcard.need_round = handcard.list.size();
    handcard.sum_value = 0;

    int three_count = 0;
    int airplane_count = 0;
    int alone_1_count = 0;
    int pairs_count = 0;
    for(int i = 0; i < (int)handcard.list.size(); ++i)
    {
        part_pai& part = handcard.list[i];
        handcard.sum_value += part.type;
        if (part.type == ddz_value_order)
            handcard.sum_value += (part.count - 5);
        if (part.type == ddz_value_order_pairs)
            handcard.sum_value += (part.count / 2 - 3)*2;
        if (part.type == ddz_value_order_airplane)
        {
            handcard.sum_value += (part.count / 3 - 2)*3;
            airplane_count += part.count / 3;
        }
        if (part.type == ddz_value_three)
            three_count++;
        if (part.type == ddz_value_alone_1)
            alone_1_count++;
        if (part.type == ddz_value_pairs)
            pairs_count++;
    }

    if (three_count != 0)
    {
        int sub_value = three_count;
        if ((alone_1_count+pairs_count) < three_count)
            sub_value = alone_1_count+pairs_count;
        handcard.need_round -= sub_value;
    }

    if (airplane_count >= 2)
    {
        int sub_value = airplane_count;
        if (alone_1_count < sub_value && pairs_count == 0)
            sub_value = 0;
        if (alone_1_count == 0 && pairs_count < airplane_count)
            sub_value = 0;
        if (alone_1_count + pairs_count*2 < airplane_count)
            sub_value = 0;

        handcard.need_round -= sub_value;
    }
}


/*-------------------------------------------------------------------------------------------------------------------*/
unsigned int g_parse_pai_time = 0;
int g_parse_pai_count  = 0;
static void real_parse_pai(int* pai_list, parse_handcard& handcard)
{
#ifdef _WIN32
    //unsigned int start_time = timeGetTime();
#endif

    g_parse_pai_count++;

    {
        handcard.list.clear();

        int list[pai_type_max];
        memcpy(list, pai_list, sizeof(int)*pai_type_max);
        parse_select_king(list, handcard);
        parse_select_airplane(list, handcard);
        n_parse_order(list, handcard);
        parse_select_alone(list, handcard);
    }
#ifdef _WIN32
    //g_parse_pai_time+= timeGetTime() - start_time;
#endif
}

static void select_hardcard_when_outpai(parse_handcard& select_handcard, const parse_handcard& handcard)
{
    std::vector<part_pai> alonePai;
    std::vector<part_pai> pairsPai;
    std::vector<part_pai> threePai;
    std::vector<part_pai> orderPai;
    std::vector<part_pai> orderPairsPai;
    std::vector<part_pai> airplanePai;

    int size = (int)handcard.list.size();
    for(int i = 0; i < size; ++i)
    {
        if (handcard.list[i].type == ddz_value_alone_1 && handcard.list[i].list[0] < pai_type_K)
            alonePai.push_back(handcard.list[i]);
        else if (handcard.list[i].type == ddz_value_pairs && handcard.list[i].list[0] < pai_type_K)
            pairsPai.push_back(handcard.list[i]);
        else if (handcard.list[i].type == ddz_value_three && handcard.list[i].list[0] <= pai_type_10)
            threePai.push_back(handcard.list[i]);
        else if (handcard.list[i].type == ddz_value_order)
            orderPai.push_back(handcard.list[i]);
        else if (handcard.list[i].type == ddz_value_order_pairs)
            orderPairsPai.push_back(handcard.list[i]);
        else if (handcard.list[i].type == ddz_value_order_airplane)
            airplanePai.push_back(handcard.list[i]);
    }

    if (!alonePai.empty())
    {
        std::sort(alonePai.begin(), alonePai.end());
        select_handcard.list.push_back(alonePai[0]);
    }
    if (!pairsPai.empty())
    {
        std::sort(pairsPai.begin(), pairsPai.end());
        select_handcard.list.push_back(pairsPai[0]);
    }
    if (!threePai.empty())
    {
        std::sort(threePai.begin(), threePai.end());
        select_handcard.list.push_back(threePai[0]);
    }
    if (!orderPai.empty())
    {
        std::sort(orderPai.begin(), orderPai.end());
        select_handcard.list.push_back(orderPai[0]);
    }
    if (!orderPairsPai.empty())
    {
        std::sort(orderPairsPai.begin(), orderPairsPai.end());
        select_handcard.list.push_back(orderPairsPai[0]);
    }
    if (!airplanePai.empty())
    {
        std::sort(airplanePai.begin(), airplanePai.end());
        select_handcard.list.push_back(airplanePai[0]);
    }

    if (select_handcard.list.empty())
    {
        for(int i = 0; i < (int)handcard.list.size(); ++i)
            select_handcard.list.push_back(handcard.list[i]);
    }
}

static void generate_move_list(const parse_handcard& handcard, std::vector<mcts_move*>& move_list)
{
    int size = (int)handcard.list.size();
    for(int i = 0; i < size; ++i)
    {
        const part_pai& part = handcard.list[i];
        if (part.type == ddz_value_alone_1)
        {
            add_move_to_list(move_list, new ddz_move(ddz_type_alone_1, part.list[0]));
        }
        else if (part.type == ddz_value_pairs)
        {
            add_move_to_list(move_list, new ddz_move(ddz_type_pair, part.list[0]));
        }
        else if (part.type == ddz_value_three)
        {
            add_move_to_list(move_list, new ddz_move(ddz_type_triple, part.list[0]));
        }
        else if (part.type == ddz_value_order_airplane)
        {
            ddz_move* move = new ddz_move(ddz_type_airplane);
            int jsize = part.count;
            for(int j = 0; j < jsize; ++j)
                move->_combo_list[move->_combo_count++] = part.list[j];
            add_move_to_list(move_list, move);
        }
        else if (part.type == ddz_value_order || part.type == ddz_value_order_pairs)
        {
            ddz_move* move = new ddz_move(part.type == ddz_value_order ? ddz_type_order : ddz_type_order_pair);
            int jsize = part.count;
            for(int j = 0; j < jsize; ++j)
                move->_combo_list[move->_combo_count++] = part.list[j];
            add_move_to_list(move_list, move);
        }
        else if (part.type == ddz_value_bomb)
        {
            if (part.count == 2 )
                add_move_to_list(move_list, new ddz_move(ddz_type_king_bomb));
            else
                add_move_to_list(move_list, new ddz_move(ddz_type_bomb, part.list[0]));
        }
    }
}

void stgy_split_card::generate_out_pai_move(int* pai_list, std::vector<mcts_move*>& out_move_list)
{
    parse_handcard	handcard;
    real_parse_pai(pai_list, handcard);

    check_whether_split_order(pai_list, handcard);

    if (handcard.list.empty())
    {
        return;
    }

    generate_move_list(handcard, out_move_list);

}

// 根据拆牌策略，有些顺子拆成对子会更好，这个函数检查能不能把顺子拆成对子
void stgy_split_card::check_whether_split_order(int* pai_list, parse_handcard &handcard)
{

}
