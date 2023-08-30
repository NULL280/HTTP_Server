#ifndef  _myhttpd_
   #define _myhttpd_

// 不变
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// 导入string
#include <string>

// 导入c++打印
#include <iostream>

// 导入fstream
#include <fstream>

// 导入sigaction
#include <signal.h>

// 导入 wait()
#include <sys/wait.h>

// 导入 线程
#include <pthread.h>

// 导入vecotr
#include <vector>

// 导入 find(), sort()
#include <algorithm>

// 导入 时间
#include <time.h>

// 导入共享内存相关
// 导入 mmap()
#include <sys/mman.h>

// 导入 dlopen()
#include <dlfcn.h>

// 导入 stat()
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>

// 导入 opendir()
#include <dirent.h>

// 导入 std::stringstream
#include <sstream>

// 导入 inet_ntoa()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif


/*
 * 变量声明
 */

const char * usage =
"                                                               \n"
"myhttpd:                                                       \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";

// 主机
std::string host;

// 互斥锁
pthread_mutex_t mutex;

// 用户名密码
std::vector<std::string> login_tokens = std::vector<std::string>();

// 共享内存结构
struct shmseg {
   time_t start_t;
   int number_of_requests;
   // 最短时间
   double minimum_service_time;
   char minimum_time_URL[1024];
   // 最长时间
   double maximum_service_time;
   char maximum_time_URL[1024];
   //
   std::string source_host_ip;
};

// 共享内存结构指针
struct shmseg *shmp;

// path entry 信息结构
struct path_entry_info {
   std::string name;
   std::string type;
   std::string entry_URL;
   std::string icon_dir;
   long size;
   time_t last_mod_time;
};

/*
 * 函数声明
 */

// Processes time request
void processHTTPRequest( int socket );

// 子进程结束处理者
void killzombie( int sig );

// 线程函数
void pool_thread_function(int masterSocket);

// 将文件发送到client
void sent_file_to_client(int fd, std::string contentType, char * document, int size);

// 更新最短最长进程时间
void update_mini_max_request_time(clock_t start, std::string request_URL);

// 获取文件大小
long get_file_size(std::string filename);

// 获取文件/路径修改日期
time_t get_mod_time(std::string filename);

// 用于sort entry array
bool compare_name_ascending(const path_entry_info &a, const path_entry_info &b);
bool compare_name_descending(const path_entry_info &a, const path_entry_info &b);
bool compare_size_ascending(const path_entry_info &a, const path_entry_info &b);
bool compare_size_descending(const path_entry_info &a, const path_entry_info &b);
bool compare_time_ascending(const path_entry_info &a, const path_entry_info &b);
bool compare_time_descending(const path_entry_info &a, const path_entry_info &b);

