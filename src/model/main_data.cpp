// -*-c++-*-

/*!
  \file main_data.cpp
  \brief main data supplier class Source File.
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

 You should have receiveEd a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "main_data.h"

#include "options.h"
#include "monitor_view_data.h"
#include "view_holder.h"

#include <rcsc/rcg.h>
#ifndef NO_TIMER
#include <rcsc/timer.h>
#endif

#ifdef HAVE_LIBRCSC_GZ
#include <rcsc/gz/gzfstream.h>
#endif

#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <cmath>

/*-------------------------------------------------------------------*/
/*!

*/
MainData::MainData()
    : M_view_holder()
    , M_view_index( 0 )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
MainData::~MainData()
{
    //std::cerr << "delete MainData" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MainData::clear()
{
    M_view_index = 0;
    M_view_holder.clear();
    M_debug_log_holder.clear();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MainData::openRCG( const std::string & file_path )
{
#ifdef HAVE_LIBRCSC_GZ
    rcsc::gzifstream fin( file_path.c_str() );
#else
    std::ifstream fin( file_path.c_str(),
                       std::ios_base::in | std::ios_base::binary );
#endif
    if ( ! fin )
    {
        std::cerr << "failed to open rcg file. [" << file_path << "]" << std::endl;
        return false;
    }

    // clear all data
    clear();

    // create rcg reader
    rcsc::rcg::Reader reader( M_view_holder );
    // create rcg parser
    rcsc::rcg::Parser::Ptr parser = rcsc::rcg::Parser::create( fin );

    // do parse & handle data
#ifndef NO_TIMER
    rcsc::Timer timer;
#endif
    if ( ! parser
         || ! parser->parse( fin, reader ) )
    {
        std::cerr << "failed to parse [" << file_path << "]." << std::endl;
        fin.close();
        return false;
    }
#ifndef NO_TIMER
    std::cerr << "paser elapsed " << timer.elapsedReal() << "[ms]" << std::endl;
#endif

    M_view_holder.pushBackLatestViewData();
    fin.close();

    std::cerr << "loaded rcg [" << file_path
              << "] data size = "
              << M_view_holder.monitorViewCont().size()
              << std::endl;
    return true;
}


/*-------------------------------------------------------------------*/
/*!

*/
bool
MainData::saveRCG( const std::string & file_path )
{
    std::cerr << "save data to the file [" << file_path << "]"
              << std::endl;

    if ( file_path.empty() )
    {
        std::cerr << "saveRCG(). Empty file path!" << std::endl;
        return false;
    }

    boost::shared_ptr< std::ostream > fout;

#ifdef HAVE_LIBRCSC_GZ
    if ( file_path.compare( file_path.length() - 3, 3, ".gz" ) == 0 )
    {
        fout = boost::shared_ptr< std::ostream >
            ( new rcsc::gzofstream( file_path.c_str() ) );
    }
    else
#endif
    {
        fout = boost::shared_ptr< std::ostream >
            ( new std::ofstream( file_path.c_str(),
                                 std::ios_base::out | std::ios_base::binary ) );
    }

    if ( ! fout
         || fout->fail() )
    {
        std::cerr << "output stream for the new rcg file. [" << file_path
                  << "] is not good." << std::endl;
        return false;
    }

    M_view_holder.saveRCG( *fout );

    fout->flush();

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MainData::update( const int width,
                  const int height )
{
    const Options & opt = Options::instance();

    MonitorViewConstPtr view = getViewData( viewIndex() );

    // update selected number
    if ( opt.isPlayerAutoSelect() )
    {
        selectBallNearestPlayer( view );
    }

    // update focus point
    if ( view )
    {
        if ( opt.focusType() == Options::FOCUS_BALL )
        {
            Options::instance().updateFocusPoint( view->ball().x(), view->ball().y() );
        }
        else if ( opt.focusType() == Options::FOCUS_PLAYER
                  && opt.selectedNumber() != 0 )
        {
            int id = opt.selectedNumber();
            if ( id < 0 )
            {
                id = -id + 11;
            }
            id -= 1;

            const Player & p = view->players()[id];
            if ( p.isAlive() )
            {
                Options::instance().updateFocusPoint( p.x(), p.y() );
            }
        }
        else
        {
            // already set
        }
    }

    // update field scale and related things
    Options::instance().updateFieldSize( width, height );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
MainData::selectBallNearestPlayer( MonitorViewConstPtr view )
{
    if ( ! view )
    {
        return;
    }

    Options::AgentSelectType old_type = Options::instance().agentSelectType();

    rcsc::Vector2D ball_pos( view->ball().x(), view->ball().y() );
    double min_dist2 = 40000.0;

    rcsc::SideID side = rcsc::NEUTRAL;
    int unum = 0;

    const std::size_t first = ( old_type == Options::SELECT_AUTO_RIGHT
                                ? 11
                                : 0 );
    const std::size_t last = ( old_type == Options::SELECT_AUTO_LEFT
                               ? 11
                               : 22 );
    const std::vector< Player > & players = view->players();
    for ( std::size_t i = first; i < last; ++i )
    {
        if ( players[i].isAlive() )
        {
            double d2 = ball_pos.dist2( rcsc::Vector2D( players[i].x(),
                                                        players[i].y() ) );
            if ( d2 < min_dist2 )
            {
                min_dist2 = d2;
                side = players[i].side();
                unum = players[i].unum();
            }
        }
    }

    if ( unum != 0 )
    {
        Options::instance().setSelectedNumber( side, unum );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MainData::setViewDataIndex( const int index )
{
    if ( index < 0 )
    {
        return false;
    }

    std::size_t idx = static_cast< std::size_t >( index );

    if ( M_view_index == idx
         || idx >= viewHolder().monitorViewCont().size() )
    {
        return false;
    }

    M_view_index = idx;

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MainData::setViewDataIndexFirst()
{
    M_view_index = 0;

    return ( ! M_view_holder.monitorViewCont().empty() );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MainData::setViewDataIndexLast()
{
    if ( M_view_holder.monitorViewCont().empty() )
    {
        M_view_index = 0;
        return false;
    }

    M_view_index = M_view_holder.monitorViewCont().size() - 1;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MainData::setViewDataCycle( const int cycle )
{

    std::size_t view_index = M_view_holder.getIndexOf( cycle );

    if ( view_index == M_view_index )
    {
        return false;
    }

    M_view_index = view_index;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MainData::setViewDataStepBack()
{
    if ( 0 < M_view_index )
    {
        --M_view_index;
        return true;
    }
    else
    {
        if ( Options::instance().autoLoopMode() )
        {
            M_view_index = viewHolder().monitorViewCont().size() - 1;
            return true;
        }
        else
        {
            return false;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
MainData::setViewDataStepForward()
{
    if ( M_view_index < viewHolder().monitorViewCont().size() - 1 )
    {
        ++M_view_index;
        return true;
    }
    else
    {
        if ( Options::instance().autoLoopMode() )
        {
            M_view_index = 0;
            return true;
        }
        else
        {
            return false;
        }
    }
}
