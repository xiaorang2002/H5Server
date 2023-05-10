
#include "ddz_move.h"
#include <algorithm>
#include <map>
#include "memory.h"

/*-------------------------------------------------------------------------------------------------------------------*/
const char*  name[14] = {"不出", "单牌", "一对", "三个", "三带一", "三带二", "顺子", "连对", "飞机", "飞机带牌", "炸弹", "王炸", "四带二单", "四带二对" };
const char*  pai_type[18] = {"", "", "", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A", "2", "小王", "大王"};

int g_new_move_count = 0;
int g_delete_move_count = 0;

int g_debug_count = 0;

ddz_move::ddz_move()
:_type(ddz_type_no_move), _alone_1(pai_type_none), _alone_2(pai_type_none), _combo_count(0), _alone_3(0), _alone_4(0), _airplane_pairs(false)
{
	g_new_move_count++;

	memset(_combo_list, 0, 20*sizeof(unsigned char));
}

ddz_move::ddz_move(unsigned char type, unsigned char alone1, unsigned char alone2, unsigned char alone3, unsigned char alone4)
:_type(type), _alone_1(alone1), _alone_2(alone2), _alone_3(alone3), _alone_4(alone4), _airplane_pairs(false), _combo_count(0)
{
	g_new_move_count++;
	memset(_combo_list, 0, 20*sizeof(unsigned char));
}

/*-------------------------------------------------------------------------------------------------------------------*/
ddz_move::~ddz_move()
{
	g_delete_move_count++;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static int move_cmp(const void *a,const void *b)
{
	return *(unsigned char *)a-*(unsigned char *)b;
}

/*-------------------------------------------------------------------------------------------------------------------*/
bool ddz_move::compare(const mcts_move* move)
{
	ddz_move* other = dynamic_cast<ddz_move*>(const_cast<mcts_move*>(move));
	if (!other)
		return false;

	if (other->_type != this->_type || other->_alone_1 != this->_alone_1 || other->_alone_2 != this->_alone_2 ||  other->_alone_3 != this->_alone_3 || other->_alone_4 != this->_alone_4 || other->_airplane_pairs != this->_airplane_pairs)
		return false;

	if (other->_combo_count != this->_combo_count)
		return false;

	if (other->_combo_count > 0)
	{
		qsort(other->_combo_list, other->_combo_count, sizeof(unsigned char), move_cmp);
		qsort(this->_combo_list, this->_combo_count, sizeof(unsigned char), move_cmp);

		int size = other->_combo_count;
		for(int i = 0; i < size; ++i)
		{
			if (other->_combo_list[i] != this->_combo_list[i])
				return false;
		}
	}

	return true;
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_move* ddz_move::clone()
{
	ddz_move* p = new ddz_move(this->_type, this->_alone_1, this->_alone_2, this->_alone_3, this->_alone_4);
	p->_airplane_pairs = this->_airplane_pairs;
	memcpy(p->_combo_list, this->_combo_list, sizeof(unsigned char)*20);
	p->_combo_count = this->_combo_count;

	return p;
}



/*-------------------------------------------------------------------------------------------------------------------*/
std::string ddz_move::get_test_value()
{
	std::string s_name = name[_type];
	if (_type != ddz_type_king_bomb)
		s_name += " ";
	switch (_type)
	{
	case ddz_type_alone_1:
	case ddz_type_pair:
	case ddz_type_triple:
	case ddz_type_bomb:
		s_name += pai_type[_alone_1];
		break;
	case ddz_type_triple_1:
		s_name += pai_type[_alone_1];
		s_name += "带";
		s_name += pai_type[_alone_2];
		break;
	case ddz_type_triple_2:
		s_name += pai_type[_alone_1];
		s_name += "带对";
		s_name += pai_type[_alone_2];
		break;
	case ddz_type_order:
		qsort(_combo_list, _combo_count, sizeof(unsigned char), move_cmp);
		for(int i = 0; i < _combo_count; ++i)
			s_name += pai_type[_combo_list[i]];
		break;
	case ddz_type_order_pair:
		qsort(_combo_list, _combo_count, sizeof(unsigned char), move_cmp);
		for(int i = 0; i < _combo_count; ++i)
			s_name += pai_type[_combo_list[i]];
		break;
	case ddz_type_airplane:
		qsort(_combo_list, _combo_count, sizeof(unsigned char), move_cmp);
		for(int i = 0; i < _combo_count; ++i)
			s_name += pai_type[_combo_list[i]];
		break;
	case ddz_type_airplane_with_pai:
		qsort(_combo_list, _combo_count, sizeof(unsigned char), move_cmp);
		for(int i = 0; i < _combo_count; ++i)
			s_name += pai_type[_combo_list[i]];
		s_name += "带";
		if (_airplane_pairs)
			s_name += "对";
		if (_alone_1 != 0)
			s_name += pai_type[_alone_1];
		if (_alone_2 != 0)
			s_name += pai_type[_alone_2];
		if (_alone_3 != 0)
			s_name += pai_type[_alone_3];
		if (_alone_4 != 0)
			s_name += pai_type[_alone_4];
		break;
	case ddz_type_four_with_alone_1:
		s_name += pai_type[_alone_1];
		s_name += "带";
		s_name += pai_type[_alone_2];
		s_name += pai_type[_alone_3];
		break;
	case ddz_type_four_with_pairs:
		s_name += pai_type[_alone_1];
		s_name += "带对";
		s_name += pai_type[_alone_2];
		s_name += pai_type[_alone_3];
		break;
	}

	return "["+s_name+"]";
}

