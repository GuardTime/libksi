#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "net_curl.h"

#define KSI_ERR_STACK_LEN 16

static int KSI_CTX_global_initCount = 0;

struct KSI_CTX_st {

	/******************
	 *  ERROR HANDLING.
	 ******************/

	/* Status code of the last executed function. */
	int statusCode;

	/* Array of errors. */
	KSI_ERR *errors;

	/* Length of error array. */
	unsigned int errors_size;

	/* Count of errors (usually #error_end - #error_start + 1, unless error count > #errors_size. */
	unsigned int errors_count;

	KSI_Logger *logger;

	/************
	 * TRANSPORT.
	 ************/

	KSI_NetProvider *netProvider;

	KSI_PKITruststore *pkiTruststore;

	KSI_PublicationsFile *publicationsFile;

};

const char *KSI_getErrorString(int statusCode) {
	switch (statusCode) {
		case KSI_OK:
			return "No errors";
		case KSI_INVALID_ARGUMENT:
			return "Invalid argument";
		case KSI_INVALID_FORMAT:
			return "Invalid format";
		case KSI_UNTRUSTED_HASH_ALGORITHM:
			return "The hash algorithm is not trusted";
		case KSI_UNAVAILABLE_HASH_ALGORITHM:
			return "The hash algorith is not implemented or unavailable";
		case KSI_BUFFER_OVERFLOW:
			return "Buffer overflow";
		case KSI_TLV_PAYLOAD_TYPE_MISMATCH:
			return "TLV payload type mismatch";
		case KSI_ASYNC_NOT_FINISHED:
			return "Asynchronous call not yet finished.";
		case KSI_INVALID_SIGNATURE:
			return "Invalid KSI signature.";
		case KSI_INVALID_PKI_SIGNATURE:
			return "Invalid PKI signature.";
		case KSI_PKI_CERTIFICATE_NOT_TRUSTED:
			return "The PKI certificate is not trusted.";
		case KSI_OUT_OF_MEMORY:
			return "Out of memory";
		case KSI_IO_ERROR:
			return "I/O error";
		case KSI_NETWORK_ERROR:
			return "Network error";
		case KSI_HTTP_ERROR:
			return "HTTP error";
		case KSI_AGGREGATOR_ERROR:
			return "Failure from aggregator.";
		case KSI_EXTENDER_ERROR:
			return "Failure from extender.";
		case KSI_EXTEND_WRONG_CAL_CHAIN:
			return "The given calendar chain is not a continuation of the signature calendar chain.";
		case KSI_EXTEND_NO_SUITABLE_PUBLICATION:
			return "There is no suitable publication yet.";
		case KSI_VERIFY_PUBLICATION_NOT_FOUND:
			return "Unknown publication";
		case KSI_VERIFY_PUBLICATION_MISMATCH:
			return "The publications in the signature and publications file mismatch.";
		case KSI_INVALID_PUBLICATION:
			return "Ivalid publication";
		case KSI_WRONG_DOCUMENT:
			return "Wrong document";
		case KSI_PUBLICATIONS_FILE_NOT_SIGNED_WITH_PKI:
			return "The publications file is not signed.";
		case KSI_CRYPTO_FAILURE:
			return "Cryptographic failure";
		case KSI_UNKNOWN_ERROR:
			return "Unknown internal error";
		default:
			return "Unknown status code";
	}
}

