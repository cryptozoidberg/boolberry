// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "html5applicationviewer.h"
#include <codecvt>

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

#define PREPARE_ARG_FROM_JSON(arg_type, var_name)   \
  arg_type var_name = AUTO_VAL_INIT(var_name); \
  view::api_response ar;  \
if (!epee::serialization::load_t_from_json(var_name, param.toStdString())) \
{                                                          \
  ar.error_code = API_RETURN_CODE_BAD_ARG;                 \
  return epee::serialization::store_t_to_json(ar).c_str(); \
}


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
  void quit_requested(const QString str);
  void update_daemon_state(const QString str);
  void update_wallet_status(const QString str);
  void update_wallet_info(const QString str);
  void money_transfer(const QString str);
  void wallet_sync_progress(const QString str);
  void handle_internal_callback(const QString str, const QString callback_name);
  void update_pos_mining_text(const QString str);
  void do_dispatch(const QString status, const QString params);  //general function



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
  emit quit_requested("{}");
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
  //connect(m_d, SIGNAL(quitRequested()), this, SLOT(on_request_quit()));

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
    m_d->quit();//  ();//on_request_quit();
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

  bool r = QSslSocket::supportsSsl();

  if (!QSystemTrayIcon::isSystemTrayAvailable())
    return;

  m_restoreAction = std::unique_ptr<QAction>(new QAction(tr("&Restore"), this));
  connect(m_restoreAction.get(), SIGNAL(triggered()), this, SLOT(showNormal()));

  m_quitAction = std::unique_ptr<QAction>(new QAction(tr("&Quit"), this));
  connect(m_quitAction.get(), SIGNAL(triggered()), this, SIGNAL(quit_requested()));

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
  this->setMinimumWidth(1200);
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

QString Html5ApplicationViewer::on_request_quit()
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
  return "OK";
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
  
  //lifehack
  if (m_last_update_daemon_status_json == json_str)
    return true;
  
  LOG_PRINT_L0("SENDING SIGNAL -> [update_daemon_state] " << info.daemon_network_state);
  m_d->update_daemon_state(json_str.c_str());
  m_last_update_daemon_status_json = json_str;
  return true;
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
  LOG_PRINT_L0("SENDING SIGNAL -> [update_wallet_status]: wallet_id: " << wsi.wallet_id << ", state: " << wsi.wallet_state);
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

  return true;
}

bool Html5ApplicationViewer::wallet_sync_progress(const view::wallet_sync_progres_param& p)
{
  LOG_PRINT_L0("SENDING SIGNAL -> [wallet_sync_progress]" << " wallet_id: " << p.wallet_id << ": " << p.progress << "%");
  m_d->wallet_sync_progress(epee::serialization::store_t_to_json(p).c_str());
  return true;
}

// bool Html5ApplicationViewer::hide_wallet()
// {
//   LOG_PRINT_L0("SENDING SIGNAL -> [hide_wallet]");
//   m_d->hide_wallet();
//   return true;
// }
// bool Html5ApplicationViewer::show_wallet()
// {
//   LOG_PRINT_L0("SENDING SIGNAL -> [show_wallet]");
//   m_d->show_wallet();
//   return true;
// }

// bool Html5ApplicationViewer::switch_view(int view_no)
// {
//   view::switch_view_info swi = AUTO_VAL_INIT(swi);
//   swi.view = view_no;
//   std::string json_str;
//   epee::serialization::store_t_to_json(swi, json_str);
//   LOG_PRINT_L0("SENDING SIGNAL -> [switch_view]");
//   m_d->switch_view(json_str.c_str());
//   return true;
// }

