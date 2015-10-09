#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <pthread.h>

#define MAXWORD 1024
#define NTHREADS 4

pthread_mutex_t tracker;
pthread_mutexattr_t attribute;


typedef struct dict {
    char *word;
    int count;
    struct dict *next;
} dict_t;

dict_t *wd = NULL;

char *
make_word( char *word ) {
    return strcpy( malloc( strlen( word )+1 ), word );
}

dict_t *
make_dict(char *word) {
    dict_t *nd = (dict_t *) malloc( sizeof(dict_t) );
    nd->word = make_word( word );
    nd->count = 1;
    nd->next = NULL;
    return nd;
}
FILE *myFile;
dict_t *
insert_word( dict_t *d, char *word ) {
    //   Insert word into dict or increment count if already there
    //   return pointer to the updated dict

    dict_t *nd;
    dict_t *pd = NULL;		// prior to insertion point
    dict_t *di = d;		// following insertion point
    // Search down list to find if present or point of insertion
    while(di && ( strcmp(word, di->word ) >= 0) ) {
        if( strcmp( word, di->word ) == 0 ) {
            di->count++;		// increment count
            return d;			// return head
        }
        pd = di;			// advance ptr pair
        di = di->next;
    }

    nd = make_dict(word);		// not found, make entry
    nd->next = di;		// entry bigger than word or tail
    if (pd) {
        pd->next = nd;
        return d;			// insert beyond head

    }
    return nd;
}

void print_dict(dict_t *d) {
    while (d) {
        printf("[%d] %s\n", d->count, d->word);
        d = d->next;
    }
}

int
get_word( char *buf, FILE *infile) {
    int inword = 0;
    int c;

    while( (c = fgetc(infile)) != EOF ) {
        if (inword && !isalpha(c)) {
            buf[inword] = '\0';	// terminate the word string
            return 1;
        }
        if (isalpha(c)) {
            buf[inword++] = c;
        }
    }
    return 0;			// no more words
}

void* execute_threads() {
    int word;
    int check;
    int characters = 1;
    char* wordbuf = malloc(sizeof(char) * (MAXWORD+1));

    while (characters) {
        word = 0;
        pthread_mutex_lock(&tracker);
        check = fgetc(myFile);

        while (word < MAXWORD && check != EOF) {
            if (!isalpha(check) && word) {
                wordbuf[word] = '\0';
                break;
            }

            if (isalpha(check)){
                wordbuf[word++] = check;
            }
            check = fgetc(myFile);
        }

        pthread_mutex_unlock(&tracker);

        if (check == EOF){
            break;
        }

        pthread_mutex_lock(&tracker);
        wd = insert_word(wd, wordbuf);
        pthread_mutex_unlock(&tracker);
    }
    free(wordbuf);
}

dict_t *
words( FILE *infile ) {
    myFile=infile;
    //dict_t *wd = NULL;
    int i;
    pthread_mutexattr_init(&attribute);
    pthread_mutexattr_settype(&attribute, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&tracker, &attribute);
    pthread_t* total_threads = malloc(sizeof(pthread_t) * NTHREADS);

    for (int i = 0; i < NTHREADS; i++) {
        pthread_create(&total_threads[i], NULL, execute_threads, (void*) (intptr_t) i);
    }

    for (i = 0; i < NTHREADS; i++)
        pthread_join(total_threads[i], NULL);

    free(total_threads);
    return wd;
}

int
main( int argc, char *argv[] ) {
    dict_t *d = NULL;
    FILE *infile = stdin;
    if (argc >= 2) {
        infile = fopen (argv[1],"r");
    }
    if( !infile ) {
        printf("Unable to open %s\n",argv[1]);
        exit( EXIT_FAILURE );
    }
    d = words( infile );
    print_dict( d );
    fclose( infile );
}
