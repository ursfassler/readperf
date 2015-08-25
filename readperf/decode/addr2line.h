/*
 * @file
 * Function to translate an address of an binary file to the corresponding
 * source file name and source function name.
 *
 * Copyright 2011 Urs Fässler, www.bitzgi.ch
 * SPDX-License-Identifier:	GPL-3.0+
 */

#ifndef _ADDR2LINE_H
#define	_ADDR2LINE_H

#ifdef	__cplusplus
extern "C" {
#endif
#include    "../util/types.h"
    
    bool get_func( u64 addr, char *bin_name, char **source_file, char **source_func );
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* _ADDR2LINE_H */

