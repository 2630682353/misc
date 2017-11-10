#ifndef __CWMP_UTILS_H__
#define __CWMP_UTILS_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include "type.h"
#include "error.h"

BOOL utils_dir_exist(const int8 *dirname);
int32 utils_mkdir(const int8 *pathname);

#ifdef  __cplusplus
}
#endif

#endif /*__CWMP_UTILS_H__*/


