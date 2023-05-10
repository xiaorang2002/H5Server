
#include "ddz_state.h"
#include "ddz_move.h"
#include <algorithm>
#include <map>
#include "random_engine.h"
#ifdef _WIN32
#include "windows.h"
#endif

/*-------------------------------------------------------------------------------------------------------------------*/
ddz_state::ddz_state(ddz_move* limit_move, int out_pai_index) : _curr_player_index(0), _curr_move(limit_move), _curr_out_player_index(out_pai_index)
{
}

/*-------------------------------------------------------------------------------------------------------------------*/
ddz_state::ddz_state() : _curr_player_index(0), _curr_move(0), _curr_out_player_index(0)
{
}

/*-------------------------------------------------------------------------------------------------------------------*/
ddz_state::~ddz_state()
{
	if (_curr_move)
	{
		delete _curr_move;
		_curr_move = 0;
	}

	for(int i = 0; i < 3; ++i)
	{
		int size = _out_pai_move_list[i].size();
		for(int j = 0; j < size; ++j)
		{
			delete _out_pai_move_list[i][j];
		}
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline int move_cmp(const void *a,const void *b)
{
	return *(unsigned char *)a-*(unsigned char *)b;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline bool is_empty(int* list)
{
	for(int i = pai_type_3; i <= pai_type_blossom; ++i)
		if (list[i] > 0)
			return false;

	return true;
}

/*-------------------------------------------------------------------------------------------------------------------*/
std::vector<mcts_move*>	ddz_state::get_moves() 
{
	if ( is_empty(_player_list[_curr_player_index].pai_list) )
		return std::vector<mcts_move*>();

	if (_curr_player_index == _curr_out_player_index)
	{
		delete _curr_move;
		_curr_move = 0;
	}

	return parse_pai(_player_list[_curr_player_index].pai_list, _out_pai_move_list[_curr_player_index], _curr_move);
}

/*-------------------------------------------------------------------------------------------------------------------*/
bool ddz_state::is_game_end()
{
	for(int i = 0; i < 3; ++i)
	{
		if (_player_list[i].is_win)
			return true;
	}

	return false;
}

/*-------------------------------------------------------------------------------------------------------------------*/
int ddz_state::get_player_to_move() const
{
	return _player_list[_curr_player_index].uid;
}

/*-------------------------------------------------------------------------------------------------------------------*/
bool ddz_state::has_moves()
{
	if (is_game_end())
		return false;

	for(int i = 0; i < 3; ++i)
	{
		if ( is_empty(_player_list[i].pai_list) )
			return false;
	}
	return true;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::do_random_move(random_engine* randomEngine)
{
	std::vector<mcts_move*> moves = this->get_moves();
	if (!moves.empty())
	{
		int index = randomEngine->get_random() % moves.size();
		ddz_move* p_m = dynamic_cast<ddz_move*>(moves[index]);
		do_move( moves[index] );
		
		int size = (int)moves.size();
		for(int i = 0; i < size; ++i)
		{
			delete moves[i];
		}
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::do_move(const mcts_move* move)
{
	if (is_game_end())
		return;

	ddz_move* p_move = dynamic_cast<ddz_move*>(const_cast<mcts_move*>(move));
	if (p_move)
	{
		if ( p_move->_type == ddz_type_no_move )
		{
			if ( _curr_out_player_index == _curr_player_index )
			{
				// 由当前玩家自由出牌
				if (_curr_move)
				{
					delete _curr_move;
					_curr_move = 0;
				}

				std::vector<mcts_move*> moves = this->get_moves();
				if (!moves.empty())
				{
					int index = _random_engine.get_random() % moves.size();
					ddz_move* p_c_move = dynamic_cast<ddz_move*>(const_cast<mcts_move*>(moves[index]));
					_curr_move = dynamic_cast<ddz_move*>(p_c_move->clone());

					do_real_move(_curr_move);

					int size = (int)moves.size();
					for(int i = 0; i < size; ++i)
					{
						delete moves[i];
					}
				}

			}
			else
				go_next_player();
		}
		else
		{
			do_real_move(p_move);
		}
	}
	else
	{
		int sss = 0;
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::do_real_move(ddz_move* p_move)
{
	if (p_move != _curr_move)
	{
		delete _curr_move;
		_curr_move = dynamic_cast<ddz_move*>(p_move->clone());
	}
	_curr_out_player_index = _curr_player_index;

	sub_valid_move(p_move);

	check_win();

	go_next_player();
}

/*-------------------------------------------------------------------------------------------------------------------*/
bool delete_same_move_from_list(std::vector<mcts_move*>& move_list, unsigned char type, unsigned char pai)
{
	bool is_delete = false;
	std::vector<mcts_move*>::iterator iter = move_list.begin();
	for( ; iter != move_list.end(); ++iter)
	{
		ddz_move* p_move = dynamic_cast<ddz_move*>(*iter);
		if (p_move->_type == type && p_move->_alone_1 == pai)
		{
			move_list.erase(iter);
			delete p_move;
			is_delete = true;
			break;
		}
	}

	return is_delete;
}

/*-------------------------------------------------------------------------------------------------------------------*/
bool delete_same_move_from_list_2(std::vector<mcts_move*>& move_list, ddz_move* move)
{
	bool is_delete = false;
	std::vector<mcts_move*>::iterator iter = move_list.begin();
	for( ; iter != move_list.end(); ++iter)
	{
		ddz_move* p_move = dynamic_cast<ddz_move*>(*iter);
		if (p_move->compare(move))
		{
			move_list.erase(iter);
			delete p_move;
			is_delete = true;
			break;
		}
	}

	return is_delete;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::sub_valid_move_out_pai(ddz_move* p_move)
{
	std::vector<mcts_move*>& move_list = _out_pai_move_list[_curr_player_index];
	bool is_no_need_parse_pai = true;

	switch (p_move->_type)
	{
	case ddz_type_alone_1:
	case ddz_type_pair:
	case ddz_type_triple:
	case ddz_type_bomb:
	case ddz_type_king_bomb:
		is_no_need_parse_pai = delete_same_move_from_list(move_list, p_move->_type, p_move->_alone_1 );
		break;
	case ddz_type_triple_1:
		is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_triple, p_move->_alone_1 );
		if (is_no_need_parse_pai)
			is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_alone_1, p_move->_alone_2 );
		break;
	case ddz_type_triple_2:
		is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_triple, p_move->_alone_1 );
		if (is_no_need_parse_pai)
			is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_pair, p_move->_alone_2 );
		break;
	case ddz_type_order:
	case ddz_type_order_pair:
	case ddz_type_airplane:
		is_no_need_parse_pai = delete_same_move_from_list_2(move_list, p_move);
		break;
	case ddz_type_airplane_with_pai:
		{
			ddz_move* delete_move = new ddz_move(ddz_type_airplane);
			delete_move->_combo_count = p_move->_combo_count;
			memcpy(delete_move->_combo_list, p_move->_combo_list, 20*sizeof(unsigned char));
			is_no_need_parse_pai = delete_same_move_from_list_2(move_list, p_move);
			if (is_no_need_parse_pai)
			{
				if (p_move->_alone_1 != 0)
					is_no_need_parse_pai = delete_same_move_from_list(move_list, p_move->_airplane_pairs ? ddz_type_pair : ddz_type_alone_1, p_move->_alone_1 );
				if (p_move->_alone_2 != 0 && is_no_need_parse_pai)
					is_no_need_parse_pai = delete_same_move_from_list(move_list, p_move->_airplane_pairs ? ddz_type_pair : ddz_type_alone_1, p_move->_alone_2 );
				if (p_move->_alone_3 != 0 && is_no_need_parse_pai)
					is_no_need_parse_pai = delete_same_move_from_list(move_list, p_move->_airplane_pairs ? ddz_type_pair : ddz_type_alone_1, p_move->_alone_3 );
				if (p_move->_alone_4 != 0 && is_no_need_parse_pai)
					is_no_need_parse_pai = delete_same_move_from_list(move_list, p_move->_airplane_pairs ? ddz_type_pair : ddz_type_alone_1, p_move->_alone_4 );
			}
			delete delete_move;
		}
		break;

	case ddz_type_four_with_alone_1:
		is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_bomb, p_move->_alone_1 );
		if (is_no_need_parse_pai)
			is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_alone_1, p_move->_alone_2 );
		if (is_no_need_parse_pai)
			is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_alone_1, p_move->_alone_3 );
		break;

	case ddz_type_four_with_pairs:
		is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_bomb, p_move->_alone_1 );
		if (is_no_need_parse_pai)
			is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_pair, p_move->_alone_2 );
		if (is_no_need_parse_pai)
			is_no_need_parse_pai = delete_same_move_from_list(move_list, ddz_type_pair, p_move->_alone_3 );
		break;
	}

	if (!is_no_need_parse_pai)
		generate_out_pai_move(_player_list[_curr_player_index].pai_list, move_list);
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::sub_valid_move(ddz_move* p_move)
{
	int* pai_list = _player_list[_curr_player_index].pai_list;

	switch (p_move->_type)
	{
	case ddz_type_alone_1:
		pai_list[p_move->_alone_1] -= 1;
		break;
	case ddz_type_pair:
		pai_list[p_move->_alone_1] -= 2;
		break;
	case ddz_type_triple:
		pai_list[p_move->_alone_1] -= 3;
		break;
	case ddz_type_triple_1:
		pai_list[p_move->_alone_1] -= 3;
		pai_list[p_move->_alone_2] -= 1;
		break;
	case ddz_type_triple_2:
		pai_list[p_move->_alone_1] -= 3;
		pai_list[p_move->_alone_2] -= 2;
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
					pai_list[i] -= 1;
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
					pai_list[i] -= 2;
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
					pai_list[i] -= 3;
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
					pai_list[i] -= 3;
				}
			}
			if (p_move->_alone_1 != 0)
			{
				pai_list[p_move->_alone_1] -= p_move->_airplane_pairs ? 2:1;
			}
			if (p_move->_alone_2 != 0)
			{
				pai_list[p_move->_alone_2] -= p_move->_airplane_pairs ? 2:1;
			}
			if (p_move->_alone_3 != 0)
			{
				pai_list[p_move->_alone_3] -= p_move->_airplane_pairs ? 2:1;
			}
			if (p_move->_alone_4 != 0)
			{
				pai_list[p_move->_alone_4] -= p_move->_airplane_pairs ? 2:1;
			}
		}
		break;
	case ddz_type_bomb:
		pai_list[p_move->_alone_1] -= 4;
		break;
	case ddz_type_king_bomb:
		pai_list[pai_type_blossom] = 0;
		pai_list[pai_type_blackjack] = 0;
		break;

	case ddz_type_four_with_alone_1:
		pai_list[p_move->_alone_1] -= 4;
		pai_list[p_move->_alone_2] -= 1;
		pai_list[p_move->_alone_3] -= 1;
		break;

	case ddz_type_four_with_pairs:
		pai_list[p_move->_alone_1] -= 4;
		pai_list[p_move->_alone_2] -= 2;
		pai_list[p_move->_alone_3] -= 2;
		break;
	}

	sub_valid_move_out_pai(p_move);

}

