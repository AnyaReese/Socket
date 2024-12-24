# 基于Socket接口实现自定义协议通信

> 实验名称： 基于Socket接口实现自定义协议通信</br>
> 实验类型： 编程实验</br>
> 同组学生： 林子昕 3220103784，佟昕 3220101844</br>
> 实验平台： Linux，MacOS 操作系统

<!-- 客户端和服务端的代码分别在 client 和 server 目录

- `make` 编译，生成的文件在 build 目录
- `make run` 运行
- `make clean` 清理编译生成的文件 -->

# 实验报告

## 一、实验目的
- 学习如何设计网络应用协议。
- 掌握Socket编程接口，编写基本的网络应用软件。

## 二、实验内容
根据自定义的协议规范，使用Socket编程接口开发基本的网络应用软件。

### 实验要求
- 掌握C语言形式的Socket编程接口用法，能够正确发送和接收网络数据包。
- 开发一个客户端，实现人机交互界面和与服务器的通信。
- 开发一个服务端，实现并发处理多个客户端的请求。
- 程序界面不作要求，可采用命令行或最简单的窗体。

### 功能要求
1. **运输层协议**：采用TCP。
2. **客户端功能**：交互菜单形式，用户可选择以下操作：
   - **连接**：请求连接到指定地址和端口的服务端。
   - **断开连接**：断开与服务端的连接。
   - **获取时间**：请求服务端返回当前时间。
   - **获取名字**：请求服务端返回其机器的名称。
   - **活动连接列表**：请求服务端返回当前连接的所有客户端信息（编号、IP地址、端口等）。
   - **发消息**：请求服务端转发消息给指定编号的客户端，该客户端收到后显示消息。
   - **退出**：断开连接并退出客户端程序。

3. **服务端功能**：响应客户端请求并完成以下任务：
   - 返回服务端所在机器的当前时间。
   - 返回服务端所在机器的名称。
   - 返回当前连接的所有客户端信息。
   - 将某客户端发送的内容转发给指定编号的其他客户端。
   - 采用异步多线程编程模式，正确处理多个客户端同时连接及消息发送。

### 开发说明
- 设计客户端和服务端之间的通信协议。
- 网络数据包发送部分必须使用底层C/C++语言形式的Socket API，禁止使用任何Socket封装类。
- 本实验可组队完成，每组最多2人，鼓励独立完成。

---

## 三、主要仪器设备
- **硬件**：联网的PC机。
- **软件**：
  - Wireshark软件。
  - Visual C++、GCC等C++集成开发环境。

## 四、操作方法与实验步骤

### 1. 数据包设计
设计请求、指示（服务器主动发给客户端的）、响应数据包的格式，至少考虑以下问题：
- **边界识别**：定义两个数据包的边界如何识别。
- **类型字段**：定义数据包的请求、指示、响应类型字段。
- **长度字段**：定义数据包的长度字段或结尾标记。
- **数据字段格式**：定义数据包内数据字段的格式，特别是客户端列表数据的表达方式。

