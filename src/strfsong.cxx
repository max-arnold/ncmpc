/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "strfsong.hxx"
#include "charset.hxx"
#include "time_format.hxx"
#include "util/UriUtil.hxx"

#include <mpd/client.h>

#include <glib.h>

#include <algorithm>

#include <string.h>

static const char *
skip(const char * p)
{
	unsigned stack = 0;

	while (*p != '\0') {
		if (*p == '[')
			stack++;
		if (*p == '#' && p[1] != '\0') {
			/* skip escaped stuff */
			++p;
		} else if (stack) {
			if(*p == ']') stack--;
		} else {
			if(*p == '&' || *p == '|' || *p == ']') {
				break;
			}
		}
		++p;
	}

	return p;
}

#ifndef NCMPC_MINI

static char *
concat_tag_values(const char *a, const char *b)
{
	return g_strconcat(a, ", ", b, nullptr);
}

static char *
song_more_tag_values(const struct mpd_song *song, enum mpd_tag_type tag,
		     const char *first)
{
	const char *p = mpd_song_get_tag(song, tag, 1);
	if (p == nullptr)
		return nullptr;

	char *buffer = concat_tag_values(first, p);
	for (unsigned i = 2; (p = mpd_song_get_tag(song, tag, i)) != nullptr;
	     ++i) {
		char *prev = buffer;
		buffer = concat_tag_values(buffer, p);
		g_free(prev);
	}

	return buffer;
}

#endif /* !NCMPC_MINI */

static char *
song_tag_locale(const struct mpd_song *song, enum mpd_tag_type tag)
{
	const char *value = mpd_song_get_tag(song, tag, 0);
	if (value == nullptr)
		return nullptr;

#ifndef NCMPC_MINI
	char *all = song_more_tag_values(song, tag, value);
	if (all != nullptr)
		value = all;
#endif /* !NCMPC_MINI */

	char *result = utf8_to_locale(value);

#ifndef NCMPC_MINI
	g_free(all);
#endif /* !NCMPC_MINI */

	return result;
}

static size_t
_strfsong(char *s,
	  size_t max,
	  const char *format,
	  const struct mpd_song *song,
	  const char **last)
{
	bool found = false;
	/* "missed" helps handling the case of mere literal text like
	   found==true instead of found==false. */
	bool missed = false;


	if (song == nullptr) {
		s[0] = '\0';
		return 0;
	}

	const char *p;
	size_t length = 0;
	for (p = format; *p != '\0' && length < max - 1;) {
		/* OR */
		if (p[0] == '|') {
			++p;
			if(missed && !found) {
				length = 0;
				missed = false;
			} else {
				p = skip(p);
			}
			continue;
		}

		/* AND */
		if (p[0] == '&') {
			++p;
			if(missed && !found) {
				p = skip(p);
			} else {
				found = false;
				missed = false;
			}
			continue;
		}

		/* EXPRESSION START */
		if (p[0] == '[') {
			size_t n = _strfsong(s + length, max - length, p + 1,
					     song, &p);
			if (n > 0) {
				length += n;
				found = true;
			} else {
				missed = true;
			}
			continue;
		}

		/* EXPRESSION END */
		if (p[0] == ']') {
			++p;
			if(missed && !found && length) {
				length = 0;
			}
			break;
		}

		/* let the escape character escape itself */
		if (p[0] == '#' && p[1] != '\0') {
			s[length++] = *(p+1);
			p+=2;
			continue;
		}

		/* pass-through non-escaped portions of the format string */
		if (p[0] != '%') {
			s[length++] = *p;
			p++;
			continue;
		}

		/* advance past the esc character */

		/* find the extent of this format specifier (stop at \0, ' ', or esc) */
		char *temp = nullptr;
		const char *end = p + 1;
		while(*end >= 'a' && *end <= 'z') {
			end++;
		}
		size_t n = end - p + 1;

		const char *value = nullptr;
		char buffer[32];

		if(*end != '%')
			n--;
		else if (strncmp("%file%", p, n) == 0)
			temp = utf8_to_locale(mpd_song_get_uri(song));
		else if (strncmp("%artist%", p, n) == 0) {
			temp = song_tag_locale(song, MPD_TAG_ARTIST);
		} else if (strncmp("%albumartist%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_ALBUM_ARTIST);
		else if (strncmp("%composer%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_COMPOSER);
		else if (strncmp("%performer%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_PERFORMER);
		else if (strncmp("%title%", p, n) == 0) {
			temp = song_tag_locale(song, MPD_TAG_TITLE);
		} else if (strncmp("%album%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_ALBUM);
		else if (strncmp("%shortalbum%", p, n) == 0) {
			temp = song_tag_locale(song, MPD_TAG_ALBUM);
			if (temp) {
				size_t temp_length = strlen(temp);
				if (temp_length > 25) {
					char *q = std::copy_n(temp, 22, buffer);
					std::copy_n("...", 4, q);
				} else {
					std::copy_n(temp, temp_length + 1, buffer);
				}
				value = buffer;
			}
		}
		else if (strncmp("%track%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_TRACK);
		else if (strncmp("%disc%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_DISC);
		else if (strncmp("%name%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_NAME);
		else if (strncmp("%date%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_DATE);
		else if (strncmp("%genre%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_GENRE);
		else if (strncmp("%shortfile%", p, n) == 0) {
			const char *uri = mpd_song_get_uri(song);
			if (strstr(uri, "://") == nullptr)
				uri = GetUriFilename(uri);
			temp = utf8_to_locale(uri);
		} else if (strncmp("%time%", p, n) == 0) {
			unsigned duration = mpd_song_get_duration(song);

			if (duration > 0)  {
				format_duration_short(buffer, sizeof(buffer),
						      duration);
				value = buffer;
			}
		}

		if (value == nullptr)
			value = temp;

		size_t value_length;

		if (value == nullptr) {
			/* just pass-through any unknown specifiers (including esc) */
			value = p;
			value_length = n;

			missed = true;
		} else {
			value_length = strlen(value);

			found = true;
		}

		if (length + value_length >= max)
			value_length = max - length - 1;

		std::copy_n(value, value_length, s + length);
		g_free(temp);
		length += value_length;

		/* advance past the specifier */
		p += n;
	}

	if(last) *last = p;

	s[length] = '\0';
	return length;
}

size_t
strfsong(char *s, size_t max, const char *format,
	 const struct mpd_song *song)
{
	return _strfsong(s, max, format, song, nullptr);
}
