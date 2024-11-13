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

#endif // FORWARS_LIST_H
