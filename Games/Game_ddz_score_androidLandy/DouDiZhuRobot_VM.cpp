
#include "DouDiZhuRobot_VM.h"

struct help_grab_landlord
{
    int need_round;
    int sum_value;
};

static const char*  name[14] = {"不出", "单牌", "一对", "三个", "三带一", "三带二", "顺子", "连对", "飞机", "飞机带牌", "炸弹", "王炸", "四带二单", "四带二对" };
static const char*  pai_type[18] = {"", "", "", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A", "2", "小王", "大王"};
static int move_cmp(const void *a,const void *b)
{
    return *(unsigned char *)a-*(unsigned char *)b;
}
static void dump_move(pai_interface_move &mv)
{
    std::string s_name = name[mv._type];
    if (mv._type != ddz_type_king_bomb)
        s_name += " ";
    switch (mv._type)
    {
    case ddz_type_alone_1:
    case ddz_type_pair:
    case ddz_type_triple:
    case ddz_type_bomb:
        s_name += pai_type[mv._alone_1];
        break;
    case ddz_type_triple_1:
        s_name += pai_type[mv._alone_1];
        s_name += "带";
        s_name += pai_type[mv._alone_2];
        break;
    case ddz_type_triple_2:
        s_name += pai_type[mv._alone_1];
        s_name += "带对";
        s_name += pai_type[mv._alone_2];
        break;
    case ddz_type_order:
        qsort(mv._combo_list, mv._combo_count, sizeof(unsigned char), move_cmp);
        for(int i = 0; i < mv._combo_count; ++i)
            s_name += pai_type[mv._combo_list[i]];
        break;
    case ddz_type_order_pair:
        qsort(mv._combo_list, mv._combo_count, sizeof(unsigned char), move_cmp);
        for(int i = 0; i < mv._combo_count; ++i)
            s_name += pai_type[mv._combo_list[i]];
        break;
    case ddz_type_airplane:
        qsort(mv._combo_list, mv._combo_count, sizeof(unsigned char), move_cmp);
        for(int i = 0; i < mv._combo_count; ++i)
            s_name += pai_type[mv._combo_list[i]];
        break;
    case ddz_type_airplane_with_pai:
        qsort(mv._combo_list, mv._combo_count, sizeof(unsigned char), move_cmp);
        for(int i = 0; i < mv._combo_count; ++i)
            s_name += pai_type[mv._combo_list[i]];
        s_name += "带";
        if (mv._airplane_pairs)
            s_name += "对";
        if (mv._alone_1 != 0)
            s_name += pai_type[mv._alone_1];
        if (mv._alone_2 != 0)
            s_name += pai_type[mv._alone_2];
        if (mv._alone_3 != 0)
            s_name += pai_type[mv._alone_3];
        if (mv._alone_4 != 0)
            s_name += pai_type[mv._alone_4];
        break;
    case ddz_type_four_with_alone_1:
        s_name += pai_type[mv._alone_1];
        s_name += "带";
        s_name += pai_type[mv._alone_2];
        s_name += pai_type[mv._alone_3];
        break;
    case ddz_type_four_with_pairs:
        s_name += pai_type[mv._alone_1];
        s_name += "带对";
        s_name += pai_type[mv._alone_2];
        s_name += pai_type[mv._alone_3];
        break;
    }

    //SHOW("[ %s ]", s_name.c_str());
}

static void calc_handcard_value(std::vector<mcts_move*>& move_list, help_grab_landlord& help_value)
{
    help_value.need_round = move_list.size();
    help_value.sum_value = 0;

    int three_count = 0;
    int airplane_count = 0;
    int alone_1_count = 0;
    int pairs_count = 0;
    for(int i = 0; i < (int)move_list.size(); ++i)
    {
        ddz_move* move = dynamic_cast<ddz_move*>(move_list[i]);
        help_value.sum_value += move->_type;
        if (move->_type == ddz_value_order)
            help_value.sum_value += (move->_combo_count - 5);
        if (move->_type == ddz_value_order_pairs)
            help_value.sum_value += (move->_combo_count / 2 - 3)*2;
        if (move->_type == ddz_value_order_airplane)
        {
            help_value.sum_value += (move->_combo_count / 3 - 2)*3;
            airplane_count += move->_combo_count / 3;
        }
        if (move->_type == ddz_value_three)
            three_count++;
        if (move->_type == ddz_value_alone_1)
            alone_1_count++;
        if (move->_type == ddz_value_pairs)
            pairs_count++;
    }

    if (three_count != 0)
    {
        int sub_value = three_count;
        if ((alone_1_count+pairs_count) < three_count)
            sub_value = alone_1_count+pairs_count;
        help_value.need_round -= sub_value;
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

        help_value.need_round -= sub_value;
    }
}

static void clean_up_hand_real(uint32_t *pHand_r)
{
    uint32_t tmp[pai_num_max] = {0};
    int j = 0;
    for (int i = 0; i < pai_num_max; i++)
    {
        uint32_t cv = pHand_r[i];
        if (cv != 0) {
            tmp[j++] = cv;
        }
    }
    memset(pHand_r, 0, pai_num_max*sizeof(uint32_t));
    for (int i = 0; i < j; i++)
    {
        pHand_r[i] = tmp[i];
    }
}

static short form_val(short v)
{
    if (v >= 15) {
        v++;
    } else if (v == 2) {
        v = 15;
    }
    return v;
}

short form_val2(short v)
{
    //0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    int n = v & 0x0F;
    short nRet = 1;
    switch (n)
    {
    case 1:
        nRet = emPokeValue_A;
        break;
    case 2:
        nRet = 15;
        break;
    case 3:
        nRet = emPokeValue_Three;
        break;
    case 4:
        nRet = emPokeValue_Four;
        break;
    case 5:
        nRet = emPokeValue_Five;
        break;
    case 6:
        nRet = emPokeValue_Six;
        break;
    case 7:
        nRet = emPokeValue_Seven;
        break;
    case 8:
        nRet = emPokeValue_Eight;
        break;
    case 9:
        nRet = emPokeValue_Nine;
        break;
    case 0xa:
        nRet = emPokeValue_Ten;
        break;
    case 0xb:
        nRet = emPokeValue_J;
        break;
    case 0xc:
        nRet = emPokeValue_Q;
        break;
    case 0xd:
        nRet = emPokeValue_K;
        break;
    case 0xe:
        nRet = 16;
        break;
    case 0xf:
        nRet = 17;
        break;
    default:
        break;
    }
    return nRet;
}

short form_color(uint8_t v)
{
    int n = v & 0xF0;
    short nRet = 1;
    switch (n)
    {
    case 0:
        nRet = 3;
        break;
    case 0x10:
        nRet = 4;
        break;
    case 0x20:
        nRet = 5;
        break;
    case 0x30:
        nRet = 6;
        break;
    default:
        break;
    }
    return nRet;
}

static short unform_val(short v)
{
    if (v >= 16) {
        v--;
    } else if (v == 15) {
        v = 2;
    }
    return v;
}

static void clean_num_val(uint8_t *pHand, uint32_t *pHand_r, uint8_t num, uint8_t val)
{
    pHand[val] -= num;
    int cnt = 0;
    for (int i = 0; i < 20; i++)
    {
        uint32_t cv = pHand_r[i];
        short v = cv & 0xff;
        if (v == val) {
            pHand_r[i] = 0;
            cnt++;
            if (cnt == num) return;
        }
    }
}

static void get_num_val(uint32_t *pHand_r, uint8_t num, uint8_t val, vector<ACard> &cds)
{
    int cnt = 0;
    for (int i = 0; i < 20; i++)
    {
        uint32_t cv = pHand_r[i];
        short v = cv & 0xff;
        short c = (cv >> 16) & 0xff;
        if (v == val) {
            cnt++;
            v = unform_val(v);
            cds.push_back(ACard((emPokeColor)c, (emPokeValue)v));
            if (cnt == num) return;
        }
    }
}

static void get_seq_val(uint32_t *pHand_r, uint8_t *list, uint8_t num, uint8_t factor, vector<ACard> &cds)
{
    uint8_t tag[pai_i_type_max] = {0};
    for (int i=0; i<num; i++)
    {
        tag[list[i]] = factor;
    }
    for (int i = 0; i < pai_num_max; i++)
    {
        uint32_t cv = pHand_r[i];
        short v = cv & 0xff;
        short c = (cv >> 16) & 0xff;
        if (tag[v] > 0) {
            v = unform_val(v);
            cds.push_back(ACard(emPokeColor(c), emPokeValue(v)));
            tag[v]--;
        }
    }
}

static void clean_seq_val(uint8_t *pHand, uint32_t *pHand_r, uint8_t *list, uint8_t num, uint8_t factor)
{
    uint8_t tag[pai_i_type_max] = {0};
    for (int i=0; i<num; i++)
    {
        uint8_t v = list[i];
        pHand[v] -= factor;
        tag[v] = factor;
    }
    for (int i = 0; i < pai_num_max; i++)
    {
        uint32_t cv = pHand_r[i];
        short v = cv & 0xff;
        if (tag[v] > 0) {
            pHand_r[i] = 0;
            tag[v]--;
        }
    }
}

static void get_alone_cards(vector<ACard> &cds, uint8_t *alone)
{
    for (vector<ACard>::iterator iter = cds.begin(); iter != cds.end(); iter++)
    {
        ACard &cd = *iter;
        short v = form_val(cd.n8CardNumber);
        alone[v]++;
    }
}

static int cycle(int start, int max = 3)
{
    if (start < 0 || start > max) return 1;
    start++;
    if (start > max) start = 1;
    return start;
}


CRobot::CRobot()
{
    Reset();
}

void CRobot::Reset()
{
    memset(_hands, 0, 3*pai_i_type_max*sizeof(uint8_t));
    memset(_grab, 0, 3*sizeof(uint8_t));
    memset(&_cur_desk_cards, 0, sizeof(pai_interface_move));
    memset(_hands_real, 0, 3*sizeof(uint32_t));
    memset(_grab_real, 0, 3*sizeof(uint32_t));
    _seat = 0;
    _landlord_seat = 0;
}

void CRobot::SetDesk(uint8_t seat, uint8_t hands[3][pai_i_type_max], uint8_t grab[3], uint32_t hands_r[3][pai_num_max], uint32_t grab_r[3])
{
    _seat = seat;

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < pai_i_type_max; j++)
        {
            _hands[i][j] = hands[i][j];
        }
        for (int j = 0; j < pai_num_max; j++)
        {
            _hands_real[i][j] = hands_r[i][j];
        }
    }

    for (int i = 0; i < 3; i++)
    {
        _grab[i] = grab[i];
        _grab_real[i] = grab_r[i];
    }
}

