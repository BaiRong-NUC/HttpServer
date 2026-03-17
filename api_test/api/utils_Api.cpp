#include "utils/utils.h"
#include "utils/buffer.h"
#include <fstream>
#include <cstdio>

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

// ---- GetFileContent ----

void TestGetFileContent_Normal()
{
    const char *tmp = "/tmp/utils_test_file.txt";
    const std::string expected = "hello utils\nbinary\x01\x02\x03";

    // 写临时文件（二进制模式，含不可见字节）
    {
        std::ofstream f(tmp, std::ios::binary);
        assert(f.is_open());
        f.write(expected.data(), expected.size());
    }

    Buffer buf;
    bool ok = Utils::GetFileContent(tmp, &buf);
    assert(ok);
    assert(buf.GetReadableSize() == expected.size());
    std::string got = buf.Read(buf.GetReadableSize());
    assert(got == expected);

    std::remove(tmp);
}

void TestGetFileContent_NotExist()
{
    Buffer buf;
    bool ok = Utils::GetFileContent("/tmp/__no_such_file_utils__.txt", &buf);
    assert(!ok);
    assert(buf.GetReadableSize() == 0);
}

void TestGetFileContent_EmptyFile()
{
    const char *tmp = "/tmp/utils_test_empty.txt";
    {
        std::ofstream f(tmp);
    }  // 创建空文件

    Buffer buf;
    bool ok = Utils::GetFileContent(tmp, &buf);
    assert(ok);
    assert(buf.GetReadableSize() == 0);

    std::remove(tmp);
}

// ---- WriteFileContent ----

void TestWriteFileContent_CreateAndReadBack()
{
    const char *tmp = "/tmp/utils_write_test.txt";
    const std::string expected = "write content\nabc\x01\x02";

    bool ok = Utils::WriteFileContent(tmp, expected);
    assert(ok);

    Buffer buf;
    ok = Utils::GetFileContent(tmp, &buf);
    assert(ok);
    assert(buf.GetReadableSize() == expected.size());
    std::string got = buf.Read(buf.GetReadableSize());
    assert(got == expected);

    std::remove(tmp);
}

void TestWriteFileContent_Overwrite()
{
    const char *tmp = "/tmp/utils_write_overwrite_test.txt";
    const std::string old_content = "old old old";
    const std::string new_content = "new";

    bool ok = Utils::WriteFileContent(tmp, old_content);
    assert(ok);
    ok = Utils::WriteFileContent(tmp, new_content);
    assert(ok);

    Buffer buf;
    ok = Utils::GetFileContent(tmp, &buf);
    assert(ok);
    std::string got = buf.Read(buf.GetReadableSize());
    assert(got == new_content);

    std::remove(tmp);
}

void TestWriteFileContent_InvalidPath()
{
    bool ok = Utils::WriteFileContent("/tmp/__utils_no_such_dir__/a.txt", "x");
    assert(!ok);
}

int main(int argc, char const *argv[])
{
    TestSplitBasic();
    TestSplitWithEmptyToken();
    TestSplitNoDelimiter();
    TestSplitMultiCharDelimiter();
    TestSplitIgnoreEmpty();

    TestGetFileContent_Normal();
    TestGetFileContent_NotExist();
    TestGetFileContent_EmptyFile();

    TestWriteFileContent_CreateAndReadBack();
    TestWriteFileContent_Overwrite();
    TestWriteFileContent_InvalidPath();

    cout << "utils_api test passed" << endl;
    return 0;
}
