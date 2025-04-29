// file实用工具类，一个文件读取模块，用于读取服务器中的内容（比如网页html），便于交给HTTP服务器做封装和响应
#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <muduo/base/Logging.h>

class FileUtil
{
public:
    FileUtil(std::string filePath) : filePath_(filePath), file_(filePath, std::ios::binary) {}

    ~FileUtil()
    {
        file_.close();
    }

    // 判断是否是有效路径
    bool isValid() const
    {
        return file_.is_open();
    }

    // 重置打开默认文件
    void resetDefaultFile()
    {
        file_.close();
        file_.open("/Gomoku/GomokuServer/resource/NotFound.html", std::ios::binary);
    }

    uint64_t size()
    {                                  // seekg和tellg是std::istream这个库下面的函数
        file_.seekg(0, std::ios::end); // 定位到文件末尾
        uint64_t fileSize = file_.tellg();
        file_.seekg(0, std::ios::beg); // 返回文件开头
        return fileSize;
    }

    void readFile(std::vector<char> &buffer)
    {
        if (file_.read(buffer.data(), size()))
        { // 隐含用this指针调用size()，即向buffer指向的内存写入size()大小的数据，.data()会返回一个指向内部连续内存的指针 char*
            LOG_INFO << "File content load into memory (" << size() << "bytes)";
        }
        else
        {
            LOG_ERROR << "File read failed";
        }
    }

private:
    std::string filePath_;
    std::ifstream file_; // 输入文件流的格式，一种构造函数原型是
    // explicit ifstream(const char* filename, ios_base::openmode mode = ios_base::in);
};