int KSI_CTX_new(KSI_CTX **context) {
	int res = KSI_UNKNOWN_ERROR;

	KSI_CTX *ctx = NULL;
	KSI_NetProvider *netProvider = NULL;
	KSI_PKITruststore *pkiTruststore = NULL;
	KSI_Logger *logger = NULL;

	ctx = KSI_new(KSI_CTX);
	if (ctx == NULL) {
		res = KSI_OUT_OF_MEMORY;
		goto cleanup;
	}

	/* Init error stack */
	ctx->errors_size = KSI_ERR_STACK_LEN;
	ctx->errors = KSI_malloc(sizeof(KSI_ERR) * ctx->errors_size);
	if (ctx->errors == NULL) {
		res = KSI_OUT_OF_MEMORY;
		goto cleanup;
	}
	ctx->errors_count = 0;
	ctx->publicationsFile = NULL;
	ctx->pkiTruststore = NULL;
	ctx->netProvider = NULL;
	ctx->logger = NULL;

	KSI_ERR_clearErrors(ctx);

	/* Create and set the logger. */
	res = KSI_Logger_new(ctx, NULL, KSI_LOG_DEBUG, &logger);
	if (res != KSI_OK) goto cleanup;
	res = KSI_setLogger(ctx, logger);
	if (res != KSI_OK) goto cleanup;

	/* Initialize curl as the net handle. */
	res = KSI_CurlNetProvider_new(ctx, &netProvider);
	if (res != KSI_OK) goto cleanup;

	/* Configure curl net provider */
	if ((res = KSI_CurlNetProvider_setSignerUrl(netProvider, KSI_DEFAULT_URI_AGGREGATOR /*"192.168.1.36:3333"*/)) != KSI_OK) goto cleanup;
	if ((res = KSI_CurlNetProvider_setExtenderUrl(netProvider, KSI_DEFAULT_URI_EXTENDER)) != KSI_OK) goto cleanup;
	if ((res = KSI_CurlNetProvider_setPublicationUrl(netProvider, KSI_DEFAULT_URI_PUBLICATIONS_FILE)) != KSI_OK) goto cleanup;
	if ((res = KSI_CurlNetProvider_setReadTimeoutSeconds(netProvider, 5)) != KSI_OK) goto cleanup;
	if ((res = KSI_CurlNetProvider_setConnectTimeoutSeconds(netProvider, 5)) != KSI_OK) goto cleanup;

	res = KSI_setNetworkProvider(ctx, netProvider);
	if (res != KSI_OK) goto cleanup;
	netProvider = NULL;

	/* Create and set the PKI truststore */
	res = KSI_PKITruststore_new(ctx, 1, &pkiTruststore);
	if (res != KSI_OK) goto cleanup;
	res = KSI_setPKITruststore(ctx, pkiTruststore);
	if (res != KSI_OK) goto cleanup;
	pkiTruststore = NULL;

	/* Return the context. */
	*context = ctx;
	ctx = NULL;

	res = KSI_OK;

cleanup:

	KSI_NetProvider_free(netProvider);
	KSI_PKITruststore_free(pkiTruststore);

	KSI_CTX_free(ctx);

	return res;
}

/**
 *
 */
void KSI_CTX_free(KSI_CTX *context) {
	if (context != NULL) {
		KSI_free(context->errors);

		KSI_Logger_free(context->logger);

		KSI_NetProvider_free(context->netProvider);
		KSI_PKITruststore_free(context->pkiTruststore);

		KSI_PublicationsFile_free(context->publicationsFile);

		KSI_free(context);
	}
}

/**
 *
 */
int KSI_global_init(void) {
	int res = KSI_UNKNOWN_ERROR;

	if (KSI_CTX_global_initCount == 0) {
		res = KSI_CurlNetProvider_global_init();
		if (res != KSI_OK) goto cleanup;

		res = KSI_PKITruststore_global_init();
		if (res != KSI_OK) goto cleanup;
	}

	KSI_CTX_global_initCount++;

	res = KSI_OK;

cleanup:

	return res;
}

/**
 *
 */
void KSI_global_cleanup(void) {
	if (KSI_CTX_global_initCount == 0) {
		KSI_CurlNetProvider_global_cleanup();
		KSI_PKITruststore_global_cleanup();
	}
	if (KSI_CTX_global_initCount > 0) {
		KSI_CTX_global_initCount--;
	}
}

int KSI_sendSignRequest(KSI_CTX *ctx, const unsigned char *request, int request_length, KSI_NetHandle **handle) {
	KSI_ERR err;
	KSI_NetHandle *hndl = NULL;
	int res;
	KSI_NetProvider *netProvider = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, request != NULL) goto cleanup;
	KSI_PRE(&err, request_length > 0) goto cleanup;

	KSI_BEGIN(ctx, &err);

	netProvider = ctx->netProvider;

	res = KSI_NetHandle_new(ctx, request, request_length, &hndl);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_NetProvider_sendSignRequest(netProvider, hndl);
	KSI_CATCH(&err, res) goto cleanup;

	*handle = hndl;
	hndl = NULL;

	KSI_SUCCESS(&err);

