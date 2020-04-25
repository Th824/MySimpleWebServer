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



## Feature



## TODO
- [x] 定时器
- [x] 使用cmake替换make
- [ ] 检查善后工作，特别是资源的释放和析构
- [ ] 单元测试gtest
- [ ] 压力测试
  - [ ] 日志类的压力测试（进行C/IO和C++/IO的对比）
  - [ ] 服务器的压力测试
    - [ ] 内存使用，泄漏
    - [ ] 请求成功数量（超时和错误）
- [ ] 对线程库的封装（包括同步原语）
- [ ] udp 支持
- [ ] 异步日志可否使用无锁队列实现（抽象成一个消息队列，前端为生产者，后端为消费者）
- [ ] 线程池调度策略的支持
- [ ] Http1.1完整协议支持


## 项目目录结构

```

```

## 面试问题
* 异步日志前端写入是否要加锁，可否使用无锁编程
* 支持的最大并发数，瓶颈在哪里