void ddz_state::check_win()
{
	if (is_empty(_player_list[_curr_player_index].pai_list))
		_player_list[_curr_player_index].is_win = true;

	if (_player_list[_curr_player_index].is_win && !_player_list[_curr_player_index].is_landlord)
	{
		for(int i = 0; i < MAX_PLAYER; ++i)
		{
			if (!_player_list[i].is_landlord)
				_player_list[i].is_win = true;
		}
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::go_next_player()
{
	if (is_game_end())
		return;

	_curr_player_index++;
	if (_curr_player_index >= MAX_PLAYER)
		_curr_player_index = 0;
}

/*-------------------------------------------------------------------------------------------------------------------*/
double ddz_state::get_result(int player_id)
{
	for(int i = 0; i < MAX_PLAYER; ++i)
	{
		if (player_id == _player_list[i].uid)
		{
			return _player_list[i].is_win ? 1 : 0;
		}
	}
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_state* ddz_state::clone()
{
	ddz_state* p = new ddz_state;
	p->cloneFrom(this);
	return p;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::cloneFrom(const mcts_state* state)
{
	ddz_state* p = dynamic_cast<ddz_state*>(const_cast<mcts_state*>(state));
	this->_curr_player_index = p->_curr_player_index;
	memcpy(this->_player_list, p->_player_list, sizeof(ddz_player)*MAX_PLAYER);

	delete this->_curr_move;
	if (p->_curr_move)
		this->_curr_move = dynamic_cast<ddz_move*>(p->_curr_move->clone());
	else
		this->_curr_move = 0;

	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < (int)_out_pai_move_list[i].size(); ++j)
		{
			delete _out_pai_move_list[i][j];
		}
		_out_pai_move_list[i].clear();
	}


	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < (int)p->_out_pai_move_list[i].size(); ++j)
		{
			_out_pai_move_list[i].push_back(p->_out_pai_move_list[i][j]->clone());
		}
	}

}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::setPlayerList(int* list, int c)
{
	for(int i = 0; i < c && i < MAX_PLAYER; ++i)
	{
		_player_list[i].uid = list[i];
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::setLandlord(int playerId)
{
	for(int i = 0; i < MAX_PLAYER; ++i)
	{
		if ( _player_list[i].uid == playerId )
		{
			_player_list[i].is_landlord = true;
			break;
		}
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::setPaiList(int playerId, const unsigned char* paiList)
{
	for(int i = 0; i < MAX_PLAYER; ++i)
	{
		if ( _player_list[i].uid == playerId )
		{
			for(int j = 0; j < pai_type_max; ++j)
			{
				if (paiList[j] != 0)
				{
					_player_list[i].pai_list[j] = paiList[j];
				}
			}

			generate_out_pai_move(_player_list[i].pai_list, _out_pai_move_list[i]);
			break;
		}
	}
}

std::string ddz_state::get_pai_list(int uid)
{
	unsigned char list_pai[120];
	memset(list_pai, 0, sizeof(unsigned char)*120);
	int index = 0;
	for(int i = 0; i < MAX_PLAYER; ++i)
	{
		if (_player_list[i].uid == uid)
		{
			for(int j = 0; j <= pai_type_blossom; ++j)
			{
				if (_player_list[i].pai_list[j] >= 1)
				{
					list_pai[index++] = j;
				}
				if(_player_list[i].pai_list[j] >= 2)
				{
					list_pai[index++] = j;
				}
				if (_player_list[i].pai_list[j] >= 3)
				{
					list_pai[index++] = j;
				}
				if (_player_list[i].pai_list[j] >= 4)
				{
					list_pai[index++] = j;
				}
			}
			break;
		}
	}
	extern const char*  pai_type[18];
	qsort(list_pai, index, sizeof(unsigned char), move_cmp);
	std::string str = "";
	for(int i = 0; i < index; ++i)
	{
		str += pai_type[list_pai[i]];
	}
	return str;
}

/*-------------------------------------------------------------------------------------------------------------------*/
const char* ddz_state::get_pai_value(int u)
{
	extern const char*  pai_type[18];
	if (u >= pai_type_3 && u <= pai_type_blossom)
	{
		return pai_type[u];
	}
	
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------*/
std::vector<mcts_move*> ddz_state::parse_pai(int* pai_list, std::vector<mcts_move*>& out_pai_list, ddz_move* limit_move)
{
	if (limit_move != 0)
	{
		return _stgy_passive_play_card.parse_pai(this, limit_move);
	}

	int size = out_pai_list.size();
	if (size == 0)
	{
		generate_out_pai_move(_player_list[_curr_player_index].pai_list, out_pai_list);
	}

	return _stgy_initiative_play_card.parse_pai(this);
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_state::generate_out_pai_move(int* pai_list, std::vector<mcts_move*>& out_move_list)
{
	for(int i = 0; i < (int)out_move_list.size(); ++i)
	{
		delete out_move_list[i];
	}
	out_move_list.clear();

	_stgy_split_card.generate_out_pai_move(pai_list, out_move_list);
}

/*-------------------------------------------------------------------------------------------------------------------*/
static inline bool is_alone_1(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 1)
		return false;

	p_move._type = ddz_type_alone_1;
	p_move._alone_1 = pai_list[0];
	return true;
}

static inline bool is_pairs(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 2 || pai_list[0] != pai_list[1])
		return false;

	p_move._type = ddz_type_pair;
	p_move._alone_1 = pai_list[0];
	return true;
}

static inline bool is_triple(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 3 || pai_list[0] != pai_list[1] || pai_list[1] != pai_list[2])
		return false;

	p_move._type = ddz_type_triple;
	p_move._alone_1 = pai_list[0];
	return true;
}

static inline bool is_triple_1(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 4)
		return false;

	struct st_help_triple_1
	{
		unsigned char val;
		unsigned char count;
	};

	st_help_triple_1 help_triple[2];
	memset(help_triple, 0, sizeof(st_help_triple_1)*2);

	for(int i = 0; i < 4; ++i)
	{
		if (help_triple[0].val == 0 || help_triple[0].val == pai_list[i])
		{
			help_triple[0].val = pai_list[i];
			help_triple[0].count++;
		}
		else if (help_triple[1].val == 0 || help_triple[1].val == pai_list[i])
		{
			help_triple[1].val = pai_list[i];
			help_triple[1].count++;
		}
		else
		{
			return false;
		}
	}
	
	unsigned char alone1, alone2 = 0;
	bool have_3_1 = false;
	if (help_triple[0].count == 3 && help_triple[1].count == 1)
	{
		have_3_1 = true;
		alone1 = help_triple[0].val; alone2 = help_triple[1].val;
	}
	else if (help_triple[1].count == 3 && help_triple[0].count == 1)
	{
		have_3_1 = true;
		alone1 = help_triple[1].val; alone2 = help_triple[0].val;
	}
	
	if (have_3_1)
	{
		p_move._type = ddz_type_triple_1;
		p_move._alone_1 = alone1;
		p_move._alone_2 = alone2;

		return true;
	}
	return false;
}

static inline bool is_triple_2(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 5)
		return false;

	struct st_help_triple_2
	{
		unsigned char val;
		unsigned char count;
	};

	st_help_triple_2 help_triple[2];
	memset(help_triple, 0, sizeof(st_help_triple_2)*2);

	for(int i = 0; i < 5; ++i)
	{
		if (help_triple[0].val == 0 || help_triple[0].val == pai_list[i])
		{
			help_triple[0].val = pai_list[i];
			help_triple[0].count++;
		}
		else if (help_triple[1].val == 0 || help_triple[1].val == pai_list[i])
		{
			help_triple[1].val = pai_list[i];
			help_triple[1].count++;
		}
		else
		{
			return false;
		}
	}

	unsigned char alone1, alone2 = 0;
	bool have_3_2 = false;
	if (help_triple[0].count == 3 && help_triple[1].count == 2)
	{
		have_3_2 = true;
		alone1 = help_triple[0].val; alone2 = help_triple[1].val;
	}
	else if (help_triple[1].count == 3 && help_triple[0].count == 2)
	{
		have_3_2 = true;
		alone1 = help_triple[1].val; alone2 = help_triple[0].val;
	}

	if (have_3_2)
	{
		p_move._type = ddz_type_triple_2;
		p_move._alone_1 = alone1;
		p_move._alone_2 = alone2;

		return true;
	}
	return false;
}

static inline bool is_bomb(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 4)
		return false;

	if (pai_list[0] != pai_list[1] || pai_list[1] != pai_list[2] || pai_list[2] != pai_list[3] )
	{
		return false;
	}

	p_move._type = ddz_type_bomb;
	p_move._alone_1 = pai_list[0];
	return true;
}

