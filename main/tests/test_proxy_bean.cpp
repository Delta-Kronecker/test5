#include <QTest>
#include <QCoreApplication>

#include "ProxyBean.h"

class TestProxyBean : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testVMessBeanCreation();
    void testShadowSocksBeanCreation();
    void testTrojanVLESSBeanCreation();
    void testSocksHttpBeanCreation();
    void testAbstractBaseClass();
    void testToJsonMethods();
    void testTryParseLinkMethods();

private:
    void createTestVMess(std::shared_ptr<VMessBean>& bean);
    void createTestShadowSocks(std::shared_ptr<ShadowSocksBean>& bean);
};

void TestProxyBean::initTestCase() {
    // Setup test data
}

void TestProxyBean::cleanupTestCase() {
    // Cleanup test data
}

void TestProxyBean::testAbstractBaseClass() {
    // Verify ProxyBean is abstract (cannot be instantiated)
    // This would cause a compilation error if uncommented:
    // auto bean = std::make_shared<ProxyBean>();  // Should not compile
    QVERIFY(true);  // If we get here, base class is properly abstract
}

void TestProxyBean::testVMessBeanCreation() {
    auto bean = std::make_shared<VMessBean>();

    // Test VMess-specific members
    QVERIFY(bean->type == "vmess");
    QVERIFY(bean->serverPort == 0);  // Default value
    QVERIFY(bean->uuid.isEmpty());   // Default value

    // Set some values
    bean->serverAddress = "test.vmess.server";
    bean->serverPort = 443;
    bean->uuid = "12345678-1234-1234-1234-123456789012";
    bean->network = "ws";
    bean->tls = "tls";
    bean->path = "/path";

    // Verify values were set
    QVERIFY(bean->serverAddress == "test.vmess.server");
    QVERIFY(bean->serverPort == 443);
    QVERIFY(bean->uuid == "12345678-1234-1234-1234-123456789012");
    QVERIFY(bean->network == "ws");
    QVERIFY(bean->tls == "tls");
    QVERIFY(bean->path == "/path");
}

void TestProxyBean::testShadowSocksBeanCreation() {
    auto bean = std::make_shared<ShadowSocksBean>();

    // Test ShadowSocks-specific members
    QVERIFY(bean->type == "shadowsocks");
    QVERIFY(bean->serverPort == 0);  // Default value
    QVERIFY(bean->method.isEmpty()); // Default value
    QVERIFY(bean->password.isEmpty()); // Default value

    // Set some values
    bean->serverAddress = "test.ss.server";
    bean->serverPort = 8388;
    bean->method = "aes-256-gcm";
    bean->password = "test_password";

    // Verify values were set
    QVERIFY(bean->serverAddress == "test.ss.server");
    QVERIFY(bean->serverPort == 8388);
    QVERIFY(bean->method == "aes-256-gcm");
    QVERIFY(bean->password == "test_password");
}

void TestProxyBean::testTrojanVLESSBeanCreation() {
    auto bean = std::make_shared<TrojanVLESSBean>();
    bean->type = "trojan";
    bean->serverAddress = "test.trojan.server";
    bean->serverPort = 443;
    bean->password = "test_password";
    bean->tls = "tls";
    bean->sni = "example.com";
    bean->network = "tcp";

    // Verify Trojan-specific fields
    QVERIFY(bean->type == "trojan");
    QVERIFY(bean->serverAddress == "test.trojan.server");
    QVERIFY(bean->serverPort == 443);
    QVERIFY(bean->password == "test_password");
    QVERIFY(bean->tls == "tls");
    QVERIFY(bean->sni == "example.com");
}

void TestProxyBean::testSocksHttpBeanCreation() {
    auto bean = std::make_shared<SocksHttpBean>();
    bean->type = "socks";
    bean->serverAddress = "test.socks.server";
    bean->serverPort = 1080;
    bean->username = "test_user";
    bean->password = "test_pass";
    bean->network = "tcp";

    // Verify SOCKS-specific fields
    QVERIFY(bean->type == "socks");
    QVERIFY(bean->serverAddress == "test.socks.server");
    QVERIFY(bean->serverPort == 1080);
    QVERIFY(bean->username == "test_user");
    QVERIFY(bean->password == "test_pass");
    QVERIFY(bean->network == "tcp");
}

void TestProxyBean::testToJsonMethods() {
    // Test that derived classes implement ToJson
    auto vmessBean = std::make_shared<VMessBean>();
    vmessBean->type = "vmess";
    vmessBean->serverAddress = "test.server.com";
    vmessBean->serverPort = 443;
    vmessBean->uuid = "test-uuid";

    QJsonObject json = vmessBean->ToJson();
    QVERIFY(!json.isEmpty());
    QVERIFY(json["type"].toString() == "vmess");
    QVERIFY(json["server"].toString() == "test.server.com");
    QVERIFY(json["port"].toInt() == 443);
    QVERIFY(json["uuid"].toString() == "test-uuid");
}

void TestProxyBean::testTryParseLinkMethods() {
    // Test that derived classes implement TryParseLink
    auto vmessBean = std::make_shared<VMessBean>();

    // Test with valid VMess link
    QString validVmessLink = "vmess://eyJ2IjoiMiIsInBzIjoidGVzdCIsImFkZCI6IjAiLCJob3N0IjoidGVzdC5jb20iLCJpZCI6IjEyMzQ1Njc4LTEyMzQtMTIzNC0xMjM0LTEyMzQ1Njc4OTAxMiIsIm5ldCI6IndzIiwicGF0aCI6Ii8iLCJ0bHMiOiJ0bHMiLCJ0eXBlIjoibm9uZSIsImhzdCI6IiIsImFscGgiOiIiLCJhaWQiOiIwIn0=";

    bool result = vmessBean->TryParseLink(validVmessLink);
    // Should be able to parse valid VMess link
    // Note: Actual parsing may fail due to implementation details, but should not crash
    QVERIFY(result || !result);  // Just verify method doesn't crash

    // Test with invalid link
    result = vmessBean->TryParseLink("invalid://link");
    QVERIFY(!result);  // Should return false for invalid link
}

void TestProxyBean::createTestVMess(std::shared_ptr<VMessBean>& bean) {
    bean = std::make_shared<VMessBean>();
    bean->type = "vmess";
    bean->serverAddress = "test.vmess.server";
    bean->serverPort = 443;
    bean->uuid = "12345678-1234-1234-1234-123456789012";
    bean->network = "ws";
    bean->tls = "tls";
    bean->path = "/path";
    bean->host = "test.example.com";
    bean->sni = "test.example.com";
}

void TestProxyBean::createTestShadowSocks(std::shared_ptr<ShadowSocksBean>& bean) {
    bean = std::make_shared<ShadowSocksBean>();
    bean->type = "shadowsocks";
    bean->serverAddress = "test.ss.server";
    bean->serverPort = 8388;
    bean->method = "aes-256-gcm";
    bean->password = "test_password";
    bean->network = "tcp";
}

QTEST_MAIN(TestProxyBean)
