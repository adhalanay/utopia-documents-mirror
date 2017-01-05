/*****************************************************************************
 *  
 *   This file is part of the Utopia Documents application.
 *       Copyright (c) 2008-2016 Lost Island Labs
 *           <info@utopiadocs.com>
 *   
 *   Utopia Documents is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU GENERAL PUBLIC LICENSE VERSION 3 as
 *   published by the Free Software Foundation.
 *   
 *   Utopia Documents is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *   Public License for more details.
 *   
 *   In addition, as a special exception, the copyright holders give
 *   permission to link the code of portions of this program with the OpenSSL
 *   library under certain conditions as described in each individual source
 *   file, and distribute linked combinations including the two.
 *   
 *   You must obey the GNU General Public License in all respects for all of
 *   the code used other than OpenSSL. If you modify file(s) with this
 *   exception, you may extend this exception to your version of the file(s),
 *   but you are not obligated to do so. If you do not wish to do so, delete
 *   this exception statement from your version.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with Utopia Documents. If not, see <http://www.gnu.org/licenses/>
 *  
 *****************************************************************************/

#ifndef MOLECULESPANEFACTORY_H
#define MOLECULESPANEFACTORY_H

#include <papyro/embeddedpanefactory.h>
#include <papyro/utils.h>
#include <utopia2/networkaccessmanager.h>
#include <utopia2/fileformat.h>
#include <utopia2/qt/webview.h>
#include <string>

#include <cmath>

#include <QBuffer>
#include <QEventLoop>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QWebFrame>

class MoleculesPane : public QWidget, public Utopia::NetworkAccessManagerMixin
{
    Q_OBJECT

public:
    MoleculesPane(QString code, QWidget * parent = 0)
        : QWidget(parent), _code(code), _webView(0), _progress(-1.0), _retryHover(false), _retryPressed(false), _downloaded(false), _retries(3), _redirects(0)
    {
        _layout = new QVBoxLayout(this);
        _layout->setContentsMargins(0, 0, 0, 0);
        _layout->setSpacing(0);

        // QNetworkAccessManager stuff for getting update information
        _checker.setInterval(1000);
        connect(&_checker, SIGNAL(timeout()), this, SLOT(check()));

        // Widget stuff
        setMouseTracking(true);
        resize(400, 400);

        // Start download
        restart();
    }

    MoleculesPane(QByteArray pdbfile, QWidget * parent = 0)
        : QWidget(parent), _pdbFile(pdbfile), _webView(0), _progress(-1.0), _retryHover(false), _retryPressed(false), _downloaded(false), _retries(3), _redirects(0)
    {
        _layout = new QVBoxLayout(this);
        _layout->setContentsMargins(0, 0, 0, 0);
        _layout->setSpacing(0);

        // QNetworkAccessManager stuff for getting update information
        _checker.setInterval(1000);
        connect(&_checker, SIGNAL(timeout()), this, SLOT(check()));

        // Widget stuff
        setMouseTracking(true);
        resize(400, 400);

        // Start download
        restart();
    }

    MoleculesPane(QUrl url, QWidget * parent = 0)
        : QWidget(parent), _url(url), _webView(0), _progress(-1.0), _retryHover(false), _retryPressed(false), _downloaded(false), _retries(3)
    {
        _layout = new QVBoxLayout(this);
        _layout->setContentsMargins(0, 0, 0, 0);
        _layout->setSpacing(0);

        // QNetworkAccessManager stuff for getting update information
        _checker.setInterval(1000);
        connect(&_checker, SIGNAL(timeout()), this, SLOT(check()));

        // Widget stuff
        setMouseTracking(true);
        resize(400, 400);

        // Start download
        restart();
    }

    virtual ~MoleculesPane()
    {
        delete _webView;
    }

public slots:
    void setupJavaScriptWindowObject()
    {
        QString command(QString("jQuery(function () { molify('#molecule-viewer', 'pdb:%1'); });").arg(_code));
        _webView->page()->mainFrame()->evaluateJavaScript(command);

    }

protected slots:
    void abort()
    {
        // Abort because of timeout
        _reply->abort();
        _checker.stop();
    }

