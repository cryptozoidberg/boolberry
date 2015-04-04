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
  void update_pos_mining_text(const QString str);
  //general function
  void dispatch(const QString status, const QString params);



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
m_backend_stopped(false), 
m_request_uri_threads_count(0), 
m_request_id_counter(0), 
m_is_stop(false)
{
  //connect(m_d, SIGNAL(quitRequested()), SLOT(close()));
  m_dispatcher = std::thread([this](){
    dispatcher();
  });
  connect(m_d, SIGNAL(quitRequested()), this, SLOT(on_request_quit()));

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(m_d);
  layout->setMargin(0);
  setLayout(layout);
}
void Html5ApplicationViewer::dispatcher()
{
  while (!m_is_stop)
  {
    dispatch_entry de;
    {
      m_dispatch_que_lock.lock();
      if (!m_dispatch_que.size())
      {
        m_dispatch_que_lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        continue;
      }
      de = m_dispatch_que.front();
      m_dispatch_que.pop_front();
      m_dispatch_que_lock.unlock();
    }
    de.cb->do_call();
  }
}
bool Html5ApplicationViewer::init_config()
{
  return true;
}

QString Html5ApplicationViewer::get_default_user_dir(const QString& param)
{
  return tools::get_default_user_dir().c_str();
}


bool Html5ApplicationViewer::toggle_mining()
{
  m_backend.toggle_pos_mining();
  return true;
}
QString Html5ApplicationViewer::get_exchange_last_top(const QString& params)
{
  return QString();
}
bool Html5ApplicationViewer::store_config()
{
  return true;
}

