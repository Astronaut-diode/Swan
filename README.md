# Swan
可以私聊以及群聊的即时通讯系统。

## 开发过程中查询的一些方法以及问题。
模板类别写实现文件。
delete的目标永远都是new出来的堆空间上的内容，如果是指向栈空间上的指针，是不需要手动进行释放的。
<sys/prctl>::prctl(PR_SET_NAME, char *buffer);  用于设置线程的名字。
<unistd.h>getcwd(char *buffer, int size);  获取进程的执行目录。失败则返回空指针。
<unistd.h>  int access(char *path, int mode);  mode为0时判断path是否存在。
<sys/stat.h>  int mkdir (const char *__path, __mode_t __mode);  创建path文件夹，，mode是权限。
<sys/time.h>  struct timeval now;  gettimeofday(&now, nullptr);  // 获取当前的时间。