// -*-c++-*-

/*!
  \file debug_server.h
  \brief Debug Server class Header File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef SOCCERWINDOW2_QT_DEBUG_SERVER_H
#define SOCCERWINDOW2_QT_DEBUG_SERVER_H

#include <QObject>

class QUdpSocket;
class ViewHolder;

struct DebugServerImpl;

class DebugServer
    : public QObject {

    Q_OBJECT

private:
    QUdpSocket * M_socket;


    //! reference to the view data holder
    ViewHolder & M_view_holder;

    //! not used
    DebugServer();
    DebugServer( const DebugServer & );
    DebugServer & operator=( const DebugServer & );
public:

    /*!
      \brief construct with the specified port number and data holder reference
      \param view_holder data holder reference
      \param port my port number to receive data
     */
    DebugServer( QObject * parent,
                 ViewHolder & view_holder,
                 const int port );

    ~DebugServer();

    //! check if socket is opened.
    bool isConnected() const;

private slots:

    void handleReceive();

};

#endif
