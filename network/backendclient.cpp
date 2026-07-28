#include "backendclient.h"

#include "analysisresponseparser.h"
#include "processing/imagerenderer.h"
#include "processing/mockdatagenerator.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>

namespace {

void finalizeHectares(AnalysisResult *result, const AnalysisRequest &request)
{
    if (!result || result->years.size() < 2) {
        return;
    }
    const YearBands *t1 = result->bandsFor(result->firstYear());
    const YearBands *t2 = result->bandsFor(result->lastYear());
    if (!t1 || !t2) {
        return;
    }
    const BandBuffer ndvi1 = BandBuffer::ndvi(t1->b4, t1->b8);
    const BandBuffer ndvi2 = BandBuffer::ndvi(t2->b4, t2->b8);
    const BandBuffer *area = result->pixelAreaM2.isEmpty() ? nullptr : &result->pixelAreaM2;
    result->hectaresDeforested = ImageRenderer::hectaresFromMask(
        ndvi1, ndvi2, request.deforestationPartial, area);
}

int expectedProgressSteps(int yearCount)
{
    // backend: lata × 2 (CHM + BEZ) + 1 (powierzchnia)
    return yearCount * 2 + 1;
}

} // namespace

BackendClient::BackendClient(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
    connect(&m_mockTimer, &QTimer::timeout, this, &BackendClient::onMockTick);
}

void BackendClient::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
}

void BackendClient::setUseMock(bool enabled)
{
    m_useMock = enabled;
}

void BackendClient::startAnalysis(const AnalysisRequest &request)
{
    cancel();
    m_pendingRequest = request;

    if (m_useMock) {
        startMock(request);
    } else {
        startHttp(request);
    }
}

void BackendClient::cancel()
{
    m_mockTimer.stop();
    m_mockProgress = 0;
    m_streamBuffer.clear();
    m_yearCountReceived = false;
    m_totalYears = 0;
    m_expectedSteps = 0;
    m_completedSteps = 0;

    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void BackendClient::startMock(const AnalysisRequest &request)
{
    m_totalYears = qMax(0, request.endYear - request.startYear + 1);
    m_expectedSteps = expectedProgressSteps(m_totalYears);
    m_completedSteps = 0;
    m_mockProgress = 0;
    emit progressChanged(0, QStringLiteral("Przygotowanie zapytania (mock)…"));
    m_mockTimer.start(80);
}

void BackendClient::onMockTick()
{
    ++m_completedSteps;
    const int percent = m_expectedSteps > 0
                            ? qMin(99, (m_completedSteps * 100) / m_expectedSteps)
                            : qMin(m_mockProgress += 4, 99);

    QString stage;
    if (m_completedSteps <= m_totalYears * 2) {
        const int pairIndex = (m_completedSteps - 1) / 2;
        const int year = m_pendingRequest.startYear + pairIndex;
        const bool withClouds = (m_completedSteps % 2 == 1);
        stage = withClouds
                    ? QStringLiteral("Pobrano dane dla roku %1 z chmurami (mock)…").arg(year)
                    : QStringLiteral("Pobrano dane dla roku %1 bez chmur (mock)…").arg(year);
    } else {
        stage = QStringLiteral("Pobrano dane o powierzchni (mock)…");
    }

    emit progressChanged(percent, stage);

    if (m_completedSteps >= m_expectedSteps) {
        m_mockTimer.stop();
        emit progressChanged(100, QStringLiteral("Gotowe"));
        AnalysisResult result = MockDataGenerator::generate(m_pendingRequest);
        finalizeHectares(&result, m_pendingRequest);
        emit finished(result);
    }
}

void BackendClient::startHttp(const AnalysisRequest &request)
{
    Q_UNUSED(request);

    m_streamBuffer.clear();
    m_yearCountReceived = false;
    m_totalYears = 0;
    m_expectedSteps = 0;
    m_completedSteps = 0;

    const QUrl url(m_baseUrl + QStringLiteral("/analiza"));
    QNetworkRequest netRequest(url);
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                         QStringLiteral("application/json"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    netRequest.setTransferTimeout(0);
#endif

    const QByteArray body = QJsonDocument(m_pendingRequest.toJson()).toJson(QJsonDocument::Compact);
    m_reply = m_network->post(netRequest, body);
    connect(m_reply, &QNetworkReply::readyRead, this, &BackendClient::onNetworkReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &BackendClient::onNetworkFinished);

    emit progressChanged(0, QStringLiteral("Łączenie z backendem (GEE)…"));
}

void BackendClient::onNetworkReadyRead()
{
    if (!m_reply) {
        return;
    }

    m_streamBuffer.append(m_reply->readAll());

    int newlineIndex = m_streamBuffer.indexOf('\n');
    while (newlineIndex >= 0) {
        const QByteArray line = m_streamBuffer.left(newlineIndex).trimmed();
        m_streamBuffer.remove(0, newlineIndex + 1);
        if (!line.isEmpty()) {
            processStreamLine(line);
        }
        newlineIndex = m_streamBuffer.indexOf('\n');
    }
}

void BackendClient::processStreamLine(const QByteArray &line)
{
    // Ostatnia linia — JSON wyniku
    if (line.startsWith('{')) {
        finishWithJson(line);
        return;
    }

    // Pierwsza linia — liczba lat (KONIEC - POCZATEK + 1)
    static const QRegularExpression yearCountRe(QStringLiteral("^\\d+$"));
    if (!m_yearCountReceived && yearCountRe.match(QString::fromUtf8(line)).hasMatch()) {
        m_yearCountReceived = true;
        m_totalYears = QString::fromUtf8(line).toInt();
        m_expectedSteps = expectedProgressSteps(m_totalYears);
        emit progressChanged(0,
                             QStringLiteral("Pobieranie danych dla %1 lat…").arg(m_totalYears));
        return;
    }

    // Komunikat postępu
    ++m_completedSteps;
    const int percent = m_expectedSteps > 0
                            ? qMin(99, (m_completedSteps * 100) / m_expectedSteps)
                            : -1;
    emit progressChanged(percent, QString::fromUtf8(line));
}

void BackendClient::finishWithJson(const QByteArray &jsonBytes)
{
    QString parseError;
    AnalysisResult result = AnalysisResponseParser::parse(jsonBytes, &parseError);
    if (!result.isValid()) {
        emit failed(parseError.isEmpty()
                        ? QStringLiteral("Nie udało się sparsować wyniku backendu.")
                        : parseError);
        return;
    }

    finalizeHectares(&result, m_pendingRequest);
    emit progressChanged(100, QStringLiteral("Gotowe"));
    emit finished(result);

    if (m_reply) {
        m_reply->disconnect(this);
    }
}

void BackendClient::onNetworkFinished()
{
    if (!m_reply) {
        return;
    }

    QNetworkReply *reply = m_reply;
    m_reply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        QString message = reply->errorString();
        if (reply->error() == QNetworkReply::ConnectionRefusedError) {
            message = QStringLiteral(
                "Nie można połączyć się z backendem na %1.\n"
                "Uruchom start-backend.bat (backend) i upewnij się, że działa uvicorn.")
                           .arg(m_baseUrl);
        }
        emit failed(message);
        reply->deleteLater();
        return;
    }

    // Reszta bufora bez końcowego \n
    if (!m_streamBuffer.trimmed().isEmpty()) {
        processStreamLine(m_streamBuffer.trimmed());
        m_streamBuffer.clear();
    }

    reply->deleteLater();
}
