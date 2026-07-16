#include "backendclient.h"

#include "analysisresponseparser.h"
#include "processing/imagerenderer.h"
#include "processing/mockdatagenerator.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

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

    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void BackendClient::startMock(const AnalysisRequest &request)
{
    Q_UNUSED(request);
    m_mockProgress = 0;
    emit progressChanged(0, QStringLiteral("Przygotowanie zapytania (mock)…"));
    m_mockTimer.start(80);
}

void BackendClient::onMockTick()
{
    m_mockProgress += 4;
    emit progressChanged(qMin(m_mockProgress, 99),
                         QStringLiteral("Generowanie syntetycznych pasm…"));

    if (m_mockProgress >= 100) {
        m_mockTimer.stop();
        emit progressChanged(100, QStringLiteral("Gotowe"));
        AnalysisResult result = MockDataGenerator::generate(m_pendingRequest);
        finalizeHectares(&result, m_pendingRequest);
        emit finished(result);
    }
}

void BackendClient::startHttp(const AnalysisRequest &request)
{
    const QUrl url(m_baseUrl + QStringLiteral("/analiza"));
    QNetworkRequest netRequest(url);
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                         QStringLiteral("application/json"));

    // GEE potrafi liczyć długo — bez twardego limitu (0 = wyłączony w Qt 6)
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    netRequest.setTransferTimeout(0);
#endif

    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    m_reply = m_network->post(netRequest, body);
    connect(m_reply, &QNetworkReply::finished, this, &BackendClient::onNetworkFinished);

    // Progres procentowy z backendu przyjdzie później — na razie tylko komunikat
    emit progressChanged(-1, QStringLiteral(
        "Wysłano żądanie do backendu (GEE). To może potrwać kilka minut…"));
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
                "Uruchom start-backend.bat i upewnij się, że działa uvicorn.")
                          .arg(m_baseUrl);
        }
        emit failed(message);
        reply->deleteLater();
        return;
    }

    const QByteArray payload = reply->readAll();
    reply->deleteLater();

    QString parseError;
    AnalysisResult result = AnalysisResponseParser::parse(payload, &parseError);
    if (!result.isValid()) {
        emit failed(parseError.isEmpty()
                        ? QStringLiteral("Nie udało się sparsować odpowiedzi backendu.")
                        : parseError);
        return;
    }

    finalizeHectares(&result, m_pendingRequest);
    emit progressChanged(100, QStringLiteral("Gotowe"));
    emit finished(result);
}
