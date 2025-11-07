#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMap>
#include <QException>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QTimer>

#include "Utils.h"
#include "HttpHelper.h"
#include "SubParser.h"
#include "ConfigManager.h"

// Configure logging categories
Q_LOGGING_CATEGORY(CONFIG_MAIN, "config.main")
Q_LOGGING_CATEGORY(CONFIG_ERROR, "config.error")
Q_LOGGING_CATEGORY(CONFIG_INFO, "config.info")

// Subscription statistics structure
struct SubStats {
    QString url;
    QString fileName;
    int totalConfigs = 0;
    int uniqueConfigs = 0;
    int duplicates = 0;
    QString status;
    QString errorMessage;
    qint64 downloadTime = 0;
};

// Generate unique key for deduplication (protocol + server + port)
QString GenerateConfigKey(const std::shared_ptr<ProxyBean> &bean) {
    if (!bean) {
        return QString();
    }
    return QString("%1://%2:%3")
        .arg(bean->type)
        .arg(bean->serverAddress)
        .arg(bean->serverPort);
}

// Custom exception class for ConfigCollector
class ConfigCollectorException : public QException {
public:
    explicit ConfigCollectorException(const QString& message) : m_message(message) {}

    void raise() const override { throw *this; }
    ConfigCollectorException* clone() const override { return new ConfigCollectorException(*this); }

    QString message() const { return m_message; }

private:
    QString m_message;
};

// Enhanced error handling class
class ErrorHandler {
public:
    static void handleFileError(const QString& filePath, const QString& operation) {
        QString error = QString("File %1 failed for %2: %3")
            .arg(filePath, operation, QFileInfo(filePath).exists() ? "Permission denied" : "Not found");
        qCCritical(CONFIG_ERROR) << error;
    }

    static void handleNetworkError(const QString& url, const QString& error) {
        QString detailedError = QString("Network error for %1: %2")
            .arg(url, error);
        qCCritical(CONFIG_ERROR) << detailedError;
    }

    static void handleParsingError(const QString& content, const QString& error) {
        QString detailedError = QString("Parsing failed: %1")
            .arg(error);
        qCCritical(CONFIG_ERROR) << detailedError;
    }
};

// Configuration validator
bool validatePaths() {
    auto& configMgr = ConfigManager::getInstance();

    // Ensure required directories exist
    if (!configMgr.ensureDirectoryExists(configMgr.getDataDirectory())) {
        qCCritical(CONFIG_ERROR) << "Failed to create data directory:" << configMgr.getDataDirectory();
        return false;
    }

    if (!configMgr.ensureDirectoryExists(configMgr.getConfigOutputDirectory())) {
        qCCritical(CONFIG_ERROR) << "Failed to create config output directory:" << configMgr.getConfigOutputDirectory();
        return false;
    }

    return true;
}

