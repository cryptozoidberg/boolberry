// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "html5applicationviewer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLinearLayout>
#include <QGraphicsWebView>
#include <QWebFrame>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include "warnings.h"
#include "net/http_client.h"

class Html5ApplicationViewerPrivate : public QGraphicsView
{
    Q_OBJECT
public:
    Html5ApplicationViewerPrivate(QWidget *parent = 0);

    void resizeEvent(QResizeEvent *event);
    static QString adjustPath(const QString &path);

public slots:
    void quit();
    void closeEvent(QCloseEvent *event);

private slots:
    void addToJavaScript();

signals:
    void quitRequested();
    void update_daemon_state(const QString str);
    void update_wallet_status(const QString str);
    void update_wallet_info(const QString str);
    void money_transfer(const QString str);
    void show_wallet();
    void hide_wallet();
    void switch_view(const QString str);
    void set_recent_transfers(const QString str);
    void handle_internal_callback(const QString str, const QString callback_name);



public:
    QGraphicsWebView *m_webView;
#ifdef TOUCH_OPTIMIZED_NAVIGATION
    NavigationController *m_controller;
#endif // TOUCH_OPTIMIZED_NAVIGATION
};

void Html5ApplicationViewerPrivate::closeEvent(QCloseEvent *event)
{

}

Html5ApplicationViewerPrivate::Html5ApplicationViewerPrivate(QWidget *parent)
    : QGraphicsView(parent)
{
    QGraphicsScene *scene = new QGraphicsScene;
    setScene(scene);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_webView = new QGraphicsWebView;
    //m_webView->setAcceptTouchEvents(true);
    //m_webView->setAcceptHoverEvents(false);
    m_webView->setAcceptTouchEvents(false);
    m_webView->setAcceptHoverEvents(true);
    //setAttribute(Qt::WA_AcceptTouchEvents, true);
    scene->addItem(m_webView);
    scene->setActiveWindow(m_webView);
#ifdef TOUCH_OPTIMIZED_NAVIGATION
    m_controller = new NavigationController(parent, m_webView);
#endif // TOUCH_OPTIMIZED_NAVIGATION
    connect(m_webView->page()->mainFrame(),
            SIGNAL(javaScriptWindowObjectCleared()), SLOT(addToJavaScript()));
}

void Html5ApplicationViewerPrivate::resizeEvent(QResizeEvent *event)
{
    m_webView->resize(event->size());
}

QString Html5ApplicationViewerPrivate::adjustPath(const QString &path)
{
#ifdef Q_OS_UNIX
#ifdef Q_OS_MAC
    if (!QDir::isAbsolutePath(path))
        return QCoreApplication::applicationDirPath()
                + QLatin1String("/../Resources/") + path;
#else
    const QString pathInInstallDir = QCoreApplication::applicationDirPath()
        + QLatin1String("/../") + path;
    if (pathInInstallDir.contains(QLatin1String("opt"))
            && pathInInstallDir.contains(QLatin1String("bin"))
            && QFileInfo(pathInInstallDir).exists()) {
        return pathInInstallDir;
    }
#endif
#endif
    return QFileInfo(path).absoluteFilePath();
}

void Html5ApplicationViewerPrivate::quit()
{
    emit quitRequested();
}

void Html5ApplicationViewerPrivate::addToJavaScript()
{
    m_webView->page()->mainFrame()->addToJavaScriptWindowObject("Qt_parent", QGraphicsView::parent());
    m_webView->page()->mainFrame()->addToJavaScriptWindowObject("Qt", this);
}

