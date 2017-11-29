#include "write_log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOGFILE  "video.runlog"
#define LEV_NUM   5
#define MSG_BUF   1024
char *Level[LEV_NUM] = {"DEBUG","INFO","WARNING","ERROR","URGENT"};

void write_log(int lev,const char*func,const char* msg)
{

    char buf[MSG_BUF] = {0};
    if(lev>=LEV_NUM||strlen(msg)>=(MSG_BUF/2))
    {
        sprintf(buf,"%s '[%-s] [%-s] [%-s]' >> %s",
                "date '+%Y-%m-%d %H:%M:%S'|xargs -i echo [{}]","URGENT","*NULL*","please check log LEVEL or MSG length!",LOGFILE);
    }
    else
    {
        sprintf(buf,"%s '[%-s] [%-s] [%-s]' >> %s",
                "date '+%Y-%m-%d %H:%M:%S'|xargs -i echo [{}]",Level[lev],func,msg,LOGFILE);

    }

    system(buf);

}
