/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hctest.h"
#include <ohos_init.h>
#include "parameter.h"
#include "samgr_lite.h"
#include <stdlib.h>
#include <securec.h>
#include <unistd.h>


void setUp(void) {}
void tearDown(void) {}
void suiteSetUp(void) { }
int suiteTearDown(int num_failures) { return num_failures; }


static TestSuiteManager g_testSuiteManager;
static BOOL CompareInputType(const char *source, const char *input);
static void RunSingleTestCase(CTestCase* cTestCase,
                              const char *caseName, const int32 flag);
static int16 g_totalSuitesNum = 0;
static int16 g_doneSuitesNum = 0;
static void RunSingleTestSuite(CTestSuite* testSuite)
{
    if (testSuite == NULL) {
        return;
    }
    int16 size = VECTOR_Size(&(testSuite->test_cases));
    UnityBegin(testSuite->file);
    int16 i;
    for (i = size - 1; i >= 0; i--) {
        CTestCase* cTestCase = (CTestCase *)(VECTOR_At(
            &(testSuite->test_cases), i));
        if (cTestCase != NULL) {
            if (i == size - 1) {
                cTestCase->lite_setup();
            }
            RunSingleTestCase(cTestCase, cTestCase->case_name, cTestCase->flag);
            if (i == 0) {
                cTestCase->lite_teardown();
            }
        }
    }
    UnityEnd();
}

static CTestSuite* GetTestSuite(const char *test_suite)
{
    CTestSuite* suite = NULL;
    TestSuiteManager* testMgr = GetTestMgrInstance();
    if (testMgr == NULL || test_suite == NULL) {
        return suite;
    }
    int16 size = VECTOR_Size(&(testMgr->test_suites));
    int16 i;
    for (i = 0; i < size; i++) {
        CTestSuite* curSuite = (CTestSuite *)(VECTOR_At(&(testMgr->test_suites), i));
        if (strcmp(curSuite->suite_name, test_suite) == 0) {
            suite = curSuite;
            break;
        }
    }

    return suite;
}

static BOOL RegisterTestSuite(CTestSuite *testSuite)
{
    VECTOR_Add(&(g_testSuiteManager.test_suites), testSuite);
    g_totalSuitesNum++;
    return TRUE;
}

static BOOL RemoveTestSuite(CTestSuite *testSuite)
{
    VECTOR_Swap(&(g_testSuiteManager.test_suites),
                VECTOR_Find(&(g_testSuiteManager.test_suites), testSuite),
                testSuite);
    return TRUE;
}

static void AddTestCase(CTestCase *testCase)
{
    if (testCase == NULL) {
        return;
    }
    CTestSuite* suite = GetTestSuite(testCase->suite_name);
    if (suite == NULL) {
        CTestSuite suite_object;
        suite_object.subsystem_name = NULL;
        suite_object.module_name = NULL;
        suite_object.suite_name = testCase->suite_name;
        suite_object.test_cases = VECTOR_Make(NULL, NULL);
        suite = &suite_object;
        VECTOR_Add(&(g_testSuiteManager.test_suites), suite);
    }
    VECTOR_Add(&(suite->test_cases), testCase);
    return;
}

