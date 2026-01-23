#pragma once
#include <string>
#include <vector>

namespace dcp::recorder
{

class FileCompress{
public:
    enum class ErrorCode {
        Success = 0,
        InvalidInputPath,
        FailedToOpenFile,
        FailedToCreateOutput,
        CompressionFailed,
        UnknownError,
        FailedToCreateTarFile
    };

    FileCompress() = default;
    virtual ~FileCompress() = default;

    static ErrorCode CompressFiles(const std::vector<std::string>& inputFiles,
                                   const std::string& outputFile);
    ErrorCode CompressSingleFileToLz4(const std::string& inputFile,
                                      const std::string& outputFile);

private:
    static ErrorCode CompressData(const std::vector<char>& input, std::vector<char>& compressedData);
    static ErrorCode GetFilesInDirectory(const std::string& directory, std::vector<std::string>& fileList);

};

}