static inline bool is_king_bomb(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 2)
		return false;

	if ((pai_list[0] == pai_type_blackjack && pai_list[1] == pai_type_blossom) || (pai_list[1] == pai_type_blackjack && pai_list[0] == pai_type_blossom))
	{
		p_move._type = ddz_type_king_bomb;
		return true;
	}

	return false;
}

static inline bool is_order(std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() < 5)
		return false;

	std::sort(pai_list.begin(), pai_list.end());
	unsigned char comp_char = pai_list[0];
	int size = (int)pai_list.size();
	for(int i = 1; i < size; ++i)
	{
		if (pai_list[i] == comp_char+1)
			comp_char = pai_list[i];
		else
			return false;
	}

	p_move._type = ddz_type_order;
	for(int i = 0; i < size; ++i)
	{
		p_move._combo_list[p_move._combo_count++] = pai_list[i];
	}
	return true;
}

static inline bool is_order_pairs(std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() < 6 || (pai_list.size() % 2) != 0)
		return false;

	int map_list[pai_type_max];
	memset(map_list, 0, sizeof(int)*pai_type_max);
	for(int i = 0; i < (int)pai_list.size(); ++i)
	{
		map_list[pai_list[i]]++;
	}
	std::vector<unsigned char> order_pairs;
	for(int i = pai_type_3; i <= pai_type_A; ++i)
	{
		if (map_list[i] == 0)
			continue;

		if (map_list[i]==2)
			order_pairs.push_back(i);
		else
			return false;
	}

	if (order_pairs.size() < 3)
		return false;

	int size = (int)order_pairs.size();
	unsigned char cmp_char = order_pairs[0];
	for(int i = 1; i < size; ++i)
	{
		if (cmp_char+1 == order_pairs[i])
			cmp_char = order_pairs[i];
		else
			return false;
	}

	p_move._type = ddz_type_order_pair;
	for(int i = 0; i < (int)pai_list.size(); ++i)
	{
		p_move._combo_list[p_move._combo_count++] = pai_list[i];
	}
	return true;
}

