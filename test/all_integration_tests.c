/*
 * Copyright 2013-2018 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#include "all_integration_tests.h"

#include "cutest/CuTest.h"
#include "support_tests.h"

#include "../src/ksi/internal.h"

#include <ksi/hash.h>
#include <ksi/compatibility.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#ifndef UNIT_TEST_OUTPUT_XML
#  define UNIT_TEST_OUTPUT_XML "_testsuite.xml"
#endif

#define KSITEST_ASYNC_SLEEP_TIME_MS 100
#define KSITEST_ASYNC_NO_RESP_TIMEOUT_MS (100 * 10 * 5)

KSI_CTX *ctx = NULL;

/**
 * Configuration object for integration tests.
 */
KSITest_Conf conf;

static CuSuite* initSuite(void) {
	CuSuite *suite = CuSuiteNew();

	addSuite(suite, AggreIntegrationTests_getSuite);
	addSuite(suite, ExtIntegrationTests_getSuite);
	addSuite(suite, PubIntegrationTests_getSuite);
	addSuite(suite, IntegrationTestPack_getSuite);
	addSuite(suite, AsyncAggrIntegrationTests_getSuite);
	addSuite(suite, AsyncExtIntegrationTests_getSuite);
	addSuite(suite, HaAggrIntegrationTests_getSuite);
	addSuite(suite, HaExtIntegrationTests_getSuite);

	return suite;
}

static int RunAllTests() {
	int failCount;
	int res;
	CuSuite* suite = initSuite();
	FILE *logFile = NULL;
	KSI_HashAlgorithm alg_id = KSI_HASHALG_INVALID_VALUE;

	if (KSI_DISABLE_NET_PROVIDER & (KSI_IMPL_NET_HTTP | KSI_IMPL_NET_TCP)) {
		fprintf(stderr, "Error: Network provider is disabled!\n");
		exit(EXIT_FAILURE);
	}

	/* Create the context. */
	res = KSI_CTX_new(&ctx);
	if (ctx == NULL || res != KSI_OK){
		fprintf(stderr, "Error: Unable to init KSI context (%s)!\n", KSI_getErrorString(res));
		exit(EXIT_FAILURE);
	}

	if (*conf.aggregator.hmac) {
		if ((alg_id = KSI_getHashAlgorithmByName(conf.aggregator.hmac)) == KSI_HASHALG_INVALID_VALUE) {
			fprintf(stderr, "Invalid hash algorithm for aggregator HMAC: '%s'\n", conf.aggregator.hmac);
			exit(EXIT_FAILURE);
		}
		KSI_CTX_setAggregatorHmacAlgorithm(ctx, alg_id);
	}

	if (*conf.extender.hmac) {
		if ((alg_id = KSI_getHashAlgorithmByName(conf.extender.hmac)) == KSI_HASHALG_INVALID_VALUE) {
			fprintf(stderr, "Invalid hash algorithm for extender HMAC: '%s'\n", conf.extender.hmac);
			exit(EXIT_FAILURE);
		}
		KSI_CTX_setExtenderHmacAlgorithm(ctx, alg_id);
	}

	/* Set default timeout values. */
	if (conf.async.timeout.sleep == 0) conf.async.timeout.sleep = KSITEST_ASYNC_SLEEP_TIME_MS;
	if (conf.async.timeout.cumulative == 0) conf.async.timeout.cumulative = KSITEST_ASYNC_NO_RESP_TIMEOUT_MS;

	res = KSI_CTX_setPublicationUrl(ctx, conf.pubfile.url);
	if (res != KSI_OK) {
		fprintf(stderr, "Unable to set publications file URL.\n");
		exit(EXIT_FAILURE);
	}

	res = KSI_CTX_setDefaultPubFileCertConstraints(ctx, conf.pubfile.certConstraints);
	if (res != KSI_OK) {
		fprintf(stderr, "Unable to set publications file verification constraints.\n");
		exit(EXIT_FAILURE);
	}

	logFile = fopen("integration_test.log", "w");
	if (logFile == NULL) {
		fprintf(stderr, "Unable to open log file.\n");
		exit(EXIT_FAILURE);
	}

	KSI_CTX_setLoggerCallback(ctx, KSI_LOG_StreamLogger, logFile);
	KSI_CTX_setLogLevel(ctx, KSI_LOG_DEBUG);

	KSI_CTX_setAggregator(ctx, KSITest_composeUri("ksi+http", &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
	KSI_CTX_setExtender(ctx, KSITest_composeUri("ksi+http", &conf.extender), conf.extender.user, conf.extender.pass);
	KSI_CTX_setConnectionTimeoutSeconds(ctx, 30);
	KSI_CTX_setTransferTimeoutSeconds(ctx, 30);

	CuSuiteRun(suite);

	printStats(suite, "==== INTEGRATION TEST RESULTS ====");

	writeXmlReport(suite, UNIT_TEST_OUTPUT_XML);

	failCount = suite->failCount;

	CuSuiteDelete(suite);

	if (logFile != NULL) {
		fclose(logFile);
	}

	KSI_CTX_free(ctx);

	return failCount;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Usage:\n %s <path to test root>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	initFullResourcePath(argv[1]);

	if (KSITest_Conf_load(getFullResourcePath("integrationtest.conf"), &conf)) {
		exit(EXIT_FAILURE);
	}


	return RunAllTests();
}