// bool Html5ApplicationViewer::set_recent_transfers(const view::transfers_array& ta)
// {
//   std::string json_str;
//   epee::serialization::store_t_to_json(ta, json_str);
//   LOG_PRINT_L0("SENDING SIGNAL -> [set_recent_transfers]");
//   m_d->set_recent_transfers(json_str.c_str());
//   return true;
// }


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
QString Html5ApplicationViewer::get_alias_coast(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::struct_with_one_t_type<std::string>, lvl);
  view::get_alias_coast_response resp;
  resp.error_code = m_backend.get_alias_coast(lvl.v, resp.coast);
  return epee::serialization::store_t_to_json(resp).c_str();
}
QString Html5ApplicationViewer::request_alias_registration(const QString& param)
{
  return que_call2<view::request_alias_param>("request_alias_registration", param, [this](const view::request_alias_param& tp, view::api_response& ar){

    view::transfer_response tr = AUTO_VAL_INIT(tr);
    currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
    std::string status = m_backend.request_alias_registration(tp.alias, tp.wallet_id, tp.fee, res_tx, tp.reward);
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
    ar.error_code = API_RETURN_CODE_OK;
    dispatch(ar, tr);
    return;
  });

}

QString Html5ApplicationViewer::transfer(const QString& json_transfer_object)
{
  return que_call2<view::transfer_params>("transfer", json_transfer_object, [this](const view::transfer_params& tp, view::api_response& ar){

    view::transfer_response tr = AUTO_VAL_INIT(tr);
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
}

void Html5ApplicationViewer::message_box(const QString& msg)
{
  show_msg_box(msg.toStdString());
}

QString Html5ApplicationViewer::get_secure_app_data(const QString& param)
{

  view::password_data pwd = AUTO_VAL_INIT(pwd);

  if (!epee::serialization::load_t_from_json(pwd, param.toStdString()))
  { 
    view::api_response ar;
    ar.error_code = API_RETURN_CODE_BAD_ARG;
    return epee::serialization::store_t_to_json(ar).c_str();
  }

  std::string app_data_buff;
  bool r = file_io_utils::load_file_to_string(m_backend.get_config_folder() + "/" + GUI_SECURE_CONFIG_FILENAME, app_data_buff);
  if (!r)
    return "";

  if (app_data_buff.size() < sizeof(app_data_file_binary_header))
  {
    LOG_ERROR("app_data_buff.size() < sizeof(app_data_file_binary_header) check failed");
    view::api_response ar;
    ar.error_code = API_RETURN_CODE_FAIL;
    return epee::serialization::store_t_to_json(ar).c_str();
  }

  crypto::chacha_crypt(app_data_buff, pwd.pass);

  const app_data_file_binary_header* phdr = reinterpret_cast<const app_data_file_binary_header*>(app_data_buff.data());
  if (phdr->m_signature != APP_DATA_FILE_BINARY_SIGNATURE)
  {
    LOG_ERROR("password missmatch: provided pass: " << pwd.pass);
    view::api_response ar;
    ar.error_code = API_RETURN_CODE_WRONG_PASSWORD;
    return epee::serialization::store_t_to_json(ar).c_str();
  }

  return app_data_buff.substr(sizeof(app_data_file_binary_header)).c_str();
}

QString Html5ApplicationViewer::store_app_data(const QString& param)
{
  bool r = file_io_utils::save_string_to_file(m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME, param.toStdString());
  view::api_response ar;
  ar.error_code = store_to_file((m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME).c_str(), param).toStdString();
  return epee::serialization::store_t_to_json(ar).c_str();
}
QString Html5ApplicationViewer::store_to_file(const QString& path, const QString& buff)
{
  bool r = file_io_utils::save_string_to_file(path.toStdString(), buff.toStdString());
  if (r)
    return API_RETURN_CODE_OK;
  else
    return API_RETURN_CODE_ACCESS_DENIED;
}

QString Html5ApplicationViewer::get_app_data()
{
  std::string app_data_buff;
  file_io_utils::load_file_to_string(m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME, app_data_buff);
  return app_data_buff.c_str();
}

QString Html5ApplicationViewer::store_secure_app_data(const QString& param, const QString& pass)
{

  std::string buff(sizeof(app_data_file_binary_header), 0);
  app_data_file_binary_header* phdr = (app_data_file_binary_header*)buff.data();
  phdr->m_signature = APP_DATA_FILE_BINARY_SIGNATURE;
  phdr->m_cb_body = 0; // for future use

  buff.append(param.toStdString());
  crypto::chacha_crypt(buff, pass.toStdString());

  bool r = file_io_utils::save_string_to_file(m_backend.get_config_folder() + "/" + GUI_SECURE_CONFIG_FILENAME, buff);
  view::api_response ar;
  if (r)
    ar.error_code = API_RETURN_CODE_OK;
  else
    ar.error_code = API_RETURN_CODE_FAIL;
  
  //TODO: @#@ delete me!!
  LOG_PRINT_L0("store app data pass:" << pass.toStdString());

  return epee::serialization::store_t_to_json(ar).c_str();
}

QString Html5ApplicationViewer::have_secure_app_data(const QString& param)
{
  view::api_response ar = AUTO_VAL_INIT(ar);

  boost::system::error_code ec;
  if (boost::filesystem::exists(m_backend.get_config_folder() + "/" + GUI_SECURE_CONFIG_FILENAME, ec))
    ar.error_code = API_RETURN_CODE_TRUE;
  else
    ar.error_code = API_RETURN_CODE_FALSE;

  return epee::serialization::store_t_to_json(ar).c_str();
}
QString Html5ApplicationViewer::drop_secure_app_data()
{
  view::api_response ar = AUTO_VAL_INIT(ar);

  boost::system::error_code ec;
  if (boost::filesystem::remove(m_backend.get_config_folder() + "/" + GUI_SECURE_CONFIG_FILENAME, ec))
    ar.error_code = API_RETURN_CODE_TRUE;
  else
    ar.error_code = API_RETURN_CODE_FALSE;
  return epee::serialization::store_t_to_json(ar).c_str();
}
QString Html5ApplicationViewer::get_all_aliases()
{
  return que_call2<view::api_void>("get_all_aliases", "{}", [this](const view::api_void& owd, view::api_response& ar){

    ar.error_code = API_RETURN_CODE_OK;
    view::alias_set a;
    if (API_RETURN_CODE_OK != m_backend.get_aliases(a))
    {
      ar.error_code = API_RETURN_CODE_CORE_BUSY;
    }
    dispatch(ar, a);
  });
}
QString Html5ApplicationViewer::validate_address(const QString& param)
{
  view::api_response ar = AUTO_VAL_INIT(ar);
  ar.error_code = m_backend.validate_address(param.toStdString());
  return epee::serialization::store_t_to_json(ar).c_str();
}
QString Html5ApplicationViewer::set_log_level(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::struct_with_one_t_type<uint64_t>, lvl);
  epee::log_space::get_set_log_detalisation_level(true, lvl.v);
  ar.error_code = API_RETURN_CODE_OK;
  LOG_PRINT_L0("[LOG LEVEL]: set to " << lvl.v);
  return epee::serialization::store_t_to_json(ar).c_str();
}
QString Html5ApplicationViewer::dump_all_offers()
{
  return que_call2<view::api_void>("dump_all_offers", "{}", [this](const view::api_void& owd, view::api_response& ar){
    view::api_void av;
    QString path = QFileDialog::getOpenFileName(this, "Select file",
      "",
      "");

    if (!path.length())
    {
      ar.error_code = API_RETURN_CODE_CANCELED;
      dispatch(ar, av);
      return;
    }

    currency::COMMAND_RPC_GET_ALL_OFFERS::response rp = AUTO_VAL_INIT(rp);
    ar.error_code = m_backend.get_all_offers(rp);

    std::string buff = epee::serialization::store_t_to_json(rp);
    bool r = file_io_utils::save_string_to_file(path.toStdString(), buff);
    if(!r)
      ar.error_code = API_RETURN_CODE_FAIL;
    else
      ar.error_code = API_RETURN_CODE_OK;

    dispatch(ar, av);
    return;
  });
}
QString Html5ApplicationViewer::webkit_launched_script()
{
  m_last_update_daemon_status_json.clear();
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
  PREPARE_ARG_FROM_JSON(view::system_filedialog_request, ofdr);
  view::system_filedialog_response ofdres = AUTO_VAL_INIT(ofdres);

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
  return que_call2<view::wallet_id_obj>("close_wallet", param, [this](const view::wallet_id_obj& owd, view::api_response& ar){

    ar.error_code = m_backend.close_wallet(owd.wallet_id);
    dispatch(ar, owd);
    return;
  });
}

QString Html5ApplicationViewer::generate_wallet(const QString& param)
{
  return que_call2<view::open_wallet_request>("generate_wallet", param, [this](const view::open_wallet_request& owd, view::api_response& ar){
    view::open_wallet_response owr = AUTO_VAL_INIT(owr);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring path_w = conv.from_bytes(owd.path);

    ar.error_code = m_backend.generate_wallet(path_w, owd.pass, owr);
    dispatch(ar, owr);
  });
}


QString Html5ApplicationViewer::restore_wallet(const QString& param)
{
  return que_call2<view::restore_wallet_request>("restore_wallet", param, [this](const view::restore_wallet_request& owd, view::api_response& ar){
    view::open_wallet_response owr = AUTO_VAL_INIT(owr);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring path_w = conv.from_bytes(owd.path);

    ar.error_code = m_backend.restore_wallet(path_w, owd.pass, owd.restore_key, owr);
    dispatch(ar, owr);
  });
}

QString Html5ApplicationViewer::open_wallet(const QString& param)
{
  return que_call2<view::open_wallet_request>("open_wallet", param, [this](const view::open_wallet_request& owd, view::api_response& ar){

    view::open_wallet_response owr = AUTO_VAL_INIT(owr);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring path_w = conv.from_bytes(owd.path);

    ar.error_code = m_backend.open_wallet(path_w, owd.pass, owr);
    dispatch(ar, owr);
  });
}

QString Html5ApplicationViewer::is_pos_allowed()
{
  return m_backend.is_pos_allowed().c_str();
}

QString Html5ApplicationViewer::run_wallet(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, wio);

  ar.error_code = m_backend.run_wallet(wio.wallet_id);
  return epee::serialization::store_t_to_json(ar).c_str();
}