Html5ApplicationViewer::Html5ApplicationViewer(QWidget *parent)
    : QWidget(parent)
    , m_d(new Html5ApplicationViewerPrivate(this)),
      m_quit_requested(false),
      m_deinitialize_done(false), 
      m_backend_stopped(false)
{
    //connect(m_d, SIGNAL(quitRequested()), SLOT(close()));

    connect(m_d, SIGNAL(quitRequested()), this, SLOT(on_request_quit()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_d);
    layout->setMargin(0);
    setLayout(layout);
}

bool Html5ApplicationViewer::init_config()
{  
  epee::serialization::load_t_from_json_file(m_config, m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME);
  if (!m_config.wallets_last_used_dir.size())
  {
    m_config.wallets_last_used_dir = tools::get_default_user_dir();
    tools::create_directories_if_necessary(m_config.wallets_last_used_dir);
  }
  return true;
}

bool Html5ApplicationViewer::store_config()
{
  epee::serialization::store_t_to_json_file(m_config, m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME);
  return true;
}

Html5ApplicationViewer::~Html5ApplicationViewer()
{
  while (m_request_uri_threads_count)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  store_config();
  delete m_d;
}
void Html5ApplicationViewer::closeEvent(QCloseEvent *event)
{
  if(!m_deinitialize_done)
  {
    on_request_quit();
    event->ignore();
  }else
  {
    event->accept();
  }
}

void Html5ApplicationViewer::changeEvent(QEvent *e)
{
	switch (e->type())
	{
	case QEvent::WindowStateChange:
	{
		if (this->windowState() & Qt::WindowMinimized)
		{
			if (m_trayIcon)
			{
				QTimer::singleShot(250, this, SLOT(hide()));
				m_trayIcon->showMessage("Boolberry app is minimized to tray",
					"You can restore it with double-click or context menu");
			}
		}

		break;
	}
	default:
		break;
	}

	QWidget::changeEvent(e);
}

void Html5ApplicationViewer::initTrayIcon(const std::string& htmlPath)
{
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	m_restoreAction = std::unique_ptr<QAction>(new QAction(tr("&Restore"), this));
	connect(m_restoreAction.get(), SIGNAL(triggered()), this, SLOT(showNormal()));

	m_quitAction = std::unique_ptr<QAction>(new QAction(tr("&Quit"), this));
	connect(m_quitAction.get(), SIGNAL(triggered()), this, SLOT(on_request_quit()));

	m_trayIconMenu = std::unique_ptr<QMenu>(new QMenu(this));
	m_trayIconMenu->addAction(m_restoreAction.get());
	m_trayIconMenu->addSeparator();
	m_trayIconMenu->addAction(m_quitAction.get());

	m_trayIcon = std::unique_ptr<QSystemTrayIcon>(new QSystemTrayIcon(this));
	m_trayIcon->setContextMenu(m_trayIconMenu.get());
#ifdef WIN32
	std::string iconPath(htmlPath + "/files/app16.png"); // windows tray icon size is 16x16
#else
	std::string iconPath(htmlPath + "/files/app22.png"); // X11 tray icon size is 22x22
#endif
	m_trayIcon->setIcon(QIcon(iconPath.c_str()));
	m_trayIcon->setToolTip("Boolberry");
	connect(m_trayIcon.get(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)), 
		this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
	m_trayIcon->show();
}

void Html5ApplicationViewer::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::ActivationReason::DoubleClick)
	{
		showNormal();
		activateWindow();
	}
}

void Html5ApplicationViewer::loadFile(const QString &fileName)
{
    m_d->m_webView->setUrl(QUrl::fromLocalFile(Html5ApplicationViewerPrivate::adjustPath(fileName)));
}

void Html5ApplicationViewer::loadUrl(const QUrl &url)
{
    m_d->m_webView->setUrl(url);
}
PUSH_WARNINGS
DISABLE_VS_WARNINGS(4065)
void Html5ApplicationViewer::setOrientation(ScreenOrientation orientation)
{
    Qt::WidgetAttribute attribute;
    switch (orientation) {
#if QT_VERSION < 0x040702
    // Qt < 4.7.2 does not yet have the Qt::WA_*Orientation attributes
    case ScreenOrientationLockPortrait:
        attribute = static_cast<Qt::WidgetAttribute>(128);
        break;
    case ScreenOrientationLockLandscape:
        attribute = static_cast<Qt::WidgetAttribute>(129);
        break;
    default:
    case ScreenOrientationAuto:
        attribute = static_cast<Qt::WidgetAttribute>(130);
        break;
#elif QT_VERSION < 0x050000
    case ScreenOrientationLockPortrait:
        attribute = Qt::WA_LockPortraitOrientation;
        break;
    case ScreenOrientationLockLandscape:
        attribute = Qt::WA_LockLandscapeOrientation;
        break;
    default:
    case ScreenOrientationAuto:
        attribute = Qt::WA_AutoOrientation;
        break;
#else
    default:
        attribute = Qt::WidgetAttribute();
#endif
    };
    setAttribute(attribute, true);
}
POP_WARNINGS

