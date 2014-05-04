#define fips_md_init(alg) fips_md_init_ctx(alg, alg)

#define fips_md_init_ctx(alg, cx) \
	int alg##_Init(cx##_CTX *c)
#define fips_cipher_abort(alg) while(0)
