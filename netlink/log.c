#include "log.h"
#include "debug.h"
#include "config.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

static void log_syslog(const int32 level,
                       const int8 *prog,
                       int8 *fmt,
                       va_list va)
{
    const int32 level_syslog[] = {
        [L_CRIT]    = LOG_CRIT,
        [L_ERR]     = LOG_ERR,
        [L_WARNING] = LOG_WARNING,
        [L_NOTICE]  = LOG_NOTICE,
        [L_INFO]    = LOG_INFO,
        [L_DEBUG]   = LOG_DEBUG
    };
    openlog(prog, 0, LOG_DAEMON);
    vsyslog(level_syslog[level], fmt, va);
    closelog();
}

static void log_stdout(const int32 level,
                       const int8 *prog,
                       int8 *fmt,
                       va_list va)
{
    int8 buf[1024];
    int8 *p = NULL;
    int32 len = 0;
    time_t t;
    struct tm *tm = NULL;
    int8 *levelstr[] = {[L_CRIT]    = "CRIT",
                        [L_ERR]     = "ERR",
                        [L_WARNING] = "WARNING",
                        [L_NOTICE]  = "NOTICE",
                        [L_INFO]    = "INFO",
                        [L_DEBUG]   = "DEBUG"};
    bzero(buf, sizeof(buf));
    t = time(NULL);
    tm = localtime(&t);
    snprintf(buf, sizeof(buf), "%d-%02d-%02d %02d:%02d:%02d [%s] [%s]: ",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, 
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        prog, levelstr[level]);
    len = strlen(buf);
    p = buf + len;
    vsnprintf(p, sizeof(buf)-len, fmt, va);
    printf("%s\r\n", buf);
}


#define LOGFILE_PATH    BASE_DIR"/log"
#define LOGFILE_MAXSIZE (10*1024*1024) /*10MB*/
#define LOGFILE_MAXNUM  (10)
static FILE *logfile_open(void)
{
    FILE *pf = NULL;
    int8 filename[256];
    int32 i = 0;
    int32 size = 0;
    if (0 != access(LOGFILE_PATH, F_OK))
        mkdir(LOGFILE_PATH, 0666);
    for (i=0; i<LOGFILE_MAXNUM; ++i)
    {
        bzero(filename, sizeof(filename));
        snprintf(filename, sizeof(filename), "%s/cwmplog%02d.log", LOGFILE_PATH, i+1);
        if (0 == access(filename, W_OK))
        {
            pf = fopen(filename, "ab");
            ASSERT(NULL!=pf);
            fseek(pf, 0, SEEK_END);
            size = ftell(pf);
            if (size < LOGFILE_MAXSIZE)
                break;
            fclose(pf);
            pf = NULL;
        }
        else
        {
            pf = fopen(filename, "w");
            ASSERT(NULL!=pf);
            break;
        }
    }
    if (NULL == pf)
    {
        struct stat st;
        uint64 time_min = 0xffffffffffffffff;
        int32 j = -1;
        for (i=0; i<LOGFILE_MAXNUM; ++i)
        {
            bzero(filename, sizeof(filename));
            snprintf(filename, sizeof(filename), "%s/cwmplog%02d.log", LOGFILE_PATH, i+1);
            stat(filename, &st);
            if (st.st_mtime < time_min)
            {
                time_min = st.st_mtime;
                j = i;
            }
        }
        bzero(filename, sizeof(filename));
        snprintf(filename, sizeof(filename), "%s/cwmplog%02d.log", LOGFILE_PATH, j+1);
        pf = fopen(filename, "wb"); /*Truncate file to zero length or create text file for writing.*/
        ASSERT(NULL!=pf);
    }
    return pf;
}

static void log_file(const int32 level,
                     const int8 *prog,
                     int8 *fmt,
                     va_list va)
{
    int8 buf[1024];
    int8 *p = NULL;
    int32 len = 0;
    time_t t;
    struct tm *tm = NULL;
    FILE *pf = NULL;
    int8 *levelstr[] = {[L_CRIT]    = "CRIT",
                        [L_ERR]     = "ERR",
                        [L_WARNING] = "WARNING",
                        [L_NOTICE]  = "NOTICE",
                        [L_INFO]    = "INFO",
                        [L_DEBUG]   = "DEBUG"};
    bzero(buf, sizeof(buf));
    t = time(NULL);
    tm = localtime(&t);
    snprintf(buf, sizeof(buf), "%d-%02d-%02d %02d:%02d:%02d [%s] [%s]: ",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, 
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        prog, levelstr[level]);
    len = strlen(buf);
    p = buf + len;
    vsnprintf(p, sizeof(buf)-len, fmt, va);
    len = strlen(buf);
    p = buf + len;
    snprintf(p, sizeof(buf)-len, "\r\n");
    pf = logfile_open();
    fwrite(buf, 1, strlen(buf), pf);
    fclose(pf);
}

void logging(const int32 level,
             int8 *fmt,...)
{
    va_list va;
    config_t *cfg = NULL;
    
    if (!LOG_LEVEL_VALID(level))
    {
        DB_ERR("invalid log level(%d)!!", level);
        goto out;
    }
    cfg = config_get();
    if (NULL == cfg)
    {
        DB_ERR("config_get() called failed!!");
        goto out;
    }
    if (level > cfg->local.log_level)
    {
        DB_WAR("this log level(%d) > cfg->local.log_level(%d), not record it!!", 
            level, cfg->local.log_level);
        goto out;
    }
    va_start(va, fmt);
    if (LOGMODE_SYSLOG & cfg->local.log_mode)
        log_syslog(level, "cwmp", fmt, va);
    if (LOGMODE_FILE & cfg->local.log_mode)
        log_file(level, "cwmp", fmt, va);
    if (LOGMODE_STDOUT & cfg->local.log_mode)
        log_stdout(level, "cwmp", fmt, va);
    va_end(va);
    
out:
    if (NULL != cfg)
        config_put(cfg);
    return ;
}
