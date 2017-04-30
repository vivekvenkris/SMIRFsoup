/***************************************************************************
 *
 *   Copyright (C) 2012 by Ben Barsdell and Andrew Jameson
 *   Licensed under the Academic Free License version 2.1
 *
 ***************************************************************************/

#ifndef NETWORK_SOCKET_EXCEPTION_H_
#define NETWORK_SOCKET_EXCEPTION_H_

#include <string>

class SocketException
{
  public:
    SocketException ( std::string s ) : m_s ( s ) {};
    ~SocketException (){};

    std::string description() { return m_s; }

  private:
    std::string m_s;

};

#endif /*NETWORK_SOCKET_EXCEPTION_H_*/