int
main( int argc, char ** argv )
{
   // 初始化共享内存
   shmp = (struct shmseg *) mmap(NULL, sizeof(struct shmseg), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

   // 开始为服务器计时
   time(&shmp->start_t);
   shmp->minimum_service_time = 1048576;
   shmp->maximum_service_time = 0;
   shmp->number_of_requests = 0;

   // 用户名密码初始化
   login_tokens.push_back("bGpqOjEyMw==");
   login_tokens.push_back("bGpqMToxMjM=");
   login_tokens.push_back("bGpqMjoxMjM=");
   login_tokens.push_back("bGpqMzoxMjM=");
   // 获取port num与模式
   std::string mode = "-o";
   int port = 32799;
   if ( argc < 2 )
   {
      fprintf( stderr, "%s", usage );
      exit( -1 );
   }
   else if (argc == 3)
   {
      // 获得模式
      mode = std::string(argv[1]);

      // 获得socket号
      port = atoi(argv[2]);
   }
   else if (argc == 2)
   {
      // 只有一个参数
      auto tmp = std::string(argv[1]);
      if (tmp[0] == '-') {
         // 模式参数
         mode = tmp;
      }
      else if (tmp.find_first_not_of("0123456789") == std::string::npos)
      {
         // port参数
         port = atoi(argv[1]);
      }
      else
      {
         // 无效参数
         fprintf( stderr, "%s", usage );
         exit( -1 );
      }
   }
   host = "data.cs.purdue.edu:" + std::to_string(port);

   

   

   // 设置IP + port
   struct sockaddr_in serverIPAddress; // sockaddr_in: IP地址 + port
   memset( &serverIPAddress, 0, sizeof(serverIPAddress) ); // 初始化
   serverIPAddress.sin_family = AF_INET;
   serverIPAddress.sin_addr.s_addr = INADDR_ANY; // 设置IP集合
   serverIPAddress.sin_port = htons((u_short) port); // 设置 port

   // 设置 socket
   // masterSocket 接受client连接的socket
   int masterSocket =  socket(PF_INET, SOCK_STREAM, 0); // SOCK_STREAM 用于TCP
   if ( masterSocket < 0) {
      perror("socket");
      exit( -1 );
   }

   // Set socket options to reuse port. Otherwise we will
   // have to wait about 2 minutes before reusing the same port number
   int optval = 1; 
   int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
               (char *) &optval, sizeof( int ) );
      
   // Bind the socket to the IP address and port
   int error = bind( masterSocket,
            (struct sockaddr *)&serverIPAddress,
            sizeof(serverIPAddress) ); // 链接ipport与socket
   if ( error ) {
      perror("bind");
      exit( -1 );
   }

   // Put socket in listening mode and set the 
   // size of the queue of unprocessed connections
   error = listen( masterSocket, 5); // socket进入监听状态
   if ( error ) {
      perror("listen");
      exit( -1 );
   }

   // 子进程结束信号处理
   // 创建信号结构
   struct sigaction signalAction;
   // 添加处理者 (function pointer)
   signalAction.sa_handler = killzombie;
   // 清除信号面具(多个信号重叠), 防止被别的信号打断
   sigemptyset(&signalAction.sa_mask);
   // 默认flag (0), 会干扰system call, SA_RESTART不会出错
   signalAction.sa_flags = SA_RESTART;
   // 子进程结束信号应答机， SIGCHLD: 子进程结束信号
   sigaction(SIGCHLD, &signalAction, NULL );

   // pool thread 模式
   if (mode == "-p")
   {
      pthread_t t1, t2, t3, t4, t5;
      // 参数初始化设置
      pthread_attr_t attr;
      pthread_attr_init( &attr );
      // 参数设置
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // PTHREAD_CREATE_DETACHED: 独立线程, 不等待pthread_join
      // 互斥锁初始化
	   pthread_mutex_init(&mutex, NULL);

      // 创建线程
      pthread_create( &t1, &attr, (void * (*)(void *)) pool_thread_function, (void *) masterSocket);
      pthread_create( &t2, &attr, (void * (*)(void *)) pool_thread_function, (void *) masterSocket);
      pthread_create( &t3, &attr, (void * (*)(void *)) pool_thread_function, (void *) masterSocket);
      pthread_create( &t4, &attr, (void * (*)(void *)) pool_thread_function, (void *) masterSocket);
      pool_thread_function(masterSocket);
      return 0;
   }

   while ( 1 ) {
      // Accept incoming connections 不变
      struct sockaddr_in clientIPAddress; // 用户 IP
      int alen = sizeof( clientIPAddress );
      // 等待接入
      // slaveSocke 与client交换信息的socket
      int slaveSocket = accept( masterSocket,
                  (struct sockaddr *)&clientIPAddress,
                  (socklen_t*)&alen); // 等待连接, 存入slaveSocket
      
      // 获得clientIP
      std::string client_ip(inet_ntoa(clientIPAddress.sin_addr));
      shmp->source_host_ip = client_ip;

      if ( slaveSocket < 0 ) {
         perror( "accept" );
         exit( -1 );
      }

      if (mode == "-f")
      {
         // process 模式
         // 创建子进程
         int ret = fork();

         // 判断是否为子进程
         if (ret == 0)
         {
            // 处理request
            processHTTPRequest( slaveSocket );
            // 退出子进程
            exit(0);
         } else if (ret < 0) {
            // 创建进程报错
            perror("fork");
            close( slaveSocket );
            return 1;
         }
         // 母进程
         close( slaveSocket ); // 清理socket(fd), 避免占用client连接和用完fd
      }
      else if (mode == "-t")
      {
         // thread 模式
         pthread_t t1;
         pthread_attr_t attr;
         pthread_attr_init( &attr );
         // 参数设置
         pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // PTHREAD_CREATE_DETACHED: 独立线程, 不等待pthread_join
         pthread_create( &t1, &attr, (void * (*)(void *)) processHTTPRequest, (void *) slaveSocket);
      }
      else
      {
         // 普通情况
         processHTTPRequest( slaveSocket );
         close( slaveSocket );
      }
   }

}

