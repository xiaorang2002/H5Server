#ifndef __MCTS_MOVE_H__
#define __MCTS_MOVE_H__

#include <string>

class mcts_move
{
public:
	virtual ~mcts_move() {};

	virtual bool			compare(const mcts_move* move)			= 0;
	virtual mcts_move*		clone()									= 0;
	virtual std::string		get_test_value()						= 0;
};

#endif