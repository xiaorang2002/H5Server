
#include "mcts_node.h"
#include "random_engine.h"
#include <iostream>
#include <iomanip>
#include <map>
#include "ddz_move.h"
//#include "windows.h"
#include "math.h"

#include "ddz_move.h"

/*-------------------------------------------------------------------------------------------------------------------*/
int g_new_node_count = 0;
int g_delete_node_count = 0;

mcts_node::mcts_node(mcts_state* state)
:_move(0)
,_parent(0)
,_wins(0)
,_visits(0)
,_UTC_score(0)
,_player_to_move(0)
{
	if (state)
	{
		_moves = state->get_moves();
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_node::mcts_node(mcts_state* state, const mcts_move* move, int player_to_move, mcts_node* parent)
:_move(move)
,_parent(parent)
,_wins(0)
,_visits(0)
,_UTC_score(0)
,_player_to_move(player_to_move)
{
	if (state)
	{
		_moves = state->get_moves();
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_node::~mcts_node()
{
	g_delete_node_count++;

	std::vector<mcts_node*>::iterator iter = _children.begin();
	for( ; iter != _children.end(); ++iter )
	{
		delete *iter;
	}
	_children.clear();

	std::vector<mcts_move*>::iterator iterMove = _moves.begin();
	for( ; iterMove != _moves.end(); ++iterMove )
	{
		delete *iterMove;
	}
	_moves.clear();

	if (_move)
	{
		delete _move;
		_move = 0;
	}
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_move* mcts_node::get_untried_move(random_engine* engine)
{
	if (_moves.empty() || !engine)
		return 0;

	return _moves[engine->get_random() % _moves.size()];
}

/*-------------------------------------------------------------------------------------------------------------------*/
bool mcts_node::has_untried_moves()
{
	return !_moves.empty();
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_node* mcts_node::best_child()
{
	if (_moves.empty() || _children.empty())
		return 0;

	int max_visit = -1;
	mcts_node* bestChild = 0;
	std::vector<mcts_node*>::iterator iter = _children.begin();
	for( ; iter != _children.end(); ++iter )
	{
		mcts_node* child = *iter;
		if ( child->_visits > max_visit )
		{
			bestChild = child;
			max_visit = bestChild->_visits;
		}
	}
	
	return bestChild;
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_node* mcts_node::min_visit_child()
{
	if (_children.empty())
		return 0;

	int max_visit = 999999999;
	mcts_node* bestChild = 0;
	std::vector<mcts_node*>::iterator iter = _children.begin();
	for( ; iter != _children.end(); ++iter )
	{
		mcts_node* child = *iter;
		if ( child->_visits < max_visit )
		{
			bestChild = child;
			max_visit = bestChild->_visits;
		}
	}

	return bestChild;
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_node* mcts_node::select_child_UCT()
{
	if (_children.empty())
		return 0;

	std::vector<mcts_node*>::iterator iter = _children.begin();
	for( ; iter != _children.end(); ++iter )
	{
		mcts_node* child = *iter;
		child->_UTC_score = static_cast<double>(child->_wins) / static_cast<double>(child->_visits) + 0.9*sqrt( log(static_cast<double>(this->_visits)) / child->_visits);
	}

	double max_UTC = -1.0;
	mcts_node* UTC_Child = 0;
	iter = _children.begin();
	for( ; iter != _children.end(); ++iter )
	{
		mcts_node* child = *iter;
		if (child->_UTC_score > max_UTC)
		{
			UTC_Child = child;
			max_UTC = UTC_Child->_UTC_score;
		}
	}

	return UTC_Child;
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_node* mcts_node::add_child(const mcts_move* move, mcts_state* state, int player_to_move)
{
	mcts_node* node = new mcts_node(state, move, player_to_move, this);
	_children.push_back(node);

	std::vector<mcts_move*>::iterator iter = _moves.begin();
	for( ; iter != _moves.end(); ++iter )
	{
		mcts_move* m = *iter;
		if (m->compare(move))
		{
			_moves.erase(iter);
			break;
		}
	}

	return node;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void mcts_node::update(double result)
{
	_visits++;
	_wins += result;
}

/*-------------------------------------------------------------------------------------------------------------------*/
static mcts_node* compute_tree(mcts_state* root_state, const compute_options options)
{
	random_engine randomEngine;
	mcts_node* root = new mcts_node(root_state);

	mcts_state* state = root_state->clone();

#ifdef _WIN32
//	unsigned int begin_time = timeGetTime();
//	unsigned int start_time = begin_time;
#else
//	struct timeval begin_tv;
//	gettimeofday(&begin_tv,NULL);
#endif

	for(int i = 1; i <= options.max_iterations; ++i)
	{
		mcts_node* node = root;
		state->cloneFrom(root_state);
		while (!node->has_untried_moves() && node->has_children())
		{
			if (node == root)
				node = node->min_visit_child();
			else
				node = node->select_child_UCT();
			state->do_move(node->_move);
		}

		if (node->has_untried_moves())
		{
			mcts_move* move = node->get_untried_move(&randomEngine);
			int player_to_move = state->get_player_to_move();
			state->do_move(move);
			node = node->add_child(move, state, player_to_move);
		}

		while (state->has_moves())
		{
			state->do_random_move(&randomEngine);
		}

		while (node != 0)
		{
			node->update(state->get_result(node->_player_to_move));
			node = node->_parent;
		}

	//	if ((timeGetTime() - start_time) > 5500)
	//		break;
	}

#ifdef _WIN32
//	unsigned int end_time = timeGetTime();
#else
//	struct timeval end_tv;
//	gettimeofday(&end_tv,NULL);
#endif


/*	extern int g_new_move_count;
	extern int g_delete_move_count;
	std::cout << "tree total time ->" << end_time - begin_time << std::endl;
	extern unsigned int g_parse_pai_time;
	std::cout << "parse total time ->" << g_parse_pai_time << std::endl;
	g_parse_pai_time = 0;*/

	delete state;

	return root;
}

/*-------------------------------------------------------------------------------------------------------------------*/
mcts_move* compute_move(mcts_state* root_state, const compute_options options )
{
	if (!root_state)
		return 0;

	std::vector<mcts_move*> moves = root_state->get_moves();
	if (moves.empty())
		return 0;
	if (moves.size() == 1)
		return moves[0];

	for(int i = 0; i < (int)moves.size(); ++i)
	{
		delete moves[i];
	}

	mcts_move* best_move = 0;
	mcts_node* root = compute_tree(root_state, options);
	int games_played = 0;
	std::map<const mcts_move*, int> mapVisits;
	std::map<const mcts_move*, double> mapWins;
	if (root)
	{
		games_played = root->_visits;
		std::vector<mcts_node*>::iterator iterChild = root->_children.begin();
		for( ; iterChild != root->_children.end(); ++iterChild )
		{
			mapVisits[(*iterChild)->_move] += (*iterChild)->_visits;
			mapWins[(*iterChild)->_move] += (*iterChild)->_wins;
		}
	}

	double best_score = -1;
	std::map<const mcts_move*, int>::iterator iterVisit = mapVisits.begin();
	for( ; iterVisit != mapVisits.end(); ++iterVisit )
	{
		const mcts_move* move = iterVisit->first;
		double v = iterVisit->second;
		double w = mapWins[move];

		double expected_success_rate = (w + 1) / (v + 2);
		if (expected_success_rate > best_score)
		{
			best_move = const_cast<mcts_move*>(move);
			best_score = expected_success_rate;
		}

		if (options.verbose)
		{
			std::cerr << "Move: " << const_cast<mcts_move*>(iterVisit->first)->get_test_value()
				<< " (" << std::setw(2) << std::right << int(100.0 * v / double(games_played) + 0.5) << "% visits)"
				<< " (" << std::setw(2) << std::right << int(100.0 * w / v + 0.5)    << "% wins)" << std::endl;
		}
	}

	if (options.verbose) {
		double best_wins = mapWins[best_move];
		double best_visits = mapVisits[best_move];
		std::cerr << "----" << std::endl;
		std::cerr << "Best: " << best_move->get_test_value()
			<< " (" << 100.0 * best_visits / double(games_played) << "% visits)"
			<< " (" << 100.0 * best_wins / best_visits << "% wins)" << std::endl;
	}

	best_move = best_move->clone();
	delete root;

	return best_move;
}