void
processHTTPRequest( int fd )
{
   // 开始为进程计时
   clock_t start = clock();

   // Buffer used to store the name received from the client 缓存
   const int MaxReq = 4096;
   char req[ MaxReq + 1 ];
   int reqLength = 0;
   int n;

   // Currently character read
   unsigned char newChar;

   //
   // The client should send <name><cr><lf>
   // Read the name of the client character by character until a
   // <CR><LF> is found.
   //

   // 获取client请求
   while ( reqLength < MaxReq &&
         ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {


         req[ reqLength ] = newChar;
         reqLength++;

         if (reqLength > 4 &&
            // '\015' octo, 表示13 = 'r', '\012' = '\n'
            req[reqLength - 4] == '\r' && req[reqLength - 3] == '\n' &&
            req[reqLength - 2] == '\r' && req[reqLength - 1] == '\n') {
            // 退出loop
            break;
         }

   }
   req[ reqLength ] = 0;
   auto request = std::string(req);

   std::cout << request;

   // 更新request数量
   shmp->number_of_requests++;

   // 处理请求
   // 身份验证
   std::string unauthorized_message =
      "HTTP/1.1 401 Unauthorized\r\n"
      "WWW-Authenticate: Basic realm=\"LJJ's Realm ID\"\r\n";
   int start_index = request.find("Authorization:");
   
   if (start_index == std::string::npos)
   {  
      // 账号密码信息缺失
      write(fd, unauthorized_message.c_str(), unauthorized_message.length());
      // 关闭连接
      close( fd );
      return;
   }
   start_index += 21;
   int end_index = request.find("\r\n", start_index);
   end_index--;
   auto base64_login = request.substr(start_index, end_index - start_index + 1);
   if (std::find(login_tokens.begin(), login_tokens.end(), base64_login) == login_tokens.end())
   {
      // 用户名密码错误(不在vector中)
      write(fd, unauthorized_message.c_str(), unauthorized_message.length());
      // 关闭连接
      close( fd );
      return;
   }

   // 处理路径
   start_index = request.find("GET");
   start_index += 4;
   end_index = request.find(' ', start_index);
   end_index--;
   auto URL_request_path = request.substr(start_index, end_index - start_index + 1);
   // 查看是否存在 QUERY_STRING
   std::string query_string = "";
   end_index = URL_request_path.find('?');
   if (end_index != std::string::npos)
   {
      // 获得 QUERY_STRING
      end_index--;
      query_string = URL_request_path.substr(end_index + 2);

      // 处理路径
      URL_request_path = URL_request_path.substr(0, end_index + 1);
   }
   // 去除 dir 路径结尾 '/'
   if (URL_request_path != "/" && URL_request_path[URL_request_path.length() - 1] == '/')
   {
      URL_request_path = URL_request_path.substr(0, URL_request_path.length() - 1);
   }


   // 获得 request URL
   std::string request_URL = host + URL_request_path;

   // 更新log_data
   std::ofstream log_file("http-root-dir/logs_data.txt", std::ios_base::app);
   log_file << "Source Host: " << shmp->source_host_ip << ", Directory: " << URL_request_path << "\n";
   log_file.close();

   // type 1 cgi-bin脚本请求
   start_index = URL_request_path.find("/cgi-bin/");
   if (start_index != std::string::npos)
   {
      // 创建子进程
      int ret = fork();

      // 判断是否为子进程
      if (ret == 0)
      {
         // 子进程

         // 是否有query string
         if (query_string != "")
         {
            // 设置 QUERY_STRING 到环境变量
            setenv("QUERY_STRING", query_string.c_str(), 1);
         }

         // 设置 REQUEST_METHOD
         setenv("REQUEST_METHOD", "GET", 1);

         // 发送header
         std::string header =
            "HTTP/1.0 200 Document follows\r\n"
            "Server: CS252_Lab5\r\n";
         write(fd, header.c_str(), header.length());

         // type 1.2 Loadable Modules请求
         if (URL_request_path.find(".so") != std::string::npos)
         {
            // 处理路径
            // 去除 "/cgi-bin"
            std::string serval_local_path = URL_request_path.substr(8);
            serval_local_path = "/homes/lang63/cs252/lab5-src" + serval_local_path;
            void * lib = dlopen(serval_local_path.c_str(), RTLD_LAZY);
            // 报错
            if ( lib == NULL ) {
               fprintf( stderr, "dlerror:%s\n", dlerror());
               perror( "dlopen");
               exit(1);
            }

            typedef void (*httprunfunc)(int ssock, const char* querystring);

            httprunfunc http_run = (httprunfunc) dlsym(lib, "httprun");
            // 报错
            if ( http_run == NULL ) {
               perror( "dlsym: hhttprun not found");
               exit(1);
            }

            // 运行脚本
            (*http_run)(fd, getenv("QUERY_STRING"));
            // 报错
            if (dlclose(lib) != 0)
            {
               perror( "dlclose");
               exit(1);
            }

            // 更新时间
            update_mini_max_request_time(start, request_URL);

            // 关闭fd, 退出子进程
            close( fd );
            exit(0);

         }

         // output 定向
         dup2(fd, 1);
         close(fd);

         // 处理路径
         std::string server_local_path = "/homes/lang63/cs252/lab5-src/http-root-dir" + URL_request_path;

         // 运行脚本
         char *const argv[] = {(char *) server_local_path.c_str()};
         execvp(argv[0], argv);

         // 更新时间
         update_mini_max_request_time(start, request_URL);

         // 退出子进程
         exit(0);
      }
      else if (ret < 0)
      {
         // 创建进程报错
         perror("fork");
         close( fd );
         return;
      }
      // 母进程
      // 关闭fd
      close( fd );
      return;
   }

   // type 2 stats界面请求
   if (URL_request_path == "/stats")
   {
      std::string html_content =
         "<title> CS 252 HTTP Test</title>"
         "<H1>Statistics</H1>"
         "<H2>Names of the Student</H2>"
         "<LI>"
         "Jianjun Lang";
      

      // 运行时间
      time_t end_t;
      time(&end_t);
      std::string server_run_time = std::to_string((int) difftime(end_t, shmp->start_t));
      html_content +=
         "<H2>Time the server has been up</H2>"
         "<LI>" + server_run_time;
      
      // request数
      std::string request_num = std::to_string(shmp->number_of_requests);
      html_content +=
         "<H2>Number of Requests</H2>"
         "<LI>" + request_num;
      
      // 最短执行时间
      std::string URL1((char *) shmp->minimum_time_URL);
      html_content +=
         "<H2>Minimum Service Time</H2>"
         "<LI>"
         "Time: " + std::to_string(shmp->minimum_service_time) +
         ", URL: " + URL1;

      // 最长执行时间
      std::string URL2((char *) shmp->maximum_time_URL);
      html_content +=
         "<H2>Maximum Service Time</H2>"
         "<LI>"
         "Time: " + std::to_string(shmp->maximum_service_time) +
         ", URL: " + URL2;

      // html结尾
      html_content += "</UL>";

      
      // 向clinet发送html
      sent_file_to_client(fd, "text/html", (char *) html_content.c_str(), html_content.length());

      // 更新时间
      update_mini_max_request_time(start, request_URL);

      // 关闭接口退出
      close( fd );
      return;
   }

   // type 3 logs界面请求
   if (URL_request_path == "/logs")
   {
      std::string html_content =
         "<title> CS 252 HTTP Test</title>"
         "<H1>Logs Page</H1>"
         "<H2>list of all the requests</H2>"
         "<UL>";

      // 逐行读取文件
      std::string line;
      std::ifstream log_file("http-root-dir/logs_data.txt");
      while (std::getline(log_file, line))
      {
         // 读取行写入网页
         html_content += "<LI>" + line;
      }
      log_file.close();

      html_content += "</UL>";
      
      // 向clinet发送html
      sent_file_to_client(fd, "text/html", (char *) html_content.c_str(), html_content.length());

      // 更新时间
      update_mini_max_request_time(start, request_URL);

      // 关闭接口退出
      close( fd );
      return;
   }

   // type 0 常规文件/路径请求

   std::string server_local_path;
   // 处理路径
   if (URL_request_path == "/") {
      server_local_path = "/homes/lang63/cs252/lab5-src/http-root-dir/htdocs/index.html";
   }
   else if (URL_request_path.find("/icons") != std::string::npos)
   {
      server_local_path = "/homes/lang63/cs252/lab5-src/http-root-dir" + URL_request_path;
   }
   else
   {
      server_local_path = "/homes/lang63/cs252/lab5-src/http-root-dir/htdocs" + URL_request_path;
   }

   // 判断文件/dir/不存在
   struct stat s;
   if(stat(server_local_path.c_str(), &s) == 0)
   {
      // dir 情况
      if( s.st_mode & S_IFDIR )
      {
         // 打开路径
         DIR * dir = opendir(server_local_path.c_str());

         // loop 路径中的文件与dir, 添加到entry信息array
         struct dirent * ent;
         std::vector<struct path_entry_info> entry_info_array = std::vector<struct path_entry_info>();
         while ((ent = readdir(dir)) != NULL) {
            // 文件/dir名
            std::string name(ent->d_name);
            // 路径类型
            auto entry_type = ent->d_type;

            // 跳过隐藏文件
            if (name[0] == '.') {
               continue;
            }

            // entry信息结构
            struct path_entry_info entry_info;
            // 处理文件
            if (entry_type != DT_DIR)
            {
               // 文件名
               entry_info.name = name;
               // 文件类型
               auto dot_index = name.find('.');
               if (dot_index == std::string::npos)
               {
                  entry_info.type = "unknown";
               }
               else
               {
                  entry_info.type = name.substr(dot_index + 1);
               }
               // 文件图标URL
               if (entry_info.type == "gif")
               {
                  entry_info.icon_dir = "../../icons/gif_icon.gif";
               }
               else
               {
                  entry_info.icon_dir = "../../icons/unknown.gif";
               }
               // 文件URL
               entry_info.entry_URL = "http://" + request_URL + "/" + name;

               // 文件大小
               entry_info.size = get_file_size(server_local_path + "/" + name);

               // 文件修改时间
               entry_info.last_mod_time = get_mod_time(server_local_path + "/" + name);
            }
            // 处理dir
            else
            {
               // dir名
               entry_info.name = name;
               // dir类型
               entry_info.type = "dir";
               // dir图标URL
               entry_info.icon_dir = "../../icons/menu.gif";
               // dir URL
               entry_info.entry_URL = "http://" + request_URL + "/" + name;
            }
            // 添加信息结构到array
            entry_info_array.push_back(entry_info);

            // 路径size设置为0
            entry_info.size = -1;

            // 路径修改时间
            entry_info.last_mod_time = get_mod_time(server_local_path + "/" + name);
         }

         // sort entry信息array
         if (query_string == "ascending_name_sort")
         {
            std::sort(entry_info_array.begin(), entry_info_array.end(), compare_name_ascending);
         }
         else if (query_string == "descending_name_sort")
         {
            std::sort(entry_info_array.begin(), entry_info_array.end(), compare_name_descending);
         }
         else if (query_string == "ascending_size_sort")
         {
            std::sort(entry_info_array.begin(), entry_info_array.end(), compare_size_ascending);
         }
         else if (query_string == "descending_size_sort")
         {
            std::sort(entry_info_array.begin(), entry_info_array.end(), compare_size_descending);
         }
         else if (query_string == "ascending_time_sort")
         {
            std::sort(entry_info_array.begin(), entry_info_array.end(), compare_time_ascending);
         }
         else if (query_string == "descending_time_sort")
         {
            std::sort(entry_info_array.begin(), entry_info_array.end(), compare_time_descending);
         }

         // 准备html
         // html文件头
         std::string html_content =
         "<!DOCTYPE html><html><head><title>HTML td align Attribute</title></head><body>";

         // 标题
         html_content += "<h1>Browse Directory</h1><h2>" + URL_request_path + "</h2>";

         // table1 头
         // 设置table参数, 500 行宽, 0 框粗
         html_content += "<table width=\"500\" border=\"0\">";
         // table title (第一行)
         html_content += "<tr><th>NAME</th><th>SIZE</th><th>MODE TIME</th></tr>";

         // loop entry 信息, 填入每一行
         for (auto & entry : entry_info_array) {
            // 行开始
            html_content += "<tr>";
            // 项开始
            html_content += "<td align=\"left\">";
            // 图标
            html_content += "<img src=\"" + entry.icon_dir + "\" alt=\"ICON NO FOUND!\">";
            // 文件名 + 超链接
            html_content += "<a href=\"" + entry.entry_URL + "\">" + entry.name + "</a>";
            // 项结束
            html_content += "</td>";
            // SIZE项
            if (entry.type == "dir")
            {
               html_content += "<th>-</th>";
            }
            else
            {
               html_content += "<th>" + std::to_string(entry.size) + "B" + "</th>";
            }
            // TIME项
            std::string time_string(ctime(&entry.last_mod_time));
            html_content += "<th>" + time_string + "</th>";
            // 行结束
            html_content += "<tr>";
         }

         // table1 尾
         html_content += "</table>";

         // 标题
         html_content += "<h2>Sort Directory</h2>";

         // table2 头
         // 设置table参数, 500 行宽, 0 框粗
         html_content += "<table width=\"500\" border=\"1\">";
         // table title (第一行)
         html_content += "<tr><th>ORDER</th><th>Name</th><th>DATE</th><th>SIZE</th></tr>";

         std::string  sort_URL;

         // 行1开始
         html_content += "<tr><td align=\"center\">Ascending</td>";

         sort_URL = "http://" + request_URL + "?ascending_name_sort";
         html_content += "<td align=\"center\"><a href=\"" + sort_URL + "\">sort</a></td>";
         sort_URL = "http://" + request_URL + "?ascending_time_sort";
         html_content += "<td align=\"center\"><a href=\"" + sort_URL + "\">sort</a></td>";
         sort_URL = "http://" + request_URL + "?ascending_size_sort";
         html_content += "<td align=\"center\"><a href=\"" + sort_URL + "\">sort</a></td>";

         // 行1结束
         html_content += "<tr>";

         // 行2开始
         html_content += "<tr><td align=\"center\">Descending</td>";

         sort_URL = "http://" + request_URL + "?descending_name_sort";
         html_content += "<td align=\"center\"><a href=\"" + sort_URL + "\">sort</a></td>";
         sort_URL = "http://" + request_URL + "?descending_time_sort";
         html_content += "<td align=\"center\"><a href=\"" + sort_URL + "\">sort</a></td>";
         sort_URL = "http://" + request_URL + "?descending_size_sort";
         html_content += "<td align=\"center\"><a href=\"" + sort_URL + "\">sort</a></td>";

         // 行2结束
         html_content += "<tr>";

         // table2 尾
         html_content += "</table>";

         // html文件尾
         html_content += "</body></html>";

         // 发送html到clinet
         sent_file_to_client(fd, "text/html", (char *) html_content.c_str(), html_content.length());

      }
      // 文件情况
      else if( s.st_mode & S_IFREG )
      {
         // 获得文件扩展名
         std::string contentType;
         auto filename_extension = server_local_path.substr(server_local_path.find('.') + 1);
         if (filename_extension == "html")
         {
            contentType = "text/html";
         }
         else if (filename_extension == "jpg")
         {
            contentType = "image/jpg";
         }
         else if (filename_extension == "gif")
         {
            contentType = "image/gif";
         }

         // 提取文件内容
         std::ifstream t;  
         int size;  
         t.open(server_local_path);      // open input file  
         t.seekg(0, std::ios::end);    // go to the end  
         size = t.tellg();           // report location (this is the length)  
         t.seekg(0, std::ios::beg);    // go back to the beginning  
         char file_buffer[size];
         // 读取文件字符到buffer
         t.read((char *) file_buffer, size);       // read the whole file into the buffer  
         t.close();                    // close file handle

         
         // 发送回复
         // 将文件发送到client
         sent_file_to_client(fd, contentType, file_buffer, size);
      }
      // 其他
      else
      {
         
      }
   }
   // 路径不存在情况
   else
   {
      // 发送回复
      std::string hdr1 =
         "HTTP/1.1 404 File Not Found\r\n"
         "Server: CS252 Lab5\r\n";
      std::string hdr2 = "Content-type: ";
      write(fd, hdr1.c_str(), hdr1.length());
      write(fd, hdr2.c_str(), hdr2.length());
      std::string document_type = "text/plain";
      write(fd, document_type.c_str(), document_type.length());
      std::string hdr3 = "\r\n\r\n";
      write(fd, hdr3.c_str(), hdr3.length());
      std::string error_message = "Didn't found dir / file!";
      write(fd, error_message.c_str(), error_message.length());
   }



   // 更新时间
   update_mini_max_request_time(start, request_URL);

   // 关闭连接
   close( fd );

}

// 更新最短最长进程时间
void update_mini_max_request_time(clock_t start, std::string request_URL)
{
   clock_t end = clock();
   double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
   // 更新最短时间
   if (cpu_time_used < shmp->minimum_service_time)
   {
      shmp->minimum_service_time = cpu_time_used;
      strcpy(shmp->minimum_time_URL, request_URL.c_str());
   }
   // 更新最长时间
   if (cpu_time_used > shmp->maximum_service_time)
   {
      shmp->maximum_service_time = cpu_time_used;
      strcpy(shmp->maximum_time_URL, request_URL.c_str());
   }
}

// 将文件发送到client
void sent_file_to_client(int fd, std::string contentType, char * document, int size)
{
   // 发送回复
   std::string hdr1 =
      "HTTP/1.1 200 Document follows\r\n"
      "Server: CS252 Lab5\r\n";
   std::string hdr2 = "Content-type: ";
   std::string hdr3 = "\r\n\r\n";
   write(fd, hdr1.c_str(), hdr1.length());
   write(fd, hdr2.c_str(), hdr2.length());
   write(fd, contentType.c_str(), contentType.length());
   write(fd, hdr3.c_str(), hdr3.length());
   write(fd, document, size);
}

// 子进程结束处理者
void killzombie( int sig )
{
  //wait3(0, 0, NULL);
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// pool thread function
void pool_thread_function(int masterSocket) {
   
   while (1)
   {
      
      // Accept incoming connections 不变
      struct sockaddr_in clientIPAddress;
      int alen = sizeof( clientIPAddress );
      // 等待接入
      pthread_mutex_lock(&mutex);
      int slaveSocket = accept( masterSocket,
                  (struct sockaddr *)&clientIPAddress,
                  (socklen_t*)&alen);
      pthread_mutex_unlock(&mutex);

      // 获得clientIP
      std::string client_ip(inet_ntoa(clientIPAddress.sin_addr));
      shmp->source_host_ip = client_ip;
      
      if ( slaveSocket < 0 ) {
         perror( "accept" );
         exit( -1 );
      }
      
      // 处理request
      processHTTPRequest( slaveSocket );
      close( slaveSocket );
   }
   
}

// 获取文件大小
long get_file_size(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

// 获取文件修改日期
time_t get_mod_time(std::string filename)
{
    struct stat stat_buf;
    stat(filename.c_str(), &stat_buf);
    return stat_buf.st_mtime;
}

// 用于sort entry array
bool compare_name_ascending(const path_entry_info &a, const path_entry_info &b)
{
   if (a.name.compare(b.name) < 0)
   {
      return true;
   }
   return false;
}
// 用于sort entry array
bool compare_name_descending(const path_entry_info &a, const path_entry_info &b)
{
   if (a.name.compare(b.name) < 0)
   {
      return false;
   }
   return true;
}
// 用于sort entry array
bool compare_size_ascending(const path_entry_info &a, const path_entry_info &b)
{
   if (a.size < b.size)
   {
      return true;
   }
   return false;
}
// 用于sort entry array
bool compare_size_descending(const path_entry_info &a, const path_entry_info &b)
{
   if (a.size < b.size)
   {
      return false;
   }
   return true;
}
// 用于sort entry array
bool compare_time_ascending(const path_entry_info &a, const path_entry_info &b)
{
   if (a.last_mod_time < b.last_mod_time)
   {
      return true;
   }
   return false;
}
// 用于sort entry array
bool compare_time_descending(const path_entry_info &a, const path_entry_info &b)
{
   if (a.last_mod_time < b.last_mod_time)
   {
      return false;
   }
   return true;
}