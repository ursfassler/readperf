/*
 * File:   errhandler.h
 * Author: urs
 *
 * Created on July 27, 2011, 11:20 AM
 */

#ifndef _ERRHANDLER_H
#define	_ERRHANDLER_H

#include    <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif
    
#define try( func )  if( !(func) ){ return false; }
#define trysys( val ) if( !(val) ){ set_last_error( errno, NULL ); return false; }
    
    enum{
        ERR_NONE    = 0,
        ERR_DECODE_SAMPLE,
        ERR_FILE_END_TO_EARLY,
        ERR_SIZE_MISMATCH,
        ERR_SAMPLES_DIFFER_IN_TYPE,
        ERR_TRACE_INFO_NOT_FOUND_FOR_CONFIG,
        ERR_ENTRY_NOT_FOUND,
        
        ERR_COUNT
    };
    
    void print_last_error();
    void set_last_error( int error, const char* info );
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* _ERRHANDLER_H */