static inline bool is_order_airplane(std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() < 6 || (pai_list.size() % 3) != 0)
		return false;

	int map_list[pai_type_max];
	memset(map_list, 0, sizeof(int)*pai_type_max);
	for(int i = 0; i < (int)pai_list.size(); ++i)
	{
		map_list[pai_list[i]]++;
	}
	std::vector<unsigned char> airplane;
	for(int i = pai_type_3; i <= pai_type_A; ++i)
	{
		if (map_list[i] == 0)
			continue;

		if (map_list[i]==3)
			airplane.push_back(i);
		else
			return false;
	}

	if (airplane.size() < 2)
		return false;

	int size = (int)airplane.size();
	unsigned char cmp_char = airplane[0];
	for(int i = 1; i < size; ++i)
	{
		if (cmp_char+1 == airplane[i])
			cmp_char = airplane[i];
		else
			return false;
	}

	p_move._type = ddz_type_airplane;
	for(int i = 0; i < (int)pai_list.size(); ++i)
	{
		p_move._combo_list[p_move._combo_count++] = pai_list[i];
	}
	return true;
}

struct st_help_with_pai
{
	unsigned char val;
	unsigned char count;
};

static inline bool is_order_airplane_with_pai(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() < 8)
		return false;

	int map_list[pai_type_max];
	memset(map_list, 0, sizeof(int)*pai_type_max);
	for(int i = 0; i < (int)pai_list.size(); ++i)
	{
		map_list[pai_list[i]]++;
	}

	std::vector<unsigned char> airplane;
	std::vector<st_help_with_pai> with_pai;
	bool is_with_pairs = true;				// 是否是对牌
	for(int i = pai_type_3; i <= pai_type_A; ++i)
	{
		if (map_list[i] > 3)
		{
			airplane.push_back(i);
			st_help_with_pai help_with_pai;
			help_with_pai.val = i;
			help_with_pai.count = 1;
			with_pai.push_back(help_with_pai);
			is_with_pairs = false;
		}
		else if (map_list[i]>2)
			airplane.push_back(i);
		else if (map_list[i] > 0)
		{
			st_help_with_pai help_with_pai;
			help_with_pai.val = i;
			help_with_pai.count = map_list[i] > 1?2:1;
			if (map_list[i] == 1)
				is_with_pairs = false;
			with_pai.push_back(help_with_pai);
		}
	}

	if (airplane.size() < 2)
		return false;

	std::sort(airplane.begin(), airplane.end());
	int size = (int)airplane.size();
	unsigned char cmp_char = airplane[0];
	for(int i = 1; i < size; ++i)
	{
		if (cmp_char+1 == airplane[i])
			cmp_char = airplane[i];
		else
			return false;
	}

	unsigned char alone[4] = {0, 0, 0, 0};
	if (is_with_pairs && size == with_pai.size())
	{
		for(int i = 0; i < size; ++i)
		{
			alone[i] = with_pai[i].val;
		}
		p_move._airplane_pairs = true;
		p_move._alone_1 = alone[0];p_move._alone_2 = alone[1];p_move._alone_3 = alone[2];p_move._alone_4 = alone[3];
	}
	else
	{
		int count = 0;
		for(int i = 0; i < (int)with_pai.size(); ++i)
			count += with_pai[i].count;
		if (count != size)
			return false;

		int index = 0;
		for(int i = 0; i < (int)with_pai.size(); ++i)
		{
			if (with_pai[i].count > 1)
				alone[index++] = with_pai[i].val;
			if (with_pai[i].count > 0)
				alone[index++] = with_pai[i].val;
		}
		p_move._airplane_pairs = false;
		p_move._alone_1 = alone[0];p_move._alone_2 = alone[1];p_move._alone_3 = alone[2];p_move._alone_4 = alone[3];
	}

	p_move._type = ddz_type_airplane_with_pai;
	p_move._combo_count = 0;
	for(int i = 0; i < size; ++i)
	{
		p_move._combo_list[p_move._combo_count++] = airplane[i];
		p_move._combo_list[p_move._combo_count++] = airplane[i];
		p_move._combo_list[p_move._combo_count++] = airplane[i];
	}

	return true;
}