QString Html5ApplicationViewer::resync_wallet(const QString& param)
{
  return que_call2<view::wallet_id_obj>("get_wallet_info", param, [this](const view::wallet_id_obj& a, view::api_response& ar){

    view::wallet_info wi = AUTO_VAL_INIT(wi);
    ar.error_code = m_backend.resync_wallet(a.wallet_id);
    dispatch(ar, wi);
  });
}

/*QString Html5ApplicationViewer::get_recent_transfers(const QString& param)
{
  return que_call2<view::wallet_id_obj>("get_recent_transfers", param, [this](const view::wallet_id_obj& a, view::api_response& ar){

    view::transfers_array ta = AUTO_VAL_INIT(ta);
    ar.error_code = m_backend.get_recent_transfers(a.wallet_id, ta);
    dispatch(ar, ta);
  });
}
*/
QString Html5ApplicationViewer::get_all_offers(const QString& param)
{
  return que_call2<view::api_void>("get_all_offers", param, [this](const view::api_void& a, view::api_response& ar){

    view::transfers_array ta = AUTO_VAL_INIT(ta);
    currency::COMMAND_RPC_GET_ALL_OFFERS::response rp = AUTO_VAL_INIT(rp);
    ar.error_code = m_backend.get_all_offers(rp);
    dispatch(ar, rp);
  });
}

