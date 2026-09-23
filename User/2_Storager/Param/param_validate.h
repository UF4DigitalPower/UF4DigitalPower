#ifndef PARAM_VALIDATE_H
#define PARAM_VALIDATE_H

#include "param_types.h"

ParamStatus_t ParamValidate_Descriptor(const ParamDescriptor_t *descriptor);
ParamStatus_t ParamValidate_Value(const ParamDescriptor_t *descriptor,
                                  const void *value, size_t size,
                                  bool check_write_access);
ParamStatus_t ParamValidate_ValueBasic(const ParamDescriptor_t *descriptor,
                                       const void *value, size_t size,
                                       bool check_write_access);
ParamStatus_t ParamValidate_MakeDefault(const ParamDescriptor_t *descriptor,
                                        void *value, size_t size);
size_t ParamType_Size(ParamType_t type);

#endif
