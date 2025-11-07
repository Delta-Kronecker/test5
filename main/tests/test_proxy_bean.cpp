#include <QTest>
#include <QCoreApplication>

#include "ProxyBean.h"

class TestProxyBean : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testProxyBeanCreation();
    void testVMessBeanCreation();
    void testShadowSocksBeanCreation();
    void testTrojanVLESSBeanCreation();
    void testSocksHttpBeanCreation();
    void testTypeValidation();
    void testSerialization();
    void testComparison();

private:
    void createTestVMess(std::shared_ptr<ProxyBean>& bean);
    void createTestShadowSocks(std::shared_ptr<ProxyBean>& bean);
};

void TestProxyBean::initTestCase() {
    // Setup test data
}

void TestProxyBean::cleanupTestCase() {
    // Cleanup test data
}

void TestProxyBean::testProxyBeanCreation() {
    // Test basic ProxyBean creation
    auto bean = std::make_shared<ProxyBean>();
    QVERIFY(bean != nullptr);

    // Test default values
    QVERIFY(bean->type.isEmpty());
    QVERIFY(bean->serverAddress.isEmpty());
    QVERIFY(bean->serverPort == 0);
}

void TestProxyBean::testVMessBeanCreation() {
    auto bean = std::make_shared<ProxyBean>();
    createTestVMess(bean);

    // Verify VMess-specific fields
    QVERIFY(bean->type == "vmess");
    QVERIFY(bean->serverAddress == "test.vmess.server");
    QVERIFY(bean->serverPort == 443);
    QVERIFY(bean->uuid == "12345678-1234-1234-1234-123456789012");
    QVERIFY(bean->alterID == 0);
    QVERIFY(bean->cipher == "auto");
    QVERIFY(bean->network == "ws");
    QVERIFY(bean->tls == "tls");
    QVERIFY(!bean->path.isEmpty());
}

void TestProxyBean::testShadowSocksBeanCreation() {
    auto bean = std::make_shared<ProxyBean>();
    createTestShadowSocks(bean);

    // Verify ShadowSocks-specific fields
    QVERIFY(bean->type == "shadowsocks");
    QVERIFY(bean->serverAddress == "test.ss.server");
    QVERIFY(bean->serverPort == 8388);
    QVERIFY(bean->method == "aes-256-gcm");
    QVERIFY(bean->password == "test_password");
    QVERIFY(bean->network == "tcp");
    QVERIFY(bean->tls == "none");
}

void TestProxyBean::testTrojanVLESSBeanCreation() {
    auto bean = std::make_shared<ProxyBean>();
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
}

void TestProxyBean::testSocksHttpBeanCreation() {
    auto bean = std::make_shared<ProxyBean>();
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
}

void TestProxyBean::testTypeValidation() {
    auto vmessBean = std::make_shared<ProxyBean>();
    vmessBean->type = "vmess";

    auto ssBean = std::make_shared<ProxyBean>();
    ssBean->type = "shadowsocks";

    // Test type validation
    QVERIFY(vmessBean->isValidType());
    QVERIFY(ssBean->isValidType());

    auto invalidBean = std::make_shared<ProxyBean>();
    invalidBean->type = "invalid_type";
    QVERIFY(!invalidBean->isValidType());
}

void TestProxyBean::testSerialization() {
    auto bean = std::make_shared<ProxyBean>();
    createTestVMess(bean);

    // Test toString
    QString str = bean->toString();
    QVERIFY(!str.isEmpty());
    QVERIFY(str.contains("vmess"));
    QVERIFY(str.contains("test.vmess.server"));

    // Test toJson
    QJsonObject json = bean->toJson();
    QVERIFY(!json.isEmpty());
    QVERIFY(json["type"].toString() == "vmess");
    QVERIFY(json["server"].toString() == "test.vmess.server");
    QVERIFY(json["port"].toInt() == 443);
}

void TestProxyBean::testComparison() {
    auto bean1 = std::make_shared<ProxyBean>();
    createTestVMess(bean1);

    auto bean2 = std::make_shared<ProxyBean>();
    createTestVMess(bean2);

    auto bean3 = std::make_shared<ProxyBean>();
    createTestShadowSocks(bean3);

    // Same configuration should be equal
    QVERIFY(bean1->equals(bean2));
    QVERIFY(bean1->getConfigKey() == bean2->getConfigKey());

    // Different configuration should not be equal
    QVERIFY(!bean1->equals(bean3));
    QVERIFY(bean1->getConfigKey() != bean3->getConfigKey());
}

void TestProxyBean::createTestVMess(std::shared_ptr<ProxyBean>& bean) {
    bean->type = "vmess";
    bean->serverAddress = "test.vmess.server";
    bean->serverPort = 443;
    bean->uuid = "12345678-1234-1234-1234-123456789012";
    bean->alterID = 0;
    bean->cipher = "auto";
    bean->network = "ws";
    bean->tls = "tls";
    bean->path = "/path";
    bean->host = "test.example.com";
    bean->sni = "test.example.com";
}

void TestProxyBean::createTestShadowSocks(std::shared_ptr<ProxyBean>& bean) {
    bean->type = "shadowsocks";
    bean->serverAddress = "test.ss.server";
    bean->serverPort = 8388;
    bean->method = "aes-256-gcm";
    bean->password = "test_password";
    bean->network = "tcp";
    bean->tls = "none";
}

QTEST_MAIN(TestProxyBean)