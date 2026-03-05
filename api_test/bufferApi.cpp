#include "../include/buffer.h"

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
    assert(buf.GetWriteIndex() == 9);
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
    assert(line3 == "last");
}

int main()
{
    testBufferBasic();
    testBufferWriteRead();
    testBufferReadLine();
    std::cout << "All buffer tests passed!" << std::endl;
    return 0;
}