cleanup:

	KSI_NetHandle_free(hndl);

	return KSI_RETURN(&err);
}

int KSI_sendExtendRequest(KSI_CTX *ctx, const unsigned char *request, int request_length, KSI_NetHandle **handle) {
	KSI_ERR err;
	KSI_NetHandle *hndl = NULL;
	int res;
	KSI_NetProvider *netProvider = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, request != NULL) goto cleanup;
	KSI_PRE(&err, request_length > 0) goto cleanup;

	KSI_BEGIN(ctx, &err);

	netProvider = ctx->netProvider;

	res = KSI_NetHandle_new(ctx, request, request_length, &hndl);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_NetProvider_sendExtendRequest(netProvider, hndl);
	KSI_CATCH(&err, res) goto cleanup;

	*handle = hndl;
	hndl = NULL;

	KSI_SUCCESS(&err);

cleanup:

	KSI_NetHandle_free(hndl);

	return KSI_RETURN(&err);
}


int KSI_sendPublicationRequest(KSI_CTX *ctx, const unsigned char *request, int request_length, KSI_NetHandle **handle) {
	KSI_ERR err;
	KSI_NetHandle *hndl = NULL;
	int res;
	KSI_NetProvider *netProvider = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;

	KSI_BEGIN(ctx, &err);

	netProvider = ctx->netProvider;

	res = KSI_NetHandle_new(ctx, request, request_length, &hndl);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_NetProvider_sendPublicationsFileRequest(netProvider, hndl);
	KSI_CATCH(&err, res) goto cleanup;

	*handle = hndl;
	hndl = NULL;

	KSI_SUCCESS(&err);

cleanup:

	KSI_NetHandle_free(hndl);

	return KSI_RETURN(&err);

}

int KSI_receivePublicationsFile(KSI_CTX *ctx, KSI_PublicationsFile **pubFile) {
	KSI_ERR err;
	int res;
	KSI_NetHandle *handle = NULL;
	const unsigned char *raw = NULL;
	int raw_len = 0;
	KSI_PublicationsFile *tmp = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, pubFile != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	/* TODO! Implement mechanism for reloading (e.g cache timeout) */
	if (ctx->publicationsFile == NULL) {
		res = KSI_sendPublicationRequest(ctx, NULL, 0, &handle);
		KSI_CATCH(&err, res) goto cleanup;

		res = KSI_NetHandle_getResponse(handle, &raw, &raw_len);
		KSI_CATCH(&err, res) goto cleanup;

		res = KSI_PublicationsFile_parse(ctx, raw, raw_len, &tmp);
		KSI_CATCH(&err, res) goto cleanup;

		res = KSI_PublicationsFile_verify(tmp, ctx->pkiTruststore);
		KSI_CATCH(&err, res) goto cleanup;

		ctx->publicationsFile = tmp;
		tmp = NULL;
	}

	*pubFile = ctx->publicationsFile;

	KSI_SUCCESS(&err);

cleanup:

	KSI_NetHandle_free(handle);
	KSI_PublicationsFile_free(tmp);

	return KSI_RETURN(&err);

}

int KSI_verifyPublicationsFile(KSI_CTX *ctx, KSI_PublicationsFile *pubFile) {
	KSI_ERR err;
	int res;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, pubFile != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	res = KSI_PublicationsFile_verify(pubFile, ctx->pkiTruststore);
	KSI_CATCH(&err, res) goto cleanup;

	KSI_SUCCESS(&err);

cleanup:

	return KSI_RETURN(&err);
}

int KSI_verifySignature(KSI_CTX *ctx, KSI_Signature *sig) {
	KSI_ERR err;
	int res;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, sig != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	res = KSI_Signature_verify(sig ,ctx);
	KSI_CATCH(&err, res) goto cleanup;

	KSI_SUCCESS(&err);

cleanup:

	return KSI_RETURN(&err);
}

