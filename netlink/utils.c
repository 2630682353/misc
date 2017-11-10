#include "utils.h"
#include "debug.h"
#include "def.h"
#include <strings.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

BOOL utils_dir_exist(const int8 *dirname)
{
    if (0 != access(dirname, F_OK))
        return FALSE;
    return TRUE;
}

int32 utils_mkdir(const int8 *pathname)
{
    int8 dirname[PATHNAME_SIZE];
    int8 *p = NULL;
    int8 *q = (int8 *)pathname;
    int32 ret = -1;
    bzero(dirname, sizeof(dirname));
    while (NULL != (p = strchr(q, '/')))
    {
        if (p != pathname)
        {
            memcpy(dirname, pathname, p-pathname);
            if (FALSE == utils_dir_exist(dirname))
            {
                ret = mkdir(dirname, 0777);
                if (0 != ret)
                {
                    DB_ERR("mkdir() call failed!!");
                    return -1;
                }
            }
        }
        q = p+1;
    }
    if (NULL!=q && '\0'!=*q)
    {
        if (FALSE == utils_dir_exist(pathname))
        {
            ret = mkdir(pathname, 0777);
            if (0 != ret)
            {
                DB_ERR("mkdir() call failed!!");
                return -1;
            }
        }
    }
    return 0;
}