void CRobot::SetLandlord(uint8_t seat)
{
    _landlord_seat = seat;
    _cur_desk_owner = seat;
    uint8_t *pHand = _hands[seat-1];
    for (int i = 0; i < 3; i++)
    {
        pHand[_grab[i]]++;
    }
    uint32_t *pHand_real = _hands_real[seat-1];
    pHand_real[17] = _grab_real[0];
    pHand_real[18] = _grab_real[1];
    pHand_real[19] = _grab_real[2];
}

int CRobot::GrabLandlord()
{
    uint8_t *pai_list1 = _hands[_seat-1];
    uint8_t next = cycle(_seat);
    uint8_t *pai_list2 = _hands[next-1];
    next = cycle(next);
    uint8_t *pai_list3 = _hands[next-1];

    stgy_split_card _stgy_split_card;
    help_grab_landlord value[3];
    memset((void*)&value[0], 0, sizeof(help_grab_landlord)*3);

    int pai_list[pai_type_max];

    memset(pai_list, 0, sizeof(int)*pai_type_max);
    for(int i = pai_type_3; i < pai_type_max; ++i)
    {
        pai_list[i] = pai_list1[i];
    }
    for(int i = 0; i < 3; ++i)
    {
        if (_grab[i] >= pai_type_3 && _grab[i] < pai_type_max)
        {
            pai_list[_grab[i]] += 1;
        }
    }
    std::vector<mcts_move*> player_move1;
    _stgy_split_card.generate_out_pai_move(pai_list, player_move1);
    calc_handcard_value(player_move1, value[0]);

    memset(pai_list, 0, sizeof(int)*pai_type_max);
    for(int i = pai_type_3; i < pai_type_max; ++i)
    {
        pai_list[i] = pai_list2[i];
    }
    for(int i = 0; i < 3; ++i)
    {
        if (_grab[i] >= pai_type_3 && _grab[i] < pai_type_max)
        {
            pai_list[_grab[i]] += 1;
        }
    }
    std::vector<mcts_move*> player_move2;
    _stgy_split_card.generate_out_pai_move(pai_list, player_move2);
    calc_handcard_value(player_move2, value[1]);

    memset(pai_list, 0, sizeof(int)*pai_type_max);
    for(int i = pai_type_3; i < pai_type_max; ++i)
    {
        pai_list[i] = pai_list3[i];
    }
    for(int i = 0; i < 3; ++i)
    {
        if (_grab[i] >= pai_type_3 && _grab[i] < pai_type_max)
        {
            pai_list[_grab[i]] += 1;
        }
    }
    std::vector<mcts_move*> player_move3;
    _stgy_split_card.generate_out_pai_move(pai_list, player_move3);
    calc_handcard_value(player_move3, value[2]);

    for(int i = 0; i < player_move1.size(); ++i)
        delete player_move1[i];

    for(int i = 0; i < player_move2.size(); ++i)
        delete player_move2[i];

    for(int i = 0; i < player_move3.size(); ++i)
        delete player_move3[i];

    int grab_score = 0;

    if (value[0].need_round <= value[1].need_round && value[0].need_round <= value[2].need_round &&
        value[0].sum_value > value[1].sum_value && value[0].sum_value > value[2].sum_value)
        grab_score = 3;

    memset(pai_list, 0, sizeof(int)*pai_type_max);
    for(int i = pai_type_3; i < pai_type_max; ++i)
    {
        pai_list[i] = pai_list1[i];
    }
    for(int i = 0; i < 3; ++i)
    {
        if (_grab[i] >= pai_type_3 && _grab[i] < pai_type_max)
        {
            pai_list[_grab[i]] += 1;
        }
    }

    if (grab_score == 0)
    {
        if (pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0 && pai_list[pai_type_2] > 1)
            grab_score = 3;
        else if (pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_blossom] > 0 && pai_list[pai_type_2] == 1)
        {
            if (pai_list[pai_type_A] > 1)
                grab_score = 3;
            else
                grab_score = 2;
        }

        if (grab_score == 0)
        {
            if (pai_list[pai_type_blossom] > 0 && pai_list[pai_type_2] > 2)
                grab_score = 3;
            else if (pai_list[pai_type_blossom] > 0 && pai_list[pai_type_2] == 2)
            {
                if (pai_list[pai_type_A] > 1)
                    grab_score = 3;
                else
                    grab_score = 2;
            }
        }

        if (grab_score == 0)
        {
            if (pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_2] > 2)
                grab_score = 3;
            else if (pai_list[pai_type_blackjack] > 0 && pai_list[pai_type_2] == 2 && pai_list[pai_type_A] > 2)
                grab_score = 3;
        }
    }
    if (grab_score < 3)
        grab_score = 0;
    return grab_score;
}

