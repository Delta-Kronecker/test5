#include <QTest>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>

// Test runner that initializes Qt application
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Set application info for tests
    app.setApplicationName("ConfigCollectorTests");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("C-Xray Project");

    // Run tests
    int result = QTest::qExec(nullptr, argc, argv);

    return result;
}