static inline bool is_order_four_with_pairs(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 8)
		return false;

	struct st_help_triple_1
	{
		unsigned char val;
		unsigned char count;
	};

	st_help_triple_1 help_triple[3];
	memset(help_triple, 0, sizeof(st_help_triple_1)*3);

	for(int i = 0; i < 8; ++i)
	{
		if (help_triple[0].val == 0 || help_triple[0].val == pai_list[i])
		{
			help_triple[0].val = pai_list[i];
			help_triple[0].count++;
		}
		else if (help_triple[1].val == 0 || help_triple[1].val == pai_list[i])
		{
			help_triple[1].val = pai_list[i];
			help_triple[1].count++;
		}
		else if (help_triple[2].val == 0 || help_triple[2].val == pai_list[i])
		{
			help_triple[2].val = pai_list[i];
			help_triple[2].count++;
		}
		else
		{
			return false;
		}
	}

	unsigned char alone1, alone2, alone3 = 0;
	bool have_4_2 = false;
	if (help_triple[0].count == 4 )
	{
		alone1 = help_triple[0].val;
		if (help_triple[1].count ==2 && help_triple[2].count == 2)
		{
			have_4_2 = true;
			alone2 = help_triple[1].val;
			alone3 = help_triple[2].val;
		}
	}
	else if (help_triple[1].count == 4)
	{
		alone1 = help_triple[1].val;
		if (help_triple[0].count == 2 && help_triple[2].count == 2)
		{
			have_4_2 = true;
			alone2 = help_triple[0].val;
			alone3 = help_triple[2].val;
		}
	}
	else if (help_triple[2].count == 4)
	{
		alone1 = help_triple[2].val;
		if (help_triple[0].count == 2 && help_triple[1].count == 2)
		{
			have_4_2 = true;
			alone2 = help_triple[0].val;
			alone3 = help_triple[1].val;
		}
	}
	if (have_4_2)
	{
		p_move._type = ddz_type_four_with_pairs;
		p_move._alone_1 = alone1;
		p_move._alone_2 = alone2;
		p_move._alone_3 = alone3;

		return true;
	}
	return false;


}

