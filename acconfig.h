/* package name */
#define PACKAGE

/* package version */
#define VERSION

#undef LIBRDF_VERSION_MAJOR
#undef LIBRDF_VERSION_MINOR
#undef LIBRDF_VERSION_RELEASE

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

/* defined if have W3C libwww available */
#undef HAVE_LIBWWW

/* defined if W3C libwww has RDF support */
#undef HAVE_LIBWWW_RDF

/* defined if specific hashes are available */
#undef HAVE_GDBM_HASH
#undef HAVE_BDB_HASH

/* Berkeley DB complexities */
/* V4.1+ */
#undef HAVE_BDB_OPEN_7_ARGS
/* V2/V3 */
#undef HAVE_BDB_DB_TXN
#undef HAVE_BDB_CURSOR
/* V2 versions differ / V3 */
#undef HAVE_BDB_CURSOR_4_ARGS
/* V3/V4 */
#undef HAVE_DB_CREATE
/* V2 */
#undef HAVE_DB_OPEN
/* V1 */
#undef HAVE_DBOPEN

/* java program location */
#undef JAVA_COMMAND

/* directory where local java files will be installed */
#undef JAVA_CLASS_DIR

/* RDF parsers */
#undef HAVE_RAPTOR_RDF_PARSER
#undef HAVE_REPAT_RDF_PARSER

/* XML parsers */
#undef NEED_EXPAT
#undef NEED_LIBXML

/* Raptor can use either expat or libxml */
#undef RAPTOR_XML_EXPAT
#undef RAPTOR_XML_LIBXML

/* expat compiled with namespaces? */
#undef HAVE_XML_SetNamespaceDeclHandler

/* for Repat */
#undef RDFPARSE_INCLUDE_XMLPARSE

/* When need expat compiled in the sources */
#undef NEED_EXPAT_SOURCE
