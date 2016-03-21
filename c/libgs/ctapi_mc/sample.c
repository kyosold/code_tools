#include <stdio.h>
#include <string.h>

#include "ctapi_mc.h"
#include <stdio.h>
#include <string.h>

#include "mcapi_mc.h"

int main(int argc, char **argv)
{
	char *mc_server = argv[1];
    char *mc_port = argv[2];
    char *mc_timeout = argv[3];

	// set mc
	char *key = "name";
    char *value = "kyosold@qq.com";
    
    int succ = set_mc(mc_server, atoi(mc_port), atoi(mc_timeout), key, value);
    if (succ == 0) {
        printf("set mc succ:%s => %s\n", key, value);
    } else {
        printf("set mc fail:%s => %s\n", key, value);
    }

	// get mc
	char *pvalue = get_mc(mc_server, atoi(mc_port), atoi(mc_timeout), key);
    if (pvalue) {
        printf("get mc succ:%s => %s\n", key, pvalue);
        
        free(pvalue);
        pvalue = NULL;
    } else {
        printf("get mc fail:%s\n", key);
    }

	// delete mc
	succ = delete_mc(mc_server, atoi(mc_port), atoi(mc_timeout), key);
    if (succ == 0) {
        printf("delete mc succ:%s\n", key);
    } else {
        printf("delete mc fail:%s\n", key);
    }
    
    return 0;
}
