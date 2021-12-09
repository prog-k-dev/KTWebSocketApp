#ifndef WEBSOCKETAPP_H
#define WEBSOCKETAPP_H

#include <QtCore/QDebug>
#include <QColor>

#include <string>
#include <sstream>
namespace WebSocketApp {

#ifdef Q_OS_WIN
    inline int vasprintf(char** buffer, const char* format, va_list& vaList) {
        int length = _vscprintf(format, vaList);
        *buffer = (char*)malloc(length + 1);
        return  vsprintf(*buffer, format, vaList);
    }
#endif

//---------------------------------

static const ushort SOCKET_PORT_DEFAULT = 5637;
static const char* SOCKET_PATH = "/WebSocketApp/takahashi_kenji";

static const QColor LOG_NORMAL_COLOR(255, 255, 255);
static const QColor LOG_INFO_COLOR(50, 255, 50);
static const QColor LOG_WARNING_COLOR(255, 255, 0);
static const QColor LOG_ERROR_COLOR(255, 0, 0);

#define FORMAT_STR(...) WebSocketApp::StringBuilder::Format(__VA_ARGS__)
#define QFORMAT_STR(...) QString::asprintf(__VA_ARGS__)

#define OUTPUT_LOG(...) OUTPUT_INFO_LOG(_VA_ARGS__)
#define OUTPUT_INFO_LOG(...) WebSocketApp::OutputColorLog(__FILE__, __func__, __LINE__, WebSocketApp::LOG_INFO_COLOR, __VA_ARGS__)
#define OUTPUT_WARNING_LOG(...) WebSocketApp::OutputWarningLog(__FILE__, __func__, __LINE__, __VA_ARGS__)
#define OUTPUT_ERROR_LOG(...) WebSocketApp::OutputErrorLog(__FILE__, __func__, __LINE__, __VA_ARGS__)
#define OUTPUT_COLOR_LOG(color, ...) WebSocketApp::OutputColorLog(__FILE__, __func__, __LINE__, color, __VA_ARGS__)

#if defined(QT_DEBUG)
#define DEBUG_OUTPUT_LOG(...) DEBUG_OUTPUT_INFO_LOG(__VA_ARGS__)
#define DEBUG_OUTPUT_INFO_LOG(...) WebSocketApp::OutputColorLog(__FILE__, __func__, __LINE__, WebSocketApp::LOG_INFO_COLOR, __VA_ARGS__)
#define DEBUG_OUTPUT_FUNCTION_LOG() WebSocketApp::OutputLog(__FILE__, __func__, __LINE__, "\n");
#define DEBUG_OUTPUT_WARNING_LOG(...) WebSocketApp::OutputWarninLog(__FILE__, __func__, __LINE__, __VA_ARGS__)
#define DEBUG_OUTPUT_ERROR_LOG(...) WebSocketApp::OutputErrorLog(__FILE__, __func__, __LINE__, __VA_ARGS__)
#define DEBUG_OUTPUT_COLOR_LOG(color, ...) WebSocketApp::OutputColorLog(__FILE__, __func__, __LINE__, color, __VA_ARGS__)
#else
#define DEBUG_OUTPUT_LOG(...)
#define DEBUG_OUTPUT_INFO_LOG(...)
#define DEBUG_OUTPUT_FUNCTION_LOG()
#define DEBUG_OUTPUT_WARNING_LOG(...)
#define DEBUG_OUTPUT_ERROR_LOG(...)
#define DEBUG_OUTPUT_COLOR_LOG(color, ...)
#endif // defined(QT_DEBUG)

//---------------------------------

class ILogReceiver {
public:
    virtual void ReceiveLog(const std::string& log) = 0;
    virtual void ReceiveColorLog(const std::string& log, const QColor& color) = 0;
    virtual void ReceiveWarningLog(const std::string& log) = 0;
    virtual void ReceiveErrorLog(const std::string& log) = 0;
    virtual void detachLogReceiver() = 0;
}; // class ILogReceiver

extern void AddLogReceiver(ILogReceiver* receiver);
extern void RemoveLogReceiver(ILogReceiver* receiver);

extern void OutputLog(const char* fileName, const char* functionName, int line);
extern void OutputLog(const char* fileName, const char* functionName, int line, const char* format, ...);
extern void OutputColorLog(const char* fileName, const char* functionName, int line, const QColor& color, const char* format, ...);
extern void OutputColorLog(const char* fileName, const char* functionName, int line, const QColor& color);
extern void OutputWarningLog(const char* fileName, const char* functionName, int line);
extern void OutputWarningLog(const char* fileName, const char* functionName, int line, const char* format, ...);
extern void OutputErrorLog(const char* fileName, const char* functionName, int line);
extern void OutputErrorLog(const char* fileName, const char* functionName, int line, const char* format, ...);

//---------------------------------

class StringBuilder {
public:
    explicit StringBuilder() {
    }

    explicit StringBuilder(bool src) {
        m_stream << (src ? "true" : "false");
    }

    explicit StringBuilder(int32_t src) {
        m_stream << std::dec << src;
    }

    explicit StringBuilder(uint32_t src) {
        m_stream << std::dec << src;
    }

    explicit StringBuilder(float src) {
        m_stream << std::fixed << src;
    }

    explicit StringBuilder(double src) {
        m_stream << std::fixed << src;
    }

    explicit StringBuilder(const std::string& src) {
        m_stream << src;
    }

    explicit StringBuilder(const char* src) {
        m_stream << src;
    }

    StringBuilder& operator<<(bool src) {
        m_stream << (src ? "true" : "false");
        return *this;
    }

    StringBuilder& operator<<(int32_t src) {
        m_stream << std::dec << src;
        return *this;
    }

    StringBuilder& operator<<(uint32_t src) {
        m_stream << std::dec << src;
        return *this;
    }

    StringBuilder& operator<<(float src) {
        m_stream << std::fixed << src;
        return *this;
    }

    StringBuilder& operator<<(double src) {
        m_stream << std::fixed << src;
        return *this;
    }

    StringBuilder& operator<<(const char* src) {
        m_stream << src;
        return *this;
    }

    StringBuilder& operator<<(const std::string& src) {
        m_stream << src;
        return *this;
    }

    static std::string Format(const char* fmt, ...) {
        std::string result;

        va_list vaList;
        {
            va_start(vaList, fmt);

            char* buffer = nullptr;
            if(vasprintf(&buffer,fmt,vaList) != -1) {
                result = buffer;
            }
            if (buffer != nullptr) {
                free(buffer);
            }
        }
        va_end(vaList);

        return result;
    }

    static std::string vaFormat(const char* fmt, va_list vaList) {
        std::string result;

        char* buffer = nullptr;
        if(vasprintf(&buffer, fmt, vaList) != -1) {
            result = buffer;
        }
        if (buffer != nullptr) {
            free(buffer);
        }

        return result;
    }

    StringBuilder& clear() {
        m_stream.clear();
        return *this;
    }

    std::string str() const {
        return m_stream.str();
    }

private:
    std::stringstream m_stream;
}; // class StringBuilder

//---------------------------------

extern bool ReadFile(const std::string& path, std::vector<char>& buffer);
extern bool writeFile(const std::string& path, const std::vector<char>& buffer);

extern int CompressGZip(const void* src, int srcLength, std::vector<char>& compressed);
extern int DecompressGZip(const void* src, int srcLength, std::vector<char>& decompressed);

} // namespace WebSocketApp

#endif // WEBSOCKETAPP_H
