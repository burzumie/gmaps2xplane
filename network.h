#pragma once

#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QSslError>

class QByteArray;
class QNetworkReply;
class QNetworkAccessManager;
class QString;
class QHttpMultiPart;

// TODO: temporary
class QAuthenticator;

#include <iostream>

//
namespace Network
{
  //
  struct Abstract_Handler
  {
    virtual ~Abstract_Handler()
    {
    }

    virtual void download_progress(int, qint64, qint64) = 0;
    virtual void upload_progress(int, qint64, qint64) = 0;
    virtual void finished(int, const QByteArray&) = 0;
    virtual void error(int n, int error_code, const QByteArray& response) = 0;

    virtual void move_handler(unsigned short int, unsigned short int) {}
  };
/*
  //
  template <class T> struct Handler_ByObject : public Abstract_Handler
  {
    typedef void (T::*HANDLER_Progress)(int, qint64, qint64);
    typedef void (T::*HANDLER_Finished)(int, const QByteArray&);

    //
    T* t;
    HANDLER_Progress on_download_progress;
    HANDLER_Progress on_upload_progress;
    HANDLER_Finished on_finished;

    Handler_ByObject(T* _t)
      : t(_t), on_download_progress(NULL), on_upload_progress(NULL), on_finished(NULL)
    {
    }

    ~Handler_ByObject()
    {
    }

    //
    virtual void download_progress(int, qint64 bytes_received, qint64 bytes_total)
    {
      if( on_download_progress )
      {
        (t->*on_download_progress)(bytes_received, bytes_total);
      }
    }

    virtual void upload_progress(int, qint64 bytes_sent, qint64 bytes_total)
    {
      if( on_upload_progress )
      {
        (t->*on_upload_progress)(bytes_sent, bytes_total);
      }
    }

    virtual void finished(int, const QByteArray& data)
    {
      if( on_finished )
      {
        (t->*on_finished)(data);
      }
    }
  };
*/
  //
  template <class T> struct Handler_ByMembers : public Abstract_Handler
  {
    typedef void (T::*HANDLER_Progress)(int, qint64, qint64);
    typedef void (T::*HANDLER_Finished)(int, const QByteArray&);
    typedef void (T::*HANDLER_Error)(int, int, const QByteArray&);

    //
    struct Handlers
    {
      HANDLER_Progress on_download_progress;
      HANDLER_Progress on_upload_progress;
      HANDLER_Finished on_finished;
      HANDLER_Error on_error;

      Handlers()
        : on_download_progress(NULL), on_upload_progress(NULL), on_finished(NULL), on_error(NULL)
      {
      }

      void reset()
      {
        on_download_progress = NULL;
        on_upload_progress = NULL;
        on_finished = NULL;
        on_error = NULL;
      }
    };

    T* t;
    static const size_t handlers_size = USHRT_MAX;
    struct Handlers handlers[handlers_size];


    //
    Handler_ByMembers(T* _t)
      : t(_t)
    {
    }

    ~Handler_ByMembers()
    {
    }

    virtual void move_handler(unsigned short int f, unsigned short int t)
    {
      handlers[t] = handlers[f];
      handlers[f].reset();
    }

    //
    void insert_finished(unsigned short int n, HANDLER_Finished _hf)
    {
      Handlers& h = handlers[n];
      h.on_finished = _hf;
    }

    void insert_download_progress(unsigned short int n, HANDLER_Progress _hp)
    {
      Handlers& h = handlers[n];
      h.on_download_progress = _hp;
    }

    void insert_upload_progress(unsigned short int n, HANDLER_Progress _hp)
    {
      Handlers& h = handlers[n];
      h.on_upload_progress = _hp;
    }

    void insert_error(unsigned short int n, HANDLER_Error _he)
    {
      Handlers& h = handlers[n];
      h.on_error = _he;
    }

    //
    virtual void download_progress(int n, qint64 bytes_received, qint64 bytes_total)
    {
      if( n < 0 || n > handlers_size )
      {
        return;
      }

      HANDLER_Progress h = handlers[n].on_download_progress;
      if( h )
      {
        (t->*h)(n, bytes_received, bytes_total);
      }
    }

    virtual void upload_progress(int n, qint64 bytes_sent, qint64 bytes_total)
    {
      if( n < 0 || n > handlers_size )
      {
        return;
      }

      HANDLER_Progress h = handlers[n].on_upload_progress;
      if( h )
      {
        (t->*h)(n, bytes_sent, bytes_total);
      }
    }

    virtual void finished(int n, const QByteArray& data)
    {
      if( n < 0 || n > handlers_size )
      {
        return;
      }

      HANDLER_Finished h = handlers[n].on_finished;
      if( h )
      {
        (t->*h)(n, data);
      }
    }

    virtual void error(int n, int error_code, const QByteArray& response)
    {
      if( n < 0 || n > handlers_size )
      {
        return;
      }

      HANDLER_Error h = handlers[n].on_error;
      if( h )
      {
        (t->*h)(n, error_code, response);
      }
    }
  };


class Core;

//
class Task : public QObject
{
  Q_OBJECT

  friend class Core;

private:
  Abstract_Handler* handler;
  unsigned short int idx;

  Core* core;
  QNetworkReply* reply;
  QString original_URL;

