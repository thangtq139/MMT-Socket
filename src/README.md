compile: gcc -o test test.c parsingLink.c

run: ./test [host] [path]

example: 

./test     students.iitk.ac.in     programmingclub/course/lectures/

./test     students.iitk.ac.in     programmingclub/course/programs/2_precedence.c

TODO:
*    create file (not write to stdout like this) [checked]
*    control error
*    parsing http link
*    download whole folder
*    ...
*    write report