    void check()
    {
        if (_lastUpdate.elapsed() > 15000)
        {
            abort();
        }
    }

    void getCompleted()
    {
        _reply->deleteLater();

        // If this is a redirect, follow it transparently, but only to a maximum of (arbitrarily)
        // four redirections
        QUrl redirectedUrl = _reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (redirectedUrl.isValid()) {
            if (redirectedUrl.isRelative()) {
                QUrl oldUrl = _reply->url();
                redirectedUrl.setScheme(oldUrl.scheme());
                redirectedUrl.setAuthority(oldUrl.authority());
            }
            if (_redirects++ < 4) {
                QNetworkRequest request = _reply->request();
                request.setUrl(redirectedUrl);
                _reply = networkAccessManager()->get(request);
                connect(_reply.data(), SIGNAL(finished()), this, SLOT(getCompleted()));
                connect(_reply.data(), SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(getFailed(QNetworkReply::NetworkError)));
                connect(_reply.data(), SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(getProgressed(qint64, qint64)));
                return;
            }
        }

        // Reset redirect count and make sure those who care deal with this reply finishing
        _redirects = 0;
        _progress = 1.0;
        _checker.stop();

        _downloaded = true;

        _replyData = _reply->readAll();

        if (isVisible()) {
            load();
        }

        update();
    }

    void getFailed(QNetworkReply::NetworkError code)
    {
        switch (code)
        {
        case QNetworkReply::ContentNotFoundError:
            _errorMessage = "Requested data not available";
            break;
        case QNetworkReply::HostNotFoundError:
            _errorMessage = "Host not found (www.rcsb.org)";
            break;
        case QNetworkReply::ConnectionRefusedError:
            _errorMessage = "Connection refused (www.rcsb.org)";
            break;
        case QNetworkReply::RemoteHostClosedError:
            _errorMessage = "Unexpected disconnection (www.rcsb.org)";
            break;
        case QNetworkReply::ProtocolFailure:
            _errorMessage = "Malformed response (www.rcsb.org)";
            break;
        case QNetworkReply::ProxyAuthenticationRequiredError:
        case QNetworkReply::AuthenticationRequiredError:
            _errorMessage = "Authentication failed (www.rcsb.org)";
            break;
        case QNetworkReply::OperationCanceledError:
        case QNetworkReply::TimeoutError:
            _errorMessage = "Network timeout occurred";
            break;
        default:
            _errorMessage = "Unknown data download error";
            break;
        }

        if (isHidden() && --_retries > 0)
        {
            QTimer::singleShot(1000, this, SLOT(restart()));
        }
    }

    void getProgressed(qint64 progress, qint64 total)
    {
        if (total > 0)
        {
            _progress = qBound(0.0, progress / (double) total, 1.0);
        }
        _lastUpdate.restart();

        update();
    }

    void load()
    {
        if (_webView == 0)
        {
            _webView = new Utopia::WebView();

            _webView->setRenderHint(QPainter::Antialiasing, true);
            _webView->setRenderHint(QPainter::TextAntialiasing, true);
            _webView->setRenderHint(QPainter::SmoothPixmapTransform, true);

            _webView->page()->mainFrame()->addToJavaScriptWindowObject("control", this);

            _webView->load(QUrl("qrc:/papyro/molecules.html"));

            _webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

            _layout->addWidget(_webView);

            _webView->show();

            update();
        }
    }

