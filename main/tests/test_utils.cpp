#include <QTest>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFileInfo>

#include "Utils.h"

class TestUtils : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testFileOperations();
    void testPathOperations();
    void testValidation();
    void testTextProcessing();
    void testDataConversion();
    void testErrorHandling();

private:
    QTemporaryDir m_tempDir;
    QString m_testFilePath;
};

void TestUtils::initTestCase() {
    m_testFilePath = m_tempDir.filePath("test_file.txt");
}

void TestUtils::cleanupTestCase() {
    // Clean up will be handled by QTemporaryDir destructor
}

void TestUtils::testFileOperations() {
    QString testContent = "Hello, World!";

    // Test writeFileText
    QVERIFY(Utils::writeFileText(m_testFilePath, testContent));

    // Test readFileText
    QString readContent = Utils::readFileText(m_testFilePath);
    QVERIFY(readContent == testContent);

    // Test file exists
    QVERIFY(QFile::exists(m_testFilePath));

    // Test getFileSize
    qint64 size = Utils::getFileSize(m_testFilePath);
    QVERIFY(size > 0);

    // Test file size string
    QString sizeStr = Utils::getFileSizeString(m_testFilePath);
    QVERIFY(!sizeStr.isEmpty());

    // Test removeFile
    QVERIFY(Utils::removeFile(m_testFilePath));
    QVERIFY(!QFile::exists(m_testFilePath));
}

void TestUtils::testPathOperations() {
    // Test getAbsolutePath
    QString absPath = Utils::getAbsolutePath("relative/path");
    QVERIFY(QDir::isAbsolutePath(absPath));

    // Test normalizePath
    QString normalized = Utils::normalizePath("path/./to/../file");
    QVERIFY(normalized == "path/file");
}

void TestUtils::testValidation() {
    // Test valid URL
    QVERIFY(Utils::isValidUrl("https://example.com"));
    QVERIFY(Utils::isValidUrl("http://test.com:8080"));

    // Test invalid URL
    QVERIFY(!Utils::isValidUrl("not a url"));
    QVERIFY(!Utils::isValidUrl(""));

    // Test valid email
    QVERIFY(Utils::isValidEmail("test@example.com"));
    QVERIFY(!Utils::isValidEmail("invalid email"));

    // Test valid port
    QVERIFY(Utils::isValidPort(8080));
    QVERIFY(Utils::isValidPort(1));
    QVERIFY(Utils::isValidPort(65535));
    QVERIFY(!Utils::isValidPort(0));
    QVERIFY(!Utils::isValidPort(65536));

    // Test valid IP
    QVERIFY(Utils::isValidIpAddress("192.168.1.1"));
    QVERIFY(Utils::isValidIpAddress("127.0.0.1"));
    QVERIFY(!Utils::isValidIpAddress("999.999.999.999"));
    QVERIFY(!Utils::isValidIpAddress("not an ip"));

    // Test valid UUID
    QVERIFY(Utils::isValidUuid("550e8400-e29b-41d4-a716-446655440000"));
    QVERIFY(!Utils::isValidUuid("not-a-uuid"));
}

void TestUtils::testTextProcessing() {
    QString textWithComments = "line1\n# comment line\nline2\nline3";
    QString cleaned = Utils::removeComments(textWithComments);

    // Should remove comment line
    QVERIFY(!cleaned.contains("# comment line"));
    QVERIFY(cleaned.contains("line1"));
    QVERIFY(cleaned.contains("line2"));

    // Test cleanString
    QString dirtyString = "  multiple   spaces  ";
    QString cleanString = Utils::cleanString(dirtyString);
    QVERIFY(cleanString == "multiple spaces");

    // Test containsValidConfig
    QString validConfig = "vmess://test";
    QString invalidConfig = "just text";
    QVERIFY(Utils::containsValidConfig(validConfig));
    QVERIFY(!Utils::containsValidConfig(invalidConfig));
}

void TestUtils::testDataConversion() {
    QString testStr = "Hello, World!";
    QByteArray testData = testStr.toUtf8();

    // Test string to byte array
    QByteArray converted = Utils::stringToByteArray(testStr);
    QVERIFY(converted == testData);

    // Test byte array to string
    QString backToString = Utils::byteArrayToString(testData);
    QVERIFY(backToString == testStr);

    // Test bytes to string
    QString sizeStr = Utils::bytesToString(1024);
    QVERIFY(sizeStr.contains("KB"));
}

void TestUtils::testErrorHandling() {
    // Clear any existing error
    Utils::clearError();
    QVERIFY(!Utils::hasError());

    // Test with invalid file
    QString invalidContent = Utils::readFileText("/non/existent/file");
    QVERIFY(invalidContent.isEmpty());
    QVERIFY(Utils::hasError());

    // Clear error
    Utils::clearError();
    QVERIFY(!Utils::hasError());
}

QTEST_MAIN(TestUtils)