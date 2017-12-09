#include <stdio.h>

int main() {
	FILE* f1 = fopen("dir/file", "w");
	fclose(f1);
}
