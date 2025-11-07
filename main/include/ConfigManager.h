#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QCoreApplication>

class ConfigManager
{
public:
    static ConfigManager& getInstance();

    // Configuration structure
    struct Config {
        QString dataDirectory;
        QString subFilePath;
        QString configOutputDirectory;
        QString workingDirectory;
        int maxConcurrentDownloads;
        int requestTimeout;
        bool createMissingDirectories;
        bool verboseLogging;
    };

    // Load and save configuration
    bool loadConfig(const QString& configPath = QString());
    bool saveConfig(const QString& configPath = QString());

    // Getters
    const Config& getConfig() const { return m_config; }
    QString getDataDirectory() const;
    QString getSubFilePath() const;
    QString getConfigOutputDirectory() const;
    QString getWorkingDirectory() const;

    // Utility methods
    QString getAbsolutePath(const QString& relativePath) const;
    bool ensureDirectoryExists(const QString& path) const;
    QString resolvePath(const QString& path) const;

    // Environment-aware path resolution
    QString getGitHubWorkspacePath() const;
    QString getDefaultDataPath() const;

    // Validation
    bool validateConfig() const;
    QStringList getConfigErrors() const;

    // Reset to defaults
    void resetToDefaults();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void initializeDefaultConfig();
    QString getEnvironmentVariable(const QString& key, const QString& defaultValue = QString()) const;
    QString getApplicationDirPath() const;

    Config m_config;
    QString m_configFilePath;
    mutable QStringList m_errors;  // mutable to allow modification in const method
};

#endif // CONFIGMANAGER_H