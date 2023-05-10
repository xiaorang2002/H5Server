#ifndef __MCTS_STATE_H__
#define __MCTS_STATE_H__

#include "mcts_move.h"
#include <vector>

class random_engine;

class mcts_state
{
public:
	virtual ~mcts_state() {}
	virtual std::vector<mcts_move*>		get_moves()	= 0;
	virtual int							get_player_to_move() const = 0;

	virtual bool						has_moves() = 0;
	virtual void						do_random_move(random_engine* randomEngine) = 0;
	virtual void						do_move(const mcts_move* move) = 0;

	virtual double						get_result(int player_id) = 0;

	virtual mcts_state*					clone() = 0;
	virtual void						cloneFrom(const mcts_state* state) = 0;
};


#endif
