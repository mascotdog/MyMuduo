#include "CurrentThread.h"

 

namespace CurrentThread {
// __thread 与 thread_local 一致
// 表示每一个线程的，这里是全局变量，则每一个线程都有一份拷贝
__thread int t_cacheTid;

void cacheTid(){
    if(t_cacheTid == 0){
        // 通过linux系统调用获取当前线程的tid值
        t_cacheTid = static_cast<pid_t>(::syscall(SYS_gettid));
    }
}
} // namespace CurrentThread