#ifndef __SAVEREPLAYREC_HEADER__
#define __SAVEREPLAYREC_HEADER__

#include "gameDefine.h"

// the special id of record replay save.
#define guid_ISaveReplay    "5796bbec-14d8-11e9-842d-001c421f700a"
// the special interface.
struct ISaveReplayRecord
{
public:
    virtual bool SaveReplayRecord(tagGameRecPlayback& rec) = 0;
};

#endif//__SAVEREPLAYREC_HEADER__
