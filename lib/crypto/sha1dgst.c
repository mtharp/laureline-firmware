/* crypto/sha/sha1dgst.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdlib.h>
#include <string.h>
#include "sha.h"

#define DATA_ORDER_IS_BIG_ENDIAN

#define HASH_LONG               SHA_LONG
#define HASH_CTX                SHA_CTX
#define HASH_CBLOCK             SHA_CBLOCK
#define HASH_MAKE_STRING(c,s)   do {	\
	unsigned long ll;		\
	ll=(c)->h0; (void)HOST_l2c(ll,(s));	\
	ll=(c)->h1; (void)HOST_l2c(ll,(s));	\
	ll=(c)->h2; (void)HOST_l2c(ll,(s));	\
	ll=(c)->h3; (void)HOST_l2c(ll,(s));	\
	ll=(c)->h4; (void)HOST_l2c(ll,(s));	\
	} while (0)

# define HASH_UPDATE             	SHA1_Update
# define HASH_TRANSFORM          	SHA1_Transform
# define HASH_FINAL              	SHA1_Final
# define HASH_INIT			SHA1_Init
# define HASH_BLOCK_DATA_ORDER   	sha1_block_data_order
# define Xupdate(a,ix,ia,ib,ic,id)	( (a)=(ia^ib^ic^id),	\
					  ix=(a)=ROTATE((a),1)	\
					)

void sha1_block_data_order (SHA_CTX *c, const void *p,size_t num);

#include "md32_common.h"

#define INIT_DATA_h0 0x67452301UL
#define INIT_DATA_h1 0xefcdab89UL
#define INIT_DATA_h2 0x98badcfeUL
#define INIT_DATA_h3 0x10325476UL
#define INIT_DATA_h4 0xc3d2e1f0UL

fips_md_init_ctx(SHA1, SHA)
	{
	memset (c,0,sizeof(*c));
	c->h0=INIT_DATA_h0;
	c->h1=INIT_DATA_h1;
	c->h2=INIT_DATA_h2;
	c->h3=INIT_DATA_h3;
	c->h4=INIT_DATA_h4;
	return 1;
	}

#define K_00_19	0x5a827999UL
#define K_20_39 0x6ed9eba1UL
#define K_40_59 0x8f1bbcdcUL
#define K_60_79 0xca62c1d6UL

/* As  pointed out by Wei Dai <weidai@eskimo.com>, F() below can be
 * simplified to the code in F_00_19.  Wei attributes these optimisations
 * to Peter Gutmann's SHS code, and he attributes it to Rich Schroeppel.
 * #define F(x,y,z) (((x) & (y))  |  ((~(x)) & (z)))
 * I've just become aware of another tweak to be made, again from Wei Dai,
 * in F_40_59, (x&a)|(y&a) -> (x|y)&a
 */
#define	F_00_19(b,c,d)	((((c) ^ (d)) & (b)) ^ (d)) 
#define	F_20_39(b,c,d)	((b) ^ (c) ^ (d))
#define F_40_59(b,c,d)	(((b) & (c)) | (((b)|(c)) & (d))) 
#define	F_60_79(b,c,d)	F_20_39(b,c,d)

#define BODY_00_15(xi)		 do {	\
	T=E+K_00_19+F_00_19(B,C,D);	\
	E=D, D=C, C=ROTATE(B,30), B=A;	\
	A=ROTATE(A,5)+T+xi;	    } while(0)

#define BODY_16_19(xa,xb,xc,xd)	 do {	\
	Xupdate(T,xa,xa,xb,xc,xd);	\
	T+=E+K_00_19+F_00_19(B,C,D);	\
	E=D, D=C, C=ROTATE(B,30), B=A;	\
	A=ROTATE(A,5)+T;	    } while(0)

#define BODY_20_39(xa,xb,xc,xd)	 do {	\
	Xupdate(T,xa,xa,xb,xc,xd);	\
	T+=E+K_20_39+F_20_39(B,C,D);	\
	E=D, D=C, C=ROTATE(B,30), B=A;	\
	A=ROTATE(A,5)+T;	    } while(0)

#define BODY_40_59(xa,xb,xc,xd)	 do {	\
	Xupdate(T,xa,xa,xb,xc,xd);	\
	T+=E+K_40_59+F_40_59(B,C,D);	\
	E=D, D=C, C=ROTATE(B,30), B=A;	\
	A=ROTATE(A,5)+T;	    } while(0)

#define BODY_60_79(xa,xb,xc,xd)	 do {	\
	Xupdate(T,xa,xa,xb,xc,xd);	\
	T=E+K_60_79+F_60_79(B,C,D);	\
	E=D, D=C, C=ROTATE(B,30), B=A;	\
	A=ROTATE(A,5)+T+xa;	    } while(0)
