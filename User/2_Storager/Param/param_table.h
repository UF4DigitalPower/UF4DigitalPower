#ifndef PARAM_TABLE_H
#define PARAM_TABLE_H

#include "param_types.h"

ParamStatus_t ParamTable_Bind(const ParamDescriptor_t *table, size_t count);
const ParamDescriptor_t *ParamTable_Find(ParamID_t id);
const ParamDescriptor_t *ParamTable_FindByName(const char *name);
const ParamDescriptor_t *ParamTable_At(size_t index);
size_t ParamTable_Count(void);

#endif
