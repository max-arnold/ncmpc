#!/usr/bin/python3
#
#  Copyright 2004-2021 The Music Player Daemon Project
#  http://www.musicpd.org/
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

#
# Load lyrics from azlyrics.com if lyrics weren't found in the lyrics directory
#
import re
import sys
import urllib.request

import requests

artist = sys.argv[1].lower()
title = sys.argv[2].lower()
artist = re.sub("[^A-Za-z0-9]+", "", artist)
title = re.sub("[^A-Za-z0-9]+", "", title)

url = "http://azlyrics.com/lyrics/" + artist + "/" + title + ".html"

try:
    r = urllib.request.urlopen(url)
    response = r.read().decode()
    start = response.find("that. -->")
    end = response.find("<!-- MxM")
    lyrics = response[start + 9 : end]
    lyrics = (
        lyrics.replace("<br>", "").replace("<div>", "").replace("</div>", "").strip()
    )
    print(lyrics)
except urllib.error.HTTPError:
    print("Lyrics not found :(", file=sys.stderr)
    exit(1)
except Exception as e:
    print("Unknown error: ", e, file=sys.stderr)
    exit(2)