    void restart()
    {
        if (isVisible()) {
            load();
        }

        return;




        _errorMessage = QString();

        if (_pdbFile.isEmpty()) {
            _progress = -1.0;
            _checker.start();
            _lastUpdate.start();
            _started.start();
            _downloaded = false;

            if (!_code.isEmpty()) {
                _url = QUrl(QString("http://www.rcsb.org/pdb/download/downloadFile.do?fileFormat=pdb&compression=NO&structureId=") + _code);
                _reply = networkAccessManager()->get(QNetworkRequest(_url));
                connect(_reply.data(), SIGNAL(finished()), this, SLOT(getCompleted()));
                connect(_reply.data(), SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(getFailed(QNetworkReply::NetworkError)));
                connect(_reply.data(), SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(getProgressed(qint64, qint64)));
            }
        } else {
            _progress = -1.0;
            _downloaded = true;

            _replyData = _pdbFile;
            if (isVisible()) {
                load();
            }
        }

        update();
    }

protected:
    QRect retryButtonGeometry()
    {
        int radius = 20;
        QRect spinnerRect(0, 0, 2*radius, 2*radius);
        spinnerRect.moveCenter(rect().center() + QPoint(0, -11) + QPoint(-1, -1));
        QRect messageRect(0, spinnerRect.bottom() + 10, width(), 12);
        QString text("Retry");
        int textWidth = fontMetrics().width(text);
        int textHeight = fontMetrics().height();
        QRect retryRect(0, 0, 12 + 6 + textWidth, qMin(12, textHeight));
        retryRect.moveCenter(rect().center());
        retryRect.moveTop(messageRect.bottom() + 20);
        return retryRect;
    }

    void mouseMoveEvent(QMouseEvent * event)
    {
        bool old = _retryHover;
        _retryHover = retryButtonGeometry().contains(event->pos());
        if (old != _retryHover)
        {
            update();
        }
    }

    void mousePressEvent(QMouseEvent * event)
    {
        bool old = _retryPressed;
        _retryPressed = event->buttons(); // & Qt::LeftButton && retryButtonGeometry().contains(event->pos());
        if (old != _retryPressed)
        {
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent * event)
    {
        bool old = _retryPressed;
        _retryPressed = false;
        if (old) // && retryButtonGeometry().contains(event->pos()))
        {
            _retries = 3;
            restart();
        }
        else if (old != _retryPressed)
        {
            update();
        }
    }

    void paintEvent(QPaintEvent * event)
    {
        if (_webView != 0) return;

        QString message;
        int radius = 20;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(230, 230, 230));
        p.drawRect(rect());
        p.setPen(QColor(140, 140, 140));

        // Spinner
        QRect spinnerRect(0, 0, 2*radius, 2*radius);
        spinnerRect.moveCenter(rect().center() + QPoint(0, -11) + QPoint(-1, -1));
        if (!_errorMessage.isEmpty())
        {
            message = _errorMessage;

            QPen pen(p.pen());
            pen.setColor(QColor(180, 140, 140));
            pen.setWidth(4.0);
            p.setBrush(Qt::NoBrush);
            p.setPen(pen);
            p.drawEllipse(spinnerRect);
            QRect lineRect(0, 0, 2*radius/sqrt(2), 2*radius/sqrt(2));
            lineRect.moveCenter(rect().center() + QPoint(0, -11));
            p.drawLine(lineRect.bottomLeft(), lineRect.topRight());

            // Retry!
            QRect retryRect = retryButtonGeometry();
            if (_retryHover || _retryPressed)
            {
                pen = p.pen();
                pen.setWidth(1.0);
                if (_retryPressed)
                {
                    p.setBrush(QColor(230, 200, 200));
                }
                else
                {
                    p.setBrush(Qt::NoBrush);
                }
                p.setPen(pen);
                p.drawRect(retryRect.adjusted(-3, -3, 3, 3));
            }
            p.drawText(retryRect.adjusted(12 + 6, 0, 0, 0), Qt::AlignVCenter, "Retry");
            pen = p.pen();
            pen.setWidth(2.0);
            p.setBrush(Qt::NoBrush);
            p.setPen(pen);
            QRect iconRect(retryRect.topLeft(), QSize(12, 12));
            p.drawArc(iconRect, 16*90.0, -16*135.0);
            p.drawLine((iconRect.left() + iconRect.right()) / 2, iconRect.top(), 2 + (iconRect.left() + iconRect.right()) / 2, 2 + iconRect.top());
            p.drawArc(iconRect, 16*270.0, -16*135.0);
            p.drawLine((iconRect.left() + iconRect.right()) / 2, iconRect.bottom(), -1 + (iconRect.left() + iconRect.right()) / 2, -1 + iconRect.bottom());
        }
        else if (_progress >= 0 && _progress < 1.0)
        {
            message = "Downloading data...";
            QPen pen(p.pen());
            pen.setWidth(1.0);
            p.setPen(pen);
            p.setBrush(QColor(140, 140, 140));
            p.drawPie(spinnerRect, 16*90.0, -16*360.0*_progress);
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(spinnerRect);
        }
        else if (_progress == 1.0 || _progress == -1.0)
        {
            message = _progress == 1.0 ? "Parsing data..." : "Downloading data...";
            int startAngle = _started.elapsed() * 5;
            int spanAngle = 120*16*2;
            QPen pen(p.pen());
            pen.setWidth(4.0);
            p.setBrush(Qt::NoBrush);
            p.setPen(pen);
            p.drawArc(spinnerRect.adjusted(2, 2, -2, -2), -startAngle, spanAngle);
            QTimer::singleShot(40, this, SLOT(update()));
        }
        else
        {
            message = "Initialising visualisation...";
        }

        QRect messageRect(0, spinnerRect.bottom() + 10, width(), 12);
        p.drawText(messageRect, Qt::AlignCenter, message);
    }