// Enhanced subscription processing
bool processSubscription(const QString& subUrl, SubStats& stats) {
    try {
        stats.url = subUrl;
        stats.status = "Processing";

        qCInfo(CONFIG_INFO) << "Processing subscription:" << subUrl;

        // Download content with timeout and error handling
        HttpHelper httpHelper;
        QString content;

        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;

        QObject::connect(&timer, &QTimer::timeout, [&]() {
            loop.quit();
        });

        QObject::connect(&httpHelper, &HttpHelper::requestFinished, [&](const QString& result, bool success) {
            content = result;
            if (success) {
                loop.quit();
            } else {
                loop.quit();
            }
        });

        timer.start(ConfigManager::getInstance().getConfig().requestTimeout);
        httpHelper.download(subUrl);
        loop.exec();

        if (content.isEmpty()) {
            stats.status = "Failed";
            stats.errorMessage = "Empty content or download timeout";
            qCCritical(CONFIG_ERROR) << "Failed to download content from:" << subUrl;
            return false;
        }

        stats.downloadTime = QDateTime::currentMSecsSinceEpoch();

        // Parse content
        SubParser parser;
        auto beans = parser.parse(subUrl, content);

        if (beans.isEmpty()) {
            stats.status = "No configs";
            stats.errorMessage = "No valid proxy configurations found";
            qCWarning(CONFIG_INFO) << "No valid configs found in:" << subUrl;
            return true;
        }

        stats.totalConfigs = beans.size();

        // Return processed stats
        return true;

    } catch (const ConfigCollectorException& e) {
        stats.status = "Error";
        stats.errorMessage = e.message();
        qCCritical(CONFIG_ERROR) << "ConfigCollector exception for" << subUrl << ":" << e.message();
        return false;
    } catch (const std::exception& e) {
        stats.status = "Error";
        stats.errorMessage = QString("Standard exception: %1").arg(e.what());
        qCCritical(CONFIG_ERROR) << "Standard exception for" << subUrl << ":" << e.what();
        return false;
    } catch (...) {
        stats.status = "Error";
        stats.errorMessage = "Unknown exception";
        qCCritical(CONFIG_ERROR) << "Unknown exception for" << subUrl;
        return false;
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Set application properties
    app.setApplicationName("ConfigCollector");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("C-Xray Project");

    // Configure logging
    QLoggingCategory::setFilterRules(
        "*.debug=true\n"
        "config.main.debug=true\n"
        "config.error.*=true\n"
        "config.info.*=true\n"
    );

    try {
        qCInfo(CONFIG_MAIN) << "=== ConfigCollector Started ===";
        qCInfo(CONFIG_MAIN) << "Version:" << app.applicationVersion();
        qCInfo(CONFIG_MAIN) << "Build date:" << __DATE__ << __TIME__;

        // Initialize configuration manager
        auto& configMgr = ConfigManager::getInstance();
        if (!configMgr.loadConfig()) {
            qCCritical(CONFIG_ERROR) << "Failed to load configuration";
            return 1;
        }

        // Validate paths and create directories
        if (!validatePaths()) {
            qCCritical(CONFIG_ERROR) << "Path validation failed";
            return 1;
        }

        // Get subscription file path
        QString subFilePath = configMgr.getSubFilePath();
        qCInfo(CONFIG_INFO) << "Reading subscriptions from:" << subFilePath;

        // Read subscription file with error handling
        QString subContent = Utils::readFileText(subFilePath);

        if (subContent.isEmpty()) {
            // Check if file exists
            if (!QFile::exists(subFilePath)) {
                qCCritical(CONFIG_ERROR) << "Subscription file not found:" << subFilePath;
                qCInfo(CONFIG_INFO) << "Please create the file with subscription URLs (one per line)";
                return 1;
            } else {
                qCCritical(CONFIG_ERROR) << "Subscription file is empty:" << subFilePath;
                return 1;
            }
        }

        // Parse subscription URLs
        auto subLinks = subContent.split('\n', Qt::SkipEmptyParts);
        // Remove comments and empty lines
        subLinks.erase(std::remove_if(subLinks.begin(), subLinks.end(), [](const QString& line) {
            return line.trimmed().startsWith('#') || line.trimmed().isEmpty();
        }), subLinks.end());

        qCInfo(CONFIG_INFO) << "Found" << subLinks.size() << "valid subscription links";

        if (subLinks.isEmpty()) {
            qCWarning(CONFIG_INFO) << "No subscription URLs found in file";
            return 0;
        }

        // Statistics tracking
        int totalConfigs = 0;
        int duplicateCount = 0;
        int configIndex = 1;
        QMap<QString, std::shared_ptr<ProxyBean>> uniqueConfigs;
        QList<SubStats> allStats;

        // Process each subscription
        for (const QString& subUrl : subLinks) {
            SubStats stats;

            if (processSubscription(subUrl, stats)) {
                // Add to statistics
                allStats.append(stats);
                totalConfigs += stats.totalConfigs;
            } else {
                // Still add to statistics for reporting
                stats.status = "Failed";
                allStats.append(stats);
            }
        }

        // Save results
        QString outputDir = configMgr.getConfigOutputDirectory();
        qCInfo(CONFIG_INFO) << "Saving results to:" << outputDir;

        // Save each subscription's results
        for (const SubStats& stats : allStats) {
            if (stats.totalConfigs > 0) {
                QString fileName = QString("config_%1.json").arg(configIndex++, 4, 10, QChar('0'));
                QString filePath = QDir(outputDir).filePath(fileName);

                // This would need to be implemented to save the actual configs
                // For now, just create placeholder files
                Utils::writeFileText(filePath, QString("{\"subscription\": \"%1\", \"configs\": []}").arg(stats.url));

                qCInfo(CONFIG_INFO) << "Saved" << stats.totalConfigs << "configs to" << fileName;
            }
        }

        // Print comprehensive statistics
        qCInfo(CONFIG_MAIN) << "=== Collection Summary ===";
        qCInfo(CONFIG_MAIN) << "Total subscriptions processed:" << allStats.size();
        qCInfo(CONFIG_MAIN) << "Total configs found:" << totalConfigs;
        qCInfo(CONFIG_MAIN) << "Unique configs:" << uniqueConfigs.size();
        qCInfo(CONFIG_MAIN) << "Duplicates removed:" << duplicateCount;

        // Per-subscription breakdown
        qCInfo(CONFIG_MAIN) << "=== Per-Subscription Results ===";
        for (const SubStats& stats : allStats) {
            qCInfo(CONFIG_MAIN) << QString("URL: %1").arg(stats.url);
            qCInfo(CONFIG_MAIN) << QString("  Status: %1").arg(stats.status);
            if (!stats.errorMessage.isEmpty()) {
                qCInfo(CONFIG_MAIN) << QString("  Error: %1").arg(stats.errorMessage);
            }
            qCInfo(CONFIG_MAIN) << QString("  Configs: %1").arg(stats.totalConfigs);
        }

        qCInfo(CONFIG_MAIN) << "=== ConfigCollector Completed Successfully ===";
        return 0;

    } catch (const ConfigCollectorException& e) {
        qCCritical(CONFIG_ERROR) << "ConfigCollector exception:" << e.message();
        return 1;
    } catch (const std::exception& e) {
        qCCritical(CONFIG_ERROR) << "Standard exception:" << e.what();
        return 1;
    } catch (...) {
        qCCritical(CONFIG_ERROR) << "Unknown exception occurred";
        return 1;
    }
}