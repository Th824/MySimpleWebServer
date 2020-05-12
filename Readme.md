# Readme

## How to build

``` bash
# project directory 
mkdir -p build && cd build
# generate makefile
cmake ..
# build porject
make 
```

## 简介

本项目参考了muduo库，基于多Reactor模型实现了一个小型网络库，该网络库主要面对的是使用Tcp连接进行高并发访问的场景，例如，可以作为一个Http静态服务器。在该网络库的基础上，可以自己定义应用层相关的回调函数来实现不同的应用层需求，例如，通过自定义http请求的解析和响应，就可以实现http服务器。

## 主要使用的技术
* Linux epoll 多路复用
* C++11 多线程编程
* C++11 智能指针

## 主要类介绍
1. TcpServer：顾名思义就是基于Tcp的服务器，负责管理Tcp连接（包装为TcpConnection类），包括连接的创建，销毁，注册TcpConnection的相关回调函数。
2. TcpConnection：封装一次Tcp连接的类，包括四个状态（连接中，已连接，断开中，已断开）。支持设置建立连接回调函数，写入完成回调函数，接收消息回调函数，关闭连接回调函数。
3. Epoll：对epoll调用的封装，提供更易使用的接口。
4. Channel：对句柄和注册事件及其回调函数的封装。
5. EventLoop：Reactor模式中的loop循环，启动后始终在循环中，等待epoll事件通知，在事件产生后，调用相关回调函数后继续回到loop循环。

## TODO
- [x] 定时器
- [x] 使用cmake替换make
- [x] 检查善后工作，特别是资源的释放和析构以及（系统调用）错误处理和日志记录
- [ ] 压力测试
  - [ ] 日志类的压力测试（进行C/IO和C++/IO的对比）
  - [x] 服务器的压力测试
    - [ ] 内存使用，泄漏
    - [ ] 请求成功数量（超时和错误）
- [ ] ~~Http1.1完整协议支持~~（特性太多，选择性支持）
  - [x] GET方法
  - [ ] POST方法
  - [ ] 头部选项
    - [ ] Cookie
    - [ ] Range
    - [ ] 
- [ ] 对线程库的封装（包括同步原语）
- [ ] 单元测试gtest
- [ ] udp 支持
- [ ] 异步日志可否使用无锁队列实现（抽象成一个消息队列，前端为生产者，后端为消费者）
- [ ] 线程池调度策略的支持


## 面试问题
* 异步日志前端写入是否要加锁，可否使用无锁编程
* 支持的最大并发数，瓶颈在哪里
* epoll两种触发模式的区别
* epoll与poll，select的不同

## 参考链接
* https://github.com/chenshuo/muduo
* https://github.com/yedf/handy
* https://github.com/linyacool/WebServer
* https://github.com/EZLippi/WebBench
* https://github.com/yhirose/cpp-httplib