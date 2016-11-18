#include <stdio.h>

#include "dictionary.h"

#define DICT_INVALID_KEY    NULL

int main(int argc, char **argv)
{
	dictionary *dict_mem = NULL;

    /* Allocate dictionary */   
    dict_mem = dictionary_new(0);

    /* Set values in dictionary */
    char key[1024] = {0};
    char val[1024] = {0};
    snprintf(key, sizeof(key), "key_1");
    snprintf(val, sizeof(val), "value_0001");

    int succ = dictionary_set(dict_mem, key, val);  
    if (succ != 0) {
        printf("dictionary_set failed\n");
    }
	printf("dictionary_set succ\n");

    /* Get values in dictionary */
    /* If key is not exist, return value which you specifiy */
    char *pval = dictionary_get(dict_mem, key, DICT_INVALID_KEY);
    if (pval == DICT_INVALID_KEY) {
        printf("cannot get value for key:%s\n", key);
    }
	printf("dictionary_get: %s => %s\n", key, pval);

    /* Unset values */
    dictionary_unset(dict_mem, key);
    if (dict_mem->n != 0) {
        printf("error deleting values\n");
    }
	printf("dictionary_unset: %s succ\n", key);

    /* Dealloc dictionary */  
    dictionary_del(dict_mem);
    dict_mem = NULL;

	return 0;
}