Html5ApplicationViewer::~Html5ApplicationViewer()
{
  m_is_stop = true;
  while (m_request_uri_threads_count)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  m_dispatcher.join();
  store_config();
  delete m_d;
}
void Html5ApplicationViewer::closeEvent(QCloseEvent *event)
{
  if (!m_deinitialize_done)
  {
    on_request_quit();
    event->ignore();
  }
  else
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
                                      m_trayIcon->showMessage("Lui app is minimized to tray",
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
  m_trayIcon->setToolTip("Lui");
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
  m_d->m_webView->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
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
    /*bool r = */QMetaObject::invokeMethod(this,
      "do_close",
      Qt::QueuedConnection);
  }
  else
  {
    m_is_stop = true;
    m_backend.send_stop_signal();
  }
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
  if (m_quit_requested)
  {
    /*bool r = */QMetaObject::invokeMethod(this,
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
  LOG_PRINT_L0("SENDING SIGNAL -> [update_daemon_state]");
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
  LOG_PRINT_L0("SENDING SIGNAL -> [update_wallet_status]");
  m_d->update_wallet_status(json_str.c_str());
  return true;
}

bool Html5ApplicationViewer::update_wallets_info(const view::wallets_summary_info& wsi)
{
  std::string json_str;
  epee::serialization::store_t_to_json(wsi, json_str);
  LOG_PRINT_L0("SENDING SIGNAL -> [update_wallets_info]");
  m_d->update_wallet_info(json_str.c_str());
  return true;
}

bool Html5ApplicationViewer::money_transfer(const view::transfer_event_info& tei)
{
  std::string json_str;
  epee::serialization::store_t_to_json(tei, json_str);

  LOG_PRINT_L0("SENDING SIGNAL -> [money_transfer]");
  m_d->money_transfer(json_str.c_str());
  if (!m_trayIcon)
    return true;
  if (!tei.ti.is_income)
    return true;
  double amount = double(tei.ti.amount) * (1e-12);

  /*
  LOG_PRINT_L0("SHOWING MESSAGE -> [money_transfer]");

  if (tei.ti.height == 0) // unconfirmed trx
  {
        QString msg = QString("%1 BBR is received").arg(amount);
    m_trayIcon->showMessage("Income transfer (unconfirmed)", msg);
  }
  else // confirmed trx
  {
    QString msg = QString("%1 BBR is confirmed").arg(amount);
    m_trayIcon->showMessage("Income transfer confirmed", msg);
  }
  */
  return true;
}


bool Html5ApplicationViewer::hide_wallet()
{
  LOG_PRINT_L0("SENDING SIGNAL -> [hide_wallet]");
  m_d->hide_wallet();
  return true;
}
bool Html5ApplicationViewer::show_wallet()
{
  LOG_PRINT_L0("SENDING SIGNAL -> [show_wallet]");
  m_d->show_wallet();
  return true;
}

bool Html5ApplicationViewer::switch_view(int view_no)
{
  view::switch_view_info swi = AUTO_VAL_INIT(swi);
  swi.view = view_no;
  std::string json_str;
  epee::serialization::store_t_to_json(swi, json_str);
  LOG_PRINT_L0("SENDING SIGNAL -> [switch_view]");
  m_d->switch_view(json_str.c_str());
  return true;
}

bool Html5ApplicationViewer::set_recent_transfers(const view::transfers_array& ta)
{
  std::string json_str;
  epee::serialization::store_t_to_json(ta, json_str);
  LOG_PRINT_L0("SENDING SIGNAL -> [set_recent_transfers]");
  m_d->set_recent_transfers(json_str.c_str());
  return true;
}

bool Html5ApplicationViewer::set_html_path(const std::string& path)
{
  initTrayIcon(path);
  loadFile(QLatin1String((path + "/index.html").c_str()));
  return true;
}
bool Html5ApplicationViewer::pos_block_found(const currency::block& block_found)
{ 
  std::stringstream ss;
  ss << "Found Block h = " << currency::get_block_height(block_found);
  LOG_PRINT_L0("SENDING SIGNAL -> [update_pos_mining_text]");
  m_d->update_pos_mining_text(ss.str().c_str());
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
  /*
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
  */

  return "";

}

QString Html5ApplicationViewer::transfer(const QString& json_transfer_object)
{
  size_t request_id = m_request_id_counter++;
  std::shared_ptr<std::string> param(new std::string(json_transfer_object.toStdString()));

  return que_call("transfer", request_id, [request_id, param, this](void){

    view::api_response ar;
    ar.request_id = std::to_string(request_id);

    view::transfer_params tp = AUTO_VAL_INIT(tp);
    view::transfer_response tr = AUTO_VAL_INIT(tr);
    tr.success = false;
    if (!epee::serialization::load_t_from_json(tp, *param))
    {
      view::api_void av;
      ar.error_code = API_RETURN_CODE_BAD_ARG;
      dispatch(ar, av);
      return;
    }

    if (!tp.destinations.size())
    {
      view::api_void av;
      ar.error_code = API_RETURN_CODE_BAD_ARG;
      dispatch(ar, av);
      return;
    }

    currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
    std::string status = m_backend.transfer(tp.wallet_id, tp, res_tx);
    if (status != API_RETURN_CODE_OK)
    {
      view::api_void av;
      ar.error_code = status;
      dispatch(ar, av);
      return;
    }
    tr.success = true;
    tr.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    tr.tx_blob_size = currency::get_object_blobsize(res_tx);
    dispatch(ar, tr);
    return;

  });
  view::api_response ar;
  ar.error_code = API_RETURN_CODE_OK;
  ar.request_id = std::to_string(request_id);
  return epee::serialization::store_t_to_json(ar).c_str();
}

void Html5ApplicationViewer::message_box(const QString& msg)
{
  show_msg_box(msg.toStdString());
}

QString Html5ApplicationViewer::get_app_data(const QString& param)
{
  std::string app_data_buff;
  file_io_utils::load_file_to_string(m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME, app_data_buff);

  return app_data_buff.c_str();
}

QString Html5ApplicationViewer::store_app_data(const QString& param)
{
  file_io_utils::save_string_to_file(m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME, param.toStdString());
  return "";
}

QString Html5ApplicationViewer::show_openfile_dialog(const QString& param)
{
  view::system_filedialog_request ofdr = AUTO_VAL_INIT(ofdr);
  view::system_filedialog_response ofdres = AUTO_VAL_INIT(ofdres);
  if (!epee::serialization::load_t_from_json(ofdr, param.toStdString()))
  {
    ofdres.error_code = API_RETURN_CODE_BAD_ARG;
    return epee::serialization::store_t_to_json(ofdres).c_str();
  }

  QString path = QFileDialog::getOpenFileName(this, ofdr.caption.c_str(),
    ofdr.default_dir.c_str(),
    ofdr.filemask.c_str());

  if (!path.length())
  {
    ofdres.error_code = API_RETURN_CODE_CANCELED;
    return epee::serialization::store_t_to_json(ofdres).c_str();
  }

  ofdres.error_code = API_RETURN_CODE_OK;
  ofdres.path = path.toStdString();
  return epee::serialization::store_t_to_json(ofdres).c_str();
}

QString Html5ApplicationViewer::show_savefile_dialog(const QString& param)
{

  view::system_filedialog_request ofdr = AUTO_VAL_INIT(ofdr);
  view::system_filedialog_response ofdres = AUTO_VAL_INIT(ofdres);
  if (!epee::serialization::load_t_from_json(ofdr, param.toStdString()))
  {
    ofdres.error_code = API_RETURN_CODE_BAD_ARG;
    return epee::serialization::store_t_to_json(ofdres).c_str();
  }

  QString path = QFileDialog::getSaveFileName(this, ofdr.caption.c_str(),
    ofdr.default_dir.c_str(),
    ofdr.filemask.c_str());

  if (!path.length())
  {
    ofdres.error_code = API_RETURN_CODE_CANCELED;
    return epee::serialization::store_t_to_json(ofdres).c_str();
  }

  ofdres.error_code = API_RETURN_CODE_OK;
  ofdres.path = path.toStdString();
  return epee::serialization::store_t_to_json(ofdres).c_str();
}

QString Html5ApplicationViewer::close_wallet(const QString& param)
{
  size_t request_id = m_request_id_counter++;
  std::shared_ptr<std::string> param_ptr(new std::string(param.toStdString()));

  return que_call("close_wallet", request_id, [request_id, param_ptr, this](void){

    view::api_response ar;
    ar.request_id = std::to_string(request_id);

    view::wallet_id_obj cwr = AUTO_VAL_INIT(cwr);
    if (!epee::serialization::load_t_from_json(cwr, *param_ptr))
    {
      view::api_void av;
      ar.error_code = API_RETURN_CODE_BAD_ARG;
      dispatch(ar, av);
      return;
    }

    ar.error_code = m_backend.close_wallet(cwr.wallet_id);
    dispatch(ar, cwr);
    return;
  });

  view::api_response ar;
  ar.error_code = API_RETURN_CODE_OK;
  ar.request_id = std::to_string(request_id);
  return epee::serialization::store_t_to_json(ar).c_str();
}


QString Html5ApplicationViewer::generate_wallet(const QString& param)
{
  size_t request_id = m_request_id_counter++;
  std::shared_ptr<std::string> param_ptr(new std::string(param.toStdString()));

  return que_call("generate_wallet", request_id, [request_id, param_ptr, this](void){

    view::api_response ar;
    ar.request_id = std::to_string(request_id);

    view::open_wallet_request owd = AUTO_VAL_INIT(owd);
    if (!epee::serialization::load_t_from_json(owd, *param_ptr))
    {
      view::api_void av;
      ar.error_code = API_RETURN_CODE_BAD_ARG;
      dispatch(ar, av);
      return;
    }

    view::wallet_id_obj owr = AUTO_VAL_INIT(owr);
    ar.error_code = m_backend.generate_wallet(owd.path, owd.pass, owr.wallet_id);
    dispatch(ar, owr);
    return;
  });

  view::api_response ar;
  ar.error_code = API_RETURN_CODE_OK;
  ar.request_id = std::to_string(request_id);
  return epee::serialization::store_t_to_json(ar).c_str();

}

QString Html5ApplicationViewer::open_wallet(const QString& param)
{
  size_t request_id = m_request_id_counter++;
  std::shared_ptr<std::string> param_ptr(new std::string(param.toStdString()));

  return que_call("open_wallet", request_id, [request_id, param_ptr, this](void){

    view::api_response ar;
    ar.request_id = std::to_string(request_id);

    view::open_wallet_request owd = AUTO_VAL_INIT(owd);
    if (!epee::serialization::load_t_from_json(owd, *param_ptr))
    {
      view::api_void av;
      ar.error_code = API_RETURN_CODE_BAD_ARG;
      dispatch(ar, av);
      return;
    }

    view::wallet_id_obj owr = AUTO_VAL_INIT(owr);
    ar.error_code = m_backend.open_wallet(owd.path, owd.pass, owr.wallet_id);
    dispatch(ar, owr);
    return;

  });
}

QString Html5ApplicationViewer::get_wallet_info(const QString& param)
{

  size_t request_id = m_request_id_counter++;
  std::shared_ptr<std::string> param_ptr(new std::string(param.toStdString()));

  return que_call("get_wallet_info", request_id, [request_id, param_ptr, this](void){

    view::api_response ar;
    ar.request_id = std::to_string(request_id);

    view::wallet_id_obj wio = AUTO_VAL_INIT(wio);
    if (!epee::serialization::load_t_from_json(wio, *param_ptr))
    {
      view::api_void av;
      ar.error_code = API_RETURN_CODE_BAD_ARG;
      dispatch(ar, av);
      return;
    }

    view::wallet_info wi = AUTO_VAL_INIT(wi);
    ar.error_code = m_backend.get_wallet_info(wio.wallet_id, wi);
    dispatch(ar, wi);
    return;

  });
}

void Html5ApplicationViewer::dispatch(const QString& status, const QString& param)
{
  m_d->dispatch(status, param);
}


#include "html5applicationviewer.moc"
