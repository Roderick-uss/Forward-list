#ifndef   FORWARD_LIST_H
#define   FORWARD_LIST_H



struct flist_t {
    int* data;
    int* next;
    int* prev;
    size_t capacity;
    // size_t head, tail;
    size_t free;
};

flist_t* list_ctor();
int      list_dtor(flist_t* list);

int list_verify(flist_t* list);
int list_print (flist_t* list);
int list_assert(flist_t* list);

int list_find(const flist_t* list, int index);

int list_empty(const flist_t* list);
int list_full (const flist_t* list);

int  init_list_logs();
int close_list_logs();

#endif // FORWARS_LIST_H