  //
  Task();
  Task(const Task& t);

  Task(const Task* t, const QString& _orig_url)
    : handler(t->handler), idx(t->idx),  core(NULL), reply(NULL), original_URL(_orig_url)
  {
  }

public:
  Task(unsigned short int i, Abstract_Handler* h)
    : idx(i), handler(h), core(NULL), reply(NULL)
  {
  } 

  ~Task()
  {
  }

  unsigned short int get_idx() const { return idx; }

  void bind(Core* c, QNetworkReply* r)
  {
    core = c;
    reply = r;
  }

  void abort();

public slots:
  void finished();

  void download_progress(qint64, qint64);
  void upload_progress(qint64, qint64);
};


//
class Core : public QObject
{
  Q_OBJECT

  friend class Task;

public:
  typedef QMap<QByteArray, QByteArray> Headers;

  static const size_t tasks_size = USHRT_MAX;

private:
  QNetworkAccessManager* net_manager;
  Task** tasks;
  unsigned short int last_pos;

  //
  void download(const QString& url, Task* task, Headers* additional_headers = NULL);
  void upload(const QString& url, const QByteArray& data, Task* task, Headers* additional_headers = NULL);
  void upload(const QString& url, QHttpMultiPart* data, Task* task, Headers* additional_headers = NULL);
  void http_delete(const QString& url, Task* task, Headers* additional_headers = NULL);

  // returns index of assigned task (0..65535), or -1 on error
  Task* insert_handler(Network::Abstract_Handler* h)
  {
    unsigned short int n = this->last_pos;

    while( true )
    {
      if( !tasks[n] )
      {
        Task* t = new Task(n, h);

        tasks[n] = t;
        this->last_pos = n+1;

        return t;
      }

      ++n;

      // full circle was done and no free space was found
      if( n == this->last_pos )
      {
        return NULL;
      }
    }

    return NULL;
  }

  Task* insert_task(Task* t, const QString& _orig_url)
  {
    Task* new_task = new Task(t, _orig_url);
    tasks[ t->get_idx() ] = new_task;
    return new_task;
  }

  void task_finished(int n)
  {
    if( n >= 0 && n < tasks_size && tasks[n] )
    {
      tasks[n] = NULL;
    }
  }

  void task_finished(Task* task)
  {
    for(size_t i = 0; i < tasks_size; ++i)
    {
      if( tasks[i] == task )
      {
        tasks[i] = NULL;
        break;
      }
    }
  }

public:
  Core();
  ~Core();

  int download(const QString& url, Network::Abstract_Handler* h, Headers* additional_headers = NULL);
  int upload(const QString& url, const QByteArray& data, Network::Abstract_Handler* h, Headers* additional_headers = NULL);
  int upload(const QString& url, QHttpMultiPart* data, Network::Abstract_Handler* h, Headers* additional_headers = NULL);
  int http_delete(const QString& url, Network::Abstract_Handler* h, Headers* additional_headers = NULL);

  QString get_task_url(int n);
  void abort_task(int n);

  // TODO: temporary
public slots:
  void slotAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator);
  void handle_SSL_errors(QNetworkReply* reply, const QList<QSslError>& errors);
};


// --------------------------------------
template <class Some_Handler> class Manager
{
private:
  Core* core;
  Handler_ByMembers<Some_Handler> handler;

public:
  Manager(Core* c, Some_Handler* _hh)
    : core(c), handler(_hh)
  {
  }

  template <class Some_Member> int download(const QString& url, Some_Member m, Core::Headers* additional_headers = NULL)
  {
    int n = core->download(url, &handler, additional_headers);
    handler.insert_finished(n, m);
    handler.insert_error(n, Some_Handler::get_error_handler());
    return n;
  }

  template <class Finished, class Progress> int download(const QString& url, Finished f, Progress p, Core::Headers* additional_headers = NULL)
  {
    int n = core->download(url, &handler, additional_headers);
    handler.insert_finished(n, f);
    handler.insert_download_progress(n, p);
    handler.insert_error(n, Some_Handler::get_error_handler());
    return n;
  }

  template <class Some_Member> int upload(const QString& url, const QByteArray& data, Some_Member m, Core::Headers* additional_headers = NULL)
  {
    int n = core->upload(url, data, &handler, additional_headers);
    handler.insert_finished(n, m);
    handler.insert_error(n, Some_Handler::get_error_handler());
    return n;
  }

  template <class Finished, class Upload_Progress> int upload(const QString& url, QHttpMultiPart* data, Finished f, Upload_Progress u, Core::Headers* additional_headers = NULL)
  {
    int n = core->upload(url, data, &handler, additional_headers);
    handler.insert_upload_progress(n, u);
    handler.insert_finished(n, f);
    handler.insert_error(n, Some_Handler::get_error_handler());
    return n;
  }

  template <class Some_Member> int http_delete(const QString& url, Some_Member m, Core::Headers* additional_headers = NULL)
  {
    int n = core->http_delete(url, &handler, additional_headers);
    handler.insert_finished(n, m);
    handler.insert_error(n, Some_Handler::get_error_handler());
    return n;
  }
/*
  void abort_task(int n)
  {
    core->abort_task(n);
  }
*/
};
// --------------------------------------


} // end of namespace Network
