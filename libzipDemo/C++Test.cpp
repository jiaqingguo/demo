// C++Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//



#include <filesystem>
#include <fstream>
#include <iostream>
#include "zip.h"
//#include <string>
//#include <vector>
#include <windows.h>
namespace fs = std::filesystem;

#define DBG_SEQ 0
#define LOGI printf
#define LOGD printf
#define LOGE printf



void extractZip(std::filesystem::path zip_file_path, const std::string& output_dir) {
    // 打开 ZIP 文件
    int err = 0;
    zip_t* zip = zip_open(zip_file_path.generic_u8string().c_str(), ZIP_RDONLY, &err);
    if (!zip) {
        zip_error_t zip_error;
        zip_error_init(&zip_error);
        zip_error_set(&zip_error, err, 0);
        std::cerr << "Failed to open zip file: " << zip_file_path << " - " << zip_error_strerror(&zip_error) << std::endl;
        zip_error_fini(&zip_error);
        return;
    }

    // 创建输出目录
    fs::create_directories(output_dir);

    // 获取 ZIP 文件中的文件数量
    zip_uint64_t num_entries = zip_get_num_entries(zip, 0);
    for (zip_uint64_t i = 0; i < num_entries; ++i) {
        const char* name = zip_get_name(zip, i, 0);
        if (!name) {
            std::cerr << "Failed to get name of entry " << i << std::endl;
            continue;
        }

        // 使用名称构造输出文件路径
        fs::path output_file_path = fs::path(output_dir) / name;

        // 如果是目录，创建目录
        if (name[strlen(name) - 1] == '/') {
            fs::create_directories(output_file_path);
            continue;
        }

        // 创建输出文件并写入内容
        zip_file_t* zip_file = zip_fopen(zip, name, 0);
        if (!zip_file) {
            std::cerr << "Failed to open entry " << name << std::endl;
            continue;
        }

        // 读取文件内容
        std::ofstream output_file(output_file_path, std::ios::binary);
        if (!output_file.is_open()) {
            std::cerr << "Failed to create output file: " << output_file_path << std::endl;
            zip_fclose(zip_file);
            continue;
        }

        char buffer[8192];
        zip_int64_t read_size;
        while ((read_size = zip_fread(zip_file, buffer, sizeof(buffer))) > 0) {
            output_file.write(buffer, read_size);
        }

        zip_fclose(zip_file);
        output_file.close();
    }

    zip_close(zip);
}


