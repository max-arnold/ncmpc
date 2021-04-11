/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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

#include "LyricsLoader.hxx"
#include "config.h"

#include <assert.h>

LyricsLoader::LyricsLoader() noexcept
	:plugins(plugin_list_load_directory(LYRICS_PLUGIN_DIR))
{
}

PluginCycle *
LyricsLoader::Load(EventLoop &event_loop,
		   const char *artist, const char *title,
		   PluginResponseHandler &handler) noexcept
{
	assert(artist != nullptr);
	assert(title != nullptr);

	const char *args[3] = { artist, title, nullptr };

	return plugin_run(event_loop, plugins, args, handler);
}