#ifndef PARSING_LINK_INCLUDED
#define PARSING_LINK_INCLUDED

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct Entry {
	char* name;
	char* url;
	struct Entry* next_entry;
} Entry;

void newEntry(char* name, char* url, Entry* e);

typedef struct {
	Entry* head;
	Entry* tail;
} EntryList;

void newList(EntryList *L);
void insertList(EntryList *L, Entry* e);

EntryList parsingLink(char* fileName);

#endif
