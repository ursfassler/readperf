
#include    "addr2line.h"
#include    <unistd.h>
#include    "../util/errhandler.h"
#include    <sys/types.h>
#include    <sys/wait.h>
#include    <stdio.h>
#include    <string.h>

#define try_cmd( val ) {if( !(val) ){ goto _err_handler; }}
#define try_except  _err_handler:


int exec_redirect( const char *path, char *const args[], int new_in, int new_out, int new_err ) {
    
    pid_t child;
    
// Fork new process
    child = fork();
    
    trysys( child >= 0 );
    
    if (child == 0) {
// Child process
// Redirect standard input
        if (new_in >= 0) {
            dup2(new_in, STDIN_FILENO);
        }
        
// Redirect standard output
        if (new_out >= 0) {
            dup2(new_out, STDOUT_FILENO);
        }
        
// Redirect standard error
        if (new_err >= 0) {
            dup2(new_err, STDERR_FILENO);
        }
        
// Execute the command
        execv(path, args);
    } else {
// Parent process
        int status = 2;
        waitpid( child, &status, 0 );
        return status;
    }
    
    return 1;
}

bool get_func( u64 addr, char *bin_name, char **source_file, char **source_func ){
    char *app = "/usr/bin/addr2line";
    char addrstr[128];
    trysys( snprintf( addrstr, sizeof(addrstr), "%llx", addr ) > 0 );
    char *const args[] = { app, "-f", "-e", bin_name, addrstr, NULL };
    
    int pstdin[2];
    int pstdout[2];
    int pstderr[2];
    
    try_cmd( pipe( pstdin ) == 0 );
    try_cmd( pipe( pstdout ) == 0 );
    try_cmd( pipe( pstderr ) == 0 );
    
    if( exec_redirect( app, args, pstdin[0], pstdout[1], pstderr[1] ) == 0 ){
        //   if( exec_redirect( app, args, pstdin[0], pstdout[1], STDERR_FILENO ) == 0 ){
        char tmp[1024];
        FILE *output = fdopen( pstdout[0], "r" );
        try_cmd( output != NULL );
        try_cmd( fscanf( output, "%s\n", tmp ) == 1 );
        *source_func = strdup( tmp );
        try_cmd( source_func != NULL );
        
        try_cmd( fscanf( output, "%s", tmp ) == 1 );
        unsigned int i;
        for( i = 0; i < sizeof(tmp)/sizeof(tmp[0]); i++ ){
            if( tmp[i] == ':' ){
                tmp[i] = '\0';
                break;
            }
        }
        *source_file = strdup( tmp );
        try_cmd( source_file != NULL );
        
        fclose( output );
        
        close( pstdin[0] );
        close( pstdin[1] );
        close( pstdout[1] );
        close( pstdout[0] );
        close( pstderr[1] );
        close( pstderr[0] );
        
        return true;
    }
    
    try_except;
    
    close( pstdin[0] );
    close( pstdin[1] );
    close( pstdout[0] );
    close( pstdout[1] );
    close( pstderr[1] );
    close( pstderr[0] );
    
    set_last_error( ERR_NOT_YET_DEFINED, __func__ );
    
    return false;
}

