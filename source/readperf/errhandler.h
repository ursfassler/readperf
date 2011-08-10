/**
 * @file
 * Routines and datatypes for error handling.
 */

#ifndef _ERRHANDLER_H
#define	_ERRHANDLER_H

#include    <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * If the parameter passed to this macro is false, then the function is aborted
 * and returns false. No error will be reported.
 * @param val boolean value, if false the function will be aborted
 */
#define try( val )  {if( !(val) ){ return false; }}

/**
 * If the parameter passed to this macro is false, then the function is aborted
 * and returns false. The error in err will be reported.
 * @param val boolean value, if false the function will be aborted
 * @param err enum value of @link errhandler_t @endlink
 * @param string additional information or NULL
 */
#define trymsg( val, err, msg ) {if( !(val) ){ set_last_error( err, msg ); return false; }}
    
/**
 * If the parameter passed to this macro is false, then the function is aborted
 * and returns false. The last system error (errno) will be reported to 
 * @link errhandler @endlink.
 */
#define trysys( val ) {if( !(val) ){ set_last_error( errno, NULL ); return false; }}

    enum errhandler_t{
        ERR_NONE    = 0,
        ERR_NOT_YET_DEFINED,
        ERR_DECODE_SAMPLE,
        ERR_FILE_END_TO_EARLY,
        ERR_SIZE_MISMATCH,
        ERR_SAMPLES_DIFFER_IN_TYPE,
        ERR_TRACE_INFO_NOT_FOUND_FOR_CONFIG,
        ERR_ENTRY_NOT_FOUND,
        ERR_NO_PERF_FILE,
        EER_EXEC_OF_EXTERNAL_PRG_FAILED,
        ERR_PROC_ALREADY_EXISTS,
        ERR_PROC_NOT_FOUND,
        ERR_EXIT_HACK_DOES_NOT_WORK,
        
        ERR_COUNT
    };
    
    void print_last_error();
    void set_last_error( int error, const char* info );
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* _ERRHANDLER_H */