QString Html5ApplicationViewer::push_offer(const QString& param)
{
  return que_call2<view::push_offer_param>("push_offer", param, [this](const view::push_offer_param& a, view::api_response& ar){

    view::transfer_response tr = AUTO_VAL_INIT(tr);
    currency::transaction res_tx = AUTO_VAL_INIT(res_tx);

    ar.error_code = m_backend.push_offer(a.wallet_id, a.od, res_tx);
    if (ar.error_code != API_RETURN_CODE_OK)
    {
      view::api_void av;
      dispatch(ar, av);
      return;
    }
    tr.success = true;
    tr.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    tr.tx_blob_size = currency::get_object_blobsize(res_tx);
    ar.error_code = API_RETURN_CODE_OK;
    dispatch(ar, tr);
  });
}

QString Html5ApplicationViewer::cancel_offer(const QString& param)
{
  return que_call2<view::cancel_offer_param>("cancel_offer", param, [this](const view::cancel_offer_param& a, view::api_response& ar){

    view::transfer_response tr = AUTO_VAL_INIT(tr);
    currency::transaction res_tx = AUTO_VAL_INIT(res_tx);

    ar.error_code = m_backend.cancel_offer(a, res_tx);
    if (ar.error_code != API_RETURN_CODE_OK)
    {
      view::api_void av;
      dispatch(ar, av);
      return;
    }
    tr.success = true;
    tr.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    tr.tx_blob_size = currency::get_object_blobsize(res_tx);
    ar.error_code = API_RETURN_CODE_OK;
    dispatch(ar, tr);
  });
}

