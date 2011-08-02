#include    "memorymap.h"
#include    <stdlib.h>
#include    <string.h>
#include    <stdio.h>

static struct mmap_entry *head = NULL;
static struct mmap_entry error = { NULL, 0, 0, 0, 0, 0, 0, "!error!" };

/**
 * Returns an entry where the address is in it. If there is no entry, a special
 * eror entry is returned.
 */
struct mmap_entry* mm_get_entry( u32 pid, u32 tid, u64 addr ){
    (void)tid;
    struct mmap_entry *itr;
    for( itr = head; itr != NULL; itr = itr->next ){
//        if( (pid == itr->pid) && (tid == itr->tid) ){
        if( pid == itr->pid ){
            if( (addr >= itr->start) && (addr <= itr->end) ){
                return itr;
            }
        }
    }
    return  &error;
}

/**
 * Adds an new entry at the beginning of the list. Therefore, the most actual
 * entry will be in the front (important if (not sure if this is the case)
 * memory space is recycled).
 */
struct mmap_entry* mm_add_entry(){
    struct mmap_entry   *old = head;
    head = (struct mmap_entry*)malloc( sizeof(*head) );
    if( head != NULL ){
        memset( head, 0, sizeof(*head) );
        head->next = old;
    }
    return head;
}

struct mmap_entry* mm_get_error(){
    return &error;
}

void mm_print( FILE *fid, bool include_zero_entries ){
    fprintf( fid, "period,samples,filename\n" );
    fprintf( fid, "%llu,%llu,\"%s\"\n", error.period, error.samples, error.filename );
    struct mmap_entry *itr;
    for( itr = head; itr != NULL; itr = itr->next ){
        if( (itr->period > 0) || include_zero_entries ){
            fprintf( fid, "%llu,%llu,\"%s\"\n", itr->period, itr->samples, itr->filename );
        }
    }
}
