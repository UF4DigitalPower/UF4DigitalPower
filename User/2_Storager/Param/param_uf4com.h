#ifndef PARAM_UF4COM_H
#define PARAM_UF4COM_H

#include "param_manager.h"

#ifndef PARAM_UF4COM_MAX_IDS
#define PARAM_UF4COM_MAX_IDS 96u
#endif

/* Bind the current parameter/object table as UF4COM's only ID registry. */
ParamStatus_t ParamUF4COM_Bind(void);

#endif