void Html5ApplicationViewer::showExpanded()
{
#if defined(Q_WS_MAEMO_5)
    showMaximized();
#else
    show();
#endif
    this->setMouseTracking(true);
    this->setMinimumWidth(800);
    this->setMinimumHeight(600);
    //this->setFixedSize(800, 600);
    m_d->m_webView->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    m_d->m_webView->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
}

QGraphicsWebView *Html5ApplicationViewer::webView() const
{
  return m_d->m_webView;
}

bool Html5ApplicationViewer::on_request_quit()
{
  m_quit_requested = true;
  if (m_backend_stopped)
  {
    bool r = QMetaObject::invokeMethod(this,
      "do_close",
      Qt::QueuedConnection);
  }else
    m_backend.send_stop_signal();
  return true;
}

bool Html5ApplicationViewer::do_close()
{
  this->close();
  return true;
}

bool Html5ApplicationViewer::on_backend_stopped()
{
  m_backend_stopped = true;
  m_deinitialize_done = true;
  if(m_quit_requested)
  {
    bool r = QMetaObject::invokeMethod(this,
                                 "do_close",
                                 Qt::QueuedConnection);
  }
  return true;
}

bool Html5ApplicationViewer::update_daemon_status(const view::daemon_status_info& info)
{
  //m_d->update_daemon_state(info);
  std::string json_str;
  epee::serialization::store_t_to_json(info, json_str);
  m_d->update_daemon_state(json_str.c_str());
  return true;
}

QString Html5ApplicationViewer::request_aliases()
{
  QString res = "{}";
  view::alias_set al_set;
  if (m_backend.get_aliases(al_set))
  {
    std::string json_str;
    epee::serialization::store_t_to_json(al_set, json_str);

    res = json_str.c_str();
  }
  return res;
}

bool Html5ApplicationViewer::show_msg_box(const std::string& message)
{
  QMessageBox::information(m_d, "Error", message.c_str(), QMessageBox::Ok);
  return true;
}

bool Html5ApplicationViewer::start_backend(int argc, char* argv[])
{
  return m_backend.start(argc, argv, this);
}

bool Html5ApplicationViewer::update_wallet_status(const view::wallet_status_info& wsi)
{
  std::string json_str;
  epee::serialization::store_t_to_json(wsi, json_str);
  m_d->update_wallet_status(json_str.c_str());
  return true;
}

bool Html5ApplicationViewer::update_wallet_info(const view::wallet_info& wsi)
{
  std::string json_str;
  epee::serialization::store_t_to_json(wsi, json_str);
  m_d->update_wallet_info(json_str.c_str());
  return true;
}

bool Html5ApplicationViewer::money_transfer(const view::transfer_event_info& tei)
{
  std::string json_str;
  epee::serialization::store_t_to_json(tei, json_str);
  m_d->money_transfer(json_str.c_str());
  if (!m_trayIcon)
	  return true;
  if (!tei.ti.is_income)
	  return true;
  double amount = double(tei.ti.amount) * (1e-12);
  if (tei.ti.height == 0) // unconfirmed trx
  {
	  QString msg = QString("BBR%1 is received").arg(amount);
	  m_trayIcon->showMessage("Income transfer (unconfirmed)", msg);
  }
  else // confirmed trx
  {
	  QString msg = QString("BBR%1 is confirmed").arg(amount);
	  m_trayIcon->showMessage("Income transfer confirmed", msg);
  }
  return true;
}


bool Html5ApplicationViewer::hide_wallet()
{
  m_d->hide_wallet();
  return true;
}
bool Html5ApplicationViewer::show_wallet()
{
  m_d->show_wallet();
  return true;
}

void Html5ApplicationViewer::close_wallet()
{
  m_backend.close_wallet();
}

bool Html5ApplicationViewer::switch_view(int view_no)
{
  view::switch_view_info swi = AUTO_VAL_INIT(swi);
  swi.view = view_no;
  std::string json_str;
  epee::serialization::store_t_to_json(swi, json_str);
  m_d->switch_view(json_str.c_str());
  return true;
}

