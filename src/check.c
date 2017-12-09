#include <stdio.h>

int main() {
	FILE* f1 = fopen("file", "r");
	FILE* f2 = fopen("file.pdf", "r");
	char c1, c2;
	int t;

	t = 0;
	while (fscanf(f1, "%c", &c1) != EOF && fscanf(f2, "%c", &c2) != EOF) {
		++t;
		if (c1 != c2) {
			printf("%d %x %x\n", t, c1, c2);
			break;
		}
	}
	fclose(f1);
	fclose(f2);
}
