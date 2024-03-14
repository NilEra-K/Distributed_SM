// 客户机
// 定义主函数
#include <unistd.h>
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/acl/lib_acl.h>
#include "types.h"
#include "client.h"

// 打印命令行用法
void usage(const char* cmd) {
    // ./client 127.0.0.1:21000;... groups                                  查看组列表
    fprintf(stderr, "GROUP : %s <taddrs> groups\n", cmd);

    // ./client 127.0.0.1:21000;... upload tnvideo tnv001 ~/videos/001.mp4  上传文件
    fprintf(stderr, "UPLOAD : %s <taddrs> upload <appid> <userid> <filepath>\n", cmd);

    // ./client 127.0.0.1:21000 filesize tnvideo tnv001 169ABC...           取文件大小
    fprintf(stderr, "FILESIZE : %s <taddrs> filesize <appid> <userid> <fileid>\n", cmd);

    // ./client 127.0.0.1:21000 download tnv001 169ABC... 0 0               下载文件
    fprintf(stderr, "DOWNLOAD : %s <taddrs> download <appid> <userid> <fileid> <offset> <size>\n", cmd);

    // ./client 127.0.0.1:21000 delete tnv001 169ABC...                     删除文件
    fprintf(stderr, "DELETE : %s <taddrs> delete <appid> <userid> <fileid>\n", cmd);
}

// 根据 用户ID 生成 文件ID
std::string genfileid(const char* userid) {
    srand(time(NULL));
    struct timeval now;
    gettimeofday(&now, NULL);

    acl::string str;
    str.format("%s@%d@%lX@%d", userid, getpid(), acl_pthread_self(), rand());

    acl::md5 md5;
    md5.update(str.c_str(), str.size());
    md5.finish();

    char buf[33] = {};
    strncpy(buf, md5.get_string(), 32);
    memmove(buf, buf + 8, 16);
    memset(buf + 16, 0, 16);

    static int count = 0;
    if (count >= 8000) {
        count = 0;
    }

    acl::string fileid;
    fileid.format("%08lx%06lx%s%04d%02d", now.tv_sec, now.tv_usec, buf, ++count, rand() % 100);

    return fileid.c_str();
}

// 主函数
int main(int argc, char* argv[]) {
    const char* cmd = argv[0];
    if (argc < 3) {
        usage(cmd);
        return -1;
    }

    const char* taddrs = argv[1];
    const char* subcmd = argv[2];

    // 初始化 ACL 库
    acl::acl_cpp_init();

    // 日志打印到标准输出
    acl::log::stdout_open(true);

    // 初始化客户机
    if (client_c::init(taddrs) != OK) {
        return -1;
    }

    // 客户机对象
    client_c client;

    if (!strcmp(subcmd, "groups")) { // 从跟踪服务器获取组列表
        std::string groups;
        if (client.groups(groups) != OK) {
            client_c::deinit();
            return -1;
        }
    } else if (!strcmp(subcmd, "upload")){ // 向存储服务器上传文件
        if (argc < 6) {
            client_c::deinit();
            usage(cmd);
            return -1;
        }
        const char* appid       = argv[3];
        const char* userid      = argv[4];
        std::string fileid      = genfileid(userid);
        const char* filepath    = argv[5];
        if (client.upload(appid, userid, fileid.c_str(), filepath) != OK) {
            client_c::deinit();
            return -1;
        }
        printf("Upload Success: %s\n", fileid.c_str());
    } else if (!strcmp(subcmd, "filesize")){ // 向存储服务器询问文件大小
        if (argc < 6) {
            client_c::deinit();
            usage(cmd);
            return -1;
        }
        const char* appid       = argv[3];
        const char* userid      = argv[4];
        const char* fileid      = argv[5];
        long long   filesize    = 0;
        if (client.filesize(appid, userid, fileid, &filesize) != OK) {
            client_c::deinit();
            return -1;
        }
        printf("Get Filesize Success: %lld\n", filesize);
    } else if (!strcmp(subcmd, "download")){ // 从存储服务器下载文件
        if (argc < 8) {
            client_c::deinit();
            usage(cmd);
            return -1;
        }
        const char* appid       = argv[3];
        const char* userid      = argv[4];
        const char* fileid      = argv[5];
        long long   offset      = atoll(argv[6]);
        long long   size        = atoll(argv[7]);
        char* filedata          = NULL;
        long long filesize      = 0;
        if (client.download(appid, userid, fileid, offset, size, &filedata, &filesize) != OK) {
            client_c::deinit();
            return -1;
        }
        printf("Download Success: %lld\n", filesize);
        free(filedata);
    } else if (!strcmp(subcmd, "delete")){ // 删除存储服务器上的文件
        if (argc < 6) {
            client_c::deinit();
            usage(cmd);
            return -1;
        }

        const char* appid       = argv[3];
        const char* userid      = argv[4];
        const char* fileid      = argv[5];
        if (client.del(appid, userid, fileid) != OK) {
            client_c::deinit();
            return -1;
        }
        printf("Delete Success: %s\n", fileid);
   
    } else {
        client_c::deinit();
        usage(cmd);
        return -1;
    }

    // 终结化客户机
    client_c::deinit();
    return 0;
}