/* package name */
#define PACKAGE

/* package version */
#define VERSION

/* type is defined by typedef or MISSING if not present */
#undef byte
#undef u16
#undef u32
#undef u64

/* defined if have local version of message digests */
#undef HAVE_LOCAL_SHA1_DIGEST
#undef HAVE_LOCAL_MD5_DIGEST
#undef HAVE_LOCAL_RIPEMD160_DIGEST

/* defined if have openssl digests library (libcrypto) available */
#undef HAVE_OPENSSL_DIGESTS
/* defined if have openssl version of message digests */
#undef HAVE_OPENSSL_CRYPTO_MD5_DIGEST
#undef HAVE_OPENSSL_CRYPTO_SHA1_DIGEST
#undef HAVE_OPENSSL_CRYPTO_RIPEMD160_DIGEST

/* defined if specific hashes are available */
#undef HAVE_GDBM_HASH
