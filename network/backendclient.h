#ifndef BACKENDCLIENT_H
#define BACKENDCLIENT_H

#include "models/analysisrequest.h"
#include "models/analysisresult.h"

#include <QObject>
#include <QString>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

/**
 * Klient HTTP do program88.py (POST /analiza).
 * Domyślnie łączy się z prawdziwym backendem; mock zostaje jako fallback offline.
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
    void onNetworkFinished();

private:
    void startMock(const AnalysisRequest &request);
    void startHttp(const AnalysisRequest &request);

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer m_mockTimer;
    AnalysisRequest m_pendingRequest;
    int m_mockProgress = 0;
    bool m_useMock = false;
    QString m_baseUrl = QStringLiteral("http://127.0.0.1:8000");
};

#endif // BACKENDCLIENT_H
