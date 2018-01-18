// -*- Mode: C++; -*-
//                              File      : getopt_impl.h
//                              Package   : omniNotify
//                              Created on: 2003/04/22
//                              Authors   : Eoin Carroll
//
//    Copyright (C) 1997 AT&T Laboratories Cambridge
//
//    This file is part of executable program omniNotify-daemon
//    and is distributed with the omniNotify release.
//
//    omniNotify-daemon is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//    USA.
//
// Description:
//    Implementation of getopt() for platforms without it. Lifted from catior.
//

#ifndef _GETOPT_IMPL_H_
#define _GETOPT_IMPL_H_

#ifndef HAVE_GETOPT

// WIN32 doesn't have an implementation of getopt() -
// supply a getopt() for this program:

char* optarg;
int optind = 1;

int
getopt(int num_args, char* const* args, const char* optstring)
{
  if (optind == num_args) return EOF;
  char* buf_left = *(args+optind);

  if ((*buf_left != '-' && *buf_left != '/') || buf_left == NULL ) return EOF;
  else if ((optind < (num_args-1)) && strcmp(buf_left,"-") == 0 &&
	   strcmp(*(args+optind+1),"-") == 0)
    {
      optind+=2;
      return EOF;
    }
  else if (strcmp(buf_left,"-") == 0)
    {
      optind++;
      return '?';
    }

  for(int count = 0; count < strlen(optstring); count++)
    {
      if (optstring[count] == ':') continue;
      if (buf_left[1] == optstring[count])
	{
	  if(optstring[count+1] == ':')
	    {
	      if (strlen(buf_left) > 2)
		{
		  optarg = (buf_left+2);
		  optind++;
		}
	      else if (optind < (num_args-1))
		{
		  optarg = *(args+optind+1);
		  optind+=2;
		}
	      else
		{
		  optind++;
		  return '?';
		}
	    }
	  else optind++;

	  return buf_left[1];
	}
    }
  optind++;
  return '?';
}

#endif  // !HAVE_GETOPT

#endif // _GETOPT_IMPL_H_
