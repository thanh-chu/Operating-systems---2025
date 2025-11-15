//LRU (Least Recently Used): Page not used for the longest time → eject.

#include <stdio.h> 
#include <stdlib.h> 

typedef struct {
    unsigned long *pages;  
    int capacity;          
    int size;              
    int *last; 
} LRUCache;

LRUCache* create_cache(int capacity) {
    LRUCache *c = (LRUCache*) malloc(sizeof(LRUCache)); 
    c->pages = (unsigned long*) malloc(sizeof(unsigned long) * capacity); 
    c->capacity = capacity; 
    c->size = 0; 
    c->last = (int*) malloc(sizeof(int) * capacity);
    return c; 
}

void free_cache(LRUCache *c) {
    free(c->pages);
    free(c->last);
    free(c);
}

int contains(LRUCache *c, unsigned long page_number) {
    for (int i = 0; i < c->size; i++) { 
        if (c->pages[i] == page_number) return i;
    } 
    return -1;
}

void update_page(LRUCache *c, unsigned long page_number, int time, long* page_faults) {
    int check_index = contains(c, page_number);
    if(check_index != -1){ //if it exists, update infomation
        c->last[check_index] = time;
    } else {
        (*page_faults)++;
        if(c->size < c->capacity){
            c->pages[c->size] = page_number;
            c->last[c->size] = time;
            c->size++;
        } else {
            int out = 0;
            for(int i = 1; i < c->capacity; i++){
                if(c->last[i] < c->last[out]){
                    out = i;
                }
            }
            c->pages[out] = page_number;
            c->last[out] = time;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: %s no_phys_pages page_size filename\n", argv[0]);
        return 1;
    }

    int no_phys_pages = atoi(argv[1]);
    int page_size = atoi(argv[2]);
    char *filename = argv[3];

    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Cannot open file");
        return 1;
    }

    LRUCache *c = create_cache(no_phys_pages);
    unsigned long address;
    unsigned long page_number;
    long page_faults = 0;
    long total_refs = 0;

    while (fscanf(f, "%lu", &address) != EOF) { 
        page_number = address / page_size;
        update_page(c, page_number, total_refs, &page_faults);
        total_refs++; 
    }

    printf("No physical pages = %d, page size = %d\n", no_phys_pages, page_size);
    printf("Reading memory trace from %s... ", filename);
    printf("Read %ld memory references\n", total_refs);
    printf("Result: %ld page faults\n", page_faults);

    fclose(f); 
    free_cache(c);

    return 0;
}
