#include "../../include/buffer.h"
using namespace std;
void testBufferBasic()
{
    Buffer buf(16);
    assert(buf.GetWriteIndex() == 0);
    assert(buf.GetReadIndex() == 0);
    assert(buf.GetReadableSize() == 0);
    assert(buf.GetWriteableSize() == 16);
    // cout << "===========" << endl;
    buf.Write("hello", 5);
    assert(buf.GetWriteIndex() == 5);
    assert(buf.GetReadableSize() == 5);
    assert(buf.GetWriteableSize() == 11);
    // cout << "===========" << endl;
    std::string s = buf.Read(5);
    assert(s == "hello");
    assert(buf.GetReadIndex() == 5);
    assert(buf.GetReadableSize() == 0);
    // cout << "===========" << endl;
    buf.Clear();
    assert(buf.GetReadIndex() == 0);
    assert(buf.GetWriteIndex() == 0);
}

void testBufferWriteRead()
{
    Buffer buf(8);
    std::string data = "abcdefg";
    buf.Write(data);
    assert(buf.GetReadableSize() == 7);
    std::string out = buf.Read(3);
    assert(out == "abc");
    assert(buf.GetReadIndex() == 3);
    buf.MoveReadIndex(2);
    assert(buf.GetReadIndex() == 5);
    buf.MoveWriteIndex(2);
    assert(buf.GetWriteIndex() == 0);
    cout << "size: " << buf.GetSize() << endl;
}

void testBufferReadLine()
{
    Buffer buf(32);
    buf.Write("first line\nsecond line\nlast");
    std::string line1 = buf.ReadLine();
    assert(line1 == "first line");
    std::string line2 = buf.ReadLine();
    assert(line2 == "second line");
    std::string line3 = buf.ReadLine();
    // std::cout << "line3: " << line3 << std::endl;
    assert(line3 == "last");
}

void testBufferMakeLarger()
{
    // 测试Buffer扩容
    Buffer buf(8); // 初始容量8
    std::cout << "Begin Size: " << buf.GetSize() << std::endl;
    std::string bigData(100, 'x'); // 100字节数据
    bool writeResult = buf.Write(bigData);
    assert(writeResult);
    std::cout << "After Write Size: " << buf.GetSize() << std::endl;
    assert(buf.GetReadableSize() == 100);
    // std::cout << "GetReadableSize after write: " << buf.GetReadableSize() << std::endl;
    // std::cout << "GetWriteableSize after write: " << buf.GetWriteableSize() << std::endl;
    // std::cout << "GetReadIndex after write: " << buf.GetReadIndex() << std::endl;
    // std::cout << "GetWriteIndex after write: " << buf.GetWriteIndex() << std::endl;
    std::string out = buf.Read(100);
    assert(out == bigData);
    // std::cout << "GetReadableSize after read: " << buf.GetReadableSize() << std::endl;
    // std::cout << "GetWriteableSize after read: " << buf.GetWriteableSize() << std::endl;
    // std::cout << "GetReadIndex after read: " << buf.GetReadIndex() << std::endl;
    // std::cout << "GetWriteIndex after read: " << buf.GetWriteIndex() << std::endl;
    // std::cout << "Buffer扩容测试通过!" << std::endl;

    std::cout << "All buffer tests passed!" << std::endl;
}

// 测试buffer遇到的极端情况
void testBufferError()
{
    // 1. 超容量读取
    Buffer buf(8);
    std::string data = "12345678";
    buf.Write(data);                         // 写满
    bool readResult = buf.Read(nullptr, 20); // 读取超容量
    char buffer[9] = {0};
    bool readResult2 = buf.Read(buffer, 0);  // 读取0字节
    bool readResult3 = buf.Read(nullptr, 8); // 读取合法容量,但是传入的指针为nullptr,应该失败
    assert(!readResult);                     // 应该失败
    assert(readResult2);                     // 读取0字节应该成功
    assert(!readResult3);                    // 读取合法容量但指针为nullptr应该失败
    buf.Read(buffer, 8);                     // 读取所有数据
    cout << buffer << endl;
    buf.Write("9", 1);
    std::string out = buf.Read(8); // 读取的数据比现有的数据大,读取失败
    assert(out == "");
    out = buf.Read(1); // 读取合法容量,应该成功
    cout << out << endl;

    // 2. 空读
    Buffer emptyBuf(8);
    std::string empty = emptyBuf.Read(5);
    assert(empty.empty());

    // 3. 指针回绕
    Buffer wrapBuf(8);
    wrapBuf.Write("abcdefgh", 8);           // 写满
    std::string wrapRead = wrapBuf.Read(4); // 读出一半
    assert(wrapRead == "abcd");
    wrapBuf.Write("ijkl", 4);                // 再写入，指针回绕
    std::string wrapRead2 = wrapBuf.Read(8); // 读剩余数据
    assert(wrapRead2 == "efghijkl");

    // 4. 满写
    Buffer fullBuf(8);
    bool fullWrite = fullBuf.Write("abcdefgh", 8);
    assert(fullWrite);
    bool overWrite = fullBuf.Write("x", 1); // 再写入1字节,应该触发扩容
    assert(overWrite);
    std::string fullRead = fullBuf.Read(9); // 读出所有数据
    assert(fullRead == "abcdefghx");

    // 5. 扩容后数据完整性
    Buffer bigBuf(8);
    std::string bigData(50, 'A');
    bigBuf.Write(bigData);
    std::string bigRead = bigBuf.Read(50);
    assert(bigRead == bigData);
    assert(bigBuf.GetReadableSize() == 0);

    // 6. 读指针移动超限
    Buffer moveBuf(8);
    moveBuf.Write("1234", 4);
    bool moveRead = moveBuf.MoveReadIndex(10);
    assert(!moveRead);

    // 7. 写指针移动超限
    Buffer moveWriteBuf(8);
    bool moveWrite = moveWriteBuf.MoveWriteIndex(10);
    assert(!moveWrite);

    std::cout << "Buffer极端情况测试通过!" << std::endl;
}

// 测试buffer之间的写入
void testBufferWriteBuffer()
{
    Buffer buf1(8);
    Buffer buf2(8);
    buf1.Write("hello", 5);
    bool writeResult = buf2.Write(buf1);
    assert(writeResult);
    assert(buf2.GetReadableSize() == 5);
    std::string out = buf2.Read(5);
    assert(out == "hello");

    buf1.Clear();
    buf2.Clear();
    buf1.Write("hello ", 6);
    buf2.Write("world", 5);
    buf1.Write(buf2);
    std::string out2 = buf1.Read(11);
    cout << out2 << endl;
    assert(out2 == "hello world");
    std::cout << "Buffer之间写入测试通过!" << std::endl;
}

// 测试连续输入大量数据
void testBufferContinuousLargeInput()
{
    Buffer buf(8);
    std::string largeData(1000, 'x');
    for (int i = 0; i < 10; ++i)
    {
        bool writeResult = buf.Write(largeData);
        assert(writeResult);
        std::cout << "第 " << i + 1 << " 次写入完成,当前缓冲区大小: " << buf.GetSize() << std::endl;
    }
    std::string out = buf.Read(10000);
    assert(out == std::string(10000, 'x'));
    std::cout << "连续输入大量数据测试通过!" << std::endl;
}
int main()
{
    testBufferBasic();
    testBufferWriteRead();
    testBufferReadLine();
    testBufferMakeLarger();
    testBufferError();
    testBufferWriteBuffer();
    testBufferContinuousLargeInput();
    return 0;
}