### 2. 小组分工
- 佟昕 负责服务端的编写。
- 林子昕 负责客户端的编写。
- 同时，我们在数据结构、功能模块的完善上，进行了有效的沟通与合作，并且采用 [git 仓库](https://github.com/AnyaReese/Socket)管理代码版本以及协作。

### 3. 客户端编写步骤
客户端需要采用多线程模式，具体步骤如下：
#### a) 初始化
- 调用 `socket()` 向操作系统申请Socket句柄。

#### b) 菜单功能
- 编写菜单，列出以下7个选项：
  - **连接**
  - **断开连接**
  - **获取时间**
  - **获取名字**
  - **活动连接列表**
  - **发送消息**
  - **退出**

#### c) 等待用户选择
- 根据用户选择执行相应操作（未连接时只能选择连接和退出功能）。

#### d) 具体功能实现
1. **连接**：
   - 用户输入服务器IP和端口。
   - 调用 `connect()` 并等待结果，连接成功后设置状态为已连接。
   - 创建接收数据的子线程，循环调用 `receive()`，收到完整响应包后通过线程通信传给主线程。
2. **断开连接**：
   - 调用 `close()` 关闭连接，设置状态为未连接。
   - 通知并等待子线程关闭。
3. **获取时间**：
   - 组装时间请求数据包。
   - 调用 `send()` 发送请求并等待子线程返回结果。
   - 打印响应中的时间信息。
4. **获取名字**：
   - 组装名字请求数据包。
   - 调用 `send()` 发送请求并等待子线程返回结果。
   - 打印响应中的名字信息。
5. **获取客户端列表**：
   - 组装列表请求数据包。
   - 调用 `send()` 发送请求并等待子线程返回结果。
   - 打印客户端列表（编号、IP地址、端口等）。
6. **发送消息**：
   - 用户输入目标客户端编号和内容。
   - 组装消息请求数据包。
   - 调用 `send()` 发送请求并等待子线程返回结果。
   - 打印发送结果。
7. **退出**：
   - 若已连接，先调用断开功能。
   - 退出程序。

#### e) 主线程任务
- 等待用户输入。
- 处理子线程消息队列，打印响应或指示消息。

### 4. 服务端编写步骤
服务端需要采用多线程模式，具体步骤如下：

#### a) 初始化
- 调用 `socket()` 向操作系统申请Socket句柄。
- 调用 `bind()` 绑定监听端口（使用学号后4位）。
- 调用 `listen()` 设置连接等待队列长度。

#### b) 主线程任务
- 循环调用 `accept()`，为新客户端创建子线程。
- 子线程处理逻辑：
  - 可选发送 `hello` 消息给客户端。
  - 循环调用 `receive()`，根据请求类型完成任务：
    1. **获取时间**：调用 `time()` 获取时间，组装响应包并 `send()` 返回。
    2. **获取名字**：将服务器名字组装入响应包并 `send()` 返回。
    3. **获取客户端列表**：组装列表数据入响应包并 `send()` 返回。
    4. **发送消息**：
       - 检查编号是否存在及状态是否连接。
       - 若失败，返回错误代码；若成功，转发消息给目标客户端。

#### c) 退出检测
- 主线程检测退出信号，通知并等待子线程退出。
- 关闭Socket，程序结束。

### 5. 功能验证
- 客户端与服务端程序运行，检查功能实现情况。
- 修复问题直至满足功能要求。
- 使用多个客户端同时连接服务端，验证并发处理能力。
- 使用Wireshark抓取各功能的交互数据包。

## 五、实验数据记录和处理

请将以下内容与实验报告一起打包成一个压缩文件上传：
- **源代码**：客户端和服务端的代码分别放置在不同目录，代码需包含较为丰富的注释。
- **可执行文件**：提供客户端和服务端的可运行文件（`.exe` 或 Linux 可执行文件），并附简易运行说明文档。

### 实验记录
以下记录需结合屏幕截图，配以文字标注：

#### 1. 请求数据包

描述格式：
请求数据包由客户端发给服务端，用于传递客户端的请求信息，包含以下字段：
- **`request`** (1字节): 请求类型，例如获取时间、获取名字、客户端列表等。
- **`target_addr`** (1字节): 目标客户端地址，仅在发送消息功能中使用。
- **`size`** (1字节): 消息内容的长度。
- **`message`** (256字节): 可选的消息内容，取决于请求类型。

![alt text](img/README/image-2.png)

#### 2. 响应数据包

![alt text](img/README/image-3.png)

从服务端发回来的数据包会被解析为一个`ThreadMessage`，包括：
- `type`：消息的类型，额外设置了 `[SHUTDOWN]` 字段特殊处理Server端断开连接的处理
- `content`：消息的具体内容，例如服务器返回的数据或通知的具体信息。
<!-- - `sender_ip`：发送者的IP地址，用于标识消息的来源。
- `sender_id`：发送者的唯一标识符或ID，用于区分不同的客户端或服务器。 -->

![alt text](img/README/image-4.png)

#### 3. 指示数据包

![alt text](img/README/image-5.png)

从服务端发回来的数据包会被解析为一个`ThreadMessage`，包括：
- `type`：消息的类型，额外设置了 `[SHUTDOWN]` 字段特殊处理Server端断开连接的处理
- `content`：消息的具体内容，例如服务器返回的数据或通知的具体信息。
<!-- - `sender_ip`：发送者的IP地址，用于标识消息的来源。
- `sender_id`：发送者的唯一标识符或ID，用于区分不同的客户端或服务器。 -->

#### 4. 客户端初始运行后显示的菜单选项。

<div class="center">
<img src="img/README/image-6.png" width="50%">
</div>

#### 5. 客户端的主线程循环关键代码（描述总体，省略细节部分）。

主线程循环的关键代码主要负责显示菜单、接收用户输入、处理用户命令，并根据用户的选择执行相应的操作：
- 显示菜单：根据客户端是否已连接到服务器，显示不同的菜单选项。
- 接收用户输入：提示用户输入选择的命令。
- 命令处理：如果用户未连接到服务器，根据用户选择执行连接操作或退出。

![alt text](img/README/image-7.png)

如果用户已连接，根据用户选择执行一项操作（`get time`等）

![alt text](img/README/image-8.png)

**发送请求**：对于某些命令，如获取时间或名称，构造请求数据包并发送到服务器。

![alt text](img/README/image-9.png)

**处理消息队列**：从消息队列中取出消息，并根据消息类型进行处理，如显示服务器响应或客户端消息。

![alt text](img/README/image-10.png)

**循环继续**：循环回到显示菜单步骤，等待下一次用户输入。

#### 6. 客户端的接收数据子线程循环关键代码（描述总体，省略细节部分）。

接收数据子线程的主要任务是持续监听来自服务器的数据，并将其放入消息队列中：
- 线程运行标志检查：在循环开始时，检查`threadRunning`标志，如果为`false`，则退出循环。
- 接收数据：使用`recv`函数从套接字中接收数据，存储到缓冲区中。
- 如果接收到的数据是服务器响应（如`[RESPONSE]`或`[SHUTDOWN]`），则直接将整个响应内容设置为`ThreadMessage`的`content`，并设置为`RESPONSE`类型。
- 消息入队：将解析后的`ThreadMessage`实例加锁后放入消息队列`messageQueue`中。
- 处理消息队列：调用`processMessageQueue`函数来处理队列中的消息，显示给用户或执行其他必要的操作。
- 循环继续：返回循环的开始，继续监听来自服务器的数据。

![alt text](img/README/image-11.png)

#### 7. 服务器初始运行后显示的界面。

![alt text](img/README/image-12.png)

#### 8. 服务器的主线程循环关键代码（描述总体，省略细节部分）。

服务器的主线程循环负责接受新的客户端连接和监控服务器的运行状态。以下是主线程循环的关键代码的总体描述：
- 初始化套接字数组：将用于存储客户端套接字的数组sockets初始化为-1，表示所有客户端套接字初始时都是空闲的。
- 设置信号处理：通过signal函数设置信号处理程序signal_handler，以便在接收到中断信号（如SIGINT和SIGTERM）时能够优雅地关闭服务器。
- 创建服务器套接字：创建服务器套接字server_fd，并设置其为非阻塞模式，允许地址重用。
- 绑定和监听：将服务器套接字绑定到指定端口，并开始监听该端口上的连接请求。

![alt text](img/README/image-13.png)

![alt text](img/README/image-14.png)

- 监控退出条件：在主循环中，监控should_exit原子变量，如果它被设置为true（通常由信号处理函数设置），则跳出循环，开始关闭服务器。
- 关闭服务器：循环结束后，关闭服务器套接字，并等待所有子线程结束，清理线程资源，然后退出主线程

![alt text](img/README/image-15.png)

#### 9. 服务器的客户端处理子线程循环关键代码（描述总体，省略细节部分）。

服务器的客户端处理子线程循环负责接收和处理来自特定客户端的请求，包括获取时间、服务器名称、客户端列表、发送消息给其他客户端以及断开连接等操作，同时监控服务器的退出信号以确保在必要时能够优雅地关闭连接并释放资源。

![alt text](img/README/image-16.png)

#### 10. 功能操作与显示内容：

 - **连接功能**：
   - 截图客户端和服务端显示的内容。
   - 使用 Wireshark 抓取相关数据包。

![alt text](img/README/image-18.png)
> 客户端连接成功后显示的内容

![alt text](img/README/image-19.png)
> 服务端显示的内容

![alt text](img/README/image-17.png)

> 设置 wireshark 抓包过滤 loopback 网卡，查看连接过程中端口为 `3784` 的数据包。

下面为客户端和服务端的 tcp 三次握手，客户端端口号为 60031，服务端端口号为 3784:

![alt text](img/README/image-20.png)
> 客户端(60031)向服务端(3784)发送SYN 报文，并置发送序列号 seq 为 0

![alt text](img/README/image-21.png)
> 服务端向客户端发送 SYN+ACK 报文，并置发送序列号 Seq 为0，确认序号为ACK=0+1=1

![alt text](img/README/image-22.png)
> 客户端向服务端发送 ACK 报文，置发送序列号 Seq 为 1，确认序列号ACK 为0+1=1

- **获取时间功能**：
   - 客户端和服务端显示内容截图。
   - Wireshark 抓取数据包截图，展开应用层数据包，标记请求、响应类型及时间数据对应位置。

![alt text](img/README/image-23.png)
> 客户端获取时间显示的内容

![alt text](img/README/image-24.png)
> 服务端显示的内容

![alt text](img/README/image-42.png)
> Wireshark 抓取的客户端发送时间请求的数据包

![alt text](img/README/image-61.png)
> Wireshark 抓取的服务端返回时间的数据包


- **获取名字功能**：
   - 客户端和服务端显示内容截图。
   - Wireshark 抓取数据包截图，展开应用层数据包，标记请求、响应类型及名字数据对应位置。
   - 相关服务器处理代码片段。

![alt text](img/README/image-28.png)
> 客户端获取名字显示的内容

![alt text](img/README/image-29.png)
> 服务端显示的内容

![alt text](img/README/image-44.png)
> Wireshark 抓取的客户端发送名字请求的数据包

![alt text](img/README/image-62.png)
> Wireshark 抓取的服务端返回名字的数据包

![alt text](img/README/image-32.png)
![alt text](img/README/image-33.png)
> 服务端处理获取名字请求的代码片段

- **获取客户端列表功能**：
   - 客户端和服务端显示内容截图。
   - Wireshark 抓取数据包截图，展开应用层数据包。
   - 相关服务器处理代码片段。

![alt text](img/README/image-35.png)
> 客户端获取客户端列表显示的内容

![alt text](img/README/image-36.png)
> 服务端显示的内容

![alt text](img/README/image-46.png)
> Wireshark 抓取的客户端发送客户端列表请求的数据包

![alt text](img/README/image-63.png)
> Wireshark 抓取的服务端返回客户端列表的数据包

![alt text](img/README/image-38.png)
> 服务端处理获取客户端列表请求的代码片段

- **发送消息功能**：
   - **发送消息的客户端显示内容截图**。
   - **服务器显示内容截图**。
   - **接收消息的客户端显示内容截图**。
   - Wireshark 抓取数据包截图，分别标记发送和接收数据包。
   - 相关服务器和客户端代码片段。

![alt text](img/README/image-39.png)
> 客户端发送消息的显示内容

![alt text](img/README/image-40.png)
> 服务端显示的内容

![alt text](img/README/image-41.png)
> 客户端接收消息的显示内容

![alt text](img/README/image-48.png)
> Wireshark 抓取的客户端发送消息的数据包

![alt text](img/README/image-64.png)
![alt text](img/README/image-65.png)
> Wireshark 抓取的服务端转发消息的数据包

![alt text](img/README/image-66.png)
> Wireshark 抓取的客户端接收消息的数据包

#### 11. 异常情况测试：

- 拔掉客户端网线：
   - 客户端退出时的 TCP 连接状态。
   - 使用 Wireshark 观察是否发送 TCP 连接释放消息。
   - 服务端的 TCP 连接状态在10分钟内是否变化。

![alt text](img/README/image-54.png)

> 切断客户端电脑网络，关闭客户端，等待十分钟，发现 TCP 连接状态 ESTABLISHED，且服务端未断开此用户连接。且抓包结果显示该用户端未向服务端发出 TCP 连接释放的消息。

- 重新连接：
   - 连上网线后重新运行客户端，连接并获取客户端列表，查看异常退出的连接状态。
   - 发送消息给异常退出的客户端时，观察结果。

![alt text](img/README/image-52.png)
> 重新连接后，客户端显示的内容，仍然显示异常退出的客户端

![alt text](img/README/image-53.png)
> 向异常客户端发送消息，显示客户端 id 不存在。

 - 修改请求频率：
   - 将获取时间功能改为自动发送100次请求。
   - 检查服务器是否正常处理，客户端是否接收到100次响应，使用 Wireshark 抓取数据包。

![alt text](img/README/image-56.png)
> 客户端显示的内容，显示发送时间请求100次, 并接收到100次响应

![alt text](img/README/image-60.png)
> wireshark 抓取的客户端发送时间请求的数据包

代码片段：

![alt text](img/README/image-58.png)
![alt text](img/README/image-57.png)

- 并发测试：
   - 多个客户端同时连接服务器，并自动连续发送时间请求100次。
   - 截图服务器和客户端运行结果。

![alt text](img/README/image-59.png)
正常运行

## 六、实验结果与分析

回答以下问题：

1. **客户端是否需要调用 `bind` 操作？**
   - 它的源端口是如何产生的？
   - 每次调用 `connect` 时客户端的端口是否保持不变？
> 客户端不需要显式调用 bind 操作
> 源端口是由操作系统自动从临时端口范围(ephemeral ports)中分配，通常是从 49152 到 65535 之间
> 每次调用 connect 时端口会变化，因为操作系统会为每个新的连接分配一个新的临时端口号
2. **调试断点测试**：
   - 如果在服务端 `listen` 和 `accept` 之间设置断点，客户端调用 `connect` 是否能马上连接成功？
> 如果在 listen 和 accept 之间设置断点，客户端的 connect 调用会阻塞
> 这是因为 TCP 三次握手过程中，服务器收到 SYN 后，如果没有及时调用 accept，客户端的连接请求会处于 SYN_RECV 状态
> 连接请求会在服务器的半连接队列（SYN Queue）中等待
3. **快速发送数据**：
   - 连续快速 `send` 多次数据后，通过 Wireshark 抓包，发送的 TCP Segment 次数是否与 `send` 的次数一致？
> 连续快速 send 多次数据时，TCP 会进行数据包的合并（Nagle 算法）
> 通过 Wireshark 可以观察到实际发送的 TCP Segment 数量通常少于 send 调用次数
> 这是因为 TCP 的滑动窗口机制和 Nagle 算法会对小数据包进行合并，以提高网络效率

4. **数据包区分**：
   - 服务端在同一端口接收多个客户端的数据，如何区分数据包所属客户端？
> 服务端通过每个连接的四元组来区分不同客户端：源IP地址, 源端口, 目标IP地址, 目标端口
> 每个连接获得独立的文件描述符（socket fd），服务器可以通过这个 fd 来区分不同客户端

5. **TCP连接状态**：
   - 客户端主动断开连接后，TCP连接状态是什么？持续时间是多少？（可用 `netstat -an` 查看）
> 客户端主动断开连接后，连接会进入 TIME_WAIT 状态
> TIME_WAIT 状态持续时间通常是 2MSL（Maximum Segment Lifetime）, 在 Linux 系统中，这个时间通常是 60 .这个状态是为了确保最后的 ACK 能够到达服务器，并防止旧连接的数据包影响新连接

6. **异常断网处理**：
   - 客户端断网后异常退出，服务端的TCP连接状态是否变化？
   - 服务端如何检测连接是否有效？

>   客户端断网后，服务端不会立即检测到连接断开
> 服务端的 TCP 连接会保持 ESTABLISHED 状态一段时间
> 服务端可以通过以下方式检测连接是否有效：
>  1. 设置 TCP keepalive 选项
>  2. 在应用层实现心跳机制
>  3. 设置读操作超时时间
>  4. 通过 select/poll/epoll 检测连接状态

## 七、讨论与心得

记录实验过程中遇到的困难、经验教训，以及对实验安排的建议。

除了需要正确处理消息队列的互斥访问，server 和 client 之间的数据结构设计也非常的重要。并且在编写代码和不断运行、甚至写报告的过程中才意识到，一些问题在正常运行时很难暴露，需要通过一些特殊的测试来验证，我们在编写代码时，应该多考虑 corner case，并且做充足的测试来增强代码的鲁棒性。
