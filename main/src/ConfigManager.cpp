#include "ConfigManager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QDirIterator>

ConfigManager& ConfigManager::getInstance()
{
    static ConfigManager instance;
    return instance;
}

void ConfigManager::initializeDefaultConfig()
{
    m_config.dataDirectory = getDefaultDataPath();
    m_config.subFilePath = getAbsolutePath("../data/Sub.txt");
    m_config.configOutputDirectory = getAbsolutePath("../data/Config");
    m_config.workingDirectory = getAbsolutePath("../data/working");
    m_config.maxConcurrentDownloads = 10;
    m_config.requestTimeout = 30000; // 30 seconds
    m_config.createMissingDirectories = true;
    m_config.verboseLogging = true;
}

QString ConfigManager::getDefaultDataPath() const
{
    // Check for GitHub Actions environment
    QString githubWorkspace = getEnvironmentVariable("GITHUB_WORKSPACE");
    if (!githubWorkspace.isEmpty()) {
        return QDir(githubWorkspace + "/data").absolutePath();
    }

    // Check for other CI environments
    QString ciWorkspace = getEnvironmentVariable("CI_WORKSPACE");
    if (!ciWorkspace.isEmpty()) {
        return QDir(ciWorkspace + "/data").absolutePath();
    }

    // Default to application directory
    return getApplicationDirPath() + "/data";
}

QString ConfigManager::getApplicationDirPath() const
{
    return QCoreApplication::applicationDirPath();
}

QString ConfigManager::getEnvironmentVariable(const QString& key, const QString& defaultValue) const
{
    return QProcessEnvironment::systemEnvironment().value(key, defaultValue);
}

bool ConfigManager::loadConfig(const QString& configPath)
{
    QString configFilePath = configPath;

    if (configFilePath.isEmpty()) {
        // Try to find config in multiple locations
        QStringList possiblePaths = {
            getApplicationDirPath() + "/config.json",
            getGitHubWorkspacePath() + "/config.json",
            getDefaultDataPath() + "/../config.json"
        };

        for (const QString& path : possiblePaths) {
            if (QFile::exists(path)) {
                configFilePath = path;
                break;
            }
        }

        if (configFilePath.isEmpty()) {
            qWarning() << "No config file found, using defaults";
            initializeDefaultConfig();
            return true;
        }
    }

    QFile file(configFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file:" << configFilePath;
        initializeDefaultConfig();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in config file";
        initializeDefaultConfig();
        return false;
    }

    QJsonObject config = doc.object();

    // Load configuration values with fallbacks
    m_config.dataDirectory = config["dataDirectory"].toString(getDefaultDataPath());
    m_config.subFilePath = config["subFilePath"].toString(getAbsolutePath("../data/Sub.txt"));
    m_config.configOutputDirectory = config["configOutputDirectory"].toString(getAbsolutePath("../data/Config"));
    m_config.workingDirectory = config["workingDirectory"].toString(getAbsolutePath("../data/working"));
    m_config.maxConcurrentDownloads = config["maxConcurrentDownloads"].toInt(10);
    m_config.requestTimeout = config["requestTimeout"].toInt(30000);
    m_config.createMissingDirectories = config["createMissingDirectories"].toBool(true);
    m_config.verboseLogging = config["verboseLogging"].toBool(true);

    m_configFilePath = configFilePath;

    if (m_config.verboseLogging) {
        qInfo() << "Configuration loaded from:" << configFilePath;
    }

    return validateConfig();
}

bool ConfigManager::saveConfig(const QString& configPath)
{
    QString savePath = configPath.isEmpty() ? m_configFilePath : configPath;
    if (savePath.isEmpty()) {
        savePath = getApplicationDirPath() + "/config.json";
    }

    QJsonObject config;
    config["dataDirectory"] = m_config.dataDirectory;
    config["subFilePath"] = m_config.subFilePath;
    config["configOutputDirectory"] = m_config.configOutputDirectory;
    config["workingDirectory"] = m_config.workingDirectory;
    config["maxConcurrentDownloads"] = m_config.maxConcurrentDownloads;
    config["requestTimeout"] = m_config.requestTimeout;
    config["createMissingDirectories"] = m_config.createMissingDirectories;
    config["verboseLogging"] = m_config.verboseLogging;
    config["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    config["applicationVersion"] = "2.0.0";
    config["configVersion"] = "1.0";

    QJsonDocument doc(config);
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save config to:" << savePath;
        return false;
    }

    file.write(doc.toJson());
    file.close();

    m_configFilePath = savePath;

    if (m_config.verboseLogging) {
        qInfo() << "Configuration saved to:" << savePath;
    }

    return true;
}