bool Html5ApplicationViewer::set_recent_transfers(const view::transfers_array& ta)
{
  std::string json_str;
  epee::serialization::store_t_to_json(ta, json_str);
  m_d->set_recent_transfers(json_str.c_str());
  return true;
}

bool Html5ApplicationViewer::set_html_path(const std::string& path)
{ 
  initTrayIcon(path);
  loadFile(QLatin1String((path + "/index.html").c_str()));
  return true;
}

QString Html5ApplicationViewer::get_version()
{
  return PROJECT_VERSION_LONG;
}

bool is_uri_allowed(const QString& uri)
{
  //TODO: add some code later here
  return true;
}

QString Html5ApplicationViewer::request_uri(const QString& url_str, const QString& params, const QString& callbackname)
{
  
  ++m_request_uri_threads_count;
  std::thread([url_str, params, callbackname, this](){
  
    view::request_uri_params prms;
    if (!epee::serialization::load_t_from_json(prms, params.toStdString()))
    {
      show_msg_box("Internal error: failed to load request_uri params");
      m_d->handle_internal_callback("", callbackname);
      --m_request_uri_threads_count;
      return;
    }
    QNetworkAccessManager NAManager;
    QUrl url(url_str);
    QNetworkRequest request(url);
    QNetworkReply *reply = NAManager.get(request);
    QEventLoop eventLoop;
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();
    QByteArray res = reply->readAll();
    m_d->handle_internal_callback(res, callbackname);
    --m_request_uri_threads_count;
  }).detach();
  

  return "";
  
}

QString Html5ApplicationViewer::transfer(const QString& json_transfer_object)
{
  view::transfer_params tp = AUTO_VAL_INIT(tp);
  view::transfer_response tr = AUTO_VAL_INIT(tr);
  tr.success = false;
  if(!epee::serialization::load_t_from_json(tp, json_transfer_object.toStdString()))
  {
    show_msg_box("Internal error: failed to load transfer params");
    return epee::serialization::store_t_to_json(tr).c_str();
  }

  if(!tp.destinations.size())
  {
    show_msg_box("Internal error: empty destinations");
    return epee::serialization::store_t_to_json(tr).c_str();
  }

  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);

  if(!m_backend.transfer(tp, res_tx))
  {
    return epee::serialization::store_t_to_json(tr).c_str();
  }
  tr.success = true;
  tr.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
  tr.tx_blob_size = currency::get_object_blobsize(res_tx);

  return epee::serialization::store_t_to_json(tr).c_str();
}

void Html5ApplicationViewer::message_box(const QString& msg)
{
  show_msg_box(msg.toStdString());
}

void Html5ApplicationViewer::generate_wallet()
{
  QFileDialog dialogFile(this);  
  std::string default_file = (tools::get_current_username() + "_wallet.bbr").c_str();
  QString path = dialogFile.getSaveFileName(this, tr("Wallet file to store"),
    (m_config.wallets_last_used_dir + "/" + default_file).c_str(),
    tr("Boolberry wallet (*.bbr *.bbr.keys);; All files (*.*)"));
  
  if (!path.length())
    return;

  m_config.wallets_last_used_dir = boost::filesystem::path(path.toStdString()).parent_path().string();

  //read password
  bool ok;
  QString pass = QInputDialog::getText(this, tr("Enter wallet password"),
    tr("Password:"), QLineEdit::Password,
    QString(), &ok);
  
  if (!ok)
    return;

  m_backend.generate_wallet(path.toStdString(), pass.toStdString());
}

void Html5ApplicationViewer::open_wallet()
{
  QString path = QFileDialog::getOpenFileName(this, tr("Open wallet File"),
                                                   m_config.wallets_last_used_dir.c_str(),
                                                   tr("Boolberry wallet (*.bbr *.bbr.keys);; All files (*.*)"));
  if(!path.length())
    return;

  m_config.wallets_last_used_dir = boost::filesystem::path(path.toStdString()).parent_path().string();


  //read password
  bool ok;
  QString pass = QInputDialog::getText(this, tr("Enter wallet password"),
                                            tr("Password:"), QLineEdit::Password,
                                            QString(), &ok);
  if(!ok)
    return;

  m_backend.open_wallet(path.toStdString(), pass.toStdString());
}

#include "html5applicationviewer.moc"
