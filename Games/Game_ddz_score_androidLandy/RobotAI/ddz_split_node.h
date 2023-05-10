#ifndef __DDZ_SPLIT_NODE_H__
#define __DDZ_SPLIT_NODE_H__

#include "ddz_move.h"

class ddz_split_node
{
public:
	ddz_split_node() {}
	~ddz_split_node() {}

	void clean();
	void split_node(int* pai_list);
	void split_child_node(int* pai_list, int split_value, int* split_list, int count);

private:
	ddz_split_node*		_parent;
	ddz_split_node*		_childs[20];
	int					_pai_list[pai_type_max];

	int					_curr_split_type;
	int					_curr_split_list[20];
	int					_curr_split_list_count;

	ddz_split_node*		_curr_min_split_node;
	int					_curr_min_split_value;
};

ddz_split_node* get_new_split_node(int pool_index);
void init_split_pool(int pool_index);

#endif