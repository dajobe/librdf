/* package name */
#define PACKAGE

/* package version */
#define VERSION

/* defined if specific types are already defined as typedefs */
#undef HAVE_BYTE_TYPEDEF
#undef HAVE_USHORT_TYPEDEF
#undef HAVE_ULONG_TYPEDEF
#undef HAVE_U16_TYPEDEF
#undef HAVE_U32_TYPEDEF

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
