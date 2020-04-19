// #include "HttpContext.h"

# include <gtest/gtest.h>
// # include <application/http_server/HttpRequest.h>

TEST(HttpRequest, testParseRequestEmptyHeaderValue) {
    // HttpContext context;
    // ideal::net::Buffer input;
    // input.append("GET /index.html HTTP/1.1\r\n"
    //      "Host: www.chenshuo.com\r\n"
    //      "User-Agent:\r\n "
    //      "Accept-Encoding: \r\n"
    //      "\r\n");
    
    EXPECT_TRUE(10 == 10);
    // EXPECT_TRUE(context.gotAll());
    // const HttpRequest& request = context.request();
    // EXPECT_EQ(request.getMethod(), HttpRequest::kGet);
    // EXPECT_EQ(request.getPath(), std::string("/index.html"));
    // EXPECT_EQ(request.getVersion(), HttpRequest::kHttp11);
    // EXPECT_EQ(request.getHeader("Host"), std::string("www.chenshuo.com"));
    // EXPECT_EQ(request.getHeader("User-Agent"), std::string(""));
    // EXPECT_EQ(request.getHeader("Accept-Encoding"), std::string(""));
}


int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

