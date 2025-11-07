#include <QTest>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDir>

// Class under test
#include "ConfigManager.h"
#include "Utils.h"

class TestConfigManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testSingletonInstance();
    void testDefaultConfiguration();
    void testEnvironmentVariableHandling();
    void testPathResolution();
    void testDirectoryCreation();
    void testConfigLoadAndSave();
    void testConfigValidation();
    void testErrorHandling();
    void testGitHubWorkspaceDetection();

private:
    QTemporaryDir m_tempDir;
    QString m_testConfigPath;
};

void TestConfigManager::initTestCase() {
    // Set up test environment
    m_testConfigPath = m_tempDir.filePath("test_config.json");

    // Set test environment variables
    qputenv("TEST_VAR", "test_value");
    qputenv("GITHUB_WORKSPACE", m_tempDir.path().toUtf8());
}

void TestConfigManager::cleanupTestCase() {
    // Clean up environment
    qunsetenv("TEST_VAR");
    qunsetenv("GITHUB_WORKSPACE");
}

void TestConfigManager::testSingletonInstance() {
    auto& config1 = ConfigManager::getInstance();
    auto& config2 = ConfigManager::getInstance();

    // Should be the same instance
    QVERIFY(&config1 == &config2);
}

void TestConfigManager::testDefaultConfiguration() {
    auto& configMgr = ConfigManager::getInstance();
    configMgr.resetToDefaults();

    const auto& config = configMgr.getConfig();

    // Check default values
    QVERIFY(!config.dataDirectory.isEmpty());
    QVERIFY(!config.configOutputDirectory.isEmpty());
    QVERIFY(config.maxConcurrentDownloads > 0);
    QVERIFY(config.requestTimeout > 0);
}

void TestConfigManager::testEnvironmentVariableHandling() {
    auto& configMgr = ConfigManager::getInstance();

    // Test with environment variable set
    qputenv("TEST_CONFIG_PATH", m_testConfigPath.toUtf8());

    // This should work without errors
    QVERIFY(configMgr.validateConfig());

    qunsetenv("TEST_CONFIG_PATH");
}

void TestConfigManager::testPathResolution() {
    auto& configMgr = ConfigManager::getInstance();

    // Test absolute path
    QString absPath = "/tmp/test";
    QString resolved = configMgr.getAbsolutePath(absPath);
    QVERIFY(resolved == absPath);

    // Test relative path
    QString relPath = "relative/path";
    QString resolvedRel = configMgr.getAbsolutePath(relPath);
    QVERIFY(!resolvedRel.isEmpty());
    QVERIFY(QDir::isAbsolutePath(resolvedRel));
}

void TestConfigManager::testDirectoryCreation() {
    auto& configMgr = ConfigManager::getInstance();

    QString testDir = m_tempDir.filePath("test_directory/subdir");

    // Directory should not exist initially
    QVERIFY(!QDir(testDir).exists());

    // Create directory
    QVERIFY(configMgr.ensureDirectoryExists(testDir));

    // Directory should now exist
    QVERIFY(QDir(testDir).exists());
}

void TestConfigManager::testConfigLoadAndSave() {
    auto& configMgr = ConfigManager::getInstance();

    // Set some test values
    auto& config = const_cast<ConfigManager::Config&>(configMgr.getConfig());
    config.dataDirectory = m_tempDir.path();
    config.maxConcurrentDownloads = 5;

    // Save config
    QVERIFY(configMgr.saveConfig(m_testConfigPath));

    // File should exist
    QVERIFY(QFile::exists(m_testConfigPath));

    // Load config into new instance
    auto& configMgr2 = ConfigManager::getInstance();
    configMgr2.resetToDefaults();
    QVERIFY(configMgr2.loadConfig(m_testConfigPath));

    // Check if values were loaded correctly
    const auto& loadedConfig = configMgr2.getConfig();
    QVERIFY(loadedConfig.dataDirectory == m_tempDir.path());
    QVERIFY(loadedConfig.maxConcurrentDownloads == 5);
}

void TestConfigManager::testConfigValidation() {
    auto& configMgr = ConfigManager::getInstance();

    // Valid config should pass validation
    configMgr.resetToDefaults();
    QVERIFY(configMgr.validateConfig());
    QVERIFY(configMgr.getConfigErrors().isEmpty());

    // Invalid config should fail validation
    auto& config = const_cast<ConfigManager::Config&>(configMgr.getConfig());
    config.maxConcurrentDownloads = -1;

    QVERIFY(!configMgr.validateConfig());
    QVERIFY(!configMgr.getConfigErrors().isEmpty());
}

void TestConfigManager::testErrorHandling() {
    auto& configMgr = ConfigManager::getInstance();

    // Test with non-existent file
    QVERIFY(!configMgr.loadConfig("/non/existent/path"));
    QVERIFY(!configMgr.getConfigErrors().isEmpty());
}

void TestConfigManager::testGitHubWorkspaceDetection() {
    auto& configMgr = ConfigManager::getInstance();

    // Test GitHub workspace path
    QString workspace = configMgr.getGitHubWorkspacePath();
    QVERIFY(workspace == m_tempDir.path());
}

QTEST_MAIN(TestConfigManager)