int KSI_createSignature(KSI_CTX *ctx, const KSI_DataHash *dataHash, KSI_Signature **sig) {
	KSI_ERR err;
	int res;
	KSI_Signature *tmp = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, dataHash != NULL) goto cleanup;
	KSI_PRE(&err, sig != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	res = KSI_Signature_create(ctx, dataHash, &tmp);
	KSI_CATCH(&err, res) goto cleanup;

	*sig = tmp;
	tmp = NULL;

	KSI_SUCCESS(&err);

cleanup:

	KSI_Signature_free(tmp);

	return KSI_RETURN(&err);
}

int KSI_extendSignature(KSI_CTX *ctx, KSI_Signature *sig, KSI_Signature **extended) {
	KSI_ERR err;
	int res;
	KSI_PublicationsFile *pubFile = NULL;
	KSI_Integer *signingTime = NULL;
	KSI_PublicationRecord *pubRec = NULL;
	KSI_Signature *extSig = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, sig != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	res = KSI_receivePublicationsFile(ctx, &pubFile);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_Signature_getSigningTime(sig, &signingTime);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_PublicationsFile_getNearestPublication(pubFile, signingTime, &pubRec);
	KSI_CATCH(&err, res) goto cleanup;

	if (pubRec == NULL) {
		KSI_FAIL(&err, KSI_EXTEND_NO_SUITABLE_PUBLICATION, NULL);
		goto cleanup;
	}

	res = KSI_Signature_extend(sig, pubRec, &extSig);
	KSI_CATCH(&err, res) goto cleanup;

	*extended = extSig;
	extSig = NULL;

	KSI_SUCCESS(&err);

cleanup:

	KSI_Signature_free(extSig);
	return KSI_RETURN(&err);
}

int KSI_extendSignatureToPublication(KSI_CTX *ctx, KSI_Signature *sig, char *pubString, KSI_Signature **extended) {
	KSI_ERR err;
	int res;
	KSI_PublicationsFile *pubFile = NULL;
	KSI_Integer *signingTime = NULL;
	KSI_PublicationRecord *pubRec = NULL;
	KSI_Signature *extSig = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, sig != NULL) goto cleanup;
	KSI_PRE(&err, pubString != NULL) goto cleanup;
	KSI_PRE(&err, extended != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	res = KSI_receivePublicationsFile(ctx, &pubFile);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_Signature_getSigningTime(sig, &signingTime);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_PublicationsFile_getNearestPublication(pubFile, signingTime, &pubRec);
	KSI_CATCH(&err, res) goto cleanup;

	if (pubRec == NULL) {
		KSI_FAIL(&err, KSI_EXTEND_NO_SUITABLE_PUBLICATION, NULL);
		goto cleanup;
	}

	res = KSI_Signature_extend(sig, pubRec, &extSig);
	KSI_CATCH(&err, res) goto cleanup;

	*extended = extSig;
	extSig = NULL;

	KSI_SUCCESS(&err);

cleanup:

	KSI_Signature_free(extSig);
	return KSI_RETURN(&err);
}

int KSI_ERR_init(KSI_CTX *ctx, KSI_ERR *err) {
	err->ctx = ctx;

	KSI_ERR_fail(err, KSI_UNKNOWN_ERROR, 0, "null", 0, "Internal error: Probably a function returned without a distinctive success or error.");

	return KSI_OK;
}

int KSI_ERR_apply(KSI_ERR *err) {
	KSI_CTX *ctx = err->ctx;
	KSI_ERR *ctxErr = NULL;

	if (ctx != NULL) {
		if (err->statusCode != KSI_OK) {
			ctxErr = ctx->errors + (ctx->errors_count % ctx->errors_size);



			ctxErr->statusCode = err->statusCode;
			ctxErr->extErrorCode = err->extErrorCode;
			ctxErr->lineNr = err->lineNr;
			strncpy(ctxErr->fileName, KSI_strnvl(err->fileName), sizeof(err->fileName));
			strncpy(ctxErr->message, KSI_strnvl(err->message), sizeof(err->message));

			ctx->errors_count++;
		}

		ctx->statusCode = err->statusCode;
	}
	/* Return the result, which does not indicate the result of this method. */
	return err->statusCode;
}

