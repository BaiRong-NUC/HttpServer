#include "utils/utils.h"

using namespace std;

void TestSplitBasic()
{
    auto result = Utils::Split("a,b,c", ",");
    assert(result.size() == 3);
    assert(result[0] == "a");
    assert(result[1] == "b");
    assert(result[2] == "c");
}

void TestSplitWithEmptyToken()
{
    auto result = Utils::Split("a,,c,", ",");
    assert(result.size() == 4);
    assert(result[0] == "a");
    assert(result[1].empty());
    assert(result[2] == "c");
    assert(result[3].empty());
}

void TestSplitNoDelimiter()
{
    auto result = Utils::Split("abc", ",");
    assert(result.size() == 1);
    assert(result[0] == "abc");
}

void TestSplitMultiCharDelimiter()
{
    auto result = Utils::Split("a--b----c", "--");
    assert(result.size() == 4);
    assert(result[0] == "a");
    assert(result[1] == "b");
    assert(result[2].empty());
    assert(result[3] == "c");
}

void TestSplitIgnoreEmpty()
{
    auto result = Utils::Split("a,,c,", ",", true);
    assert(result.size() == 2);
    assert(result[0] == "a");
    assert(result[1] == "c");
}

int main(int argc, char const *argv[])
{
    TestSplitBasic();
    TestSplitWithEmptyToken();
    TestSplitNoDelimiter();
    TestSplitMultiCharDelimiter();
    TestSplitIgnoreEmpty();

    cout << "utils_api test passed" << endl;
    return 0;
}
