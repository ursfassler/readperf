/*
 * File:   buildstat.h
 * Author: urs
 *
 * Created on August 24, 2011, 10:04 AM
 */

#ifndef _BUILDSTAT_H
#define	_BUILDSTAT_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include    "processes.h"
#include    "../perffile/records.h"
    
    bool buildstat( record_order_tree_t *tree );
    
#ifdef	__cplusplus
}
#endif

#endif	/* _BUILDSTAT_H */

