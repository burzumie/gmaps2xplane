#include "network.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QList>
#include <QByteArray>
#include <QHttpMultiPart>

//#include <iostream>

//
void Network::Task::finished()
{
  QObject::disconnect();
  reply->deleteLater();
  this->deleteLater();
/*
  // extract supplementary data
  QVariant v = reply->request().attribute(QNetworkRequest::UserMax);
  bool b = false;
  int n = v.toInt(&b);
*/

  QNetworkReply::NetworkError error = reply->error();
  QString orig_URL = reply->url().url();

  // handle errors
  if( error /* || !v.isValid() || !b || n != idx */ )
  {
    QByteArray data = reply->readAll();
/*
std::cout << "!!! Error #" << error << " in task " << idx << ": '"<< reply->errorString().toLocal8Bit().data() << "', URL is: '"
          << orig_URL.toLocal8Bit().data() << "', server response is:"
          << std::endl << data.data() << std::endl;
*/
//    handler->error(idx, error, data);
    handler->error(idx, error, reply->errorString().toLocal8Bit());
    core->task_finished(this);
    return;
  }

  // handle HTTP redirection
  QVariant redirection_target = reply->attribute( QNetworkRequest::RedirectionTargetAttribute );
  if( !reply->error() && !redirection_target.isNull() )
  {
    QUrl new_url = reply->url().resolved( redirection_target.toUrl() );

//std::cout << "HTTP redirection: " << orig_URL.toLocal8Bit().data() << " -> " << new_url.url().toLocal8Bit().data() << std::endl;

    core->task_finished(idx);
//    Task* new_task = core->insert_handler(this->handler);
    Task* new_task = core->insert_task(this, orig_URL);
//    this->handler->move_handler(idx, new_task->get_idx());

    QList<QByteArray> old_headers = reply->request().rawHeaderList();

    if( old_headers.isEmpty() )
    {
      core->download(new_url.url(), new_task, NULL);
    }
    else
    {
      // copy additional headers from original request, if any
      QList<QByteArray>::const_iterator iter = old_headers.constBegin();

      Network::Core::Headers new_headers;

      while( iter != old_headers.constEnd() )
      {
        new_headers[*iter] = reply->request().rawHeader(*iter);
        ++iter;
      }

      core->download(new_url.url(), new_task, &new_headers);
    }

    return;
  }

  // handle downloaded data
  QByteArray data = reply->readAll();

//std::cout << "task no." << idx << ": " << data.size() << " bytes" << std::endl;

  handler->finished(idx, data);

  // it must be last, after data handling
  core->task_finished(idx);
}

void Network::Task::download_progress(qint64 bytes_received, qint64 bytes_total)
{
  handler->download_progress(idx, bytes_received, bytes_total);
}

void Network::Task::upload_progress(qint64 bytes_sent, qint64 bytes_total)
{
/*
  if( bytes_total > 0 )
  {
    std::cout << "upload progress: " << int( 100 * bytes_sent/bytes_total ) << "% (" << bytes_sent << " bytes of " << bytes_total << " total)" << std::endl;
  }
  else
  {
    std::cout << "upload progress: " << bytes_sent << " bytes" << std::endl;
  }
*/
  handler->upload_progress(idx, bytes_sent, bytes_total);
}

void Network::Task::abort()
{
  reply->abort();
}

//
Network::Core::Core()
  : last_pos(0)
{
  tasks = new Task* [tasks_size];
  for(size_t i = 0; i < tasks_size; ++i)
  {
    tasks[i] = NULL;
  }

  net_manager = new QNetworkAccessManager;

  // TODO: temporary, try to avoid HTTP-auth
  QObject::connect(net_manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            this, SLOT(slotAuthenticationRequired(QNetworkReply*,QAuthenticator*)));

  // TODO: temporary, try to ignore wrong certificates
  QObject::connect(net_manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(handle_SSL_errors(QNetworkReply*, const QList<QSslError>&)));
}

Network::Core::~Core()
{
  delete net_manager;

  for(size_t i = 0; i < tasks_size; ++i)
  {
    if( tasks[i] )
    {
      delete tasks[i];
    }
  }
  delete[] tasks;
}

// TODO: temporary
#include <QAuthenticator>
void Network::Core::slotAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator)
{
  authenticator->setUser("user");
  authenticator->setPassword("password");
}

void Network::Core::handle_SSL_errors(QNetworkReply* reply, const QList<QSslError>& errors)
{
  // TODO: see documentation for 'void QNetworkReply::ignoreSslErrors(const QList<QSslError> & errors)'
  reply->ignoreSslErrors(errors);
}

//
int Network::Core::download(const QString& url, Network::Abstract_Handler* h, Network::Core::Headers* additional_headers)
{
  Task* task = this->insert_handler(h);

  if( !task )
  {
    return -1;
  }

  this->download(url, task, additional_headers);

  return task->get_idx();
}

void Network::Core::download(const QString& url, Task* task, Network::Core::Headers* additional_headers)
{
  QUrl qurl(url);
  QNetworkRequest request( qurl );

//    request.setAttribute(QNetworkRequest::UserMax, QVariant( task->get_idx() ));

  if( additional_headers )
  {
    Network::Core::Headers::const_iterator iter = additional_headers->constBegin();
    while( iter != additional_headers->constEnd() )
    {
      request.setRawHeader(iter.key(), iter.value());

//std::cout << "H> " << iter.key().data() << ": " << iter.value().data() << std::endl;

      ++iter;
    }
  }

//std::cout << "GET " << url.toLocal8Bit().data() << std::endl;

  QNetworkReply* reply = net_manager->get(request);

  task->bind(this, reply);

  QObject::connect(reply, SIGNAL(finished()), task, SLOT(finished()));
  QObject::connect(reply, SIGNAL(uploadProgress(qint64,qint64)), task, SLOT(upload_progress(qint64,qint64)));
  QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), task, SLOT(download_progress(qint64,qint64)));
}

