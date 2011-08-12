/*
 * File:   funcstat.h
 * Author: urs
 *
 * Created on August 2, 2011, 9:44 AM
 */

#ifndef _FUNCSTAT_H
#define	_FUNCSTAT_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    #include    <stdio.h>
    #include    "../util/types.h"
    
    
    struct func_dir {
        struct func_dir *next;
        u64     addr;
        char    *bin_name;
        char    *source_file;
        char    *source_func;
        
        u64     period;
        u64     samples;
    };
    
    struct func_dir* func_error();
    void func_print_overview( FILE* fid );
    void func_print_detailed( FILE* fid );
    struct func_dir* force_entry( u64 addr, const char *bin_name );
    
#ifdef	__cplusplus
}
#endif

#endif	/* _FUNCSTAT_H */

