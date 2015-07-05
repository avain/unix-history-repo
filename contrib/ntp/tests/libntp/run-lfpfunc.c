/* AUTOGENERATED FILE. DO NOT EDIT. */

//=======Test Runner Used To Run Each Test Below=====
#define RUN_TEST(TestFunc, TestLineNum) \
{ \
  Unity.CurrentTestName = #TestFunc; \
  Unity.CurrentTestLineNumber = TestLineNum; \
  Unity.NumberOfTests++; \
  if (TEST_PROTECT()) \
  { \
      setUp(); \
      TestFunc(); \
  } \
  if (TEST_PROTECT() && !TEST_IS_IGNORED) \
  { \
    tearDown(); \
  } \
  UnityConcludeTest(); \
}

//=======Automagically Detected Files To Include=====
#include "unity.h"
#include <setjmp.h>
#include <stdio.h>

//=======External Functions This Runner Calls=====
extern void setUp(void);
extern void tearDown(void);
void resetTest(void);
extern void test_AdditionLR();
extern void test_AdditionRL();
extern void test_SubtractionLR();
extern void test_SubtractionRL();
extern void test_Negation();
extern void test_Absolute();
extern void test_FDF_RoundTrip();
extern void test_SignedRelOps();
extern void test_UnsignedRelOps();


//=======Test Reset Option=====
void resetTest()
{
  tearDown();
  setUp();
}

char *progname;


//=======MAIN=====
int main(int argc, char *argv[])
{
  progname = argv[0];
  Unity.TestFile = "lfpfunc.c";
  UnityBegin("lfpfunc.c");
  RUN_TEST(test_AdditionLR, 320);
  RUN_TEST(test_AdditionRL, 339);
  RUN_TEST(test_SubtractionLR, 358);
  RUN_TEST(test_SubtractionRL, 373);
  RUN_TEST(test_Negation, 391);
  RUN_TEST(test_Absolute, 412);
  RUN_TEST(test_FDF_RoundTrip, 447);
  RUN_TEST(test_SignedRelOps, 479);
  RUN_TEST(test_UnsignedRelOps, 522);

  return (UnityEnd());
}
