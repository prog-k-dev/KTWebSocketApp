#include "WebSocketApp.h"

#include <QByteArray>
#include <vector>
#include <algorithm>
#include <fstream>
#include <istream>
#include "External/zlib/zlib.h"


namespace {

#define GZIP_WINDOWS_BIT 15 + 16
#define GZIP_CHUNK_SIZE 32 * 1024

} // namespace

//---------------------------------


using namespace WebSocketApp;

namespace WebSocketApp {

std::vector<ILogReceiver*> _logReceivers;

void AddLogReceiver(ILogReceiver* receiver) {
    auto it = std::find(_logReceivers.begin(), _logReceivers.end(), receiver);
    if (it != _logReceivers.end()) {
        DEBUG_OUTPUT_ERROR_LOG("[receiver(0x%0X)] already exists", receiver);
    } else {
        _logReceivers.push_back(receiver);
    }
}

void RemoveLogReceiver(ILogReceiver* receiver) {
    auto it = std::find(_logReceivers.begin(), _logReceivers.end(), receiver);
    if (it == _logReceivers.end()) {
        DEBUG_OUTPUT_ERROR_LOG("[receiver(0x%0X)] does not exist", receiver);
    } else {
        receiver->detachLogReceiver();
        _logReceivers.erase(it);
    }
}

static void WriteLog(const std::string& log) {
    for (auto receiver : _logReceivers) {
        receiver->ReceiveLog(log);
    }
}

static void WriteLog(const std::string& log, const QColor& color) {
    for (auto receiver : _logReceivers) {
        receiver->ReceiveColorLog(log, color);
    }
}

static void WriteWarningLog(const std::string& log) {
    for (auto receiver : _logReceivers) {
        receiver->ReceiveWarningLog(log);
    }
}

static void WriteErrorLog(const std::string& log) {
    for (auto receiver : _logReceivers) {
        receiver->ReceiveErrorLog(log);
    }
}

void OutputLog(const char* fileName, const char* functionName, int line) {
    OutputLog(fileName, functionName, line, nullptr);
}

void OutputLog(const char* fileName, const char* functionName, int line, const char* format, ...) {
    if (_logReceivers.empty()) {
        return;
    }

    std::string log;
#if defined(QT_DEBUG)
    log = StringBuilder::Format("%s(%d): %s(): ", fileName, line, functionName);
#endif
    if (format != nullptr) {
        va_list vaList;
        va_start(vaList, format);
        log += StringBuilder::vaFormat(format, vaList);
        va_end(vaList);
    }
    WriteLog(log);
}

void OutputColorLog(const char* fileName, const char* functionName, int line, const QColor& color) {
    OutputColorLog(fileName, functionName, line, color, nullptr);
}
void OutputColorLog(const char* fileName, const char* functionName, int line, const QColor& color, const char* format, ...) {
    if (_logReceivers.empty()) {
        return;
    }

    std::string log;
#if defined(QT_DEBUG)
    log = StringBuilder::Format("%s(%d): %s(): ", fileName, line, functionName);
#endif
    if (format != nullptr) {
        va_list vaList;
        va_start(vaList, format);
        log += StringBuilder::vaFormat(format, vaList);
        va_end(vaList);
    }
    WriteLog(log, color);
}

void OutputWarningLog(const char* fileName, const char* functionName, int line) {
    OutputWarningLog(fileName, functionName, line, nullptr);
}
void OutputWarningLog(const char* fileName, const char* functionName, int line, const char* format, ...) {
    if (_logReceivers.empty()) {
        return;
    }

    std::string log;
#if defined(QT_DEBUG)
    log = StringBuilder::Format("%s(%d): %s(): ", fileName, line, functionName);
#endif
    if (format != nullptr) {
        va_list vaList;
        va_start(vaList, format);
        log += StringBuilder::vaFormat(format, vaList);
        va_end(vaList);
    }
    WriteWarningLog(log);
}

void OutputErrorLog(const char* fileName, const char* functionName, int line) {
    OutputErrorLog(fileName, functionName, line, nullptr);
}
void OutputErrorLog(const char* fileName, const char* functionName, int line, const char* format, ...) {
    if (_logReceivers.empty()) {
        return;
    }

    std::string log;
#if defined(QT_DEBUG)
    log = StringBuilder::Format("%s(%d): %s(): ", fileName, line, functionName);
#endif
    if (format != nullptr) {
        va_list vaList;
        va_start(vaList, format);
        log += StringBuilder::vaFormat(format, vaList);
        va_end(vaList);
    }
    WriteErrorLog(log);
}

bool ReadFile(const std::string& path, std::vector<char>& buffer) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if(!file) {
        OUTPUT_ERROR_LOG("ファイルの読み込み失敗：%s", path.c_str());
        return false;
    }

    auto fileLength = file.seekg(0, std::ios::end).tellg();
    file.seekg(0, std::ios::beg);
    if (fileLength <= 0) {
        buffer.clear();
    }
    else {
        buffer.resize((int)fileLength);
        auto length = file.read(&buffer[0], fileLength).gcount();
        if (length != fileLength) {
            return false;
        }
    }
    file.close();

    return true;
}

