/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* AUTHORS : Space Concordia 2014, Joseph
*
* TITLE : Utl-test.cpp
*
* DESCRIPTION : Contains Unit tests for the utility classes.
*
*----------------------------------------------------------------------------*/
#include "CppUTest/TestHarness.h"
#include "Date.h"
#include "SpaceString.h"


TEST_GROUP(SpaceStringTestGroup){
    void setup(){
    }
    void teardown(){
    }
};

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* NAME : SpaceStringTestGroup :: getUInt_returnCorrectIntegerValue
* 
* PURPOSE :
*
*-----------------------------------------------------------------------------*/
TEST(SpaceStringTestGroup, getUInt_returnCorrectIntegerValue){
    char cstr[4] = { 0x00, 0xEF, 0xCD, 0xAB };
    size_t expected = 2882400000LL;

    size_t actual = SpaceString::getUInt(cstr);

    CHECK_EQUAL(expected, actual);
}

TEST_GROUP(DateTestGroup){
    void setup(){
    }
    void teardown(){
    }
};

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* NAME : GetTimeT_returns_time_t
* 
* PURPOSE : Converts YYYYMMDD to time_t back and forth.
*
*-----------------------------------------------------------------------------*/
TEST(DateTestGroup, GetTimeT_returns_time_t){
    Date date(2014, 1, 1);

    time_t rawtime = date.GetTimeT();

    Date date_raw(rawtime);

    STRCMP_EQUAL(date.GetString(), date_raw.GetString()); 

}
