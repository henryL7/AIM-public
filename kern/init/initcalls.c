#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <aim/console.h>
#include <aim/panic.h>
#include <aim/initcalls.h>

extern initcall_t initearly_start[],initearly_end[],init_start[],init_end[];

int do_early_initcalls()
{
    initcall_t* start=initearly_start;
    for(;start<initearly_end;start++)
        if((*start)()!=0)
            return -1;
    return 0;
}

int do_initcalls()
{
    initcall_t* start=init_start;
    for(;start<init_end;start++)
        if((*start)()!=0)
            return -1;
    return 0;
}


