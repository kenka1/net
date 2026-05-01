#include <gtest/gtest.h>

#include <string>

#define TCP_CHAT_TESTING
#include "../fork_tcp_chat.cpp"

TEST(ServerChat, ParseBufferPositive) {
    std::string str(99, 'a');
    str += '\n';
    auto res = ParseBuffer(str);

    EXPECT_NE(res, std::nullopt);
    EXPECT_EQ(*res, 100);
}

TEST(ServerChat, ParseBufferNegative) {
    std::string str(10, 'a');
    auto res = ParseBuffer(str);

    EXPECT_EQ(res, std::nullopt);
}

TEST(ServerChat, DefaultConstructor) {
    ServerChat chat;

    EXPECT_TRUE(chat.Empty());
    EXPECT_EQ(chat.Size(), 0);
    EXPECT_EQ(chat.GetStart(), 0);
}

TEST(ServerChat, ExtractMsgs_SmallMessageNegative) {
    ServerChat chat;
    std::string msg("Hello");

    auto res = chat.ExtractMsgs(msg, msg.size());

    EXPECT_TRUE(chat.Empty());
    EXPECT_EQ(chat.Size(), 0);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res, 0);
}

TEST(ServerChat, ExtractMsgs_SmallMessagePositive) {
    ServerChat chat;
    std::string msg("Hello\n");

    auto res = chat.ExtractMsgs(msg, msg.size());

    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(chat.Size(), 1);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res, msg.size());
}

TEST(ServerChat, ExtractMsgs_MessageBoundPositive) {
    ServerChat chat;
    std::string msg(MSG::SIZE, 'a');
    msg[msg.size() - 1] = '\n';

    auto res = chat.ExtractMsgs(msg, msg.size());

    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(chat.Size(), 1);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res, msg.size());
}

TEST(ServerChat, ExtractMsgs_MessageBoundNegative) {
    ServerChat chat;
    std::string msg(MSG::SIZE + 1, 'a');
    msg[msg.size() - 1] = '\n';

    auto res = chat.ExtractMsgs(msg, msg.size());

    EXPECT_TRUE(chat.Empty());
    EXPECT_EQ(chat.Size(), 0);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res, msg.size());
}

TEST(ServerChat, ExtractMsgs_MultipleMessages) {
    ServerChat chat;
    std::string msg(MSG::NUM / 2, '\n');

    chat.ExtractMsgs(msg, msg.size());

    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(chat.Size(), MSG::NUM / 2);
    EXPECT_EQ(chat.GetStart(), 0);
}

TEST(ServerChat, ExtractMsgs_MaxNumber) {
    ServerChat chat;
    std::string msg(MSG::NUM + 1, '\n');

    chat.ExtractMsgs(msg, msg.size());

    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(chat.Size(), MSG::NUM);
    EXPECT_EQ(chat.GetStart(), 1);
}

TEST(ServerChat, MakeMsgs_Empty) {
    ServerChat chat;

    auto res = chat.MakeMsgs();

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(chat.Size(), 0);
    EXPECT_EQ(chat.GetStart(), 0);
}

TEST(ServerChat, MakeMsgs_Positive) {
    ServerChat chat;
    std::string msg = "one\n";

    chat.ExtractMsgs(msg, msg.size());
    auto res = chat.MakeMsgs();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(chat.Size(), 1);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res->stamp_, 1);
    EXPECT_EQ(res->data_, msg);
}

TEST(ServerChat, MakeMsgs_MultipleMessagesPositive) {
    ServerChat chat;
    std::string msg;
    for (std::size_t i = 0; i < MSG::NUM / 2; i++) {
        msg.append(std::to_string(i) + '\n');
    }

    chat.ExtractMsgs(msg, msg.size());
    auto res = chat.MakeMsgs();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(chat.Size(), MSG::NUM / 2);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res->stamp_, MSG::NUM / 2);
    EXPECT_EQ(res->data_, msg);
}

