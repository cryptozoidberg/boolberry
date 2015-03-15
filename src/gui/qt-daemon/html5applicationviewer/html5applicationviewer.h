// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HTML5APPLICATIONVIEWER_H
#define HTML5APPLICATIONVIEWER_H

#include <QWidget>
#include <QUrl>
#include <QSystemTrayIcon>
#include <QMenu>
#include "view_iface.h"
#ifndef Q_MOC_RUN
#include "daemon_backend.h"
#include "gui_config.h"
#endif

class QGraphicsWebView;


class Html5ApplicationViewer : public QWidget, public view::i_view
{
  Q_OBJECT

public:
  enum ScreenOrientation {
    ScreenOrientationLockPortrait,
    ScreenOrientationLockLandscape,
    ScreenOrientationAuto
  };

  explicit Html5ApplicationViewer(QWidget *parent = 0);
  virtual ~Html5ApplicationViewer();

  // Note that this will only have an effect on Fremantle.
  void setOrientation(ScreenOrientation orientation);

  void showExpanded();

  QGraphicsWebView *webView() const;
  bool start_backend(int argc, char* argv[]);
protected:

  private slots :
  bool do_close();
  bool on_request_quit();
  public slots:

  QString show_openfile_dialog(const QString& param);
  QString show_savefile_dialog(const QString& param);

  QString open_wallet(const QString& param);
  QString generate_wallet(const QString& param);
  QString get_wallet_info(const QString& param);
  QString close_wallet(const QString& wallet_id);
  QString get_version();
  QString transfer(const QString& json_transfer_object);
  void message_box(const QString& msg);
  QString request_uri(const QString& url_str, const QString& params, const QString& callbackname);
  QString request_aliases();
  bool init_config();
  bool toggle_mining();
  void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
  QString get_exchange_last_top(const QString& params);

private:
  void loadFile(const QString &fileName);
  void loadUrl(const QUrl &url);
  void closeEvent(QCloseEvent *event);
  void changeEvent(QEvent *e);

  bool store_config();

  //------- i_view ---------
  virtual bool update_daemon_status(const view::daemon_status_info& info);
  virtual bool on_backend_stopped();
  virtual bool show_msg_box(const std::string& message);
  virtual bool update_wallet_status(const view::wallet_status_info& wsi);
  virtual bool update_wallets_info(const view::wallets_summary_info& wsi);
  virtual bool money_transfer(const view::transfer_event_info& tei);
  virtual bool show_wallet();
  virtual bool hide_wallet();
  virtual bool switch_view(int view_no);
  virtual bool set_recent_transfers(const view::transfers_array& ta);
  virtual bool set_html_path(const std::string& path);
  virtual bool pos_block_found(const currency::block& block_found);
  //----------------------------------------------
  bool is_uri_allowed(const QString& uri);

  void initTrayIcon(const std::string& htmlPath);
  void dispatcher();

  class Html5ApplicationViewerPrivate *m_d;
  daemon_backend m_backend;
  std::atomic<bool> m_quit_requested;
  std::atomic<bool> m_deinitialize_done;
  std::atomic<bool> m_backend_stopped;
  gui_config m_config;

  std::atomic<size_t> m_request_uri_threads_count;

  std::unique_ptr<QSystemTrayIcon> m_trayIcon;
  std::unique_ptr<QMenu> m_trayIconMenu;
  std::unique_ptr<QAction> m_restoreAction;
  std::unique_ptr<QAction> m_quitAction;
  

  struct dispatch_entry
  {
    std::shared_ptr<typename epee::misc_utils::call_basic> cb;
  };
  
  
  std::list<dispatch_entry> m_dispatch_que;
  std::atomic<size_t> m_request_id_counter;
  std::atomic<bool> m_is_stop;
  critical_section m_dispatch_que_lock;
  std::thread m_dispatcher;

  template<typename callback_t>
  QString que_call(size_t request_id, callback_t cb)
  {
    CRITICAL_REGION_LOCAL(m_dispatch_que_lock);
    if (m_dispatch_que.size() > GUI_DISPATCH_QUE_MAXSIZE)
    {
      view::api_response air;
      air.request_id = std::to_string(request_id);
      air.error_code = API_RETURN_CODE_INTERNAL_ERROR_QUE_FULL;
      return epee::serialization::store_t_to_json(air).c_str();
    }
    m_dispatch_que.push_back(dispatch_entry());
    dispatch_entry& de = m_dispatch_que.back();
    de.cb = epee::misc_utils::build_abstract_callback(cb);

    view::api_response air;
    air.request_id = std::to_string(request_id);
    air.error_code = API_RETURN_CODE_OK;
    return epee::serialization::store_t_to_json(air).c_str();
  }

  template<typename t_response>
  void dispatch(const view::api_response& status, const t_response& rsp)
  {
    QString status_obj_str = epee::serialization::store_t_to_json(status).c_str();
    QString rsp_obj_str = epee::serialization::store_t_to_json(rsp).c_str();
    m_d->dispatch(status_obj_str, rsp_obj_str);
  }


};

#endif // HTML5APPLICATIONVIEWER_H
