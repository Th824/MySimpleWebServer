# include <thread>
# include <iostream>
# include <functional>

using namespace std;

void func() {
  cout << "new thread" << endl;
}

class test {
public:
  test() {cout << "construct" << endl;}
  ~test() {}
  void run() {
    thread_ = move(thread(bind(&func)));
  }

  void stop() {
    thread_.join();
  }
private:
  thread thread_;
};

int main() {
  test a;
  a.run();
  a.stop();
}