static inline bool is_order_four_with_alone1(const std::vector<unsigned char>& pai_list, ddz_move& p_move)
{
	if (pai_list.size() != 6)
		return false;

	struct st_help_triple_1
	{
		unsigned char val;
		unsigned char count;
	};

	st_help_triple_1 help_triple[3];
	memset(help_triple, 0, sizeof(st_help_triple_1)*3);

	for(int i = 0; i < 6; ++i)
	{
		if (help_triple[0].val == 0 || help_triple[0].val == pai_list[i])
		{
			help_triple[0].val = pai_list[i];
			help_triple[0].count++;
		}
		else if (help_triple[1].val == 0 || help_triple[1].val == pai_list[i])
		{
			help_triple[1].val = pai_list[i];
			help_triple[1].count++;
		}
		else if (help_triple[2].val == 0 || help_triple[2].val == pai_list[i])
		{
			help_triple[2].val = pai_list[i];
			help_triple[2].count++;
		}
		else
		{
			return false;
		}
	}

	unsigned char alone1, alone2, alone3 = 0;
	bool have_4_1 = false;
	if (help_triple[0].count == 4 )
	{
		alone1 = help_triple[0].val;
		if (help_triple[1].count == 1 && help_triple[2].count == 1)
		{
			have_4_1 = true;
			alone2 = help_triple[1].val;
			alone3 = help_triple[2].val;
		}
		else if (help_triple[1].count == 0 && help_triple[2].count == 2)
		{
			have_4_1 = true;
			alone2 = help_triple[2].val;
			alone3 = help_triple[2].val;
		}
		else if (help_triple[2].count == 0 && help_triple[1].count == 2)
		{
			have_4_1 = true;
			alone2 = help_triple[1].val;
			alone3 = help_triple[1].val;
		}
	}
	else if (help_triple[1].count == 4)
	{
		alone1 = help_triple[1].val;
		if (help_triple[0].count == 1 && help_triple[2].count == 1)
		{
			have_4_1 = true;
			alone2 = help_triple[0].val;
			alone3 = help_triple[2].val;
		}
		else if (help_triple[0].count == 0 && help_triple[2].count == 2)
		{
			have_4_1 = true;
			alone2 = help_triple[2].val;
			alone3 = help_triple[2].val;
		}
		else if (help_triple[2].count == 0 && help_triple[0].count == 2)
		{
			have_4_1 = true;
			alone2 = help_triple[0].val;
			alone3 = help_triple[0].val;
		}
	}
	else if (help_triple[2].count == 4)
	{
		alone1 = help_triple[2].val;
		if (help_triple[0].count == 1 && help_triple[1].count == 1)
		{
			have_4_1 = true;
			alone2 = help_triple[0].val;
			alone3 = help_triple[1].val;
		}
		else if (help_triple[0].count == 0 && help_triple[1].count == 2)
		{
			have_4_1 = true;
			alone2 = help_triple[1].val;
			alone3 = help_triple[1].val;
		}
		else if (help_triple[1].count == 0 && help_triple[0].count == 2)
		{
			have_4_1 = true;
			alone2 = help_triple[0].val;
			alone3 = help_triple[0].val;
		}
	}
	if (have_4_1)
	{
		p_move._type = ddz_type_four_with_alone_1;
		p_move._alone_1 = alone1;
		p_move._alone_2 = alone2;
		p_move._alone_3 = alone3;

		return true;
	}
	return false;
}

