#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commoner.h"
#include "colors.h"
#include "forward_list.h"

#define ASSERT_LIST(list) assert(list_assert(list));

static const int MIN_LIST_CAPACITY = 16;
static const int MAX_LIST_CAPACITY = 128;

static const int MAX_TERMINAL_REQUEST_SIZE = 100;

static const char* const LOG_HTML_FILE = "logs/list_logs.html";
static const char* const DUMP_DOT_FILE = "logs/list_dump.dot";
static const char* const DUMP_SVG_FILE = "list_dump.svg";
// static FILE* const stdlog = fopen(LOG_HTML_FILE, "wb");

static int list_dump      (const flist_t* list, const int error);
static int graph_dump_run (const flist_t* list, const int error);

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
    int error = 0;
    if (!(list = (flist_t*)calloc(1, sizeof(flist_t)))) {LOG_FATAL("cant calloc memory\n"); return 0;}
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
        list->next[i] = (int)i + 1;
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
    if (!list->next) error |= ZERO_NEXT;

    // if (list->size     > list->capacity   ) return BIG_SIZE;
    if (list->capacity > MAX_LIST_CAPACITY) return BIG_CAPACITY;

    return error;
}
int list_print (const flist_t* list) {
    assert(!list_dump(list, list_verify(list) | PRINT_LIST));
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

int list_find(const flist_t* list, int index) {
    ASSERT_LIST(list);

    int i = 0;
    int cnt = 0;

    while (cnt != index && list->next != 0) {
        i = list->next[i];
        cnt++;
    }
    LOG_INFO("to find: %d; finded: %d\n", index, i);

    if (cnt != index) return -1;
    return i;
}

int list_clear (flist_t* list) {
    ASSERT_LIST(list);

    *list->next = *list->prev = 0;
    for(size_t i = 1; i < list->capacity; ++i) {
        list->next[i] = (int)i + 1;
        list->prev[i] = -1;
    }
    list->next[list->capacity - 1] = 0;
    list->free  = 1;

    return 0;
}

int list_insert(flist_t* list, int item, int index) {
    ASSERT_LIST(list);

    if (list_full(list)) {
        LOG_FATAL("LIST IS FULL\nNO INSERTION PERFORMED\n");
        return 0;
    }
    int prev = 0, next = 0, current = 0;

    prev = list_find(list, index);
    if (!(current = (int)list->free))                      return 0;
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

    list->next[current] = (int)list->free;
    list->prev[current] = -1;

    list->free = current;

    return current;
}

static int list_dump(const flist_t* list, const int error) {
    if (!error) return 0;
    FILE* stdlog = fopen(LOG_HTML_FILE, "a");
    fprintf(stdlog, "<p style=\"color:#7FFF00;\">\n");

    if (ZERO_LIST & error) {
        fprintf(stdlog, "<span style=\"color:red;\">");
        fprintf(stdlog, "NO POINTER TO LIST<br></span>\n");
        fprintf(stdlog, "</p>\n");
        LOG_FATAL("NO POINTER TO LIST\n");
        return error & ~PRINT_LIST;
    }
    fprintf(stdlog, "POINTER TO LIST: "
                    "<span style=\"color:cyan;\" >%p<br></span>\n", list);
    LOG_GREEN("list:");
    LOG_CYAN ("%p\n", list);

    if (ZERO_DATA & error) {
        fprintf(stdlog, "<span style=\"color:red;\">");
        fprintf(stdlog, "NO POINTER TO DATA<br></span>\n");
        LOG_FATAL("NO POINTER TO DATA\n");
    }
    else {
        fprintf(stdlog, "data[" "<span style=\"color:cyan;\" >%p</span>" "]: " , list->data);
        LOG_GREEN("data[" CYAN "%p" GREEN "]: ", list->data);

        if (!(BIG_CAPACITY & error)) {
            fprintf(stdlog, "<span style=\"color:magenta;\">");
            fprintf(stderr, PURPLE);
            for (size_t i = 0; i < list->capacity; ++i) {
                fprintf(stdlog, "%2.2d ", list->data[i]);
                fprintf(stderr, "%2.2d ", list->data[i]);
            }
            fprintf(stdlog, "<br></span>\n");
            fprintf(stderr, "\n" DEFAULT);
        }
    }

    if (ZERO_NEXT & error) {
        fprintf(stdlog, "<span style=\"color:red;\">");
        fprintf(stdlog, "NO POINTER TO NEXT<br></span>\n");
        LOG_FATAL("NO POINTER TO NEXT\n");
    }
    else {
        fprintf(stdlog, "next[" "<span style=\"color:cyan;\" >%p</span>" "]: ", list->next);
        LOG_GREEN("next[" CYAN "%p" GREEN "]: ", list->next);

        if (!(BIG_CAPACITY & error)) {
            fprintf(stdlog, "<span style=\"color:magenta;\">");
            fprintf(stderr, PURPLE);
            for (size_t i = 0; i < list->capacity; ++i) {
                fprintf(stdlog, "%2.2d ", list->next[i]);
                fprintf(stderr, "%2.2d ", list->next[i]);
            }
            fprintf(stdlog, "</span><br></span>\n");
            fprintf(stderr, "\n" DEFAULT);
        }
    }

    if (ZERO_PREV & error) {
        fprintf(stdlog, "<p style=\"color:red;\">");
        fprintf(stdlog, "NO POINTER TO PREV<br></p>\n");
        LOG_FATAL("NO POINTER TO PREV\n");
    }
    else {
        fprintf(stdlog, "prev[" "<span style=\"color:cyan;\" >%p</span>" "]: " , list->prev);
        LOG_GREEN("prev[" CYAN "%p" GREEN "]: ", list->prev);

        if (!(BIG_CAPACITY & error)) {
            fprintf(stdlog, "<span style=\"color:magenta;\">");
            fprintf(stderr, PURPLE);
            for (size_t i = 0; i < list->capacity; ++i) {
                fprintf(stderr, list->prev[i] != -1 ? "%2.2d " : "%1.1d ", list->prev[i]);
                fprintf(stdlog, list->prev[i] != -1 ? "%2.2d " : "%1.1d ", list->prev[i]);
            }
            fprintf(stderr, "\n" DEFAULT);
            fprintf(stdlog, "<br></span>\n");
        }
    }

    if (BIG_CAPACITY & error) {
        fprintf(stdlog, "<span style=\"color:red;\">");
        fprintf(stdlog, "TOO BIG CAPACITY<br>capacity = </span>");
        fprintf(stdlog, "<span style=\"color:yellow;\">%lu<br></span>\n", list->capacity);
        fprintf(stdlog, "</p>\n");
        LOG_FATAL("TOO BIG CAPACITY\n capacity = " YELLOW "%lu\n", list->capacity);
        return error & ~PRINT_LIST;
    }
    else {
        fprintf(stdlog, "capacity = " "<span style=\"color:yellow;\" >%lu<br></span>\n", list->capacity);
        LOG_GREEN("capacity = " YELLOW "%lu\n", list->capacity);
    }

    if (BROKEN_FREE & error) {
        fprintf(stdlog, "<span style=\"color:red;\">");
        fprintf(stdlog, "BROKEN FREE<br>free = </span>");
        fprintf(stdlog, "<span style=\"color:yellow;\">%lu<br></span>\n", list->free);
        LOG_FATAL("BROKEN FREE\n free = " YELLOW "%lu\n", list->free);
    }
    else {
        fprintf(stdlog, "free = " "<span style=\"color:yellow;\">%lu<br></span>\n", list->free);
        LOG_GREEN("free = " YELLOW "%lu\n", list->free);
    }
    if (error & ~PRINT_LIST) {
        fprintf(stdlog, "</p>\n");
        return error & ~PRINT_LIST;
    }

    if (graph_dump_run (list, error)) LOG_FATAL("ERROR WHILE DOT GRAPH DUMP\n");
    else {
        char term_request[MAX_TERMINAL_REQUEST_SIZE] = {};
        sprintf(term_request, "dot %s -Tsvg -o logs/%s", DUMP_DOT_FILE, DUMP_SVG_FILE);
        system(term_request);
        sleep(1);
    }


    fprintf(stdlog, "<img src=\"%s\" alt=\"Ordinary list dump\">\n\n", DUMP_SVG_FILE);

    fprintf(stdlog, "</p>\n\n");
    fprintf(stderr, "\n");

    fclose(stdlog);
    return error & ~PRINT_LIST;
}

int open_list_logs() {
    #ifndef INIT_LIST_LOGS
        #define INIT_LIST_LOGS
        FILE* stdlog = fopen(LOG_HTML_FILE, "w");
        fprintf(stdlog, "<!DOCTYPE html>\n<html>\n<body style=\"background-color:black;\">");
        fclose(stdlog);
        return 0;
    #else
        LOG_FATAL("LOGS ALREADY OPENED\n");

        return 1;
    #endif
}
int close_list_logs() {
    #ifndef INIT_LIST_LOGS
    LOG_FATAL("LOGS DID NOT OPENED");

    return 1;
    #else
        #ifndef CLOSE_LIST_LOGS
            #define CLOSE_LIST_LOGS
            FILE* stdlog = fopen(LOG_HTML_FILE, "a");
            fprintf(stdlog, "</body>\n</html>\n");

            fclose(stdlog);
            return 0;
        #else
            LOG_FATAL("LOGS ALREADY CLOSED\n");

            return 1;
        #endif
    #endif
}

static int graph_dump_run (const flist_t* list, const int error) {
    if (error & ~PRINT_LIST) return 1;

    FILE* stddot  = fopen(DUMP_DOT_FILE, "w");

    const char start[] =
        "\
digraph structs {\n\
    bgcolor=\"black\"\n\
    nodesep=0;\n\
    node [fontname=\"Impact\"; fillcolor=\"#800000\"; shape=\"box\"; penwidth=\"3\";fontsize=\"24\"; fontcolor=\"#00FFAA\"]\n\
    free_name [style=\"invis\"]\n\
    data_name [shape=plaintext; label=\"DATA:\"]\n\
    next_name [shape=plaintext; label=\"NEXT:\"]\n\
    prev_name [shape=plaintext; label=\"PREV:\"]\n\
    node[fontcolor=\"#A2FF05\"]\n\0\
        ";
    const char mid[] =
        "\
    edge[minlen=2;style=\"invis\"]\n\
    data_name->free_name->next_name->prev_name\n\
    data->free:f->next->prev\n\
    edge[color=\"#1F51FF\";arrowhead=\"empty\";constraint=false;penwidth=2.5; style =\"\"; minlen=\"\"];\n\0\
        ";
    const char end[] = "}\n\0";
    const char free_init[][71] = {"\tfree [label=\"<f>FREE|<i>", "\";shape=Mrecord;color=\"#F52789\";fontcolor=\"#A2FF05\";style=\"filled\";]\n\0"};
    // else                     const char free_init[] = "\tfree [label=\"<f>FREE|<i>$\";shape=Mrecord;color=\"#F52789\";fontcolor=\"#179000\";]\n\0";
    const char data_init[][111] =
        {
"\
    data [label=<\n\
        <TABLE BORDER=\"0\" CELLBORDER=\"3\" CELLSPACING=\"0\" COLOR=\"#FF5E00\">\n\
            <TR>\n\0\
",
"\
            </TR>\n\
        </TABLE>\n\
    >];\n\0\
"
        };
    const char data_elem_full[][79] =
        {
"\t\t\t\t<TD PORT=\"i\0",
"\" WIDTH=\"40\" HEIGHT=\"40\"><FONT COLOR=\"#00FEFC\" FACE=\"Impact\" POINT-SIZE=\"25\">\0",
"</FONT></TD>\n\0",
        };
    const char data_elem_empty[][81]  =
        {
"\t\t\t\t<TD PORT=\"i\0",
"\" WIDTH=\"40\"><FONT COLOR=\"#FF0000\" FACE=\"\"       POINT-SIZE=\"25\">X</FONT></TD>\n\0"
        };
    // else const char data_init[][92] = {"\tdata [label=\"NULL POINTER TO DATA\"; color=\"#FF5E00\"; style=\"filled\"]\0"};
    const char next_init[][128] =
        {
"\
    next [shape=plaintext; label=<\n\
        <TABLE BORDER=\"0\" CELLBORDER=\"3\" CELLSPACING=\"0\" COLOR=\"#DD0EFF\">\n\
            <TR>\n\0\
",
"\
            </TR>\n\
        </TABLE>\n\
    >];\n\0\
"
        };

    const char next_elem_full[][79]  =
        {
"\t\t\t\t<TD PORT=\"i\0",
"\" WIDTH=\"40\" HEIGHT=\"40\"><FONT COLOR=\"#E7EE4F\" FACE=\"Impact\" POINT-SIZE=\"25\">\0",
"</FONT></TD>\n\0",
        };
    const char next_elem_empty[][79]  =
        {
"\t\t\t\t<TD PORT=\"i\0",
"\" WIDTH=\"40\" HEIGHT=\"40\"><FONT COLOR=\"#7FFF00\" FACE=\"Impact\" POINT-SIZE=\"25\">\0",
"</FONT></TD>\n\0",
        };
    // else const char next_init[][92] = {"\tnext [label=\"NULL POINTER TO NEXT\"; color=\"#DD0EFF\"; style=\"filled\"]\0"};
    const char prev_init[][128] =
        {
"\
    prev [shape=plaintext; label=<\n\
        <TABLE BORDER=\"0\" CELLBORDER=\"3\" CELLSPACING=\"0\" COLOR=\"#7D12FF\">\n\
            <TR>\n\0\
",
"\
            </TR>\n\
        </TABLE>\n\
    >];\n\0\
"
        };
    const char prev_elem_full[][79]  =
        {
"\t\t\t\t<TD PORT=\"i\0",
"\" WIDTH=\"40\" HEIGHT=\"40\"><FONT COLOR=\"#E7EE4F\" FACE=\"Impact\" POINT-SIZE=\"25\">\0",
"</FONT></TD>\n\0",
        };
    const char prev_elem_empty[][79]  =
        {
"\t\t\t\t<TD PORT=\"i\0",
"\" WIDTH=\"40\" HEIGHT=\"40\"><FONT COLOR=\"#737373\" FACE=\"Impact\" POINT-SIZE=\"25\">\0",
"</FONT></TD>\n\0",
        };
    // else const char prev_init[][92] = {"\tprev [label=\"NULL POINTER TO PREV\"; color=\"#7D12FF\"; style=\"filled\"]\0"};
    const char next_links[][24] =
        {
            "\tnext:i\0", ":n->next:i\0", ":n[color=\"#1F51FF\"]\n\0",
            "\tnext:i\0", ":s->next:i\0", ":s[color=\"#FF3131\"]\n\0",
        };
    const char prev_links[][24] =
        {
            "\tprev:i\0", ":n->prev:i\0", ":n[color=\"#1F51FF\"]\n\0",
            "\tprev:i\0", ":s->prev:i\0", ":s[color=\"#FF3131\"]\n\0",
        };
    char free_links[][25] =
        {
            "\tedge[color=\"#FFFFAA\"]\n\0",
            "\tfree:i:n->data:i\0",";\n\0",
            "\tfree:i:s->next:i\0",";\n\0",
        };


    fprintf(stddot, "%s", start);
    // LOG_YELLOW("%s", start);
    { //! INIT
        fprintf(stddot, "%s%lu%s", &free_init[0][0], list->free, &free_init[1][0]);

        fprintf(stddot, "%s", &data_init[0][0]);
        for (size_t i = 0; i < list->capacity; ++i) {
            if (list->prev[i] != -1) fprintf(stddot, "%s%lu%s%d%s", &data_elem_full [0][0], i, &data_elem_full [1][0], list->data[i], &data_elem_full[2][0]);
            else                     fprintf(stddot, "%s%lu%s"    , &data_elem_empty[0][0], i, &data_elem_empty[1][0]);
        }
        fprintf(stddot, "%s", &data_init[1][0]);

        fprintf(stddot, "%s", &next_init[0][0]);
        for (size_t i = 0; i < list->capacity; ++i) {
            if (list->prev[i] != -1) fprintf(stddot, "%s%lu%s%d%s", &next_elem_full [0][0], i, &next_elem_full [1][0], list->next[i], &next_elem_full [2][0]);
            else                     fprintf(stddot, "%s%lu%s%d%s", &next_elem_empty[0][0], i, &next_elem_empty[1][0], list->next[i], &next_elem_empty[2][0]);
        }
        fprintf(stddot, "%s", &next_init[1][0]);

        fprintf(stddot, "%s", &prev_init[0][0]);
        for (size_t i = 0; i < list->capacity; ++i) {
            if (list->prev[i] != -1) fprintf(stddot, "%s%lu%s%d%s", &prev_elem_full [0][0], i, &prev_elem_full [1][0], list->prev[i], &prev_elem_full [2][0]);
            else                     fprintf(stddot, "%s%lu%s%d%s", &prev_elem_empty[0][0], i, &prev_elem_empty[1][0], list->prev[i], &prev_elem_empty[2][0]);
        }
        fprintf(stddot, "%s", &prev_init[1][0]);

    } //! END INIT
    fprintf(stddot, "%s", mid);
    // LOG_YELLOW("%s",mid);
    { //! LINKS
        fprintf(stddot, "%s", mid);
        for(int i = 0; i < (int)list->capacity; ++i) {
            if (i % 2) {
                fprintf(stddot, "%s%d%s%d%s", &next_links[0][0], i, &next_links[1][0], list->next[i], &next_links[2][0]);
                if (list->prev[i] != -1)
                fprintf(stddot, "%s%d%s%d%s", &prev_links[0][0], i, &prev_links[1][0], list->prev[i], &prev_links[2][0]);
            }
            else {
                fprintf(stddot, "%s%d%s%d%s", &next_links[3][0], i, &next_links[4][0], list->next[i], &next_links[5][0]);
                if (list->prev[i] != -1)
                fprintf(stddot, "%s%d%s%d%s", &prev_links[3][0], i, &prev_links[4][0], list->prev[i], &prev_links[5][0]);
            }
        }
        fprintf(stddot, "%s%s%lu%s%s%lu%s", &free_links[0][0], &free_links[1][0], list->free, &free_links[2][0],
                                                               &free_links[3][0], list->free, &free_links[2][0]);
    } //! END LINKS

    fprintf(stddot, "%s", end);
    // LOG_YELLOW("%s", end);

    fclose(stddot);
    return 0;
}
