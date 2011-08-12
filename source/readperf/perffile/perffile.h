/*
 * File:   perffile.h
 * Author: urs
 *
 * Created on August 12, 2011, 3:11 PM
 */

#ifndef _PERFFILE_H
#define	_PERFFILE_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include    "records.h"
    
    extern record_order_tree_t orderTree;
    extern int count[PERF_RECORD_MAX];
    
    bool read_perf_file( int fd );
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* _PERFFILE_H */

