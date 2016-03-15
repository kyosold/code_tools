#include <stdio.h>

#include "base64.h"

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("%s <enc/dec> <string>\n", argv[0]);
		return 1;
	}

	if (strcasecmp(argv[1], "enc") == 0) {
		char *in = argv[2];
    	char *out = NULL;

		size_t b64_att_len = base64_encode_alloc(in, strlen(in), &out);
		printf("%d:%s\n", b64_att_len, out);

		if (out != NULL) {
			free(out);
			out = NULL;
		}

	} else if (strcasecmp(argv[1], "dec") == 0) {
		char *in = argv[2];
		char *out = NULL;
		size_t outlen = 0;

		struct base64_decode_context ctx;
		base64_decode_ctx_init(&ctx);
		bool ret = base64_decode_alloc_ctx(&ctx, in, strlen(in), &out, &outlen);
		printf("%d:%s\n", outlen, out);
		
		if (out != NULL) {
			free(out);
			out = NULL;
		}
	}

	return 0;
}
