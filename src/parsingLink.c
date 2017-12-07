#include "parsingLink.h"

void newEntry(char* name, char* url, Entry** e) {
	(*e) = (Entry*)calloc(1, sizeof(Entry));
	(*e)->name = name;
	(*e)->url = url;
	(*e)->next_entry = NULL;
}

void newList(EntryList *L) {
	L->head = NULL;
	L->tail = NULL;
}

void insertList(EntryList *L, char* name, char* url) {
	Entry* e;
	newEntry(name, url, &e);

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

void clearList(EntryList* L) {
	Entry* temp;
	for (; L->head; ) {
		temp = L->head;
		L->head = L->head->next_entry;
		free(temp);
	}
}

int insertChar(char c, char* s, int len) {
	if (len%256 == 0) 
		s = (char*)realloc(s, (len+257)*sizeof(char));

	s[len++] = c;
	s[len] = '\0';
	return len;
}

int readPrefix(FILE* f) {	// Read <a.*href="
	char c;
	if (fscanf(f, "%c", &c) == EOF) return 0;
	if (c != '<') return 0;
	if (fscanf(f, "%c", &c) == EOF) return 0;
	if (c != 'a') return 0;
	do {
		if (fscanf(f, "%c", &c) == EOF) return 0;
		if (c == '>') return 0;
	} while (c != 'h');
	if (fscanf(f, "%c", &c) == EOF) return 0;
	if (c != 'r') return 0;
	if (fscanf(f, "%c", &c) == EOF) return 0;
	if (c != 'e') return 0;
	if (fscanf(f, "%c", &c) == EOF) return 0;
	if (c != 'f') return 0;
	if (fscanf(f, "%c", &c) == EOF) return 0;
	if (c != '=') return 0;
	if (fscanf(f, "%c", &c) == EOF) return 0;
	if (c != '\"') return 0;
	return 1;
}



EntryList parsingFile(char* fileName) {
	// hyperlink regex: <a href=".+">.+</a>
	EntryList L;
	FILE* f = fopen(fileName, "r");
	while (!feof(f)) {
		if (readPrefix(f)) {
			char *url, *name;
			char c;
			int len;

			len = 0;
			while (fscanf(f, "%c", &c) != EOF) {
				if (c == '\"') break;
				len = insertChar(c, url, len);
			}
			
			len = 0;
			
		}
	} 
	fclose(f);
}
