#include "utils/buffer.h"
#include <cassert>
#include <iostream>
using namespace std;

// 测试基本读写和索引操作
void testBufferBasic()
{
    Buffer buf(16);
    assert(buf.GetWriteIndex() == 0);
    assert(buf.GetReadIndex() == 0);
    assert(buf.GetReadableSize() == 0);
    assert(buf.GetWriteableSize() == 16);

    buf.Write("hello", 5);
    assert(buf.GetWriteIndex() == 5);
    assert(buf.GetReadableSize() == 5);
    assert(buf.GetWriteableSize() == 11);

    std::string s = buf.Read(5);
    assert(s == "hello");
    assert(buf.GetReadIndex() == 5);
    assert(buf.GetReadableSize() == 0);

    buf.Clear();
    assert(buf.GetReadIndex() == 0);
    assert(buf.GetWriteIndex() == 0);
    std::cout << "testBufferBasic 通过!" << std::endl;
}

// 测试读写指针偏移和缓冲区回绕
void testBufferWriteRead()
{
    Buffer buf(8);
    buf.Write(std::string("abcdefg"));
    assert(buf.GetReadableSize() == 7);

    std::string out = buf.Read(3);
    assert(out == "abc");
    assert(buf.GetReadIndex() == 3);

    bool r1 = buf.MoveReadIndex(2);
    assert(r1);
    assert(buf.GetReadIndex() == 5);

    // 已读5字节，可读2字节，可写6字节，移动写指针2使其回绕到0
    bool r2 = buf.MoveWriteIndex(2);
    assert(r2);
    assert(buf.GetWriteIndex() == 0);
    std::cout << "testBufferWriteRead 通过! Size=" << buf.GetSize() << std::endl;
}

// 测试 ReadLine：结果不包含换行符
void testBufferReadLine()
{
    // 基本按行读取
    {
        Buffer buf(32);
        buf.Write("first line\nsecond line\nlast");

        std::string line1 = buf.ReadLine();
        assert(line1 == "first line");  // 不含 '\n'

        std::string line2 = buf.ReadLine();
        assert(line2 == "second line");  // 不含 '\n'

        // 最后一行没有换行符，include_newline=true(默认)时读出
        std::string line3 = buf.ReadLine();
        assert(line3 == "last");

        assert(buf.GetReadableSize() == 0);
    }

    // include_newline=false：没有换行符时返回空串，不消耗数据
    {
        Buffer buf(32);
        buf.Write("no newline here");

        std::string line = buf.ReadLine(false);
        assert(line == "");                   // 不读出
        assert(buf.GetReadableSize() == 15);  // 数据未被消耗

        // 再默认读取，应该读出全部
        std::string all = buf.ReadLine(true);
        assert(all == "no newline here");
        assert(buf.GetReadableSize() == 0);
    }

    // 空行（连续换行符）
    {
        Buffer buf(32);
        buf.Write("\nline\n\n");

        std::string l1 = buf.ReadLine();
        assert(l1 == "");  // 空行

        std::string l2 = buf.ReadLine();
        assert(l2 == "line");

        std::string l3 = buf.ReadLine();
        assert(l3 == "");  // 空行

        assert(buf.GetReadableSize() == 0);
    }

    // ReadLine 跨指针回绕
    {
        Buffer buf(16);
        buf.Write("123456789012", 12);  // 写12字节
        buf.Read(10);                   // 读走10字节，_readIndex=10
        buf.Write("ab\ncd", 5);         // 写5字节，_writeIndex 回绕

        std::string line = buf.ReadLine();
        assert(line == "12ab");  // 回绕后正确拼接
        assert(buf.GetReadableSize() == 2);
    }

    std::cout << "testBufferReadLine 通过!" << std::endl;
}

// 测试缓冲区扩容
void testBufferMakeLarger()
{
    Buffer buf(8);
    std::cout << "扩容前 Size=" << buf.GetSize() << std::endl;

    std::string bigData(100, 'x');
    bool writeResult = buf.Write(bigData);
    assert(writeResult);
    std::cout << "扩容后 Size=" << buf.GetSize() << std::endl;
    assert(buf.GetReadableSize() == 100);

    std::string out = buf.Read(100);
    assert(out == bigData);
    assert(buf.GetReadableSize() == 0);
    std::cout << "testBufferMakeLarger 通过!" << std::endl;
}

// 测试边界与异常情况
void testBufferError()
{
    // 1. 读取超容量 / nullptr
    {
        Buffer buf(8);
        buf.Write("12345678", 8);

        assert(!buf.Read(nullptr, 20));  // 超容量，失败
        char tmp[9] = {0};
        assert(buf.Read(tmp, 0));       // 读0字节，成功
        assert(!buf.Read(nullptr, 8));  // 合法长度但指针为空，失败

        buf.Read(tmp, 8);
        buf.Write("9", 1);
        assert(buf.Read(8) == "");  // 读取超出可读量，失败
        assert(buf.Read(1) == "9");
    }

    // 2. 空缓冲区读取
    {
        Buffer buf(8);
        assert(buf.Read(5).empty());
    }

    // 3. 指针回绕读写
    {
        Buffer buf(8);
        buf.Write("abcdefgh", 8);
        assert(buf.Read(4) == "abcd");
        buf.Write("ijkl", 4);  // 回绕写入
        assert(buf.Read(8) == "efghijkl");
    }

    // 4. 满写后触发扩容
    {
        Buffer buf(8);
        assert(buf.Write("abcdefgh", 8));
        assert(buf.Write("x", 1));  // 触发扩容
        assert(buf.Read(9) == "abcdefghx");
    }

    // 5. 扩容后数据完整性
    {
        Buffer buf(8);
        std::string bigData(50, 'A');
        buf.Write(bigData);
        assert(buf.Read(50) == bigData);
        assert(buf.GetReadableSize() == 0);
    }

    // 6. 读指针偏移超限
    {
        Buffer buf(8);
        buf.Write("1234", 4);
        assert(!buf.MoveReadIndex(10));
    }

    // 7. 写指针偏移超限
    {
        Buffer buf(8);
        assert(!buf.MoveWriteIndex(10));
    }

    std::cout << "testBufferError 通过!" << std::endl;
}

// 测试 Buffer 之间互写
void testBufferWriteBuffer()
{
    // 单向写入
    {
        Buffer buf1(8), buf2(8);
        buf1.Write("hello", 5);
        assert(buf2.Write(buf1));
        assert(buf2.GetReadableSize() == 5);
        assert(buf2.Read(5) == "hello");
    }

    // 拼接写入
    {
        Buffer buf1(8), buf2(8);
        buf1.Write("hello ", 6);
        buf2.Write("world", 5);
        buf1.Write(buf2);
        std::string out = buf1.Read(11);
        assert(out == "hello world");
    }

    std::cout << "testBufferWriteBuffer 通过!" << std::endl;
}

// 测试连续大量写入后一次性读出
void testBufferContinuousLargeInput()
{
    Buffer buf(8);
    std::string largeData(1000, 'x');
    for (int i = 0; i < 10; ++i)
    {
        assert(buf.Write(largeData));
        std::cout << "第 " << i + 1 << " 次写入完成, Size=" << buf.GetSize() << std::endl;
    }
    assert(buf.Read(10000) == std::string(10000, 'x'));
    std::cout << "testBufferContinuousLargeInput 通过!" << std::endl;
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
    std::cout << "\n所有测试通过!" << std::endl;
    return 0;
}
