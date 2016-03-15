#include <stdio.h>

#include "utf8str.h"

int main(int argc, char **argv)
{
	char *in = "hello, 你好,谢谢";
    unsigned int n = atoi(argv[1]);
    char out[31] = {0};
    char *p_des = NULL;

    p_des = utf8_strncpy(out, utf8_ltrim(in), n);
    if (*p_des != '\0' && *out != '\0') {
        printf("out:%s\n", out);
    }	

	return 0;
}