bool writeFile(const std::string& path, const std::vector<char>& buffer) {
    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    {
        if (!file) {
            return false;
        }

        if (!buffer.empty()) {
            if (!file.write(&(buffer[0]), buffer.size())) {
                return false;
            }
        }
    }
    file.close();

    return true;
}

//---------------------------------

int CompressGZip(z_stream& stream, const void* src, int srcLength, std::vector<char>& dest) {
    // Declare vars
    int flush = 0;

    // Extract pointer to input data
    const uchar* input_data = (const uchar*)src;
    int input_data_left = srcLength;

    uchar *output_data = (uchar *)&(dest[0]);
    int output_length = 0;
    uchar out[GZIP_CHUNK_SIZE];

    // Compress data until available
    int ret = Z_OK;
    do {
        // Determine current chunk size
        int chunk_size = qMin(GZIP_CHUNK_SIZE, input_data_left);

        // Set deflater references
        stream.next_in = (uchar *)input_data;
        stream.avail_in = chunk_size;

        // Update interval variables
        input_data += chunk_size;
        input_data_left -= chunk_size;

        // Determine if it is the last chunk
        flush = (input_data_left <= 0 ? Z_FINISH : Z_NO_FLUSH);

        // Deflate chunk and cumulate output
        do {
            // Set deflater references
            stream.next_out = (uchar*)out;
            stream.avail_out = GZIP_CHUNK_SIZE;

            // Try to deflate chunk
            ret = deflate(&stream, flush);

            // Check errors
            if(ret == Z_STREAM_ERROR)
            {
                return -1;
            }

            // Determine compressed size
            int have = (GZIP_CHUNK_SIZE - stream.avail_out);

            // Cumulate result
            if(have > 0) {
                int nextLength = output_length + have;
                if (dest.size() < nextLength) {
                     dest.resize(nextLength * 2);
                     output_data = (uchar *)&(dest[output_length]);
                }

                memcpy(output_data, out, have);
                output_data += have;
                output_length += have;
            }
        } while (stream.avail_out == 0);
    } while (flush != Z_FINISH);

    return (ret != Z_STREAM_END) ? -1 : output_length;
}

int DecompressGZip(z_stream& stream, const void* src, int srcLength, std::vector<char>& dest) {
    const uchar *input_data = (const uchar*)src;
    int input_data_left = srcLength;

    uchar *output_data = (uchar *)&(dest[0]);
    int output_length = 0;
    uchar out[GZIP_CHUNK_SIZE];

    int ret = Z_OK;
    do {
        int chunk_size = std::min<int>(GZIP_CHUNK_SIZE, input_data_left);

        if(chunk_size <= 0) {
        break;
        }

        stream.next_in = (uchar*)input_data;
        stream.avail_in = chunk_size;

        // Update interval variables
        input_data += chunk_size;
        input_data_left -= chunk_size;

        // Inflate chunk and cumulate output
        do {
            // Set inflater references
            stream.next_out = out;
            stream.avail_out = GZIP_CHUNK_SIZE;

            // Try to inflate chunk
            ret = inflate(&stream, Z_NO_FLUSH);

            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            case Z_STREAM_ERROR:
                return -1;
            }

            // Determine decompressed size
            int have = (GZIP_CHUNK_SIZE - stream.avail_out);

            // Cumulate result
            if(have > 0) {
                int nextLength = output_length + have;
                if (dest.size() < nextLength) {
                    dest.resize(nextLength * 2);
                    output_data = (uchar *)&(dest[output_length]);
                }

                memcpy(output_data, out, have);
                output_data += have;
                output_length += have;
            }
        } while (stream.avail_out == 0);

    } while (ret != Z_STREAM_END);

    return (ret != Z_STREAM_END) ? -1 : output_length;
}

int CompressGZip(const void* src, int srcLength, std::vector<char>& compressed) {
    int result = -1;

    z_stream stream;
    {
        stream.zalloc = nullptr;
        stream.zfree = nullptr;
        stream.opaque = nullptr;
        stream.avail_in = 0;
        stream.next_in = nullptr;

        if (deflateInit2(&stream, Z_BEST_SPEED, Z_DEFLATED, GZIP_WINDOWS_BIT, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            return -1;
        }

        std::vector<char> buffer(srcLength);
        result = CompressGZip(stream, src, srcLength, buffer);
        if (result > 0) {
            compressed.resize(result);
            memcpy(&(compressed[0]), &(buffer[0]), result);
        }
    }
    deflateEnd(&stream);

    return result;
}

int DecompressGZip(const void* src, int srcLength, std::vector<char>& decompressed) {
    int result = -1;

    z_stream stream;
    {
        stream.zalloc = nullptr;
        stream.zfree = nullptr;
        stream.opaque = nullptr;
        stream.avail_in = 0;
        stream.next_in = nullptr;

        if (inflateInit2(&stream, GZIP_WINDOWS_BIT) != Z_OK) {
            return -1;
        }

        std::vector<char> buffer(srcLength);
        result = DecompressGZip(stream, src, srcLength, buffer);
        if (result > 0) {
            decompressed.resize(result);
            memcpy(&(decompressed[0]), &(buffer[0]), result);
        }
    }
    inflateEnd(&stream);

    return result;
}

} // namespace WebSocketApp
