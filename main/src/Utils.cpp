#include "Utils.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QProcess>
#include <QDateTime>
#include <QUrl>
#include <QLoggingCategory>
#include <random>
#include <chrono>

Q_LOGGING_CATEGORY(UTILS, "utils")

QString Utils::m_lastError;

// Base64 decoding implementation
QByteArray DecodeB64IfValid(const QString &input, QByteArray::Base64Options options) {
    if (input.isEmpty()) {
        return QByteArray();
    }

    // Remove whitespace and validate base64 characters
    QString cleanInput = input.trimmed();
    if (cleanInput.isEmpty()) {
        return QByteArray();
    }

    // Check for valid base64 characters
    QRegularExpression base64Regex("^[A-Za-z0-9+/]*={0,2}$");
    if (!base64Regex.match(cleanInput).hasMatch()) {
        return QByteArray();
    }

    QByteArray decoded = QByteArray::fromBase64(cleanInput.toUtf8(), options);
    return decoded;
}

// String helper functions
QString SubStrBefore(const QString &str, const QString &sep) {
    int index = str.indexOf(sep);
    if (index >= 0) {
        return str.left(index);
    }
    return str;
}

QString SubStrAfter(const QString &str, const QString &sep) {
    int index = str.indexOf(sep);
    if (index >= 0) {
        return str.mid(index + sep.length());
    }
    return QString();
}

QString GetQueryValue(const QUrlQuery &q, const QString &key, const QString &def) {
    if (q.hasQueryItem(key)) {
        return q.queryItemValue(key);
    }
    return def;
}

// JSON helper functions
QJsonObject QString2QJsonObject(const QString &jsonString) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || doc.isNull() || !doc.isObject()) {
        return QJsonObject();
    }
    return doc.object();
}

QString QJsonObject2QString(const QJsonObject &jsonObject, bool compact) {
    QJsonDocument doc;
    doc.setObject(jsonObject);
    return compact ? doc.toJson(QJsonDocument::Compact) : doc.toJson(QJsonDocument::Indented);
}

