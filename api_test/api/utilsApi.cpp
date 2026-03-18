#include "utils/utils.h"
#include "utils/buffer.h"
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

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

// ---- UrlEncode ----

void TestUrlEncode_UnreservedChars()
{
    const std::string input = "abcXYZ012-_.~";
    const std::string encoded = Utils::UrlEncode(input);
    assert(encoded == input);
}

void TestUrlEncode_SpaceMode()
{
    const std::string input = "a b";
    const std::string plus_mode = Utils::UrlEncode(input, true);
    const std::string percent_mode = Utils::UrlEncode(input, false);

    assert(plus_mode == "a+b");
    assert(percent_mode == "a%20b");
}

void TestUrlEncode_SpecialChars()
{
    const std::string encoded = Utils::UrlEncode("/?:@&=+$,#");
    assert(encoded == "%2F%3F%3A%40%26%3D%2B%24%2C%23");
}

// ---- UrlDecode ----

void TestUrlDecode_NormalPercent()
{
    const std::string decoded = Utils::UrlDecode("a%20b%2Fc%3F");
    assert(decoded == "a b/c?");
}

void TestUrlDecode_PlusMode()
{
    const std::string space_mode = Utils::UrlDecode("a+b+c", true);
    const std::string keep_plus_mode = Utils::UrlDecode("a+b+c", false);

    assert(space_mode == "a b c");
    assert(keep_plus_mode == "a+b+c");
}

void TestUrlDecode_InvalidPercentKeepRaw()
{
    // 非法 % 序列应按普通字符保留
    const std::string decoded = Utils::UrlDecode("abc%2G%Z1%");
    assert(decoded == "abc%2G%Z1%");
}

void TestUrlDecode_RoundTrip()
{
    const std::string raw = "/login?q=hello world&x=1+2";
    const std::string encoded = Utils::UrlEncode(raw, false);
    const std::string decoded = Utils::UrlDecode(encoded, false);
    assert(decoded == raw);
}

// ---- GetStatusMessage ----

void TestGetStatusMessage_CommonStatus()
{
    assert(Utils::GetStatusMessage(200) == "OK");
    assert(Utils::GetStatusMessage(404) == "Not Found");
    assert(Utils::GetStatusMessage(501) == "Not Implemented");
}

void TestGetStatusMessage_EdgeCases()
{
    assert(Utils::GetStatusMessage(100) == "Continue");
    assert(Utils::GetStatusMessage(511) == "Network Authentication Required");
}

void TestGetStatusMessage_UnknownStatus()
{
    assert(Utils::GetStatusMessage(999) == "Unknown Status");
    assert(Utils::GetStatusMessage(-1) == "Unknown Status");
    assert(Utils::GetStatusMessage(0) == "Unknown Status");
}

// ---- GetMimeType ----

void TestGetMimeType_Common()
{
    assert(Utils::GetMimeType("a.txt") == "text/plain");
    assert(Utils::GetMimeType("b.html") == "text/html");
    assert(Utils::GetMimeType("c.jpg") == "image/jpeg");
    assert(Utils::GetMimeType("d.png") == "image/png");
    assert(Utils::GetMimeType("e.json") == "application/json");
}

void TestGetMimeType_Unknown()
{
    assert(Utils::GetMimeType("file.unknownext") == "application/octet-stream");
    assert(Utils::GetMimeType("noext") == "application/octet-stream");
    assert(Utils::GetMimeType(".hiddenfile") == "application/octet-stream");
    assert(Utils::GetMimeType("/path/to/file.with.many.dots.abc") == "application/octet-stream");
}

// ---- IsDirectory ----

void TestIsDirectory()
{
    // 1. 当前目录一定是目录
    assert(Utils::IsDirectory("."));
    // 2. /tmp 一般存在且是目录
    assert(Utils::IsDirectory("/tmp"));
    // 3. /etc/passwd 一定是文件不是目录
    assert(!Utils::IsDirectory("/etc/passwd"));
    // 4. /dev/null 是字符设备不是目录
    assert(!Utils::IsDirectory("/dev/null"));
    // 5. 不存在的路径
    assert(!Utils::IsDirectory("/no_such_dir_1234567890"));
    // 6. 新建临时目录
    const char *tmpdir = "/tmp/utils_test_dir";
    mkdir(tmpdir, 0700);
    assert(Utils::IsDirectory(tmpdir));
    rmdir(tmpdir);
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

    TestUrlEncode_UnreservedChars();
    TestUrlEncode_SpaceMode();
    TestUrlEncode_SpecialChars();

    TestUrlDecode_NormalPercent();
    TestUrlDecode_PlusMode();
    TestUrlDecode_InvalidPercentKeepRaw();
    TestUrlDecode_RoundTrip();

    TestGetStatusMessage_CommonStatus();
    TestGetStatusMessage_EdgeCases();
    TestGetStatusMessage_UnknownStatus();

    TestGetMimeType_Common();
    TestGetMimeType_Unknown();

    TestIsDirectory();

    // std::cout << Utils::UrlEncode("/login?user=hello&passwd=123") << std::endl;

    cout << "utils_api test passed" << endl;
    return 0;
}
