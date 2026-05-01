#include <gtest/gtest.h>

#define TCP_CHAT_TESTING
#include "../iterative_tcp_chat.cpp"

TEST(ServerChat, DefaultConstructor) {
    ServerChat chat;

    EXPECT_TRUE(chat.Empty());
    EXPECT_EQ(chat.Size(), 0);
}

TEST(ServerChat, ExtractMsgs_SmallMessageNegative) {
    ServerChat chat;
    std::string msg("Hello");

    auto res = chat.ExtractMsgs(msg, msg.size());

    EXPECT_TRUE(chat.Empty());
    EXPECT_EQ(res, 0);
}

TEST(ServerChat, ExtractMsgs_SmallMessagePositive) {
    ServerChat chat;
    std::string msg("Hello\n");

    auto res = chat.ExtractMsgs(msg, msg.size());

    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(res, msg.size());
}

TEST(ServerChat, ExtractMsgs_MessageBoundPositive) {
    ServerChat chat;
    std::string msg(MSG::SIZE, 'a');
    msg[msg.size() - 1] = '\n';

    auto res = chat.ExtractMsgs(msg, msg.size());

    EXPECT_FALSE(chat.Empty());
    EXPECT_EQ(res, msg.size());
}

TEST(ServerChat, ExtractMsgs_MessageBoundNegative) {
    ServerChat chat;
    std::string msg(MSG::SIZE + 1, 'a');
    msg[msg.size() - 1] = '\n';

    auto res = chat.ExtractMsgs(msg, msg.size());

    EXPECT_TRUE(chat.Empty());
    EXPECT_EQ(res, msg.size());
}

TEST(ServerChat, ExtractMsgs_MultipleMessages) {
    ServerChat chat;
    std::string msg(10, '\n');

    chat.ExtractMsgs(msg, msg.size());

    EXPECT_EQ(chat.Size(), 10);
}

TEST(ServerChat, ExtractMsgs_MaxNumber) {
    ServerChat chat;
    std::string msg(MSG::NUM * 2, '\n');

    chat.ExtractMsgs(msg, msg.size());

    EXPECT_EQ(chat.Size(), MSG::NUM);
}

TEST(ServerChat, MakeMsgs_Empty) {
    ServerChat chat;

    auto res = chat.MakeMsgs();

    ASSERT_FALSE(res.has_value());
}

TEST(ServerChat, MakeMsgs_Positive) {
    ServerChat chat;
    std::string msg = "one\n";

    chat.ExtractMsgs(msg, msg.size());
    auto res = chat.MakeMsgs();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->stamp_, 1);
    EXPECT_EQ(res->data_, msg);
}

TEST(ServerChat, MakeMsgs_MultipleMessagesPositive) {
    ServerChat chat;
    std::string msg = "one\ntwo\nthree\n";

    chat.ExtractMsgs(msg, msg.size());
    auto res = chat.MakeMsgs();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->stamp_, 3);
    EXPECT_EQ(res->data_, msg);
}

TEST(ServerChat, MakeMsgs_DropMessage) {
    ServerChat chat;
    std::string msg1("hello\n");
    std::string msg2(MSG::SIZE * 2, 'a');

    msg2[msg2.size() - 1] = '\n';
    msg2 += msg1;

    chat.ExtractMsgs(msg2, msg2.size());
    auto res = chat.MakeMsgs();

    ASSERT_FALSE(!res);
    EXPECT_EQ(res->data_, msg1);
}

TEST(ServerChat, MakeMsgs_SameStamp) {
    ServerChat chat;
    std::string msg = "one\n";

    chat.ExtractMsgs(msg, msg.size());
    chat.MakeMsgs();
    auto res = chat.MakeMsgs(1);

    ASSERT_FALSE(res.has_value());
}

TEST(ServerChat, MakeMsgs_GetByStamp) {
    ServerChat chat;
    std::string msg1 = "one\ntwo\nthree\n";
    std::string msg2 = "four\nfive\nsix\nseven\neight\nnine\n";

    chat.ExtractMsgs(msg1, msg1.size());
    chat.ExtractMsgs(msg2, msg2.size());
    auto res = chat.MakeMsgs(3);

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->stamp_, 9);
    EXPECT_EQ(res->data_, msg2);
}