int CRobot::GrabLandlordEX()
{
    int aiScore = 0;
//	unsigned int start = timeGetTime();
    //SHOW("GrabLandlordEX start = %u", start);
    for (int iter=0; iter<1; iter++)
    {
        ddz_state state;
        int playerid[3] = {1,2,3};
        state.setPlayerList(playerid, 3);
        state.setLandlord(1);

        // 复制牌
        uint8_t hands[3][pai_i_type_max] = {0};
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < pai_i_type_max; j++)
            {
                hands[i][j] = _hands[i][j];
            }
        }
        for (int i=0; i<3; i++)
        {
            hands[0][_grab[i]]++;
        }

        state.setPaiList(1, hands[_seat-1]);
        int next1 = cycle(_seat);
        state.setPaiList(2, hands[next1-1]);
        int next2 = cycle(next1);
        state.setPaiList(3, hands[next2-1]);

        compute_options player1_options;
        player1_options.max_iterations = 120;
        player1_options.number_of_threads = 1;
        player1_options.verbose = false;

        int count = 0;
        while (state.has_moves())
        {
            mcts_move* move = compute_move(&state, player1_options);
            if (move)
            {
                state.do_move(move);
                delete move;
            }

            move = compute_move(&state, player1_options);
            if (move)
            {
                state.do_move(move);
                delete move;
            }

            move = compute_move(&state, player1_options);
            if (move)
            {
                state.do_move(move);
                delete move;
            }

            count++;
            if (count > 80)
                break;
        }

        if (state.get_win(0))
            aiScore++;
    }
    //SHOW("GrabLandlordEX end = %u, aiScore = %d", timeGetTime(), aiScore);
    return aiScore;
}

