/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Data Differential YATL (i.e. libtest)  library
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "libtest/yatlcon.h"
#include <libtest/common.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

#ifdef HAVE_POLL_H
# include <poll.h>
#endif

#ifndef HAVE_MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

#if defined(HAVE_CYASSL) && HAVE_CYASSL
# include <cyassl/ssl.h>
#endif

#define CA_CERT_PEM "/home/brian/cyassl/certs/ca-cert.pem"
#define CERT_PEM "/home/brian/cyassl/certs/server-cert.pem"
#define CERT_KEY_PEM "/home/brian/cyassl/certs/server-key.pem"

namespace libtest {

SimpleClient::SimpleClient(const std::string& hostname_, in_port_t port_) :
  _is_connected(false),
  _is_ssl(false),
  _hostname(hostname_),
  _port(port_),
  sock_fd(INVALID_SOCKET),
  _error_file(NULL),
  _error_line(0),
  requested_message(1),
  _ctx_ssl(NULL),
  _ssl(NULL)
{
  init_ssl();
}

void SimpleClient::init_ssl()
{
  if (_is_ssl)
  {
#if defined(HAVE_CYASSL) && HAVE_CYASSL
    CyaSSL_Init();

    if ((_ctx_ssl= CyaSSL_CTX_new(CyaTLSv1_client_method())) == NULL)
    {
      FATAL("CyaSSL_CTX_new error" == NULL);
    }

    if (CyaSSL_CTX_load_verify_locations(_ctx_ssl, CA_CERT_PEM, 0) != SSL_SUCCESS)
    {
      FATAL("CyaSSL_CTX_load_verify_locations(%s) cannot obtain certificate", CA_CERT_PEM);
    }

    if (CyaSSL_CTX_use_certificate_file(_ctx_ssl, CERT_PEM, SSL_FILETYPE_PEM) != SSL_SUCCESS)
    {   
      FATAL("CyaSSL_CTX_use_certificate_file(%s) cannot obtain certificate", CERT_PEM);
    }

    if (CyaSSL_CTX_use_PrivateKey_file(_ctx_ssl, CERT_KEY_PEM, SSL_FILETYPE_PEM) != SSL_SUCCESS)
    {   
      FATAL("CyaSSL_CTX_use_PrivateKey_file(%s) cannot obtain certificate", CERT_KEY_PEM);
    }
#endif // defined(HAVE_CYASSL) && HAVE_CYASSL
  }
}

void SimpleClient::error(const char* file_, int line_, const std::string& error_)
{
  _error.clear();
  _error_file= file_;
  _error_line= line_;
  vchar_t buffer;
  buffer.resize(1024);
  snprintf(&buffer[0], buffer.size() -1, "%s:%d: %s", file_, line_, error_.c_str());
  _error.append(&buffer[0]);
}

bool SimpleClient::ready(int event_)
{
  struct pollfd fds[1];
  fds[0].fd= sock_fd;
  fds[0].events= event_;
  fds[0].revents= 0;

  int timeout= 5000;
  if (_is_connected == false)
  {
    timeout= timeout * 30;
  }

  int ready_fds= poll(fds, 1, timeout);

  if (ready_fds == -1)
  {
    error(__FILE__, __LINE__, strerror(errno));
    return false;
  }
  else if (ready_fds == 1)
  {
    if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
    {
      int err;
      socklen_t len= sizeof (err);
      // We replace errno with err if getsockopt() passes, but err has been
      // set.
      if (getsockopt(fds[0].fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0)
      {
        // We check the value to see what happened wth the socket.
        if (err == 0)
        {
          error(__FILE__, __LINE__, "getsockopt() returned no error but poll() indicated one existed");
          return false;
        }
        errno= err;
      }
      error(__FILE__, __LINE__, strerror(errno));

      return false;
    }

    _is_connected= true;
    if (fds[0].revents & event_)
    {
      return true;
    }
  }

  fatal_assert(ready_fds == 0);
  error(__FILE__, __LINE__, "TIMEOUT");

  return false;
}

struct addrinfo* SimpleClient::lookup()
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype= SOCK_STREAM;
  hints.ai_protocol= IPPROTO_TCP;

  libtest::vchar_t service;
  service.resize(NI_MAXSERV);
  (void)snprintf(&service[0], service.size(), "%d", _port);

  int getaddrinfo_error;
  if ((getaddrinfo_error= getaddrinfo(_hostname.c_str(), &service[0], &hints, &_ai)) != 0)
  {
    if (getaddrinfo_error != EAI_SYSTEM)
    {
      error(__FILE__, __LINE__, gai_strerror(getaddrinfo_error));
      return NULL;
    }
    else
    {
      error(__FILE__, __LINE__, strerror(getaddrinfo_error));
      return NULL;
    }
  }

  return _ai;
}

SimpleClient::~SimpleClient()
{
  free_addrinfo();
  close_socket();
#if defined(HAVE_CYASSL) && HAVE_CYASSL
  CyaSSL_CTX_free(_ctx_ssl);
  _ctx_ssl= NULL;
#endif
}

void SimpleClient::close_socket()
{
  if (sock_fd != INVALID_SOCKET)
  {
#if defined(HAVE_CYASSL) && HAVE_CYASSL
    CyaSSL_shutdown(_ssl); 
    CyaSSL_free(_ssl); 
    _ssl= NULL;
#endif
    close(sock_fd);
    sock_fd= INVALID_SOCKET;
  }
}

bool SimpleClient::instance_connect()
{
  _is_connected= false;
  if (lookup())
  {
    {
      struct addrinfo* address_info_next= _ai;

      while (address_info_next and sock_fd == INVALID_SOCKET)
      {
        if ((sock_fd= socket(address_info_next->ai_family, address_info_next->ai_socktype, address_info_next->ai_protocol)) != SOCKET_ERROR)
        {
          if (connect(sock_fd, address_info_next->ai_addr, address_info_next->ai_addrlen) == SOCKET_ERROR)
          {
            switch (errno)
            {
            case EINTR:
              close_socket();
              continue;

            case EINPROGRESS: // nonblocking mode - first return
            case EALREADY: // nonblocking mode - subsequent returns
              continue; // Jump to while() and continue on


            case ECONNREFUSED:
            default:
              break;
            }

            close_socket();
            error(__FILE__, __LINE__, strerror(errno));
          }
          else
          {
            return true;
          }
        }
        else
        {
          FATAL(strerror(errno));
        }
        address_info_next= address_info_next->ai_next;
      }

      free_addrinfo();
    }

    if (sock_fd == INVALID_SOCKET)
    {
      fatal_assert(is_error());
    }

    return bool(sock_fd != INVALID_SOCKET);
  }

  return false;
}

bool SimpleClient::is_valid()
{
  _error.clear();
  if (sock_fd == INVALID_SOCKET)
  {
    if (instance_connect())
    {
#if defined(HAVE_CYASSL) && HAVE_CYASSL
      if (_ctx_ssl)
      {
        _ssl= CyaSSL_new(_ctx_ssl);
        if (_ssl == NULL)
        {
          error(__FILE__, __LINE__, "CyaSSL_new failed");
          return false;
        }

        int ssl_error;
        if ((ssl_error= CyaSSL_set_fd(_ssl, sock_fd)) != SSL_SUCCESS)
        {
          error(__FILE__, __LINE__, "CyaSSL_set_fd() should not be returning an error.");
          return false;
        }
      }
#endif

      return true;
    }

    return false;
  }

  return true;
}

bool SimpleClient::message(const char* ptr, const size_t len)
{
  if (is_valid())
  {
    if (ready(POLLOUT))
    {
      off_t offset= 0;
      do
      {
        ssize_t nw;
#if defined(HAVE_CYASSL) && HAVE_CYASSL
        if (_ssl)
        {
          nw= CyaSSL_write(_ssl, (const void*)(ptr +offset), int(len -offset));
          if (nw < 0)
          {
            int sendErr= CyaSSL_get_error(_ssl, 0);
            if (sendErr != SSL_ERROR_WANT_WRITE)
            {
              char errorString[80];
              int err = CyaSSL_get_error(_ssl, 0);
              CyaSSL_ERR_error_string(err, errorString);
              error(__FILE__, __LINE__, errorString);
            }
            else
            {
              error(__FILE__, __LINE__, "SSL_ERROR_WANT_WRITE");
            }

            return false;
          }
        }
        else
#endif
        {
          nw= send(sock_fd, ptr + offset, len - offset, MSG_NOSIGNAL);
        }

        if (nw == -1)
        {
          if (errno != EINTR)
          {
            error(__FILE__, __LINE__, strerror(errno));
            return false;
          }
        }
        else
        {
          offset += nw;
        }
      } while (offset < ssize_t(len));

      return true;
    }
  }

  fatal_assert(is_error());

  return false;
}

bool SimpleClient::send_message(const std::string& arg)
{
  if (message(arg.c_str(), arg.size()) == true)
  {
    return message("\r\n", 2);
  }

  return false;
}

bool SimpleClient::send_data(const libtest::vchar_t& message_, libtest::vchar_t& response_)
{
  requested_message++;
  if (message(&message_[0], message_.size()))
  {
    return response(response_);
  }

  return false;
}

bool SimpleClient::send_message(const std::string& message_, std::string& response_)
{
  requested_message++;
  if (send_message(message_))
  {
    return response(response_);
  }

  return false;
}

bool SimpleClient::response(libtest::vchar_t& response_)
{
  response_.clear();

  if (is_valid())
  {
    if (ready(POLLIN))
    {
      bool more= true;
      char buffer[2];
      buffer[1]= 0;
      do
      {
        ssize_t nr;
#if defined(HAVE_CYASSL) && HAVE_CYASSL
        if (_ssl)
        {
          nr= CyaSSL_read(_ssl, buffer, 1);
          if (_ssl and nr < 0)
          {
            int readErr = CyaSSL_get_error(_ssl, 0);
            if (readErr != SSL_ERROR_WANT_READ)
            {
              error(__FILE__, __LINE__, "CyaSSL_read failed");
            }
            else
            {
              error(__FILE__, __LINE__, "CyaSSL_read (unknown) failed");
            }

            return false;
          }
        }
        else
#endif
        {
          nr= recv(sock_fd, buffer, 1, MSG_NOSIGNAL);
        }

        if (nr == -1)
        {
          if (errno != EINTR)
          {
            error(__FILE__, __LINE__, strerror(errno));
            return false;
          }
        }
        else if (nr == 0)
        {
          close_socket();
          more= false;
        }
        else
        {
          response_.reserve(response_.size() + nr +1);
          fatal_assert(nr == 1);
          if (buffer[0] == '\n')
          {
            more= false;
          }
          response_.insert(response_.end(), buffer, buffer +nr);
        }
      } while (more);

      return response_.size();
    }
  }

  fatal_assert(is_error());
  return false;
}

bool SimpleClient::response(std::string& response_)
{
  response_.clear();

  if (is_valid())
  {
    if (ready(POLLIN))
    {
      bool more= true;
      char buffer[2];
      buffer[1]= 0;
      do
      {
        ssize_t nr;
#if defined(HAVE_CYASSL) && HAVE_CYASSL
        if (_ssl)
        {
          nr= CyaSSL_read(_ssl, buffer, 1);
          if (_ssl and nr < 0)
          {
            int readErr = CyaSSL_get_error(_ssl, 0);
            if (readErr != SSL_ERROR_WANT_READ)
            {
              error(__FILE__, __LINE__, "CyaSSL_read failed");
            }
            else
            {
              error(__FILE__, __LINE__, "CyaSSL_read (unknown) failed");
            }

            return false;
          }
        }
        else
#endif
        nr= recv(sock_fd, buffer, 1, MSG_NOSIGNAL);
        if (nr == -1)
        {
          if (errno != EINTR)
          {
            error(__FILE__, __LINE__, strerror(errno));
            return false;
          }
        }
        else if (nr == 0)
        {
          close_socket();
          more= false;
        }
        else
        {
          fatal_assert(nr == 1);
          if (buffer[0] == '\n')
          {
            more= false;
          }
          response_.append(buffer);
        }
      } while (more);

      return response_.size();
    }
  }

  fatal_assert(is_error());
  return false;
}

} // namespace libtest
