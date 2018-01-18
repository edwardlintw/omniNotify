#    Copyright (C) 2003 AT&T Laboratories -- Research
#
#    This file is part of a set of test programs distributed with
#    the omniNotify release (under the cosnotify_tests directory).
#
#    The test programs are free software; you can redistribute and/or modify
#    them under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    The tests are distributed in the hope that they will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this file; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
#    USA.

# Module TopConf

# top-level configuration params

TRACE_LEVEL = 0 # omniORB.traceLevel
VERBOSE = 0
MAX_TESTS = 0 # max tests of each kind -- 0 means no limit

RUN_SUPCON = 1
SUPCON_MAX_WAIT = 20  # wait no more than this many seconds
# if RUN_SUPCON is set to 1, these control sub-cases
SUPCON_DO_NORMAL   = 1
SUPCON_DO_SPECIAL  = 1
SUPCON_DO_INCOMPAT = 1
# at least one of these must be == 1
SUPCON_PUSH_CLIENTS = 1
SUPCON_PULL_CLIENTS = 1

RUN_IFILT = 0
IFILT_MAX_WAIT = 20  # wait no more than this many seconds
# at least one of these must be == 1
IFILT_PUSH_CLIENTS = 1
IFILT_PULL_CLIENTS = 1

RUN_STANDALONE = 0
