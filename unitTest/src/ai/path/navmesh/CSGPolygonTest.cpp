#include <cppunit/TestSuite.h>
#include <cppunit/TestCaller.h>
#include "UrchinCommon.h"
#include "UrchinAIEngine.h"

#include "CSGPolygonTest.h"
#include "AssertHelper.h"
using namespace urchin;

void CSGPolygonTest::simplifySquare()
{
    CSGPolygon<float> polygon("p1", {Point2<float>(0.0, 0.0), Point2<float>(0.0, 1.0), Point2<float>(1.0, 1.0), Point2<float>(1.0, 0.0)});

    CSGPolygon<float> simplifiedPolygon = polygon.simplify(1.0, 0.1);

    AssertHelper::assertUnsignedInt(simplifiedPolygon.getCwPoints().size(), 4);
    AssertHelper::assertPolygonFloatEquals(simplifiedPolygon.getCwPoints(), {Point2<float>(0.0, 0.0), Point2<float>(0.0, 1.0), Point2<float>(1.0, 1.0), Point2<float>(1.0, 0.0)});
}

void CSGPolygonTest::simplifyMergeTwoNearPoints()
{
    CSGPolygon<float> polygon("p1", {Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0), Point2<float>(1.05, 1.0), Point2<float>(1.0, 0.0)});

    CSGPolygon<float> simplifiedPolygon = polygon.simplify(1.0, 0.1);

    AssertHelper::assertUnsignedInt(simplifiedPolygon.getCwPoints().size(), 3);
    AssertHelper::assertPolygonFloatEquals(simplifiedPolygon.getCwPoints(), {Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0), Point2<float>(1.0, 0.0)});
}

void CSGPolygonTest::simplifyWithThreeNearPoints()
{
    CSGPolygon<float> polygon("p1", {Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0), Point2<float>(1.06, 1.0), Point2<float>(1.10, 1.0), Point2<float>(1.0, 0.0)});

    CSGPolygon<float> simplifiedPolygon = polygon.simplify(1.0, 0.1);

    AssertHelper::assertUnsignedInt(simplifiedPolygon.getCwPoints().size(), 4);
    AssertHelper::assertPolygonFloatEquals(simplifiedPolygon.getCwPoints(), {Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0), Point2<float>(1.10, 1.0), Point2<float>(1.0, 0.0)});
}

void CSGPolygonTest::simplifyMergeStartEndPoints()
{
    CSGPolygon<float> polygon("p1", {Point2<float>(1.05, 1.0), Point2<float>(1.0, 0.0), Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0)});

    CSGPolygon<float> simplifiedPolygon = polygon.simplify(1.0, 0.1);

    AssertHelper::assertUnsignedInt(simplifiedPolygon.getCwPoints().size(), 3);
    AssertHelper::assertPolygonFloatEquals(simplifiedPolygon.getCwPoints(), {Point2<float>(0.0, 0.0), Point2<float>(1.05, 1.0), Point2<float>(1.0, 0.0)});
}

void CSGPolygonTest::simplifyFlatTriangle()
{
    CSGPolygon<float> polygon("p1", {Point2<float>(0.0, 0.0), Point2<float>(1.0, 0.0), Point2<float>(0.0, -0.02)});

    CSGPolygon<float> simplifiedPolygon = polygon.simplify(0.99619469483 /*5 degrees*/, 0.0);

    AssertHelper::assertUnsignedInt(simplifiedPolygon.getCwPoints().size(), 0);
}

void CSGPolygonTest::simplifyUselessPoint()
{
    CSGPolygon<float> polygon("p1", {Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0), Point2<float>(1.0, 0.5), Point2<float>(1.0, 0.0)});

    CSGPolygon<float> simplifiedPolygon = polygon.simplify(0.99619469483 /*5 degrees*/, 0.0);

    AssertHelper::assertUnsignedInt(simplifiedPolygon.getCwPoints().size(), 3);
    AssertHelper::assertPolygonFloatEquals(simplifiedPolygon.getCwPoints(), {Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0), Point2<float>(1.0, 0.0)});
}

void CSGPolygonTest::simplifyTwoUselessPoints()
{
    CSGPolygon<float> polygon("p1", {Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0), Point2<float>(1.0, 0.5), Point2<float>(1.0, 0.3), Point2<float>(1.0, 0.0)});

    CSGPolygon<float> simplifiedPolygon = polygon.simplify(0.99619469483 /*5 degrees*/, 0.0);

    AssertHelper::assertUnsignedInt(simplifiedPolygon.getCwPoints().size(), 3);
    AssertHelper::assertPolygonFloatEquals(simplifiedPolygon.getCwPoints(), {Point2<float>(0.0, 0.0), Point2<float>(1.0, 1.0), Point2<float>(1.0, 0.0)});
}

CppUnit::Test *CSGPolygonTest::suite()
{
    CppUnit::TestSuite *suite = new CppUnit::TestSuite("CSGPolygonTest");

    suite->addTest(new CppUnit::TestCaller<CSGPolygonTest>("simplifySquare", &CSGPolygonTest::simplifySquare));
    suite->addTest(new CppUnit::TestCaller<CSGPolygonTest>("simplifyMergeTwoNearPoints", &CSGPolygonTest::simplifyMergeTwoNearPoints));
    suite->addTest(new CppUnit::TestCaller<CSGPolygonTest>("simplifyWithThreeNearPoints", &CSGPolygonTest::simplifyWithThreeNearPoints));
    suite->addTest(new CppUnit::TestCaller<CSGPolygonTest>("simplifyMergeStartEndPoints", &CSGPolygonTest::simplifyMergeStartEndPoints));
    suite->addTest(new CppUnit::TestCaller<CSGPolygonTest>("simplifyFlatTriangle", &CSGPolygonTest::simplifyFlatTriangle));
    suite->addTest(new CppUnit::TestCaller<CSGPolygonTest>("simplifyUselessPoint", &CSGPolygonTest::simplifyUselessPoint));
    suite->addTest(new CppUnit::TestCaller<CSGPolygonTest>("simplifyTwoUselessPoints", &CSGPolygonTest::simplifyTwoUselessPoints));

    return suite;
}