TEST(ServerChat, MakeMsgs_HalfData) {
    ServerChat chat;
    std::array<char, MSG::BUF> buf{};
    std::size_t buf_size = 0;

    std::memcpy(buf.data(), "hel", 3);
    buf_size = 3;
    std::size_t parsed = chat.ExtractMsgs(buf, buf_size);
    EXPECT_EQ(parsed, 0);
    EXPECT_TRUE(chat.Empty());

    std::memcpy(buf.data() + buf_size, "lo\n", 3);
    buf_size += 3;
    parsed = chat.ExtractMsgs(buf, buf_size);
    EXPECT_EQ(parsed, 6);

    auto res = chat.MakeMsgs();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->stamp_, 1);
    EXPECT_EQ(res->data_, "hello\n");
}

TEST(ServerChat, MakeMsgs_DropMessage) {
    ServerChat chat;
    std::string msg1("hello\n");
    std::string msg2(MSG::SIZE * 2, 'a');

    msg2[msg2.size() - 1] = '\n';
    msg2 += msg1;

    chat.ExtractMsgs(msg2, msg2.size());
    auto res = chat.MakeMsgs();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(chat.Size(), 1);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res->stamp_, 1);
    EXPECT_EQ(res->data_, msg1);
}

TEST(ServerChat, MakeMsgs_SameStamp) {
    ServerChat chat;
    std::string msg = "one\n";

    chat.ExtractMsgs(msg, msg.size());
    chat.MakeMsgs();
    auto res = chat.MakeMsgs(1);

    ASSERT_FALSE(res.has_value());
    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(chat.Size(), 1);
    EXPECT_EQ(chat.GetStart(), 0);
}

TEST(ServerChat, MakeMsgs_GetByStamp) {
    ServerChat chat;
    std::string msg1, msg2;
    for (std::size_t i = 0; i < MSG::NUM / 2; i++) {
        msg1.append(std::to_string(i) + '\n');
    }
    for (std::size_t i = MSG::NUM / 2; i < MSG::NUM; i++) {
        msg2.append(std::to_string(i) + '\n');
    }

    chat.ExtractMsgs(msg1, msg1.size());
    chat.ExtractMsgs(msg2, msg2.size());
    auto res = chat.MakeMsgs(MSG::NUM / 2);

    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(chat.Size(), MSG::NUM);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res->stamp_, MSG::NUM);
    EXPECT_EQ(res->data_, msg2);
}

TEST(ServerChat, MakeMsgs_Overflow) {
    ServerChat chat;
    std::string msg1, msg2;
    for (std::size_t i = 0; i < MSG::NUM; i++) {
        msg1.append(std::to_string(i) + '\n');
    }
    for (std::size_t i = MSG::NUM; i < MSG::NUM * 2; i++) {
        msg2.append(std::to_string(i) + '\n');
    }

    chat.ExtractMsgs(msg1, msg1.size());
    chat.ExtractMsgs(msg2, msg2.size());
    auto res = chat.MakeMsgs();

    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(chat.Size(), MSG::NUM);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res->stamp_, MSG::NUM * 2);
    EXPECT_EQ(res->data_, msg2);
}

TEST(ServerChat, MakeMsgs_OverflowByStamp) {
    ServerChat chat;
    std::string msg1, msg2, msg3;
    for (std::size_t i = 0; i < MSG::NUM; i++) {
        msg1.append(std::to_string(i) + '\n');
    }
    for (std::size_t i = MSG::NUM; i < MSG::NUM + MSG::NUM / 2; i++) {
        msg2.append(std::to_string(i) + '\n');
    }

    for (std::size_t i = MSG::NUM + MSG::NUM / 2; i < MSG::NUM * 2; i++) {
        msg3.append(std::to_string(i) + '\n');
    }

    chat.ExtractMsgs(msg1, msg1.size());
    chat.ExtractMsgs(msg2, msg2.size());
    chat.ExtractMsgs(msg3, msg3.size());
    auto res = chat.MakeMsgs(MSG::NUM + MSG::NUM / 2);

    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(chat.Size(), MSG::NUM);
    EXPECT_EQ(chat.GetStart(), 0);
    EXPECT_EQ(res->stamp_, MSG::NUM * 2);
    EXPECT_EQ(res->data_, msg3);
}
