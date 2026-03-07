#include "../../include/log.h"
#include "../../include/buffer.h"
using namespace std;
// 测试日志输出
void testLog()
{
    LOG(INFO, "This is an info message.");
    LOG(ERROR, "This is an error message.");
    LOG(WARNING, "This is a warning message.");

    Buffer buf(8);
    std::string largeData(1000, 'x');
    for (int i = 0; i < 10; ++i)
    {
        bool writeResult = buf.Write(largeData);
        assert(writeResult);
        LOG(INFO, "第 " << i + 1 << " 次写入完成,当前缓冲区大小: " << buf.GetSize());
    }
    std::string out = buf.Read(10000);
    assert(out == std::string(10000, 'x'));
    LOG(INFO, "连续输入大量数据测试通过!");
}

void testLogApi()
{
    cout << GetCurrentTime() << endl;
}

int main(int argc, char const *argv[])
{
    testLog();
    // testLogApi();
    return 0;
}