static BOOL CompareInputType(const char *source, const char *input)
{
    if (strcmp(input, CONST_STRING_SPACE) != 0 && strcmp(input, source) != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL g_isBreak = FALSE;
static void RunSingleTestCase(CTestCase* cTestCase,
                              const char *caseName, const int32 flag)
{
    if (cTestCase != NULL) {
        if (CompareInputType(cTestCase->case_name, caseName)
            || (cTestCase->flag != flag)) {
            g_isBreak = TRUE;
            return;
        }
        UnityDefaultTestRun(cTestCase->execute_func, cTestCase->case_name, cTestCase->line_num);
    }
}

static void RunSpecialTestSuite(const char *subSystemName,
                                const char *moduleName,
                                const char *suiteName,
                                const char *caseName,
                                int caseLevel)
{
    int16 i;
    int16 j;
    int16 size = VECTOR_Size(&(g_testSuiteManager.test_suites));
    UNITY_BEGIN();
    for (i = 0; i < size; i++) {
        if (g_isBreak) {
            break;
        }
        CTestSuite* curSuite = (CTestSuite *)(VECTOR_At(&(g_testSuiteManager.test_suites), i));
        if (curSuite != NULL) {
            if (CompareInputType(curSuite->subsystem_name, subSystemName)
                || CompareInputType(curSuite->module_name, moduleName)
                || CompareInputType(curSuite->suite_name, suiteName)) {
                continue;
            }
            for (j = 0; j < VECTOR_Size(&(curSuite->test_cases)); j++) {
                CTestCase* cTestCase = (CTestCase *)(VECTOR_At(
                    &(curSuite->test_cases), j));
                RunSingleTestCase(cTestCase, caseName, caseLevel);
            }
        }
    }
    UNITY_END();
}

static void RunTestSuite(const char* suite_name)
{
    printf("Start to run test suite:%s\n", suite_name);
    CTestSuite* curSuite = GetTestSuite(suite_name);
    if (curSuite != NULL) {
        g_doneSuitesNum++;
        int16 times = curSuite->times;
        int16 i;
        for (i = 0; i < times; i++) {
            sleep(1);
            printf("Run test suite %d times\n", i+1);
            RunSingleTestSuite(curSuite);
        }
        if (g_totalSuitesNum == g_doneSuitesNum) {
            printf("All the test suites finished!\n");
        }
    }
}

void LiteTestPrint(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf(fmt, ap);
    va_end(ap);
}

void ObtainProductParams(void)
{
    char* bootloaderVersion = GetBootloaderVersion();
    if (bootloaderVersion != NULL) {
        printf("The bootloaderVersion is [%s]\n", bootloaderVersion);
        free(bootloaderVersion);
    }
	
    char* securityPatchTag = GetSecurityPatchTag();
    if (securityPatchTag != NULL) {
        printf("The Security Patch is [%s]\n", securityPatchTag);
        free(securityPatchTag);
    }
	
    char* abiList = GetAbiList();
    if (abiList != NULL) {
        printf("The AbiList is [%s]\n", abiList);
        free(abiList);
    }
	
    char* sdkApiLevel = GetSdkApiLevel();
    if (sdkApiLevel != NULL) {
        printf("The sdkApiLevel is [%s]\n", sdkApiLevel);
        free(sdkApiLevel);
    }
	
    char* firstApiLevel = GetFirstApiLevel();
    if (firstApiLevel != NULL) {
        printf("The firstApiLevel is [%s]\n", firstApiLevel);
        free(firstApiLevel);
    }

    char* incrementalVersion = GetIncrementalVersion();
    if (incrementalVersion != NULL) {
        printf("The productSeries is [%s]\n", incrementalVersion);
        free(incrementalVersion);
    }
	
    char* versionId = GetVersionId();
    if (versionId != NULL) {
        printf("The VersionID is [%s]\n", versionId);
        free(versionId);
    }

    char* buildType = GetBuildType();
    if (buildType != NULL) {
        printf("The buildType is [%s]\n", buildType);
        free(buildType);
    }

    char* buildUser = GetBuildUser();
    if (buildUser != NULL) {
        printf("The buildUser is [%s]\n", buildUser);
        free(buildUser);
    }

    char* buildHost = GetBuildHost();
    if (buildHost != NULL) {
        printf("The buildHost is [%s]\n", buildHost);
        free(buildHost);
    }
	
    char* buildTime = GetBuildTime();
    if (buildTime != NULL) {
        printf("The buildTime is [%s]\n", buildTime);
        free(buildTime);
    }	

    char* buildRootHash = GetBuildRootHash();
    if (buildRootHash != NULL) {
        printf("The BuildRootHash is [%s]\n", buildRootHash);
        free(buildRootHash);
    }
}


void ObtainSystemParams(void)
{
    printf("******To Obtain Product Params Start******\n");
    char* productType = GetProductType();
    if (productType != NULL) {
        printf("The Product Type is [%s]\n", productType);
        free(productType);
    }

    char* manuFacture = GetManufacture();
    if (manuFacture != NULL) {
        printf("The manuFacture is [%s]\n", manuFacture);
        free(manuFacture);
    }

    char* brand = GetBrand();
    if (brand != NULL) {
        printf("The brand is [%s]\n", brand);
        free(brand);
    }

    char* marketName = GetMarketName();
    if (marketName != NULL) {
        printf("The marketName is [%s]\n", marketName);
        free(marketName);
    }

    char* productSeries = GetProductSeries();
    if (productSeries != NULL) {
        printf("The productSeries is [%s]\n", productSeries);
        free(productSeries);
    }
	
    char* softwareModel = GetSoftwareModel();
    if (softwareModel != NULL) {
        printf("The softwareModel is [%s]\n", softwareModel);
        free(softwareModel);
    }
	
    char* hardWareModel = GetHardwareModel();
    if (hardWareModel != NULL) {
        printf("The HardwareModel is [%s]\n", hardWareModel);
        free(hardWareModel);
    }

    char* hardWareProfile = GetHardwareProfile();
    if (hardWareProfile != NULL) {
        printf("The HardwareProfile is [%s]\n", hardWareProfile);
        free(hardWareProfile);
    }	
	
    char* serial = GetSerial();
    if (serial != NULL) {
        printf("The serial is [%s]\n", serial);
        free(serial);
    }

    char* osName = GetOsName();
    if (osName != NULL) {
        printf("The osName is [%s]\n", osName);
        free(osName);
    }
	
    char* displayVersion = GetDisplayVersion();
    if (displayVersion != NULL) {
        printf("The OS Version is [%s]\n", displayVersion);
        free(displayVersion);
    }
    ObtainProductParams();

    printf("******To Obtain Product Params End  ******\n");
    return;
}

static void InitTestSuiteMgr(void)
{
    g_testSuiteManager.test_suites = VECTOR_Make(NULL, NULL);
    g_testSuiteManager.GetTestSuite = GetTestSuite;
    g_testSuiteManager.RegisterTestSuite = RegisterTestSuite;
    g_testSuiteManager.AddTestCase = AddTestCase;
    g_testSuiteManager.RemoveTestSuite = RemoveTestSuite;
    g_testSuiteManager.RunSpecialTestSuite = RunSpecialTestSuite;
    g_testSuiteManager.RunTestSuite = RunTestSuite;
    printf("[%10s] HCTest Framework inited.\n",  "HCtest Service");
    ObtainSystemParams();
}
CORE_INIT(InitTestSuiteMgr);
TestSuiteManager* GetTestMgrInstance(void)
{
    return &g_testSuiteManager;
}
