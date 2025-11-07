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
    QSignalSpy spy(&httpHelper, &HttpHelper::requestFinished);

    // Use a simple HTTP test service
    QString testUrl = "http://httpbin.org/status/200";

    httpHelper.download(testUrl);

    // Wait for signal with timeout
    QVERIFY(spy.wait(10000));

    // Check signal arguments
    QVERIFY(spy.count() == 1);

    QList<QVariant> arguments = spy.takeFirst();
    QString result = arguments.at(0).toString();
    bool success = arguments.at(1).toBool();

    // Should be successful and have content
    QVERIFY(success);
    QVERIFY(!result.isEmpty());
}

void TestHttpHelper::testDownloadError() {
    HttpHelper httpHelper;
    QSignalSpy spy(&httpHelper, &HttpHelper::requestFinished);

    // Use invalid URL
    QString invalidUrl = "http://invalid-domain-that-does-not-exist-12345.com";

    httpHelper.download(invalidUrl);

    // Wait for signal with timeout
    QVERIFY(spy.wait(15000));

    QList<QVariant> arguments = spy.takeFirst();
    QString result = arguments.at(0).toString();
    bool success = arguments.at(1).toBool();

    // Should fail
    QVERIFY(!success);
}

void TestHttpHelper::testTimeoutHandling() {
    HttpHelper httpHelper;
    QSignalSpy spy(&httpHelper, &HttpHelper::requestFinished);

    // Use a URL that will timeout
    QString timeoutUrl = "http://httpbin.org/delay/10";

    // Set a short timeout via environment or modify HttpHelper if needed
    httpHelper.download(timeoutUrl);

    // Wait for signal (should timeout and return empty)
    QVERIFY(spy.wait(5000));

    QList<QVariant> arguments = spy.takeFirst();
    QString result = arguments.at(0).toString();
    bool success = arguments.at(1).toBool();

    // Should timeout/fail
    QVERIFY(!success || result.isEmpty());
}

void TestHttpHelper::testInvalidUrl() {
    HttpHelper httpHelper;
    QSignalSpy spy(&httpHelper, &HttpHelper::requestFinished);

    // Test with various invalid URLs
    QStringList invalidUrls = {
        "",
        "not-a-url",
        "ftp://unsupported://protocol",
        "http://",
    };

    for (const QString& invalidUrl : invalidUrls) {
        spy.clear();
        httpHelper.download(invalidUrl);

        // Should handle gracefully (may or may not emit signal)
        // Just verify no crashes occur
        QTest::qWait(100);
    }
}

void TestHttpHelper::testNetworkManagerCleanup() {
    // Test that HttpHelper doesn't leak network managers
    // This is a basic test to ensure proper cleanup

    {
        HttpHelper httpHelper;
        QSignalSpy spy(&httpHelper, &HttpHelper::requestFinished);

        // Trigger a request
        httpHelper.download("http://httpbin.org/status/200");

        // Wait briefly
        QTest::qWait(100);
    }

    // Object should be properly destroyed without leaks
    // (In real scenario, would need memory leak detection tools)
}

QTEST_MAIN(TestHttpHelper)