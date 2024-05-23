1.下载cmake
2.下载Jsoncpp,
https://github.com/open-source-parsers/jsoncpp
3.使用cmake生成vs项目
4.进入项目编译jsoncpp_lib文件生成jsoncpp.lib和jsoncpp.ddl文件
5.在下载的Jsoncpp文件中找到include文件,和上面两个放一起
6.配置项目
1)VC++->包含目录->输入头文件的目录(include目录,所以使用时为#include <json/json.h>)
2)VC++->库目录->输入lib文件所在的目录
3)链接器->输入->附加依赖项->输入jsoncpp.lib
7.如果成功编译了可执行文件但无法执行,将ddl文件放入源文件所在的目录
