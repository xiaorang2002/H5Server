#ifndef __I_SEND_TABLE_DATA_H__
#define __I_SEND_TABLE_DATA_H__

#ifndef INVALID_CHAIR
#define INVALID_CHAIR	(-1)
#endif//INVALID_CHAIR

#include <types.h>

class ISendTableData
{
public:
	virtual bool send_table_data(WORD me_chair_id, WORD chair_id, WORD sub_cmdid, void* data, WORD data_size, bool lookon, WORD exclude_chair_id = INVALID_CHAIR) = 0;
	virtual WORD get_table_id() = 0;
};
#endif//__I_SEND_TABLE_DATA_H__
