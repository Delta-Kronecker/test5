#include <QTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "HttpHelper.h"

class TestHttpHelper : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDownloadData();
    void testDownloadError();
    void testTimeoutHandling();
    void testInvalidUrl();
    void testNetworkManagerCleanup();

private:
    QNetworkAccessManager* m_networkManager;
};

void TestHttpHelper::initTestCase() {
    m_networkManager = new QNetworkAccessManager(this);
}

void TestHttpHelper::cleanupTestCase() {
    // Cleanup handled by Qt parent-child relationship
}

void TestHttpHelper::testDownloadData() {
    HttpHelper httpHelper;

    // Use the new HttpGet method
    HttpResponse response = httpHelper.HttpGet("http://httpbin.org/status/200");

    // Check response
    QVERIFY(response.error.isEmpty());  // Should have no error
    QVERIFY(!response.data.isEmpty());  // Should have some data
}

void TestHttpHelper::testDownloadError() {
    HttpHelper httpHelper;

    // Use invalid URL
    HttpResponse response = httpHelper.HttpGet("http://invalid-domain-that-does-not-exist-12345.com");

    // Should have an error
    QVERIFY(!response.error.isEmpty());
}

void TestHttpHelper::testTimeoutHandling() {
    HttpHelper httpHelper;

    // Use a URL that might timeout
    HttpResponse response = httpHelper.HttpGet("http://httpbin.org/delay/10");

    // Should either timeout or return empty/error
    if (!response.error.isEmpty()) {
        QVERIFY(response.error.contains("timeout") || response.error.contains("network"));
    }
    // Response should be empty or have error
    QVERIFY(response.data.isEmpty() || !response.error.isEmpty());
}

void TestHttpHelper::testInvalidUrl() {
    HttpHelper httpHelper;

    // Test with various invalid URLs
    QStringList invalidUrls = {
        "",
        "not-a-url",
        "ftp://unsupported://protocol",
        "http://",
    };

    for (const QString& invalidUrl : invalidUrls) {
        // Should handle gracefully without crashing
        HttpResponse response = httpHelper.HttpGet(invalidUrl);
        // Just verify it returns some error indication
        QVERIFY(response.data.isEmpty() || !response.error.isEmpty());
    }
}

void TestHttpHelper::testNetworkManagerCleanup() {
    // Test that HttpHelper doesn't leak network managers
    // This is a basic test to ensure proper cleanup

    {
        HttpHelper httpHelper;
        // Trigger a request
        HttpResponse response = httpHelper.HttpGet("http://httpbin.org/status/200");
        // Verify it works
        QVERIFY(response.error.isEmpty() || !response.data.isEmpty());
    }

    // Object should be properly destroyed without leaks
    // (In real scenario, would need memory leak detection tools)
}

QTEST_MAIN(TestHttpHelper)
