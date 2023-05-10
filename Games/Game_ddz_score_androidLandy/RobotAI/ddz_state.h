#ifndef __DDZ_STATE_H__
#define __DDZ_STATE_H__

#include "memory.h"
#include "mcts_state.h"
#include "ddz_move.h"
#include <vector>
#include "random_engine.h"
#include <algorithm>

#include "stgy_initiative_play_card.h"
#include "stgy_passive_play_card.h"
#include "stgy_split_card.h"

class ddz_state : public mcts_state
{
public:
	enum { MAX_PLAYER = 3 };
	ddz_state();
	ddz_state(ddz_move* limit_move, int out_pai_index);
	virtual ~ddz_state();

	virtual std::vector<mcts_move*>		get_moves();
	virtual int							get_player_to_move() const;

	virtual bool						has_moves();
	virtual void						do_random_move(random_engine* randomEngine);
	virtual void						do_move(const mcts_move* move);

	virtual double						get_result(int player_id);

	virtual mcts_state*					clone();
	virtual void						cloneFrom(const mcts_state* state);

	void								setPlayerList(int* list, int c);
	void								setLandlord(int playerId);
	void								setPaiList(int playerId, const unsigned char* paiList);

	// 拆牌
	std::vector<mcts_move*>				parse_pai(int* pai_list, std::vector<mcts_move*>& out_pai_list, ddz_move* limit_move = 0);

	std::string							get_pai_list(int uid);
	const char* 						get_pai_value(int u);

	int									get_uid(int index) { return _player_list[index].uid; }
	bool								get_win(int index) { return _player_list[index].is_win; }

	bool								get_player_enter(std::string str, ddz_move* p_move);

	inline ddz_move*					get_curr_move() { return _curr_move; }
	inline void							clear_curr_move() { delete _curr_move; _curr_move = 0; }

	inline int							get_curr_player_uid() {return _player_list[_curr_player_index].uid;}
	inline int							get_out_player_uid() {return _player_list[_curr_out_player_index].uid;}

	bool								is_less(ddz_move* m1, ddz_move* m2);
	bool								is_greater_than(ddz_move* m1, ddz_move* m2);

private:
	friend class stgy_initiative_play_card;
	friend class stgy_passive_play_card;
	friend class stgy_split_card;

private:

	void								go_next_player();
	bool								is_game_end();
	void								do_real_move(ddz_move* p_move);
	void								check_win();
	void								sub_valid_move(ddz_move* p_move);
	void								sub_valid_move_out_pai(ddz_move* p_move);

	void								generate_out_pai_move(int* pai_list, std::vector<mcts_move*>& out_move_list);

	struct ddz_player
	{
		ddz_player ():uid(0), is_landlord(false), is_win(false) { memset(pai_list, 0, sizeof(int)*pai_type_max); }
		int						uid;
		bool					is_landlord;
		bool					is_win;
		int						pai_list[pai_type_max];
	};

	std::vector<mcts_move*>		_out_pai_move_list[3];

	ddz_player							_player_list[MAX_PLAYER];
	int									_curr_player_index;
	int									_curr_out_player_index;
	ddz_move*							_curr_move;
	random_engine						_random_engine;

	stgy_passive_play_card				_stgy_passive_play_card;
	stgy_initiative_play_card			_stgy_initiative_play_card;
	stgy_split_card						_stgy_split_card;
};


#endif