bool ddz_state::is_greater_than(ddz_move* m1, ddz_move* m2)
{
	if (m2->_type == ddz_type_king_bomb)
		return false;

	if (m1->_type == ddz_type_king_bomb)
		return true;

	if (m2->_type == ddz_type_bomb && m1->_type != ddz_type_bomb)
		return false;

	if (m1->_type == ddz_type_bomb && m2->_type != ddz_type_bomb)
		return true;

	if (m1->_type != m2->_type)
		return false;

	bool greater = false;
	switch (m1->_type)
	{
	case ddz_type_alone_1:
	case ddz_type_pair:
	case ddz_type_triple:
	case ddz_type_bomb:
	case ddz_type_triple_1:
	case ddz_type_triple_2:
	case ddz_type_four_with_alone_1:
	case ddz_type_four_with_pairs:
		if (m1->_alone_1 > m2->_alone_1)
			greater = true;
		break;
	case ddz_type_order:
	case ddz_type_order_pair:
	case ddz_type_airplane:
		if (m1->_combo_count == m2->_combo_count)
		{
			qsort(m2->_combo_list, m2->_combo_count, sizeof(unsigned char), move_cmp);
			qsort(m1->_combo_list, m1->_combo_count, sizeof(unsigned char), move_cmp);
			if (m1->_combo_list[0] > m2->_combo_list[0])
				greater = true;
		}
		break;
	case ddz_type_airplane_with_pai:
		if (m1->_combo_count == m2->_combo_count && m1->_airplane_pairs == m2->_airplane_pairs)
		{
			qsort(m2->_combo_list, m2->_combo_count, sizeof(unsigned char), move_cmp);
			qsort(m1->_combo_list, m1->_combo_count, sizeof(unsigned char), move_cmp);
			if (m1->_combo_list[0] > m2->_combo_list[0])
				greater = true;
		}
		break;
	}

	return greater;
}