QString ConfigManager::getGitHubWorkspacePath() const
{
    return getEnvironmentVariable("GITHUB_WORKSPACE", getApplicationDirPath());
}

QString ConfigManager::getDataDirectory() const
{
    QString path = m_config.dataDirectory;
    if (QDir::isRelativePath(path)) {
        path = getAbsolutePath(path);
    }
    return QDir::cleanPath(path);
}

QString ConfigManager::getSubFilePath() const
{
    QString path = m_config.subFilePath;
    if (QDir::isRelativePath(path)) {
        path = getAbsolutePath(path);
    }
    return QDir::cleanPath(path);
}

QString ConfigManager::getConfigOutputDirectory() const
{
    QString path = m_config.configOutputDirectory;
    if (QDir::isRelativePath(path)) {
        path = getAbsolutePath(path);
    }
    return QDir::cleanPath(path);
}

QString ConfigManager::getWorkingDirectory() const
{
    QString path = m_config.workingDirectory;
    if (QDir::isRelativePath(path)) {
        path = getAbsolutePath(path);
    }
    return QDir::cleanPath(path);
}

QString ConfigManager::getAbsolutePath(const QString& relativePath) const
{
    if (QDir::isAbsolutePath(relativePath)) {
        return relativePath;
    }

    // Try to resolve relative to application directory first
    QString appDir = getApplicationDirPath();
    QString tryPath = QDir(appDir).filePath(relativePath);
    if (QFile::exists(QFileInfo(tryPath).absolutePath()) || m_config.createMissingDirectories) {
        return QDir::cleanPath(tryPath);
    }

    // Fallback to GitHub workspace
    QString workspace = getGitHubWorkspacePath();
    tryPath = QDir(workspace).filePath(relativePath);
    if (QFile::exists(QFileInfo(tryPath).absolutePath()) || m_config.createMissingDirectories) {
        return QDir::cleanPath(tryPath);
    }

    // Last resort: relative to current working directory
    return QDir::cleanPath(QDir::current().absoluteFilePath(relativePath));
}

bool ConfigManager::ensureDirectoryExists(const QString& path) const
{
    QDir dir(path);
    if (!dir.exists()) {
        if (m_config.createMissingDirectories) {
            if (dir.mkpath(".")) {
                if (m_config.verboseLogging) {
                    qInfo() << "Created directory:" << path;
                }
                return true;
            } else {
                qWarning() << "Failed to create directory:" << path;
                return false;
            }
        }
        return false;
    }
    return true;
}

QString ConfigManager::resolvePath(const QString& path) const
{
    if (QDir::isAbsolutePath(path)) {
        return QDir::cleanPath(path);
    }

    // Try multiple resolution strategies
    QStringList searchPaths = {
        getApplicationDirPath(),
        getGitHubWorkspacePath(),
        QDir::current().absolutePath()
    };

    for (const QString& basePath : searchPaths) {
        QString candidate = QDir(basePath).filePath(path);
        QFileInfo info(candidate);

        if (info.exists() || m_config.createMissingDirectories) {
            return QDir::cleanPath(candidate);
        }
    }

    // Fallback: return resolved path anyway
    return QDir::cleanPath(QDir::current().absoluteFilePath(path));
}

bool ConfigManager::validateConfig() const
{
    m_errors.clear();

    // Validate data directory
    if (m_config.dataDirectory.isEmpty()) {
        m_errors.append("Data directory is not specified");
    }

    // Validate sub file path
    if (m_config.subFilePath.isEmpty()) {
        m_errors.append("Subscription file path is not specified");
    }

    // Validate output directory
    if (m_config.configOutputDirectory.isEmpty()) {
        m_errors.append("Config output directory is not specified");
    }

    // Validate working directory
    if (m_config.workingDirectory.isEmpty()) {
        m_errors.append("Working directory is not specified");
    }

    // Validate numeric values
    if (m_config.maxConcurrentDownloads <= 0) {
        m_errors.append("Max concurrent downloads must be positive");
    }

    if (m_config.requestTimeout <= 0) {
        m_errors.append("Request timeout must be positive");
    }

    return m_errors.isEmpty();
}

QStringList ConfigManager::getConfigErrors() const
{
    return m_errors;
}

void ConfigManager::resetToDefaults()
{
    initializeDefaultConfig();
    m_configFilePath.clear();
    m_errors.clear();
    if (m_config.verboseLogging) {
        qInfo() << "Configuration reset to defaults";
    }
}