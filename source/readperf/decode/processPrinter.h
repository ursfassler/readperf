/**
 * @file
 * Routines to print content of @link processes.h @endlink
 */

#ifndef _PROCESSPRINTER_H
#define	_PROCESSPRINTER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include    <stdio.h>
#include    "processes.h"
    
    void print_process_init( FILE* fid );
    void print_process( struct process *proc );
    void print_processes( process_tree_t *tree );
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* _PROCESSPRINTER_H */

