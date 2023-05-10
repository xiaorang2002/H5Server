#ifndef __MCTS_NODE_H__
#define __MCTS_NODE_H__

#include "mcts_state.h"
#include "mcts_move.h"

#include <vector>

class random_engine;

class compute_options
{
public:
	compute_options() :number_of_threads(1), max_iterations(10000), max_time(-1.0), verbose(false) {}

	int			number_of_threads;
	int			max_iterations;
	double		max_time;
	bool		verbose;
};

class mcts_node
{
public:
	mcts_node(mcts_state* state);
	~mcts_node();
	
	bool has_untried_moves();
	mcts_move* get_untried_move(random_engine* engine);
	mcts_node* best_child();
	mcts_node* min_visit_child();
	bool has_children() const { return !_children.empty(); }

	mcts_node* select_child_UCT();
	mcts_node* add_child(const mcts_move* move, mcts_state* state, int player_to_move);
	void update(double result);

	mcts_node* const			_parent;
	const mcts_move*			_move;
	const int					_player_to_move;
	
	double						_wins;
	int							_visits;

	std::vector<mcts_move*>		_moves;
	std::vector<mcts_node*>		_children;

private:
	mcts_node(mcts_state* state, const mcts_move* move, int player_to_move, mcts_node* parent);

	double						_UTC_score;
};

mcts_move* compute_move(mcts_state* root_state, const compute_options options = compute_options() );

#endif