#include "param_table.h"

#include <string.h>

static const ParamDescriptor_t *s_table;
static size_t s_count;

ParamStatus_t ParamTable_Bind(const ParamDescriptor_t *table, size_t count)
{
    size_t i;
    size_t j;
    if ((table == NULL) || (count == 0u)) return PARAM_ERROR_ARGUMENT;

    for (i = 0u; i < count; ++i)
    {
        if ((table[i].runtime == NULL) || (table[i].size == 0u) ||
            (table[i].name == NULL)) return PARAM_ERROR_ARGUMENT;
        for (j = i + 1u; j < count; ++j)
        {
            if ((table[i].id == table[j].id) ||
                (strcmp(table[i].name, table[j].name) == 0))
                return PARAM_ERROR_ARGUMENT;
        }
    }
    s_table = table;
    s_count = count;
    return PARAM_OK;
}

const ParamDescriptor_t *ParamTable_Find(ParamID_t id)
{
    for (size_t i = 0u; i < s_count; ++i)
        if (s_table[i].id == id) return &s_table[i];
    return NULL;
}

const ParamDescriptor_t *ParamTable_FindByName(const char *name)
{
    if (name == NULL) return NULL;
    for (size_t i = 0u; i < s_count; ++i)
        if (strcmp(s_table[i].name, name) == 0) return &s_table[i];
    return NULL;
}

const ParamDescriptor_t *ParamTable_At(size_t index)
{
    return (index < s_count) ? &s_table[index] : NULL;
}

size_t ParamTable_Count(void) { return s_count; }
