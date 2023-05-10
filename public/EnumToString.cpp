

#include "./EnumToString.h"

#ifndef UENUM_STRING
#define UENUM_STRING

template <>
char const *enumStrings<UE>::data[] = {
        "sign_get_count",
        "sign_get_lasttimestamp",
        "end"
        };

#endif
