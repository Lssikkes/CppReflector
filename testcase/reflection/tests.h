#pragma once

//@[Serializable,NetSerializable]
class Test
{
public:
	virtual void Print() {}  
};

//@[Serializable]
class TestCollection
{
public:
	~TestCollection();

	void AddTest(Test* test);
	template <class T> void ExecuteForAll(const T& v) { for (int i = 0; i < NumTests; i++) v(*Tests[i]); }
protected:
	int NumTests = 0;
	Test** Tests = 0;						//@<[ArraySize:NumTests,1]
	int b;
	int c;
};

//@[Serializable]
class TestPrintNumber : public Test
{
public:
	TestPrintNumber();
	TestPrintNumber(int a);

	virtual void Print();
	int Number = 0;
};