SVar CRobot::search_cards(int seat, pai_interface_move &ptn)
{
    uint8_t *pHand = _hands[seat-1];
    uint32_t *pHand_r = _hands_real[seat-1];
    vector<ACard> list;
    switch (ptn._type)
    {
    case ddz_type_i_alone_1:
        {
            get_num_val(pHand_r, 1, ptn._alone_1, list);
        }
        break;
    case ddz_type_i_pair:
        {
            get_num_val(pHand_r, 2, ptn._alone_1, list);
        }
        break;
    case ddz_type_i_triple:
        {
            get_num_val(pHand_r, 3, ptn._alone_1, list);
        }
        break;
    case ddz_type_i_triple_1:
        {
            get_num_val(pHand_r, 3, ptn._alone_1, list);
            get_num_val(pHand_r, 1, ptn._alone_2, list);
        }
        break;
    case ddz_type_i_triple_2:
        {
            get_num_val(pHand_r, 3, ptn._alone_1, list);
            get_num_val(pHand_r, 2, ptn._alone_2, list);
        }
        break;
    case ddz_type_i_order:
        {
            get_seq_val(pHand_r, ptn._combo_list, ptn._combo_count, 1, list);
        }
        break;
    case ddz_type_i_order_pair:
        {
            get_seq_val(pHand_r, ptn._combo_list, ptn._combo_count, 2, list);
        }
        break;
    case ddz_type_i_airplane:
        {
            get_seq_val(pHand_r, ptn._combo_list, ptn._combo_count, 3, list);
        }
        break;
    case ddz_type_i_airplane_with_pai:
        {
            get_seq_val(pHand_r, ptn._combo_list, ptn._combo_count, 3, list);
            uint8_t factor = ptn._airplane_pairs ? 2 : 1;
            if (ptn._alone_1 > 0) {
                get_num_val(pHand_r, factor, ptn._alone_1, list);
            }
            if (ptn._alone_2 > 0) {
                get_num_val(pHand_r, factor, ptn._alone_2, list);
            }
            if (ptn._alone_3 > 0) {
                get_num_val(pHand_r, factor, ptn._alone_3, list);
            }
            if (ptn._alone_4 > 0) {
                get_num_val(pHand_r, factor, ptn._alone_4, list);
            }
        }
        break;
    case ddz_type_i_bomb:
        {
            get_num_val(pHand_r, 4, ptn._alone_1, list);
        }
        break;
    case ddz_type_i_king_bomb:
        {
            get_num_val(pHand_r, 1, 16, list);
            get_num_val(pHand_r, 1, 17, list);
        }
        break;
    case ddz_type_i_four_with_alone1:
        {
            get_num_val(pHand_r, 4, ptn._alone_1, list);
            get_num_val(pHand_r, 1, ptn._alone_2, list);
            get_num_val(pHand_r, 1, ptn._alone_3, list);
        }
        break;
    case ddz_type_i_four_with_pairs:
        {
            get_num_val(pHand_r, 4, ptn._alone_1, list);
            get_num_val(pHand_r, 2, ptn._alone_2, list);
            get_num_val(pHand_r, 2, ptn._alone_3, list);
        }
        break;
    }
    SVar s;
    ACard::ACardsToSVar(list,"pai", s);
    return s;
}

void CRobot::remove_played_cards(int seat, pai_interface_move &ptn)
{
    uint8_t *pHand = _hands[seat-1];
    uint32_t *pHand_r = _hands_real[seat-1];
    switch (ptn._type)
    {
        case ddz_type_i_alone_1:
            {
                clean_num_val(pHand, pHand_r, 1, ptn._alone_1);
            }
            break;
        case ddz_type_i_pair:
            {
                clean_num_val(pHand, pHand_r, 2, ptn._alone_1);
            }
            break;
        case ddz_type_i_triple:
            {
                clean_num_val(pHand, pHand_r, 3, ptn._alone_1);
            }
            break;
        case ddz_type_i_triple_1:
            {
                clean_num_val(pHand, pHand_r, 3, ptn._alone_1);
                clean_num_val(pHand, pHand_r, 1, ptn._alone_2);
            }
            break;
        case ddz_type_i_triple_2:
            {
                clean_num_val(pHand, pHand_r, 3, ptn._alone_1);
                clean_num_val(pHand, pHand_r, 2, ptn._alone_2);
            }
            break;
        case ddz_type_i_order:
            {
                clean_seq_val(pHand, pHand_r, ptn._combo_list, ptn._combo_count, 1);
            }
            break;
        case ddz_type_i_order_pair:
            {
                clean_seq_val(pHand, pHand_r, ptn._combo_list, ptn._combo_count, 2);
            }
            break;
        case ddz_type_i_airplane:
            {
                clean_seq_val(pHand, pHand_r, ptn._combo_list, ptn._combo_count, 3);
            }
            break;
        case ddz_type_i_airplane_with_pai:
            {
                clean_seq_val(pHand, pHand_r, ptn._combo_list, ptn._combo_count, 3);
                uint8_t factor = ptn._airplane_pairs ? 2 : 1;
                if (ptn._alone_1 > 0) {
                    clean_num_val(pHand, pHand_r, factor, ptn._alone_1);
                }
                if (ptn._alone_2 > 0) {
                    clean_num_val(pHand, pHand_r, factor, ptn._alone_2);
                }
                if (ptn._alone_3 > 0) {
                    clean_num_val(pHand, pHand_r, factor, ptn._alone_3);
                }
                if (ptn._alone_4 > 0) {
                    clean_num_val(pHand, pHand_r, factor, ptn._alone_4);
                }
            }
            break;
        case ddz_type_i_bomb:
            {
                clean_num_val(pHand, pHand_r, 4, ptn._alone_1);
            }
            break;
        case ddz_type_i_king_bomb:
            {
                clean_num_val(pHand, pHand_r, 1, 16);
                clean_num_val(pHand, pHand_r, 1, 17);
            }
            break;
        case ddz_type_i_four_with_alone1:
            {
                clean_num_val(pHand, pHand_r, 4, ptn._alone_1);
                clean_num_val(pHand, pHand_r, 1, ptn._alone_2);
                clean_num_val(pHand, pHand_r, 1, ptn._alone_3);
            }
            break;
        case ddz_type_i_four_with_pairs:
            {
                clean_num_val(pHand, pHand_r, 4, ptn._alone_1);
                clean_num_val(pHand, pHand_r, 2, ptn._alone_2);
                clean_num_val(pHand, pHand_r, 2, ptn._alone_3);
            }
            break;
    }
    clean_up_hand_real(pHand_r);
}

