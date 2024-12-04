#include <stdio.h>

#include "forward_list.h"
#include "commoner.h"

int main() {
    freopen("input.txt", "rt", stdin);

    open_list_logs();
    flist_t* list = list_ctor();
    LOG_BLUE("LIST CREATED\n");
    LOG_BLUE("lst_ptr = %p\n", list);

    for(int i = 0; i < 9; ++i) {
        LOG_BLUE("list_insert(list, %d, %d)\n", (i + 1) * 10, i);
        list_insert(list, (i + 1) * 10, i);
        list_print(list);
    }
    LOG_BLUE("list_erase (list, 5)\n");
    list_erase (list, 5);
    list_print (list);
    LOG_BLUE("list_erase (list, 5)\n");
    list_erase (list, 5);
    list_print (list);
    LOG_BLUE("list_erase (list, 3)\n");
    list_erase (list, 3);
    list_print (list);
    LOG_BLUE("list_insert(list, 77, 4)\n");
    list_insert(list, 77, 4);
    list_print (list);
    LOG_BLUE("list_insert(list, 5, 0)\n");
    list_insert(list, 5, 0);
    list_print (list);
    LOG_BLUE("list_clear(list)\n");
    list_clear(list);
    list_print(list);

    list_dtor(list);
    LOG_CERR("LIST DESTRUCTED");
    close_list_logs();
}
