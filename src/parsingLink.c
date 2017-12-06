#include "parsingLink.h"

void newEntry(char* name, char* url, Entry* e) {
	e = (Entry*)calloc(1, sizeof(Entry));
	e->name = name;
	e->url = url;
	e->next_entry = NULL;
}

void newList(EntryList *L) {
	L->head = NULL;
	L->tail = NULL;
}

void insertList(EntryList *L, Entry* e) {
	if (L->tail) {	// list not empty
		L->tail->next_entry = e;
		L->tail = e;
		L->tail->next_entry = NULL;
	}
	else {
		L->head = e;
		L->tail = e;
		L->tail->next_entry = NULL;
	}
}

EntryList parsingLink(char* fileName) {
}
