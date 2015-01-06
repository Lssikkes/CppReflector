#include "tests.h"
#include <stdio.h>
#include <string.h>

TestCollection::~TestCollection()
{
	for (int i = 0; i < NumTests; i++)
		delete Tests[i];

	delete[] Tests;
}
 
void TestCollection::AddTest(Test* test)
{
	auto OldTests = Tests;
	Tests = new Test*[NumTests+1];
	memcpy(Tests, OldTests, NumTests * sizeof(Test*));
	Tests[NumTests] = test;
	delete [] OldTests;
	++NumTests;
}

void TestPrintNumber::Print()
{
	printf("Number: %d\n", Number);
}

TestPrintNumber::TestPrintNumber(int a) : Number(a)
{

}

TestPrintNumber::TestPrintNumber()
{

}