void CompressFile2Zip(std::filesystem::path unZipFilePath,const char* relativeName, zip_t* zipArchive) 
{
    std::ifstream file(unZipFilePath, std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t bufferSize = file.tellg();
    char* bufferData = (char*)malloc(bufferSize);

    file.seekg(0, std::ios::beg);
    file.read(bufferData, bufferSize);

    //第四个参数如果非0，会自动托管申请的资源，直到zip_close之前自动销毁。
    zip_source_t* source =
        zip_source_buffer(zipArchive, bufferData, bufferSize, 1);

    if (source) 
    {
        if (zip_file_add(zipArchive, relativeName, source, ZIP_FL_OVERWRITE) < 0) {
            std::cerr << "Failed to add file " << unZipFilePath
                << " to zip: " << zip_strerror(zipArchive) << std::endl;
            zip_source_free(source);
        }
    }
    else {
        std::cerr << "Failed to create zip source for " << unZipFilePath << ": "
            << zip_strerror(zipArchive) << std::endl;
    }
}

void CompressFile(std::filesystem::path unZipFilePath, std::filesystem::path zipFilePath) 
{
    int errorCode = 0;
    zip_t* zipArchive = zip_open(zipFilePath.generic_u8string().c_str(),
        ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
    if (zipArchive) 
    {
        CompressFile2Zip(unZipFilePath, unZipFilePath.filename().string().c_str(),
            zipArchive);

        errorCode = zip_close(zipArchive);
        if (errorCode != 0)
        {
            zip_error_t zipError;
            zip_error_init_with_code(&zipError, errorCode);
            std::cerr << zip_error_strerror(&zipError) << std::endl;
            zip_error_fini(&zipError);
        }
    }
    else {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, errorCode);
        std::cerr << "Failed to open output file " << zipFilePath << ": "
            << zip_error_strerror(&zipError) << std::endl;
        zip_error_fini(&zipError);
    }
}

void CompressDirectory2Zip(std::filesystem::path rootDirectoryPath,
    std::filesystem::path directoryPath,
    zip_t* zipArchive)
{
    if (rootDirectoryPath != directoryPath) 
    {
        if (zip_dir_add(zipArchive,
            std::filesystem::relative(directoryPath, rootDirectoryPath)
            .generic_u8string()
            .c_str(),
            ZIP_FL_ENC_UTF_8) < 0) {
            std::cerr << "Failed to add directory " << directoryPath
                << " to zip: " << zip_strerror(zipArchive) << std::endl;
        }
    }

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
    {
        if (entry.is_regular_file()) 
        {
            CompressFile2Zip(
                entry.path()/*.generic_u8string()*/,
                std::filesystem::relative(entry.path(), rootDirectoryPath)
                .generic_u8string()
                .c_str(),
                zipArchive);
        }
        else if (entry.is_directory())
        {
            CompressDirectory2Zip(rootDirectoryPath, entry.path()/*.generic_u8string()*/,
                zipArchive);
        }
    }
}

void CompressDirectory(std::filesystem::path directoryPath,
    std::filesystem::path zipFilePath) 
{
    int errorCode = 0;
    zip_t* zipArchive = zip_open(zipFilePath.generic_u8string().c_str(),
        ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
    if (zipArchive)
    {
        CompressDirectory2Zip(directoryPath, directoryPath, zipArchive);

        errorCode = zip_close(zipArchive);
        if (errorCode != 0) 
        {
            zip_error_t zipError;
            zip_error_init_with_code(&zipError, errorCode);
            std::cerr << zip_error_strerror(&zipError) << std::endl;
            zip_error_fini(&zipError);
        }
    }
    else {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, errorCode);
        std::cerr << "Failed to open output file " << zipFilePath << ": "
            << zip_error_strerror(&zipError) << std::endl;
        zip_error_fini(&zipError);
    }
}



// 只支持多个文件 目录的话会缺少层级;
void CompressMultFile(const std::vector<fs::path>& paths, const fs::path& zipFilePath) {
    int errorCode = 0;
    zip_t* zipArchive = zip_open(zipFilePath.u8string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
    if (!zipArchive) {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, errorCode);
        std::cerr << "Failed to open output file " << zipFilePath << ": " << zip_error_strerror(&zipError) << std::endl;
        zip_error_fini(&zipError);
        return;
    }

    for (const auto& path : paths)
    {
        if (fs::exists(path)) 
        {
            if (fs::is_regular_file(path)) {
                CompressFile2Zip(path, path.filename().u8string().c_str(), zipArchive);
            }
            else if (fs::is_directory(path)) {
                CompressDirectory2Zip(path, path, zipArchive);
            }
        }
        else {
            std::cerr << "Path does not exist: " << path << std::endl;
        }
    }

    // 关闭 ZIP 文件
    errorCode = zip_close(zipArchive);
    if (errorCode != 0) {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, errorCode);
        std::cerr << zip_error_strerror(&zipError) << std::endl;
        zip_error_fini(&zipError);
    }
}

void CompressMult2Zip(const std::vector<fs::path>& paths, const fs::path& zipFilePath)
{

}
int main()
{
    // libzip库的使用
  // 要压缩的文件和目录列表
    std::vector<fs::path> paths = {
        "D:/CS/贾庆国/qsvgd.dll",
        "D:/CS/贾庆国/test.txt",
        "D:/CS/贾庆国/qjpegd.dll",
        "D:/CS/贾庆国/qwebpd.dll",
    
        // 可以在这里添加更多的文件和目录
    };

    // 压缩成一个 ZIP 文件
    CompressMultFile(paths, "D:/CS/贾庆国/multiple_files.zip");

   // std::vector<std::string> filenames;
   // std::filesystem::path zip_file_path = "D:/CS/贾庆国/10.27.rar";
   // std::string zip_file_path = "D:/CS/贾庆国/resources.zip";  // ZIP 文件路径
  //  std::string output_dir = "D:/CS/贾庆国/out7z";              // 解压目标目录

  //  extractZip(zip_file_path, output_dir);
    // 压缩文件
   // compress(filenames, zip_filename);
    //decompress(zip_filename, "D:/CS/exampleCE");

    //压缩文件
  //压缩文件夹   不可以中文;
    //CompressDirectory(("D:/CS/贾庆国/printsupport"), ("D:/CS/贾庆国/printsupport11.zip"));
    system("pause");


    std::cout << "Hello World!\n";

    /*  std::vector<fs::path> paths;
      paths.push_back("D:/CS/贾庆国/qicod.dll");
      paths.push_back("D:/CS/贾庆国/qjpegd.dll");
      paths.push_back("D:/CS/贾庆国/qsvgd.dll");*/

      //  std::vector<fs::path> paths;
      //  paths.push_back("D:/CS/贾庆国/test.txt"); // 使用测试文件

        //create_zip("D:/CS/贾庆国/qtDllCS.zip", paths);

}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
