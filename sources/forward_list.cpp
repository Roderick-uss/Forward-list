#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "commoner.h"
#include "forward_list.h"

#define ASSERT_LIST(list) assert(list_assert(list));

static int list_dump(const flist_t* list, int error);

static const int MIN_LIST_CAPACITY = 16;
static const int MAX_LIST_CAPACITY = 128;

static const int MAX_TERMINAL_REQUEST_SIZE = 50;

static const char* LOG_FILE_NAME = "list_logs.html";
static const char* dump_dot_file = "graph/list_dump.dot";
static const char* dump_svg_file = "logs/list_dump.svg";
static FILE* const STDLOG = fopen(LOG_FILE_NAME, "wb");

enum errors {
    PRINT_LIST   = 1<<0,
    ZERO_LIST    = 1<<2,
    ZERO_DATA    = 1<<3,
    ZERO_PREV    = 1<<4,
    ZERO_NEXT    = 1<<5,
    // BIG_SIZE     = 1<<6,
    BIG_CAPACITY = 1<<7,
    BROKEN_FREE  = 1<<8,
};

flist_t* list_ctor() {
    flist_t* list = 0;
    int error = 1;
    if (!(list = (flist_t*)calloc(1, sizeof(flist_t)))) return 0;
    list->capacity = MIN_LIST_CAPACITY;

    if (!error && !(list->data = (int*)calloc(list->capacity, sizeof(int)))) error |= ZERO_DATA;
    if (!error && !(list->prev = (int*)calloc(list->capacity, sizeof(int)))) error |= ZERO_PREV;
    if (!error && !(list->next = (int*)calloc(list->capacity, sizeof(int)))) error |= ZERO_NEXT;

    if (error) {
        if (!(ZERO_DATA & error)) free(list->data);
        if (!(ZERO_PREV & error)) free(list->prev);
        if (!(ZERO_NEXT & error)) free(list->next);
        free(list);

        return 0;
    }

    for(size_t i = 1; i < list->capacity; ++i) {
        list->next[i] = i + 1;
        list->prev[i] = -1;
    }
    list->next[list->capacity - 1] = 0;
    list->free = 1;

    return list;
}
int list_dtor(flist_t* list) {
    ASSERT_LIST(list);

    free(list->data);
    free(list->next);
    free(list->prev);
    *list = {};
    free(list);

    return 0;
}

int list_verify(const flist_t* list) {
    int error = 0;
    if (!list) return ZERO_LIST;

    if (!list->data) error |= ZERO_DATA;
    if (!list->prev) error |= ZERO_PREV;
    if (!list->next) error != ZERO_NEXT;

    // if (list->size     > list->capacity   ) return BIG_SIZE;
    if (list->capacity > MAX_LIST_CAPACITY) return BIG_CAPACITY;

    return error;
}
int list_print (const flist_t* list) {
    assert(list_dump(list, list_verify(list) | PRINT_LIST));
    return 0;
}
int list_assert(const flist_t* list) {
    return !list_dump(list, list_verify(list));
}

int list_full (const flist_t* list) {
    assert(list);
    return !list->free;
}

int list_empty(const flist_t* list) {
    assert(list);
    return !list->next[0];
}

int list_insert(flist_t* list, int item, int index) {
    ASSERT_LIST(list);

    if (list_full(list)) {
        LOG_FATAL("LIST IS FULL\nNO INSERTION PERFORMED\n");
        return 0;
    }
    int prev = 0, next = 0, current = 0;

    if (!(prev = list_find(list, index))) return 0;
    if (!(current = list->free))          return 0;
    next = list->next[prev];

    assert(current);

    list->free = list->next[list->free];
    list->data[current] = item;

    list->next[current] = next;
    list->prev[current] = prev;

    list->next[prev] = current;
    list->prev[next] = current;

    return current;
}

int list_erase(flist_t* list, int index) {
    ASSERT_LIST(list);

    if (list_empty(list)) {
        LOG_FATAL("LIST IS EMPTY\nNO DELETION PERFORMED\n");
        return 0;
    }

    int current = list_find(list, index);
    if (current == -1) return 0;
    int next = list->next[current];
    int prev = list->prev[current];

    assert(current);

    list->prev[next] = prev;
    list->next[prev] = next;

    list->next[current] = list->free;
    list->prev[current] = -1;

    list->free = current;

    return current;
}

int list_find(const flist_t* list, int index) {
    ASSERT_LIST(list);

    int i = 0;
    int cnt = 0;

    while (cnt != index && list->next != 0) {
        i = list->next[i];
        cnt++;
    }

    if (cnt != index) return -1;
    return i;
}

static int list_dump(const flist_t* list, int error) {
    if (!error) return 0;

    if (ZERO_LIST & error) {
        LOG_FATAL("NO POINTER TO LIST\n");
        return error;
    }

    //todo dump

    char term_request[MAX_TERMINAL_REQUEST_SIZE] = {};
    sprintf(term_request, "dot %s -Tpng -o %s", dump_dot_file, dump_svg_file);
    system(term_request);

    return error & ~PRINT_LIST;
}

int close_list_logs() {
    fclose(STDLOG);
}