SVar CRobot::HelpPlayCard(ddz_state &state)
{
    int playerid[3] = {1,2,3};
    state.setPlayerList(playerid, 3);

    int temp = _seat;
    int nLandIdx = 1;
    for (int i=2; i<=3; i++)
    {
        int next = cycle(temp);
        if (next == _landlord_seat) {
            nLandIdx = i;
            break;
        }
        temp = next;
    }
    state.setLandlord(nLandIdx);

    state.setPaiList(1, _hands[_seat-1]);
    int next1 = cycle(_seat);
    state.setPaiList(2, _hands[next1-1]);
    int next2 = cycle(next1);
    state.setPaiList(3, _hands[next2-1]);

    compute_options player1_options;
    player1_options.max_iterations = 300;
    player1_options.number_of_threads = 1;
    player1_options.verbose = false;

    mcts_move* move = compute_move(&state, player1_options);
    pai_interface_move pim;

    if (move == 0) {
        return SVar();
    }
    ddz_move* dz_move = dynamic_cast<ddz_move*>(move);
    if (dz_move)
    {
        memset(&pim, 0, sizeof(pai_interface_move));
        pim._type = dz_move->_type;
        pim._alone_1 = dz_move->_alone_1;
        pim._alone_2 = dz_move->_alone_2;
        pim._alone_3 = dz_move->_alone_3;
        pim._alone_4 = dz_move->_alone_4;
        pim._airplane_pairs = dz_move->_airplane_pairs ? 1 : 0;
        pim._combo_count = dz_move->_combo_count;
        memcpy(pim._combo_list, dz_move->_combo_list, 20*sizeof(uint8_t));
        delete move;
        return search_cards(_seat, pim);
    }
    delete move;
    return SVar();
}

SVar CRobot::PlayCard()
{
    if (_cur_desk_owner == _seat)
    {
        // 主动出牌
        //SHOW("play card %d initive, landord = %d", _seat, _landlord_seat);
        ddz_state ddzState;
        return HelpPlayCard(ddzState);
    }
    else
    {
        //SHOW("play card %d passive, landlord = %d, outindex = %d", _seat, _landlord_seat, _cur_desk_owner);
        ddz_move* limit_move = new ddz_move(_cur_desk_cards._type, _cur_desk_cards._alone_1,_cur_desk_cards._alone_2, _cur_desk_cards._alone_3, _cur_desk_cards._alone_4);
        limit_move->_airplane_pairs = _cur_desk_cards._airplane_pairs == 1;
        limit_move->_combo_count = _cur_desk_cards._combo_count;
        memcpy(limit_move->_combo_list, _cur_desk_cards._combo_list, 20*sizeof(uint8_t));

        int temp = _seat;
        int nOutIndex = 1;
        for (int i=2; i<=3; i++)
        {
            int next = cycle(temp);
            if (next == _cur_desk_owner) {
                nOutIndex = i;
                break;
            }
            temp = next;
        }

        ddz_state ddzState(limit_move, nOutIndex-1);
        return HelpPlayCard(ddzState);
    }
}

