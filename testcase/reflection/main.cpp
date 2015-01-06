#include <stdio.h>
#include "tests.h"

int main(int argc, char** argv)
{
	TestCollection testCollection;
	testCollection.AddTest(new TestPrintNumber(10));
	testCollection.AddTest(new TestPrintNumber(20));
	testCollection.AddTest(new TestPrintNumber(30));
	testCollection.ExecuteForAll([](Test& test) { test.Print(); });

	//cppex::reflection::
	return 0;
}
