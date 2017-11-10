#ifndef __CWMP_LOG_H__
#define __CWMP_LOG_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include "type.h"

typedef enum {
	L_CRIT      = 0,
    L_ERR       = 1,
	L_WARNING   = 2,
	L_NOTICE    = 3,
	L_INFO      = 4,
	L_DEBUG     = 5
}LOG_LEVEL;
#define LOG_LEVEL_VALID(level)  ((level)>=L_CRIT && (level)<=L_DEBUG)

typedef enum logmode_en{
    LOGMODE_SYSLOG  = 0x01,
    LOGMODE_STDOUT  = 0x02,
    LOGMODE_FILE    = 0x04,
    LOGMODE_ALL     = 0x07
}logmode_e;

void logging(const int32 level,
             int8 *fmt,...);
#define LOGGING_CRIT(fmt,ARGS...)       logging(L_CRIT,fmt,##ARGS)
#define LOGGING_ERR(fmt,ARGS...)        logging(L_ERR,fmt,##ARGS)
#define LOGGING_WARNING(fmt,ARGS...)    logging(L_WARNING,fmt,##ARGS)
#define LOGGING_NOTICE(fmt,ARGS...)     logging(L_NOTICE,fmt,##ARGS)
#define LOGGING_INFO(fmt,ARGS...)       logging(L_INFO,fmt,##ARGS)
#define LOGGING_DEBUG(fmt,ARGS...)      logging(L_DEBUG,fmt,##ARGS)

#ifdef  __cplusplus
}
#endif

#endif /*__CWMP_LOG_H__*/
