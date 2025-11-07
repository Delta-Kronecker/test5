#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>
#include <QTextStream>
#include <QTimer>
#include <QEventLoop>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QUrl>
#include <QRegularExpression>

// Logging category for Utils
Q_DECLARE_LOGGING_CATEGORY(UTILS)

// Base64 decoding
QByteArray DecodeB64IfValid(const QString &input, QByteArray::Base64Options options = QByteArray::Base64Encoding);

// String helpers
QString SubStrBefore(const QString &str, const QString &sep);
QString SubStrAfter(const QString &str, const QString &sep);
QString GetQueryValue(const QUrlQuery &q, const QString &key, const QString &def = "");

// JSON helpers
QJsonObject QString2QJsonObject(const QString &jsonString);
QString QJsonObject2QString(const QJsonObject &jsonObject, bool compact = false);

// Enhanced file operations class
class Utils {
public:
    // File I/O operations
    static QString readFileText(const QString& filePath);
    static bool writeFileText(const QString& filePath, const QString& content);
    static bool appendFileText(const QString& filePath, const QString& content);
    static QStringList readFileLines(const QString& filePath);
    static bool writeFileLines(const QString& filePath, const QStringList& lines);

    // Legacy compatibility
    static QString ReadFileText(const QString& path) { return readFileText(path); }
    static bool WriteFileText(const QString& path, const QString& text) { return writeFileText(path, text); }

    // Safe file operations with error handling
    static bool readFile(const QString& filePath, QByteArray& data);
    static bool writeFile(const QString& filePath, const QByteArray& data);

    // File system utilities
    static bool ensureDirectoryExists(const QString& path);
    static bool copyFile(const QString& source, const QString& destination);
    static bool removeFile(const QString& filePath);
    static QString getFileSizeString(const QString& filePath);
    static qint64 getFileSize(const QString& filePath);

    // Path utilities
    static QString getAbsolutePath(const QString& relativePath, const QString& basePath = QString());
    static QString getRelativePath(const QString& absolutePath, const QString& basePath = QString());
    static QString normalizePath(const QString& path);

    // Validation utilities
    static bool isValidUrl(const QString& url);
    static bool isValidEmail(const QString& email);
    static bool isValidPort(int port);
    static bool isValidIpAddress(const QString& ip);
    static bool isValidUuid(const QString& uuid);

    // Text processing utilities
    static QString removeComments(const QString& text, const QString& commentStart = "#");
    static QStringList splitLines(const QString& text, Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive);
    static QString cleanString(const QString& input);
    static bool containsValidConfig(const QString& content);

    // Data conversion utilities
    static QByteArray stringToByteArray(const QString& str);
    static QString byteArrayToString(const QByteArray& data);
    static QString bytesToString(qint64 bytes);

    // URL and encoding utilities
    static QString urlDecode(const QString& url);
    static QString urlEncode(const QString& url);
    static QString percentDecode(const QString& str);
    static QString percentEncode(const QString& str);

    // Time and date utilities
    static QString getCurrentTimestamp();
    static QString formatFileTime(const QDateTime& dateTime);
    static QDateTime parseTimestamp(const QString& timestamp);

    // Application utilities
    static QString getApplicationDirPath();
    static QString getUserDataPath();
    static QString getTempPath();
    static QString generateTempFileName(const QString& prefix = "config");

    // Error handling
    static QString getLastError();
    static void setLastError(const QString& error);
    static bool hasError();
    static void clearError();

private:
    static QString m_lastError;
};

// Helper to get query from URL
QUrlQuery GetQuery(const QUrl &url);

#endif // UTILS_H