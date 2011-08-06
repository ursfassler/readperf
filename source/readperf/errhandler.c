#include    "errhandler.h"
#include    <string.h>
#include    <stdio.h>

static int last_error = ERR_NONE;
static const char* last_info = NULL;

static const char *ERROR_TEXT[ERR_COUNT] = {
    "no error",
    "decode sample",
    "file ended to early",
    "size mismatch",
    "samples differ in type",
    "trace info not found for config",
    "entry not found",
    "no perf file",
    "execution of external program failed",
    "process already exists",
    "process not found",
    "EXIT-hack does not work"
};

/**
 * Prints the last error the stderr.
 */
void print_last_error(){
    char buffer[256];
    const char *errmsg;
    
    if( last_error < 0 ){
        errmsg = strerror( last_error );
        if( errno != 0 ){
            snprintf( buffer, sizeof(buffer), "unknown system error #%i", last_error );
            errmsg = buffer;
        }
    } else {
        errmsg = ERROR_TEXT[last_error];
    }
    
    if( last_info != NULL ){
        fprintf( stderr, "error: %s (%s)\n", errmsg, last_info );
    } else {
        fprintf( stderr, "error: %s\n", errmsg );
    }
}

/**
 * Sets the last error with some information.
 * @param error if <= 0 then it is interpreted as system error, otherwise as
 *        element of @link errhandler_t @endlink.
 * @param info additional information or NULL
 */
void set_last_error( int error, const char* info ){
    last_error  = error;
    last_info   = info;
}