void KSI_ERR_success(KSI_ERR *err) {
	err->statusCode = KSI_OK;
	*err->message = '\0';
}

int KSI_ERR_fail(KSI_ERR *err, int statusCode, long extErrorCode, char *fileName, int lineNr, char *message) {
	err->extErrorCode = extErrorCode;
	err->statusCode = statusCode;
	if (message == NULL) {
		strncpy(err->message, KSI_getErrorString(statusCode), sizeof(err->message));
	} else {
		strncpy(err->message, KSI_strnvl(message), sizeof(err->message));
	}
	strncpy(err->fileName, KSI_strnvl(fileName), sizeof(err->fileName));
	err->lineNr = lineNr;

	return KSI_OK;
}

void KSI_ERR_clearErrors(KSI_CTX *ctx) {
	if (ctx != NULL) {
		ctx->statusCode = KSI_UNKNOWN_ERROR;
		ctx->errors_count = 0;
	}
}

int KSI_ERR_statusDump(KSI_CTX *ctx, FILE *f) {
	int res = KSI_UNKNOWN_ERROR;
	KSI_ERR *err = NULL;
	unsigned int i;

	fprintf(f, "KSI error trace:\n");
	if (ctx->errors_count == 0) {
		printf("  No errors.\n");
		goto cleanup;
	}

	/* List all errors, starting from the most general. */
	for (i = 0; i < ctx->errors_count && i < ctx->errors_size; i++) {
		err = ctx->errors + ((ctx->errors_count - i - 1) % ctx->errors_size);
		fprintf(f, "  %3u) %s:%u - (%d/%ld) %s\n", ctx->errors_count - i, err->fileName, err->lineNr,err->statusCode, err->extErrorCode, err->message);
	}

	/* If there where more errors than buffers for the errors, indicate the fact */
	if (ctx->errors_count > ctx->errors_size) {
		fprintf(f, "  ... (more errors)\n");
	}

	res = KSI_OK;

cleanup:

	return res;
}

void *KSI_malloc(size_t size) {
	return malloc(size);
}

void *KSI_calloc(size_t num, size_t size) {
	return calloc(num, size);
}

void *KSI_realloc(void *ptr, size_t size) {
	return realloc(ptr, size);
}

void KSI_free(void *ptr) {
	free(ptr);
}

/**
 *
 */
int KSI_CTX_getStatus(KSI_CTX *ctx) {
	/* Will fail with segfault if context is null. */
	return ctx == NULL ? KSI_INVALID_ARGUMENT : ctx->statusCode;
}


#define CTX_VALUEP_SETTER(var, nam, typ, fre)												\
int KSI_set##nam(KSI_CTX *ctx, typ *var) { 													\
	int res = KSI_UNKNOWN_ERROR;															\
	if (ctx == NULL) {																		\
		res = KSI_INVALID_ARGUMENT;															\
		goto cleanup;																		\
	}																						\
	if (ctx->var != NULL) {																	\
		fre(ctx->var);																		\
	}																						\
	ctx->var = var;																			\
	res = KSI_OK;																			\
cleanup:																					\
	return res;																				\
} 																							\

#define CTX_VALUEP_GETTER(var, nam, typ) 													\
int KSI_get##nam(KSI_CTX *ctx, typ **var) { 												\
	int res = KSI_UNKNOWN_ERROR;															\
	if (ctx == NULL || var == NULL) {														\
		res = KSI_INVALID_ARGUMENT;															\
		goto cleanup;																		\
	}																						\
	*var = ctx->var;																		\
	res = KSI_OK;																			\
cleanup:																					\
	return res;																				\
} 																							\

#define CTX_GET_SET_VALUE(var, nam, typ, fre) 												\
	CTX_VALUEP_SETTER(var, nam, typ, fre)													\
	CTX_VALUEP_GETTER(var, nam, typ)														\

CTX_GET_SET_VALUE(pkiTruststore, PKITruststore, KSI_PKITruststore, KSI_PKITruststore_free)
CTX_GET_SET_VALUE(netProvider, NetworkProvider, KSI_NetProvider, KSI_NetProvider_free)
CTX_GET_SET_VALUE(logger, Logger, KSI_Logger, KSI_Logger_free)