void CRobot::FreshCards(uint8_t seat, vector<ACard> &cds, uint8_t px)
{
    //SHOW("FreshCards self:%d pre:%d now:%d", _seat, _cur_desk_owner, seat);
    _cur_desk_owner = seat;
    memset(&_cur_desk_cards, 0, sizeof(pai_interface_move));
    switch (px) {
        case emCODDZPaiXing_DanPai:
            {
                _cur_desk_cards._type = ddz_type_i_alone_1;
                _cur_desk_cards._alone_1 = form_val(cds[0].n8CardNumber);
            }
            break;
        case emCODDZPaiXing_DuiZi:
            {
                _cur_desk_cards._type = ddz_type_i_pair;
                _cur_desk_cards._alone_1 = form_val(cds[0].n8CardNumber);
            }
            break;
        case emCODDZPaiXing_LianDui:
            {
                _cur_desk_cards._type = ddz_type_i_order_pair;
                uint8_t alone[pai_i_type_max] = {0};
                get_alone_cards(cds, alone);
                int j = 0;
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] >= 2) {
                        _cur_desk_cards._combo_list[j++] = i;
                    }
                }
                _cur_desk_cards._combo_count = j;
            }
            break;
        case emCODDZPaiXing_SanZhang:
            {
                _cur_desk_cards._type = ddz_type_i_triple;
                _cur_desk_cards._alone_1 = form_val(cds[0].n8CardNumber);
            }
            break;
        case emCODDZPaiXing_SanDaiDanPai:
            {
                _cur_desk_cards._type = ddz_type_i_triple_1;
                uint8_t alone[pai_i_type_max] = {0};
                get_alone_cards(cds, alone);
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] == 3) {
                        _cur_desk_cards._alone_1 = i;
                    } else if (alone[i] == 1) {
                        _cur_desk_cards._alone_2 = i;
                    }
                }
            }
            break;
        case emCODDZPaiXing_SanDaiDuiZi:
            {
                _cur_desk_cards._type = ddz_type_i_triple_2;
                uint8_t alone[pai_i_type_max] = {0};
                get_alone_cards(cds, alone);
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] == 3) {
                        _cur_desk_cards._alone_1 = i;
                    } else if (alone[i] == 2) {
                        _cur_desk_cards._alone_2 = i;
                    }
                }
            }
            break;
        case emCODDZPaiXing_ShunZi:
            {
                _cur_desk_cards._type = ddz_type_i_order;
                uint8_t alone[pai_i_type_max] = {0};
                get_alone_cards(cds, alone);
                int j = 0;
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] == 1) {
                        _cur_desk_cards._combo_list[j++] = i;
                    }
                }
                _cur_desk_cards._combo_count = j;
            }
            break;
        case emCODDZPaiXing_SanShunZi:
            {
                _cur_desk_cards._type = ddz_type_i_airplane;
                _cur_desk_cards._alone_1 = form_val(cds[0].n8CardNumber);
            }
            break;
        case emCODDZPaiXing_FeiJiDaiDanPai:
            {
                _cur_desk_cards._type = ddz_type_i_airplane_with_pai;
                _cur_desk_cards._airplane_pairs = false;
                uint8_t alone[pai_i_type_max] = {0};
                get_alone_cards(cds, alone);
                int j = 0;
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] >= 3) {
                        _cur_desk_cards._combo_list[j++] = i;
                        alone[i] -= 3;
                    }
                }
                _cur_desk_cards._combo_count = j;
                int cnt = 0;
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] == 1) {
                        cnt++;
                        if (cnt == 1) {
                            _cur_desk_cards._alone_1 = i;
                        } else if (cnt == 2) {
                            _cur_desk_cards._alone_2 = i;
                        } else if (cnt == 3) {
                            _cur_desk_cards._alone_3 = i;
                        } else if (cnt == 4) {
                            _cur_desk_cards._alone_4 = i;
                        }
                    }
                }
            }
            break;
        case emCODDZPaiXing_FeiJiDaiDuiZi:
            {
                _cur_desk_cards._type = ddz_type_i_airplane_with_pai;
                _cur_desk_cards._airplane_pairs = true;
                uint8_t alone[pai_i_type_max] = {0};
                get_alone_cards(cds, alone);
                int j = 0;
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] >= 3) {
                        _cur_desk_cards._combo_list[j++] = i;
                        alone[i] -= 3;
                    }
                }
                _cur_desk_cards._combo_count = j;
                int cnt = 0;
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] == 2) {
                        cnt++;
                        if (cnt == 1) {
                            _cur_desk_cards._alone_1 = i;
                        } else if (cnt == 2) {
                            _cur_desk_cards._alone_2 = i;
                        } else if (cnt == 3) {
                            _cur_desk_cards._alone_3 = i;
                        } else if (cnt == 4) {
                            _cur_desk_cards._alone_4 = i;
                        }
                    }
                }
            }
            break;
        case emCODDZPaiXing_4Dai2DanPai:
            {
                _cur_desk_cards._type = ddz_type_i_four_with_alone1;
                uint8_t alone[pai_i_type_max] = {0};
                get_alone_cards(cds, alone);
                int cnt = 0;
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] == 4) {
                        _cur_desk_cards._alone_1 = i;
                    } else if (alone[i] == 1) {
                        cnt++;
                        if (cnt == 1) {
                            _cur_desk_cards._alone_2 = i;
                        } else if (cnt == 2) {
                            _cur_desk_cards._alone_3 = i;
                        }
                    }
                }
            }
            break;
        case emCODDZPaiXing_4Dai2DuiZi:
            {
                _cur_desk_cards._type = ddz_type_i_four_with_pairs;
                uint8_t alone[pai_i_type_max] = {0};
                get_alone_cards(cds, alone);
                int cnt = 0;
                for (int i=pai_i_type_3; i<pai_i_type_max; i++)
                {
                    if (alone[i] == 4) {
                        _cur_desk_cards._alone_1 = i;
                    } else if (alone[i] == 2) {
                        cnt++;
                        if (cnt == 1) {
                            _cur_desk_cards._alone_2 = i;
                        } else if (cnt == 2) {
                            _cur_desk_cards._alone_3 = i;
                        }
                    }
                }
            }
            break;
        case emCODDZPaiXing_YingZha:
            {
                _cur_desk_cards._type = ddz_type_i_bomb;
                _cur_desk_cards._alone_1 = form_val(cds[0].n8CardNumber);
            }
            break;
        case emCODDZPaiXing_HuoJian:
            {
                _cur_desk_cards._type = ddz_type_i_king_bomb;
            }
            break;
    }
    remove_played_cards(seat, _cur_desk_cards);
    //SHOW("====================seat = %d======================", seat);
    //for (int i=0; i<3; i++)
    //{
    //	SHOW("{\t0,0,0,%d,%d,%d,%d,%d,%d,%d, %d,%d,%d,%d,%d,%d,%d,%d}",
    //		_hands[i][3],_hands[i][4],_hands[i][5],_hands[i][6],_hands[i][7],_hands[i][8],_hands[i][9],_hands[i][10],
    //		_hands[i][11],_hands[i][12],_hands[i][13],_hands[i][14],_hands[i][15],_hands[i][16],_hands[i][17]);
    //}
    //dump_move(_cur_desk_cards);
}


RobotVM::RobotVM()
{
    m_mapRobot.clear();
}

