#include "kegmanager.h"

Keg kegs[MAX_KEGS];

void kegManagerBegin()
{
    kegs[0].setName("Fat 1");
    kegs[1].setName("Fat 2");
}