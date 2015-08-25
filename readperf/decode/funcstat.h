/*
 * @file
 * Stores source file name and function as well as the corresponding number of
 * samples and period assigned to this function.
 *
 * Copyright 2011 Urs Fässler, www.bitzgi.ch
 * SPDX-License-Identifier:	GPL-3.0+
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
        u64             addr[8];        // since samples often contains the same address, we have to call addr2line less if we already have the information
        unsigned char   addhead;
        char            *bin_name;
        char            *source_file;
        char            *source_func;
        
        u64             period;
        u64             samples;
    };
    
    struct func_dir* func_error();
    void func_print_overview( FILE* fid );
    void func_print_detailed( FILE* fid );
    struct func_dir* force_entry( u64 addr, char *bin_name );
    
#ifdef	__cplusplus
}
#endif

#endif	/* _FUNCSTAT_H */