void RobotVM::SC_CODDZ_FAPAI(int nSeat, LONGLONG playerId, SVar &s)
{
    map<LONGLONG, CRobot*>::iterator iter = m_mapRobot.find(playerId);
    if (iter != m_mapRobot.end()) {
        CRobot * pr = (CRobot *)(iter->second);
        delete pr;
        m_mapRobot.erase(iter);
    }
    CRobot *pRobot = new CRobot;
    m_mapRobot.insert(std::make_pair(playerId, pRobot));

    uint8_t hands[3][pai_i_type_max] = {0};
    uint32_t hands_real[3][pai_num_max] = {0};
    SVar cards = s["cards"];
    for (int i = 1; i <= cards.GetSize(); i++) {
        SVar &sc = cards[i];
        vector<ACard> cds = ACard::SVarToACards(sc, "pai");
        uint8_t seat = sc["seat"].ToNumber<uint8_t>();
        uint8_t *hand = hands[seat-1];
        uint32_t *hr = hands_real[seat-1];
        int j = 0;
        for (vector<ACard>::iterator iter = cds.begin(); iter != cds.end(); iter++) {
            ACard &cd = *iter;
            short v = cd.n8CardNumber;
            short c = cd.n8CardColor;
            v = form_val(v);
            hand[v]++;
            hr[j++] = c << 16 | v;
        }
    }
    vector<ACard> extra = ACard::SVarToACards(s, "extra");
    uint8_t grab[3] = {0};
    uint32_t grab_real[3] = {0};
    int i = 0;

    for (vector<ACard>::iterator iter = extra.begin(); iter != extra.end(); iter++) {
        ACard &cd = *iter;
        short v = cd.n8CardNumber;
        short c = cd.n8CardColor;
        v = form_val(v);
        grab[i] = v;
        grab_real[i++] = c << 16 | v;
    }

    pRobot->SetDesk(nSeat, hands, grab, hands_real, grab_real);
}

void RobotVM::SC_CODDZ_SET_DIZHU(int nSeat, LONGLONG playerId, SVar &s)
{
    uint8_t landlord_seat = s["dizhuseat"].ToNumber<uint8_t>();
    map<LONGLONG, CRobot*>::iterator iter = m_mapRobot.find(playerId);
    if (iter == m_mapRobot.end()) {
        //SHOW("SC_CODDZ_SET_DIZHU robot not exist");
        return;
    }

    CRobot *pRobot = (CRobot *)(iter->second);
    pRobot->SetLandlord(landlord_seat);
}

int RobotVM::SC_CODDZ_JIAODIZHU_NOTIFY(int nSeat, LONGLONG playerId)
{
    map<LONGLONG, CRobot*>::iterator iter = m_mapRobot.find(playerId);
    if (iter == m_mapRobot.end()) {
        //SHOW("SC_CODDZ_JIAODIZHU_NOTIFY robot not exist");
        return 0;
    }

    CRobot *pRobot = (CRobot *)(iter->second);
    return pRobot->GrabLandlord();
}

SVar RobotVM::SC_CODDZ_REQUEST_CHUPAI(int nSeat, LONGLONG playerId)
{
    map<LONGLONG, CRobot*>::iterator iter = m_mapRobot.find(playerId);
    if (iter == m_mapRobot.end()) {
        //SHOW("SC_CODDZ_JIAODIZHU_NOTIFY robot not exist");
        return SVar();
    }

    CRobot *pRobot = (CRobot *)(iter->second);
    return pRobot->PlayCard();
}

void RobotVM::SC_CODDZ_CHUPAI_END(int nSeat, LONGLONG playerId, SVar &s)
{
    map<LONGLONG, CRobot*>::iterator iter = m_mapRobot.find(playerId);
    if (iter == m_mapRobot.end()) {
        //SHOW("SC_CODDZ_JIAODIZHU_NOTIFY robot not exist");
        return;
    }

    uint8_t seat = s["seat"].ToNumber<uint8_t>();
    CRobot *pRobot = (CRobot *)(iter->second);
    vector<ACard> cd = ACard::SVarToACards(s, "pai");
    uint8_t px = s["paixin"].ToNumber<uint8_t>();
    //SHOW("SC_CODDZ_CHUPAI_END %I64d", playerId);
    pRobot->FreshCards(seat, cd, px);
}

void RobotVM::SC_CODDZ_SET_STATE(LONGLONG playerId, int state)
{
    if (state != 6) {
        return;
    }
    //SHOW("SC_CODDZ_SET_STATE clean robot %I64d", playerId);
    map<LONGLONG, CRobot*>::iterator iter = m_mapRobot.find(playerId);
    if (iter == m_mapRobot.end()) {
        //SHOW("SC_CODDZ_JIAODIZHU_NOTIFY robot not exist");
        return;
    }
    delete (CRobot*)(iter->second);
    m_mapRobot.erase(iter);
}

int RobotVM::SC_CODDZ_PLAYER_ADD_MUTI_NOTIFY()
{
    // 暂时都不加倍
    return 0;
}

