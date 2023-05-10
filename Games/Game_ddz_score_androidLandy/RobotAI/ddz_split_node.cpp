
#include "ddz_split_node.h"
#include "stgy_split_card.h"
#include <memory.h>

/*-------------------------------------------------------------------------------------------------------------------*/
static ddz_split_node g_ddz_split_node_pool[16][1024];
static int g_split_node_pool_index[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void init_split_pool(int pool_index)
{
	if (pool_index < 0 || pool_index > 15)
		return;

	g_split_node_pool_index[pool_index] = 0;
}

ddz_split_node* get_new_split_node(int pool_index)
{
	if (pool_index < 0 || pool_index > 15)
		return 0;

	int index = g_split_node_pool_index[pool_index]++;

	ddz_split_node* node =  &g_ddz_split_node_pool[pool_index][index];
	node->clean();
	
	return node;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_split_node::clean()
{
	memset((void*)&_childs[0], 0, sizeof(ddz_split_node*)*20);
	memset((void*)&_pai_list[0], 0, sizeof(int)*pai_type_max);

	_curr_split_type = ddz_value_none;
	_curr_split_list_count = 0;
	_curr_min_split_node = 0;
	_curr_min_split_value = 99999;
	_parent = 0;
}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_split_node::split_node(int* pai_list)
{

}

/*-------------------------------------------------------------------------------------------------------------------*/
void ddz_split_node::split_child_node(int* pai_list, int split_value, int* split_list, int count)
{

}
