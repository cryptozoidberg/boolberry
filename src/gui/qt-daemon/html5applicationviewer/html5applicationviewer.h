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
  QString browse_wallet(bool existing);
  void open_wallet();
  QString generate_wallet();
  QString get_password();
  QString get_seed_text();
  //void restore_wallet(const QString& restore_text);
  bool restore_wallet(const QString& restore_text, const QString& password,
    const QString& path);
  void close_wallet();
  void add_address(const QString& name, const QString& address,
    const QString& alias, const QString& paymentId);
  void delete_address(const QString& name, const QString& address,
    const QString& alias, const QString& paymentId);
  QString get_addressbook();
  void place_to_clipboard(const QString& data);
  QString get_version();
  QString transfer(const QString& json_transfer_object);
  QString sign_text(const QString& text_to_sign);
  void message_box(const QString& msg);
  QString request_uri(const QString& url_str, const QString& params, const QString& callbackname);
  QString request_aliases();
  bool init_config();
  void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

  QString get_gui_lang();
  void set_gui_lang(const QString& str_config);
  QString parse_transfer_target(const QString& transfer_target_str);

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
  virtual bool update_wallet_info(const view::wallet_info& wsi);
  virtual bool money_transfer(const view::transfer_event_info& tei);
  virtual bool show_wallet();
  virtual bool hide_wallet();
  virtual bool switch_view(int view_no);
  virtual bool set_recent_transfers(const view::transfers_array& ta);
  virtual bool set_html_path(const std::string& path);
  //----------------------------------------------
  bool is_uri_allowed(const QString& uri);

  void initTrayIcon(const std::string& htmlPath);

  class Html5ApplicationViewerPrivate *m_d;
  daemon_backend m_backend;
  std::atomic<bool> m_quit_requested;
  std::atomic<bool> m_deinitialize_done;
  std::atomic<bool> m_backend_stopped;
  gui::gui_config m_config;

  std::atomic<size_t> m_request_uri_threads_count;

  std::unique_ptr<QSystemTrayIcon> m_trayIcon;
  std::unique_ptr<QMenu> m_trayIconMenu;
  std::unique_ptr<QAction> m_restoreAction;
  std::unique_ptr<QAction> m_quitAction;
};

#endif // HTML5APPLICATIONVIEWER_H