void RobotVM::UNIT_TEST()
{
    //pai_interface_move pm;
    //memset(&pm, 0, sizeof(pai_interface_move));
    //pm._type = ddz_type_pair;
    //pm._alone_1 = 5;
    //ddz_move* limit_move = new ddz_move(pm._type, pm._alone_1,pm._alone_2, pm._alone_3, pm._alone_4);
    //limit_move->_airplane_pairs = pm._airplane_pairs == 1;
    //limit_move->_combo_count = pm._combo_count;
    //memcpy(limit_move->_combo_list, pm._combo_list, 20*sizeof(uint8_t));

    int _seat = 3;
    int _landlord_seat = 3;
    int desk_owner = 3;
    int temp_seat = _seat;
    int nOutIndex = 1;
    for (int i=2; i<=3; i++)
    {
        int next = cycle(temp_seat);
        if (next == desk_owner) {
            nOutIndex = i;
            break;
        }
        temp_seat = next;
    }

    //ddz_state state(limit_move, nOutIndex);
    ddz_state state;

    uint8_t _hands[3][pai_i_type_max] = {
        //	0,1,2,3,4,5,6,7,8,9,10,J,Q,K,A,2,B,S,

        {   0,0,0,1,0,1,2,1,2,4, 3,2,0,1,0,0,0,0},
        {   0,0,0,1,2,0,2,1,1,0, 0,2,3,1,3,1,0,0},
        {   0,0,0,0,0,1,0,2,1,0, 1,0,1,2,1,3,1,1},

        /*
        {	0,0,0,2,1,1,1,2,2,1, 2,1,0,0,3,1,0,0},
        {	0,0,0,1,0,1,1,0,0,0, 1,1,2,1,1,3,1,1},
        {	0,0,0,0,0,0,0,0,0,0, 0,0,0,1,0,0,0,0},

        {   0,0,0,1,2,1,1,1,1,1, 1,2,0,1,1,0,0,0},
        {   0,0,0,0,0,0,0,0,0,0, 0,0,0,0,2,3,0,0},
        {   0,0,0,0,0,0,0,2,0,1, 0,0,4,0,0,0,0,0},

        {	0,0,0,2,2,2,2,1,2,0, 1,0,1,1,1,0,0,0},
        {	0,0,0,1,1,1,2,3,0,1, 3,0,0,1,1,2,0,0},
        {	0,0,0,0,1,1,0,0,0,2, 0,4,2,2,2,1,0,0},

        {	0,0,0,0,4,2,1,1,1,1, 1,2,2,0,1,1,0,0},
        {	0,0,0,1,0,0,1,1,1,1, 1,1,1,2,1,1,1,1},
        {	0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,0},

        {   0,0,0,0,0,0,0,0,0,0, 0,0,1,1,0,0,0,0},
        {   0,0,0,0,0,1,0,1,0,0, 0,0,0,0,0,1,0,0},
        {   0,0,0,0,0,0,0,0,0,0, 1,0,0,0,0,0,0,0},
        {	0,0,0,0,1,1,1,3,2,1, 1,1,1,1,2,0,0,0},
        {	0,0,0,4,0,0,0,1,0,1, 1,1,1,1,0,0,0,0},
        {	0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,0},

        {	0,0,0,0,0,0,1,3,0,0, 1,1,1,1,1,0,0,0},
        {	0,0,0,2,1,0,1,0,2,0, 0,1,1,0,3,1,1,0},
        {	0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1},

        {	0,0,0,0,0,1,0,1,0,0, 0,1,0,0,0,1,0,0},
        {	0,0,0,2,2,1,0,0,1,0, 0,0,0,0,0,0,0,0},
        {	0,0,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,0},

        {	0,0,0,0,1,3,2,1,1,0, 2,1,1,1,2,1,0,1},
        {	0,0,0,0,0,0,0,2,2,1, 2,1,2,1,1,3,1,0},
        {	0,0,0,0,2,1,2,1,1,0, 0,2,1,2,1,0,0,0},

        {	0,0,0,1,1,2,1,1,1,1, 2,1,1,1,2,1,1,0},
        {	0,0,0,1,1,1,1,2,2,1, 2,2,1,1,2,2,0,1},
        {	0,0,0,2,2,1,2,1,1,2, 0,1,2,2,0,1,0,0},

        {	0,0,0,0,0,0,0,0,4,0, 0,0,0,0,1,1,0,0},
        {	0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,0},
        {	0,0,0,0,0,0,0,1,0,1, 0,1,0,1,0,1,0,0},

        {	0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0},
        {	0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0},
        {	0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0},

        {	0,0,0,0,1,1,2,3,0,0, 0,2,3,0,0,0,0,0},
        {	0,0,0,1,0,0,0,0,0,0, 0,0,0,2,1,0,0,0},
        {	0,0,0,0,1,0,0,0,0,0, 0,0,0,0,0,0,0,0},
        {	0,0,0,0,1,2,1,3,2,3, 1,1,0,1,2,0,0,0},
        {	0,0,0,0,2,0,1,0,0,1, 2,2,2,2,0,3,0,1},
        {	0,0,0,1,1,1,2,1,2,0, 1,1,2,1,2,1,1,0}
        */
    };

    int playerid[3] = {1,2,3};
    state.setPlayerList(playerid, 3);

    int temp = _seat;
    int nLandIdx = 1;
    for (int i=2; i<=3; i++)
    {
        int next = cycle(temp);
        if (next == _landlord_seat) {
            nLandIdx = i;
            break;
        }
        temp = next;
    }
    state.setLandlord(nLandIdx);

    state.setPaiList(1, _hands[_seat-1]);
    int next1 = cycle(_seat);
    state.setPaiList(2, _hands[next1-1]);
    int next2 = cycle(next1);
    state.setPaiList(3, _hands[next2-1]);

    compute_options player1_options;
    player1_options.max_iterations = 300;
    player1_options.number_of_threads = 1;
    player1_options.verbose = false;

    mcts_move* move = compute_move(&state, player1_options);
    pai_interface_move pim;

    if (move == 0) {
        //return SVar();
    }
    ddz_move* dz_move = dynamic_cast<ddz_move*>(move);
    if (dz_move)
    {
        memset(&pim, 0, sizeof(pai_interface_move));
        pim._type = dz_move->_type;
        pim._alone_1 = dz_move->_alone_1;
        pim._alone_2 = dz_move->_alone_2;
        pim._alone_3 = dz_move->_alone_3;
        pim._alone_4 = dz_move->_alone_4;
        pim._airplane_pairs = dz_move->_airplane_pairs ? 1 : 0;
        pim._combo_count = dz_move->_combo_count;
        memcpy(pim._combo_list, dz_move->_combo_list, 20*sizeof(uint8_t));
        delete move;
        return;
        //return search_cards(_seat, pim);
    }
    delete move;
}