    void showEvent(QShowEvent * event)
    {
        if (_downloaded && _webView == 0)
        {
            QTimer::singleShot(0, this, SLOT(load()));
        }
    }

private:
    QString _code;
    QByteArray _pdbFile;
    QUrl _url;
    QString _errorMessage;
    QVBoxLayout * _layout;
    Utopia::WebView * _webView;

    QTimer _checker;
    QPointer< QNetworkReply > _reply;
    QByteArray _replyData;
    double _progress;
    QTime _lastUpdate;
    QTime _started;
    bool _retryHover;
    bool _retryPressed;
    bool _downloaded;
    int _retries;
    int _redirects;
};

class MoleculesPaneFactory : public Papyro::EmbeddedPaneFactory
{

public:
    QSet< Utopia::FileFormat * > formats;

    // Constructor
    MoleculesPaneFactory()
        : Papyro::EmbeddedPaneFactory()
    {
        formats = Utopia::FileFormat::get(Utopia::StructureFormat);
    }

    // Destructor
    virtual ~MoleculesPaneFactory()
    {}

    virtual QWidget * create(Spine::AnnotationHandle annotation, QWidget * parent = 0)
    {
        MoleculesPane * pane = 0;
        std::string concept = annotation->getFirstProperty("concept");
        std::string identifier = annotation->getFirstProperty("property:identifier");
        std::vector< std::string > media = annotation->getProperty("session:media");
        if (concept == "DatabaseEntry" && identifier.substr(0, 23)=="http://bio2rdf.org/pdb:")
        {
            std::string code(identifier.substr(23));
            pane = new MoleculesPane(Papyro::qStringFromUnicode(code), parent);
        }
        else if (concept == "DataLink" && media.size() > 0)
        {
            QString mime_type, url, name;
            foreach(std::string media_link, media)
            {
                QString encoded(Papyro::qStringFromUnicode(media_link));
                QStringList pairs(encoded.split("&"));
                foreach(QString pair, pairs)
                {
                    QString key(QUrl::fromPercentEncoding(pair.section('=', 0, 0).toUtf8()));
                    QString value(QUrl::fromPercentEncoding(pair.section('=', 1, 1).toUtf8()));
                    if (key == "type") mime_type = value;
                    else if (key == "src") url = value;
                    else if (key == "name") name = value;
                }
            }
            if (!url.isEmpty())
            {
                if (mime_type == "chemical/x-pdb")
                {
                    pane = new MoleculesPane(QUrl(url), parent);
                }
                else if (!name.isEmpty())
                {
                    foreach (Utopia::FileFormat * format, formats)
                    {
                        if (format->contains(name.section(".", -1)))
                        {
                            pane = new MoleculesPane(QUrl(url), parent);
                            break;
                        }
                    }
                }
            }
        }

        return pane;
    }

    virtual QString title()
    {
        return "3D Molecule";
    }

}; // class MoleculesPaneFactory

#endif // MOLECULESPANEFACTORY_H