QString Html5ApplicationViewer::push_update_offer(const QString& param)
{
  return que_call2<view::update_offer_param>("cancel_offer", param, [this](const view::update_offer_param& a, view::api_response& ar){

    view::transfer_response tr = AUTO_VAL_INIT(tr);
    currency::transaction res_tx = AUTO_VAL_INIT(res_tx);

    ar.error_code = m_backend.push_update_offer(a, res_tx);
    if (ar.error_code != API_RETURN_CODE_OK)
    {
      view::api_void av;
      dispatch(ar, av);
      return;
    }
    tr.success = true;
    tr.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    tr.tx_blob_size = currency::get_object_blobsize(res_tx);
    ar.error_code = API_RETURN_CODE_OK;
    dispatch(ar, tr);
  });
}

QString Html5ApplicationViewer::get_mining_history(const QString& param)
{

  return que_call2<view::wallet_id_obj>("get_mining_history", param, [this](const view::wallet_id_obj& a, view::api_response& ar)
  {
    tools::wallet_rpc::mining_history mh = AUTO_VAL_INIT(mh);

    ar.error_code = m_backend.get_mining_history(a.wallet_id, mh);
    if (ar.error_code != API_RETURN_CODE_OK)
    {
      view::api_void av;
      dispatch(ar, av);
      return;
    }
    ar.error_code = API_RETURN_CODE_OK;
    dispatch(ar, mh);
  });
}
QString Html5ApplicationViewer::start_pos_mining(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, wo);
  ar.error_code = m_backend.start_pos_mining(wo.wallet_id);
  return epee::serialization::store_t_to_json(ar).c_str();
}
QString Html5ApplicationViewer::stop_pos_mining(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, wo);
  ar.error_code = m_backend.stop_pos_mining(wo.wallet_id);
  return epee::serialization::store_t_to_json(ar).c_str();
}

QString Html5ApplicationViewer::get_smart_safe_info(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, wo);
  view::get_restore_info_response res;
  res.error_code = m_backend.get_wallet_restore_info(wo.wallet_id, res.restore_key);
  return epee::serialization::store_t_to_json(res).c_str();
}
QString Html5ApplicationViewer::get_mining_estimate(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::request_mining_estimate, me);
  view::response_mining_estimate res;
  res.error_code = m_backend.get_mining_estimate(me.amount_coins, me.time, res.final_amount, res.all_coins_and_pos_diff_rate, res.days_estimate);
  return epee::serialization::store_t_to_json(res).c_str();
}
QString Html5ApplicationViewer::backup_wallet_keys(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::backup_keys_request, me);
  ar.error_code = m_backend.backup_wallet(me.wallet_id, me.path);
  return epee::serialization::store_t_to_json(ar).c_str();
}
QString Html5ApplicationViewer::reset_wallet_password(const QString& param)
{
  PREPARE_ARG_FROM_JSON(view::reset_pass_request, me);
  ar.error_code = m_backend.reset_wallet_password(me.wallet_id, me.pass);
  return epee::serialization::store_t_to_json(ar).c_str();
}

void Html5ApplicationViewer::dispatch(const QString& status, const QString& param)
{
  m_d->do_dispatch(status, param);
}


#include "html5applicationviewer.moc"