//
int Network::Core::upload(const QString& url, const QByteArray& data, Network::Abstract_Handler* h, Network::Core::Headers* additional_headers)
{
  Task* task = this->insert_handler(h);

  if( !task )
  {
    return -1;
  }

  this->upload(url, data, task, additional_headers);

  return task->get_idx();
}

int Network::Core::upload(const QString& url, QHttpMultiPart* data, Network::Abstract_Handler* h, Network::Core::Headers* additional_headers)
{
  Task* task = this->insert_handler(h);

  if( !task )
  {
    return -1;
  }

  this->upload(url, data, task, additional_headers);

  return task->get_idx();
}

void Network::Core::upload(const QString& url, const QByteArray& data, Task* task, Network::Core::Headers* additional_headers)
{
  QUrl qurl(url);
  QNetworkRequest request( qurl );

//    request.setAttribute(QNetworkRequest::UserMax, QVariant( task->get_idx() ));

  if( additional_headers )
  {
    Network::Core::Headers::const_iterator iter = additional_headers->constBegin();
    while( iter != additional_headers->constEnd() )
    {
      request.setRawHeader(iter.key(), iter.value());

//std::cout << "H> " << iter.key().data() << ": " << iter.value().data() << std::endl;

      ++iter;
    }
  }

//std::cout << "POST " << url.toLocal8Bit().data() << std::endl;

/*
std::cout << "uploading to " << url.toLocal8Bit().data() << ":\n" << std::endl << std::endl;

QList<QByteArray> hh = request.rawHeaderList();
for(int i = 0; i < hh.size(); ++i)
{
  std::cout << hh[i].data() << ": " << request.rawHeader(hh[i]).data() << std::endl;
}
std::cout << std::endl << data.data() << std::endl;
*/
  QNetworkReply* reply = net_manager->post(request, data);

  task->bind(this, reply);

  QObject::connect(reply, SIGNAL(finished()), task, SLOT(finished()));
  QObject::connect(reply, SIGNAL(uploadProgress(qint64,qint64)), task, SLOT(upload_progress(qint64,qint64)));
  QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), task, SLOT(download_progress(qint64,qint64)));
}

void Network::Core::upload(const QString& url, QHttpMultiPart* data, Task* task, Network::Core::Headers* additional_headers)
{
  QUrl qurl(url);
  QNetworkRequest request( qurl );

//    request.setAttribute(QNetworkRequest::UserMax, QVariant( task->get_idx() ));

  if( additional_headers )
  {
    Network::Core::Headers::const_iterator iter = additional_headers->constBegin();
    while( iter != additional_headers->constEnd() )
    {
      request.setRawHeader(iter.key(), iter.value());

//std::cout << "H> " << iter.key().data() << ": " << iter.value().data() << std::endl;

      ++iter;
    }
  }

std::cout << "POST " << url.toLocal8Bit().data() << std::endl;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + data->boundary());

  QNetworkReply* reply = net_manager->post(request, data);

  data->setParent(reply);

  task->bind(this, reply);

  QObject::connect(reply, SIGNAL(finished()), task, SLOT(finished()));
  QObject::connect(reply, SIGNAL(uploadProgress(qint64,qint64)), task, SLOT(upload_progress(qint64,qint64)));
  QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), task, SLOT(download_progress(qint64,qint64)));
}

//
int Network::Core::http_delete(const QString& url, Network::Abstract_Handler* h, Network::Core::Headers* additional_headers)
{
  Task* task = this->insert_handler(h);

  if( !task )
  {
    return -1;
  }

//std::cout << "DELETE " << url.toLocal8Bit().data() << std::endl;

  this->http_delete(url, task, additional_headers);

  return task->get_idx();
}

void Network::Core::http_delete(const QString& url, Task* task, Network::Core::Headers* additional_headers)
{
  QUrl qurl(url);
  QNetworkRequest request( qurl );

//    request.setAttribute(QNetworkRequest::UserMax, QVariant( task->get_idx() ));

  if( additional_headers )
  {
    Network::Core::Headers::const_iterator iter = additional_headers->constBegin();
    while( iter != additional_headers->constEnd() )
    {
      request.setRawHeader(iter.key(), iter.value());

//std::cout << "H> " << iter.key().data() << ": " << iter.value().data() << std::endl;

      ++iter;
    }
  }

//std::cout << "downloading " << url.toLocal8Bit().data() << std::endl;

  QNetworkReply* reply = net_manager->deleteResource(request);

  task->bind(this, reply);

  QObject::connect(reply, SIGNAL(finished()), task, SLOT(finished()));
  QObject::connect(reply, SIGNAL(uploadProgress(qint64,qint64)), task, SLOT(upload_progress(qint64,qint64)));
  QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), task, SLOT(download_progress(qint64,qint64)));
}

//
QString Network::Core::get_task_url(int n)
{
  static const QString empty_string;

  if( n >= 0 && n < tasks_size && tasks[n] )
  {
    Task* task = tasks[n];
    QString url = task->reply->request().url().url();

    // use original url for case of redirection
    if( !task->original_URL.isEmpty() )
    {
      return task->original_URL;
    }

    return url;
  }

  return empty_string;
}

void Network::Core::abort_task(int n)
{
  if( n >= 0 && n < tasks_size && tasks[n] )
  {
    tasks[n]->abort();
  }
}
