# RuOSPFd

20级信息类-6系 大三下计算机网络实验 提高层次实验——OSPF协议实现（基于Linux内核）


> 笑死，原本想写成守护进程的所以取名后有一个d；但最终并没有，不过鉴于尾缀d有种平衡的美感，就不改删了

### 目录结构

- `doc`：文档，包含一些实现时的注意事项
- `src`：实现代码

```
main.cpp				# 主函数，开关程序以及相应线程，初始化
setting.cpp/.h			# OSPF配置 + 全局通用变量/常量
common.h				# 通用头文件汇总，供.cpp直接引用，根据规约不在.h中引用
----------------
ospf_packet.cpp/.h		# 定义OSPF报文和LSA数据结构
interface.cpp/.h		# 定义OSPF协议中接口数据结构
neighbor.cpp/.h			# 定义OSPF协议中邻居数据结构
lsdb.cpp/.h				# 定义OSPF协议中连接状态数据库结构
----------------
package_manage.cpp/.h	# 负责处理报文收发逻辑
lsa_manage.cpp/.h		# 负责处理LSA的生成
routing.cpp/.h			# 负责路由计算，内核路由表更新与重置（提供route_manager）
----------------
retransmitter.cpp/.h	# 重传工具
```

- `try`：一些功能技术小试验

### 总体设计

<img src="https://i.postimg.cc/k44LcsRs/image.png" style="width:60%;" />

### 运行方式

根目录 `make` 编译，然后`sudo ./my_ospf` 执行 （啊目前还没写检测环境+动态配置，所以不手动调整`setting.cpp/.h` 的话只能在我自己的虚拟机上跑doge）；输入 `exit` 退出程序

**精简控制台输出：**去除 `setting.h` 的 `#define DEBUG`

调试环境中g++用的标准是C++14，但估计顶多用到C++11的功能为止

**仿真验证环境搭建：**华为ensp接入VMWare。在ensp搭建网络拓扑结构，然后连入VMWare的NAT虚拟网络适配器（`VMware Network Adapter VMnet8`）；同时配置虚拟机网络连接为NAT模式。（参考链接：[Ensp接入VMWare](https://blog.51cto.com/u_15107455/5173737)，其中端口映射的类型若为E，然后使用ensp路由器GE接口连接，似乎也没问题；不过感觉设置为一样最好）

<img src="https://i.postimg.cc/nhkZ8M5d/ensp.png" style="width:60%;" />

### 参考资料
- RFC 2328：有种规范的美感
- 《OSPF完全实现》及其源码：比较复杂完整，没有太多精力借鉴。[源码分享](https://pan.baidu.com/s/1tMO2Cf92Iy1mc2eP56qvlQ)，提取码：dz89 
- [pcyin/OSPF_Router](https://github.com/pcyin/OSPF_Router)：感恩，对照着写的（但读了遍有好多bug，感觉跑不起来的样子）