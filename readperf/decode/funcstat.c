#include    "funcstat.h"

#include    <string.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    "../util/errhandler.h"
#include    "addr2line.h"

static struct func_dir* head = NULL;
static struct func_dir error = { NULL, {0}, 0, NULL, NULL, NULL, 0, 0 };

struct func_dir* func_error(){
    return  &error;
};

void func_print_overview( FILE* fid ){
    u64 sum = error.period;
    
    struct func_dir *itr;
    for( itr = head; itr != NULL; itr = itr->next ){
        sum += itr->period;
    }
    
    fprintf( fid, "   per       period samples function\n" );
    for( itr = head; itr != NULL; itr = itr->next ){
        fprintf( fid, "%6.2f %12llu %7llu %s\n", (itr->period*100.0)/sum, itr->period, itr->samples, itr->source_func );
    }
    fprintf( fid, "%6.2f %12llu %7llu %s\n", (error.period*100.0)/sum, error.period, error.samples, "" );
    
}

/**
 * This function prints a list of function names together with the source file
 * name, sample count and period.
 */
void func_print_detailed( FILE* fid ){
    fprintf( fid, "period,samples,filename,function\n" );
    fprintf( fid, "%llu,%llu,\"%s\",\"%s\"\n", error.period, error.samples, error.source_file, error.source_func );
    struct func_dir *itr;
    for( itr = head; itr != NULL; itr = itr->next ){
        fprintf( fid, "%llu,%llu,\"%s\",\"%s\"\n", itr->period, itr->samples, itr->source_file, itr->source_func );
    }
    
}

static bool has_addr( struct func_dir *fdir, u64 addr ){
    unsigned int idx;
    for( idx = 0; idx < sizeof(fdir->addr)/sizeof(fdir->addr[0]); idx++ ){
        if( fdir->addr[idx] == addr ){
            return true;
        }
    }
    return false;
};

static struct func_dir* find_entry_addr( u64 addr, const char *bin_name ){
    struct func_dir *itr;
    for( itr = head; itr != NULL; itr = itr->next ){
        if( has_addr(itr,addr) && (strcmp( itr->bin_name, bin_name ) == 0) ){
            return itr;
        }
    }
    return  NULL;
};

static struct func_dir* find_entry_name( const char *func_name, const char *file_name ){
    struct func_dir *itr;
    for( itr = head; itr != NULL; itr = itr->next ){
        if( strcmp( itr->source_func, func_name ) == 0 ){
            if(strcmp( itr->source_file, file_name ) == 0){
                return itr;
            }
        }
    }
    return  NULL;
};

static struct func_dir* add_entry(){
    struct func_dir   *old = head;
    head = (struct func_dir*)malloc( sizeof(*head) );
    if( head != NULL ){
        memset( head, 0, sizeof(*head) );
        unsigned int idx;
        for( idx = 0; idx < sizeof(head->addr)/sizeof(head->addr[0]); idx++ ){
            head->addr[idx] = (u64)-1;
        }
        head->next = old;
    }
    return head;
};

/**
 * Returns an entry which identifies a source function together with the source
 * file and additional information like the sample count and period. First, it
 * searches for an entry with this binary name and instruction pointer. If not
 * found, it retrieves the source file name and source function name and 
 * searches for an entry with that. If this also does not leads to an valid 
 * entry, a new one is created.
 */
struct func_dir* force_entry( u64 addr, char *bin_name ){
    struct func_dir *entry = NULL;
    entry = find_entry_addr( addr, bin_name );
    if( entry == NULL ){
        char *file, *func;
        if( !get_func( addr, bin_name, &file, &func ) ){
            return NULL;
        }
        entry = find_entry_name( func, file );
        if( entry != NULL ){
            entry->addr[entry->addhead] = addr;     // overwrite oldest addr
            entry->addhead = (entry->addhead + 1) % (sizeof(entry->addr)/sizeof(entry->addr[0]));
            if( strcmp( entry->source_file, file ) != 0 ){
                return  NULL;
            }
            if( strcmp( entry->source_func, func ) != 0 ){
                return  NULL;
            }
            free( file );
            free( func );
        } else {
            entry = add_entry();
            if( entry != NULL ){
                entry->addr[0] = addr;
                entry->bin_name = strdup( bin_name );
                entry->source_file = file;
                entry->source_func = func;
            }
        }
    }
    return  entry;
};
