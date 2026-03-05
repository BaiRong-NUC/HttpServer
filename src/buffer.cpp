#include "../include/buffer.h"
#include "buffer.h"

// 构造函数
Buffer::Buffer(size_t size = BUFFER_DEFAULT_SIZE) : _readIndex(0), _writeIndex(0),
                                                    _size(size), _buffer(size) {}

// 获取当前写位置
uint64_t Buffer::GetWriteIndex() const
{
    return this->_writeIndex;
}

// 获取当前读位置
uint64_t Buffer::GetReadIndex() const
{
    return this->_readIndex;
}

// 清理缓冲区
void Buffer::Clear()
{
    this->_writeIndex = 0;
    this->_readIndex = 0;
}

// 获取当前可读数据大小
uint64_t Buffer::GetReadableSize() const
{
    return (this->_writeIndex - this->_readIndex + this->_size) % this->_size;
}

// 获取当前可写空间大小
uint64_t Buffer::GetWriteableSize() const
{
    return this->_size - this->GetReadableSize() - 1; // 留一个字节区分满和空
}

// 读指针偏移len
bool Buffer::MoveReadIndex(uint64_t len)
{
    if (len > this->GetReadableSize()) // 超过可读数据大小,无法移动
        return false;
    this->_readIndex = (this->_readIndex + len) % this->_size;
    return true;
}

// 写指针偏移len
bool Buffer::MoveWriteIndex(uint64_t len)
{
    if (len > this->GetWriteableSize()) // 超过可写空间大小,无法移动
        return false;
    this->_writeIndex = (this->_writeIndex + len) % this->_size;
    return true;
}

// 确保有size大小的可写空间
bool Buffer::_HaveSpace(uint64_t size)
{
    if (size > this->GetWriteableSize())
    {
        // 扩容
        }
    return true;
}