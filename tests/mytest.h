#include <cxxtest/TestSuite.h>

#include <Nightlight.h>

class MyTestSuite : public CxxTest::TestSuite 
{
public:
    void testAddition( void )
    {
    	Nightlight n(12345);

        TS_ASSERT( 1 + 1 > 1 );
        TS_ASSERT_EQUALS( 1 + 1, 2 );
    }
};

