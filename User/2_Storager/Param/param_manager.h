#ifndef PARAM_MANAGER_H
#define PARAM_MANAGER_H

#include "param_table.h"

#ifndef PARAM_SNAPSHOT_CAPACITY
#define PARAM_SNAPSHOT_CAPACITY 1024u
#endif

/*
 * Startup order:
 *   1. Flash_Init_RegisterInternal()/Flash_Init_RegisterW25QXX()
 *   2. Param_Init(table, count)
 *   3. Param_Load()
 * PARAM_ERROR_NOT_FOUND from Param_Load means defaults are active and dirty;
 * Param_Commit() creates the first persistent snapshot.
 */
ParamStatus_t Param_Init(const ParamDescriptor_t *table, size_t count);
ParamStatus_t Param_Load(void);
ParamStatus_t Param_Commit(void);
ParamStatus_t Param_Rollback(void);

ParamStatus_t Param_Get(ParamID_t id, void *value, size_t size);
ParamStatus_t Param_Set(ParamID_t id, const void *value, size_t size);
ParamStatus_t Param_GetByName(const char *name, void *value, size_t size);
ParamStatus_t Param_SetByName(const char *name, const void *value, size_t size);

ParamStatus_t Param_Reset(ParamID_t id);
ParamStatus_t Param_ResetGroup(ParamGroup_t group);
ParamStatus_t Param_ResetAll(void);
bool Param_IsDirty(void);
bool Param_IsStorageDirty(ParamStorageID_t storage);
const ParamDescriptor_t *Param_GetDescriptor(ParamID_t id);

#endif