// Enhanced file operations
QString Utils::readFileText(const QString& filePath) {
    clearError();

    QFile file(filePath);
    if (!file.exists()) {
        setLastError(QString("File does not exist: %1").arg(filePath));
        return QString();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setLastError(QString("Cannot open file for reading: %1 - %2").arg(filePath, file.errorString()));
        return QString();
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    qCDebug(UTILS) << "Successfully read file:" << filePath << "Size:" << content.size() << "bytes";
    return content;
}

bool Utils::writeFileText(const QString& filePath, const QString& content) {
    clearError();

    // Ensure directory exists
    if (!ensureDirectoryExists(QFileInfo(filePath).absolutePath())) {
        setLastError(QString("Cannot create directory for file: %1").arg(filePath));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setLastError(QString("Cannot open file for writing: %1 - %2").arg(filePath, file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream << content;
    file.close();

    qCDebug(UTILS) << "Successfully wrote file:" << filePath << "Size:" << content.size() << "bytes";
    return true;
}

bool Utils::appendFileText(const QString& filePath, const QString& content) {
    clearError();

    // Ensure directory exists
    if (!ensureDirectoryExists(QFileInfo(filePath).absolutePath())) {
        setLastError(QString("Cannot create directory for file: %1").arg(filePath));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        setLastError(QString("Cannot open file for appending: %1 - %2").arg(filePath, file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream << content;
    file.close();

    qCDebug(UTILS) << "Successfully appended to file:" << filePath << "Size:" << content.size() << "bytes";
    return true;
}

QStringList Utils::readFileLines(const QString& filePath) {
    QString content = readFileText(filePath);
    if (content.isEmpty()) {
        return QStringList();
    }
    return content.split('\n', Qt::SkipEmptyParts);
}

bool Utils::writeFileLines(const QString& filePath, const QStringList& lines) {
    QString content = lines.join('\n');
    return writeFileText(filePath, content);
}

bool Utils::readFile(const QString& filePath, QByteArray& data) {
    clearError();

    QFile file(filePath);
    if (!file.exists()) {
        setLastError(QString("File does not exist: %1").arg(filePath));
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        setLastError(QString("Cannot open file for reading: %1 - %2").arg(filePath, file.errorString()));
        return false;
    }

    data = file.readAll();
    file.close();

    qCDebug(UTILS) << "Successfully read binary file:" << filePath << "Size:" << data.size() << "bytes";
    return true;
}

bool Utils::writeFile(const QString& filePath, const QByteArray& data) {
    clearError();

    // Ensure directory exists
    if (!ensureDirectoryExists(QFileInfo(filePath).absolutePath())) {
        setLastError(QString("Cannot create directory for file: %1").arg(filePath));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        setLastError(QString("Cannot open file for writing: %1 - %2").arg(filePath, file.errorString()));
        return false;
    }

    qint64 written = file.write(data);
    file.close();

    if (written != data.size()) {
        setLastError(QString("Incomplete write to file: %1").arg(filePath));
        return false;
    }

    qCDebug(UTILS) << "Successfully wrote binary file:" << filePath << "Size:" << data.size() << "bytes";
    return true;
}

// File system utilities
bool Utils::ensureDirectoryExists(const QString& path) {
    clearError();

    QDir dir(path);
    if (dir.exists()) {
        return true;
    }

    if (!dir.mkpath(".")) {
        setLastError(QString("Cannot create directory: %1").arg(path));
        return false;
    }

    qCDebug(UTILS) << "Created directory:" << path;
    return true;
}

bool Utils::copyFile(const QString& source, const QString& destination) {
    clearError();

    if (!QFile::exists(source)) {
        setLastError(QString("Source file does not exist: %1").arg(source));
        return false;
    }

    // Ensure destination directory exists
    if (!ensureDirectoryExists(QFileInfo(destination).absolutePath())) {
        setLastError(QString("Cannot create destination directory: %1").arg(destination));
        return false;
    }

    if (!QFile::copy(source, destination)) {
        setLastError(QString("Failed to copy file from %1 to %2").arg(source, destination));
        return false;
    }

    qCDebug(UTILS) << "Successfully copied file from" << source << "to" << destination;
    return true;
}

bool Utils::removeFile(const QString& filePath) {
    clearError();

    if (!QFile::exists(filePath)) {
        return true; // File doesn't exist, consider it success
    }

    if (!QFile::remove(filePath)) {
        setLastError(QString("Failed to remove file: %1").arg(filePath));
        return false;
    }

    qCDebug(UTILS) << "Successfully removed file:" << filePath;
    return true;
}

QString Utils::getFileSizeString(const QString& filePath) {
    qint64 size = getFileSize(filePath);
    return bytesToString(size);
}

qint64 Utils::getFileSize(const QString& filePath) {
    QFileInfo info(filePath);
    return info.size();
}

// Path utilities
QString Utils::getAbsolutePath(const QString& relativePath, const QString& basePath) {
    QString base = basePath.isEmpty() ? QDir::currentPath() : basePath;
    return QDir(base).absoluteFilePath(relativePath);
}

QString Utils::getRelativePath(const QString& absolutePath, const QString& basePath) {
    QString base = basePath.isEmpty() ? QDir::currentPath() : basePath;
    return QDir(base).relativeFilePath(absolutePath);
}

QString Utils::normalizePath(const QString& path) {
    return QDir::cleanPath(path);
}

// Validation utilities
bool Utils::isValidUrl(const QString& url) {
    QUrl testUrl(url);
    return testUrl.isValid() && !testUrl.scheme().isEmpty();
}

bool Utils::isValidEmail(const QString& email) {
    QRegularExpression emailRegex("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    return emailRegex.match(email).hasMatch();
}

bool Utils::isValidPort(int port) {
    return port >= 1 && port <= 65535;
}

bool Utils::isValidIpAddress(const QString& ip) {
    QRegularExpression ipRegex("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
    return ipRegex.match(ip).hasMatch();
}

bool Utils::isValidUuid(const QString& uuid) {
    QRegularExpression uuidRegex("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
    return uuidRegex.match(uuid).hasMatch();
}

// Text processing utilities
QString Utils::removeComments(const QString& text, const QString& commentStart) {
    QString result = text;
    int commentIndex = result.indexOf(commentStart);
    while (commentIndex >= 0) {
        int lineEnd = result.indexOf('\n', commentIndex);
        if (lineEnd == -1) {
            result.remove(commentIndex, result.length() - commentIndex);
        } else {
            result.remove(commentIndex, lineEnd - commentIndex);
        }
        commentIndex = result.indexOf(commentStart, commentIndex);
    }
    return result;
}

QStringList Utils::splitLines(const QString& text, Qt::CaseSensitivity caseSensitivity) {
    Q_UNUSED(caseSensitivity)
    return text.split('\n', Qt::SkipEmptyParts);
}

QString Utils::cleanString(const QString& input) {
    return input.trimmed().simplified();
}

bool Utils::containsValidConfig(const QString& content) {
    return content.contains("vmess://") || content.contains("ss://") ||
           content.contains("vless://") || content.contains("trojan://");
}

// Data conversion utilities
QByteArray Utils::stringToByteArray(const QString& str) {
    return str.toUtf8();
}

QString Utils::byteArrayToString(const QByteArray& data) {
    return QString::fromUtf8(data);
}

QString Utils::bytesToString(qint64 bytes) {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
}

// URL and encoding utilities
QString Utils::urlDecode(const QString& url) {
    return QUrl::fromPercentEncoding(url.toUtf8());
}

QString Utils::urlEncode(const QString& url) {
    return QUrl::toPercentEncoding(url);
}

QString Utils::percentDecode(const QString& str) {
    return QUrl::fromPercentEncoding(str.toUtf8());
}

QString Utils::percentEncode(const QString& str) {
    return QUrl::toPercentEncoding(str);
}

// Time and date utilities
QString Utils::getCurrentTimestamp() {
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}

QString Utils::formatFileTime(const QDateTime& dateTime) {
    return dateTime.toString("yyyy-MM-dd HH:mm:ss");
}

QDateTime Utils::parseTimestamp(const QString& timestamp) {
    return QDateTime::fromString(timestamp, Qt::ISODate);
}

// Application utilities
QString Utils::getApplicationDirPath() {
    return QCoreApplication::applicationDirPath();
}

QString Utils::getUserDataPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Utils::getTempPath() {
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

QString Utils::generateTempFileName(const QString& prefix) {
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(0, 9999);

    return QString("%1_%2_%3.tmp")
        .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(distribution(generator));
}

// Error handling
QString Utils::getLastError() {
    return m_lastError;
}

void Utils::setLastError(const QString& error) {
    m_lastError = error;
    qCCritical(UTILS) << "Utils error:" << error;
}

bool Utils::hasError() {
    return !m_lastError.isEmpty();
}

void Utils::clearError() {
    m_lastError.clear();
}

// Helper function for URL queries
QUrlQuery GetQuery(const QUrl &url) {
    return QUrlQuery(url);
}