bool ddz_state::is_less(ddz_move* m1, ddz_move* m2)
{
	if (m1->_type == ddz_type_king_bomb)
		return false;

	if (m2->_type == ddz_type_king_bomb)
		return true;

	if (m1->_type == ddz_type_bomb && m2->_type != ddz_type_bomb)
		return false;

	if (m1->_type != m2->_type)
		return false;

	bool less = true;

	switch (m1->_type)
	{
	case ddz_type_alone_1:
	case ddz_type_pair:
	case ddz_type_triple:
	case ddz_type_bomb:
	case ddz_type_triple_1:
	case ddz_type_triple_2:
	case ddz_type_four_with_alone_1:
	case ddz_type_four_with_pairs:
		if (m1->_alone_1 > m2->_alone_1)
			less = false;
		break;
	case ddz_type_order:
	case ddz_type_order_pair:
	case ddz_type_airplane:
		if (m1->_combo_count == m2->_combo_count)
		{
			qsort(m2->_combo_list, m2->_combo_count, sizeof(unsigned char), move_cmp);
			qsort(m1->_combo_list, m1->_combo_count, sizeof(unsigned char), move_cmp);
			if (m1->_combo_list[0] > m2->_combo_list[0])
				less = false;
		}
		else
		{
			less = false;
		}
		break;
	case ddz_type_airplane_with_pai:
		if (m1->_combo_count == m2->_combo_count && m1->_airplane_pairs == m2->_airplane_pairs)
		{
			qsort(m2->_combo_list, m2->_combo_count, sizeof(unsigned char), move_cmp);
			qsort(m1->_combo_list, m1->_combo_count, sizeof(unsigned char), move_cmp);
			if (m1->_combo_list[0] > m2->_combo_list[0])
				less = false;
		}
		else
		{
			less = false;
		}
		break;
	}

	return less;
}


bool ddz_state::get_player_enter(std::string str, ddz_move* p_move)
{
	transform(str.begin(), str.end(), str.begin(), ::toupper);

	if (str.length() == 1 && str[0] == 'N')
	{
		return true;
	}

	std::vector<unsigned char> pai_list;

	int size = (int)str.length();
	for(int i = 0; i < size; )
	{
		if (str[i] >= '3' && str[i] <= '9')
		{
			pai_list.push_back(str[i] - '3' + 3);
			++i;
		}
		else if (str[i] == '1')
		{
			pai_list.push_back(pai_type_10);
			i += 2;
		}
		else if (str[i] == 'J')
		{
			pai_list.push_back(pai_type_J);
			++i;
		}
		else if (str[i] == 'Q')
		{
			pai_list.push_back(pai_type_Q);
			++i;
		}
		else if (str[i] == 'K')
		{
			pai_list.push_back(pai_type_K);
			++i;
		}
		else if (str[i] == 'A')
		{
			pai_list.push_back(pai_type_A);
			++i;
		}
		else if (str[i] == '2')
		{
			pai_list.push_back(pai_type_2);
			++i;
		}
		else if (str[i] == 'S')
		{
			pai_list.push_back(pai_type_blackjack);
			++i;
		}
		else if (str[i] == 'T')
		{
			pai_list.push_back(pai_type_blossom);
			++i;
		}
		else
		{
			++i;
		}
	}

	if (pai_list.empty())
		return false;

	for(int i = 0; i < (int)pai_list.size(); ++i)
	{
		if ( pai_list[i] < pai_type_3 || pai_list[i] > pai_type_blossom )
			return false;
//		if (_player_list[1].pai_list[pai_list[i]] <= 0)
//			return false;
	}

	ddz_move move;

	// 检查出牌的合法性
	bool valid_pai = is_alone_1(pai_list, move);
	if (!valid_pai)
		valid_pai = is_pairs(pai_list, move);
	if (!valid_pai)
		valid_pai = is_triple(pai_list, move);
	if (!valid_pai)
		valid_pai = is_triple_1(pai_list, move);
	if (!valid_pai)
		valid_pai = is_triple_2(pai_list, move);
	if (!valid_pai)
		valid_pai = is_order(pai_list, move);
	if (!valid_pai)
		valid_pai = is_order_pairs(pai_list, move);
	if (!valid_pai)
		valid_pai = is_order_airplane(pai_list, move);
	if (!valid_pai)
		valid_pai = is_order_airplane_with_pai(pai_list, move);
	if (!valid_pai)
		valid_pai = is_bomb(pai_list, move);
	if (!valid_pai)
		valid_pai = is_king_bomb(pai_list, move);
	if (!valid_pai)
		valid_pai = is_order_four_with_alone1(pai_list, move);
	if (!valid_pai)
		valid_pai = is_order_four_with_pairs(pai_list, move);
	if (!valid_pai)
		return false;

	if (_curr_move && is_less(&move, _curr_move))
		return false;

	p_move->_type = move._type;
	p_move->_alone_1 = move._alone_1;p_move->_alone_2 = move._alone_2;p_move->_alone_3 = move._alone_3;p_move->_alone_4 = move._alone_4;
	p_move->_airplane_pairs = move._airplane_pairs;
	p_move->_combo_count = move._combo_count;
	memcpy(p_move->_combo_list, move._combo_list, 20*sizeof(unsigned char));
	return true;
}
