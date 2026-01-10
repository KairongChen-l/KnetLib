#include <gtest/gtest.h>
#include "Buffer.h"
#include <cstring>
#include <string>

class BufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = new Buffer();
    }

    void TearDown() override {
        delete buffer;
    }

    Buffer* buffer;
};

// 测试基本构造
TEST_F(BufferTest, Constructor) {
    EXPECT_EQ(buffer->readableBytes(), 0);
    EXPECT_GE(buffer->writableBytes(), Buffer::kInitialSize);
    EXPECT_EQ(buffer->prependableBytes(), Buffer::kCheapPrepend);
}

// 测试 append 和 retrieve
TEST_F(BufferTest, AppendAndRetrieve) {
    const char* data = "Hello, World!";
    size_t len = strlen(data);
    
    buffer->append(data, len);
    EXPECT_EQ(buffer->readableBytes(), len);
    
    std::string result = buffer->retrieveAsString(len);
    EXPECT_EQ(result, data);
    EXPECT_EQ(buffer->readableBytes(), 0);
}

// 测试 append string
TEST_F(BufferTest, AppendString) {
    std::string data = "Test String";
    buffer->append(data);
    EXPECT_EQ(buffer->readableBytes(), data.length());
    
    std::string result = buffer->retrieveAllAsString();
    EXPECT_EQ(result, data);
}

// 测试多次 append
TEST_F(BufferTest, MultipleAppend) {
    buffer->append("Hello, ", 7);
    buffer->append("World!", 6);
    
    EXPECT_EQ(buffer->readableBytes(), 13);
    std::string result = buffer->retrieveAllAsString();
    EXPECT_EQ(result, "Hello, World!");
}

// 测试 retrieve 部分数据
TEST_F(BufferTest, PartialRetrieve) {
    buffer->append("Hello, World!", 13);
    
    buffer->retrieve(7);
    EXPECT_EQ(buffer->readableBytes(), 6);
    
    std::string result = buffer->retrieveAllAsString();
    EXPECT_EQ(result, "World!");
}

// 测试 findCRLF
TEST_F(BufferTest, FindCRLF) {
    buffer->append("Line1\r\nLine2\r\n", 14);
    
    const char* crlf = buffer->findCRLF();
    ASSERT_NE(crlf, nullptr);
    EXPECT_EQ(crlf - buffer->peek(), 5);  // "Line1" 的长度
}

// 测试 findEOL
TEST_F(BufferTest, FindEOL) {
    buffer->append("Line1\nLine2\n", 12);
    
    const char* eol = buffer->findEOL();
    ASSERT_NE(eol, nullptr);
    EXPECT_EQ(eol - buffer->peek(), 5);
}

// 测试网络字节序转换
TEST_F(BufferTest, NetworkByteOrder) {
    int32_t value = 0x12345678;
    buffer->appendInt32(value);
    
    EXPECT_EQ(buffer->readableBytes(), sizeof(int32_t));
    
    int32_t result = buffer->peekInt32();
    EXPECT_EQ(result, value);
    
    int32_t readValue = buffer->readInt32();
    EXPECT_EQ(readValue, value);
    EXPECT_EQ(buffer->readableBytes(), 0);
}

// 测试 prepend
TEST_F(BufferTest, Prepend) {
    buffer->append("World!", 6);
    buffer->prepend("Hello, ", 7);
    
    std::string result = buffer->retrieveAllAsString();
    EXPECT_EQ(result, "Hello, World!");
}

// 测试 prependInt32
TEST_F(BufferTest, PrependInt32) {
    buffer->append("data", 4);
    int32_t value = 0x12345678;
    buffer->prependInt32(value);
    
    EXPECT_EQ(buffer->readableBytes(), sizeof(int32_t) + 4);
    
    int32_t result = buffer->peekInt32();
    EXPECT_EQ(result, value);
}

// 测试 swap
TEST_F(BufferTest, Swap) {
    Buffer buffer1;
    Buffer buffer2;
    
    buffer1.append("Buffer1", 7);
    buffer2.append("Buffer2", 7);
    
    buffer1.swap(buffer2);
    
    EXPECT_EQ(buffer1.retrieveAllAsString(), "Buffer2");
    EXPECT_EQ(buffer2.retrieveAllAsString(), "Buffer1");
}

// 测试 clear (兼容 API)
TEST_F(BufferTest, Clear) {
    buffer->append("Test data", 9);
    EXPECT_GT(buffer->readableBytes(), 0);
    
    buffer->clear();
    EXPECT_EQ(buffer->readableBytes(), 0);
}

// 测试 size (兼容 API)
TEST_F(BufferTest, Size) {
    buffer->append("Test", 4);
    EXPECT_EQ(buffer->size(), 4);
}

// 测试 c_str (兼容 API)
TEST_F(BufferTest, CStr) {
    buffer->append("Test", 4);
    const char* str = buffer->c_str();
    EXPECT_EQ(memcmp(str, "Test", 4), 0);
}

// 测试自动扩容
TEST_F(BufferTest, AutoResize) {
    // 写入大量数据，测试自动扩容
    std::string largeData(10000, 'A');
    buffer->append(largeData);
    
    EXPECT_EQ(buffer->readableBytes(), 10000);
    std::string result = buffer->retrieveAllAsString();
    EXPECT_EQ(result, largeData);
}

// 测试 retrieveUntil
TEST_F(BufferTest, RetrieveUntil) {
    buffer->append("Hello, World!", 13);
    
    const char* comma = buffer->findCRLF();
    if (!comma) {
        // 如果没有 CRLF，找逗号
        const char* data = buffer->peek();
        comma = static_cast<const char*>(memchr(data, ',', buffer->readableBytes()));
    }
    
    if (comma) {
        buffer->retrieveUntil(comma + 1);
        std::string result = buffer->retrieveAllAsString();
        EXPECT_EQ(result, " World!");
    }
}

