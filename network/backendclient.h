#ifndef BACKENDCLIENT_H
#define BACKENDCLIENT_H

#include "models/analysisrequest.h"
#include "models/analysisresult.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

/**
 * Klient HTTP do backend.py (POST /analiza, StreamingResponse).
 * Progres: pierwsza linia = liczba lat; potem lata×2 + 1 komunikatów; na końcu JSON.
 */
class BackendClient : public QObject
{
    Q_OBJECT

public:
    explicit BackendClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);
    QString baseUrl() const { return m_baseUrl; }

    void setUseMock(bool enabled);
    bool useMock() const { return m_useMock; }

    void startAnalysis(const AnalysisRequest &request);
    void cancel();

signals:
    void progressChanged(int percent, const QString &stage);
    void finished(const AnalysisResult &result);
    void failed(const QString &errorMessage);

private slots:
    void onMockTick();
    void onNetworkReadyRead();
    void onNetworkFinished();

private:
    void startMock(const AnalysisRequest &request);
    void startHttp(const AnalysisRequest &request);
    void processStreamLine(const QByteArray &line);
    void finishWithJson(const QByteArray &jsonBytes);

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer m_mockTimer;
    AnalysisRequest m_pendingRequest;
    int m_mockProgress = 0;
    bool m_useMock = false;
    QString m_baseUrl = QStringLiteral("http://127.0.0.1:8000");

    QByteArray m_streamBuffer;
    bool m_yearCountReceived = false;
    int m_totalYears = 0;
    int m_expectedSteps = 0;
    int m_completedSteps = 0;
};

#endif // BACKENDCLIENT_H
