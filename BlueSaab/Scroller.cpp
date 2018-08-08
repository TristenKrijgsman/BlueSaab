/* C++ class for scrolling retrieved track metadata on SAAB SID (SAAB Information Display)
 * Copyright (C) 2018 Girts Linde
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Scroller.h"
#include "utf_convert.h"

Scroller scroller;

Scroller::Scroller(): info_lock(1) {
	position = 0;
	artist[0] = 0;
	artist[ARTIST_BUF_SIZE - 1] = 0;
	title[0] = 0;
	title[TITLE_BUF_SIZE - 1] = 0;
}

void Scroller::clear() {
	info_lock.wait();
	artist[0] = 0;
	title[0] = 0;
	position = 0;
	info_lock.release();
}

void Scroller::set_info(const char *artist_,const char *title_) {
	info_lock.wait();
	utf_convert(artist_, artist, sizeof(artist));
	utf_convert(title_, title, sizeof(title));
	position = 0;
	info_lock.release();
}

const char* Scroller::get() {
	buffer.clear();

	info_lock.wait();

	buffer.cut(position);
	bool got_all_artist = buffer.add(artist);
	if (artist[0] != 0 && title[0] != 0) {
		buffer.add(" - ");
	}
	bool got_all_title = buffer.add(title);
	bool got_all = ((title[0] != 0 && got_all_title)
			|| (title[0] == 0 && got_all_artist));
	if (got_all) {
		position = 0;
	} else {
		position++;
	}

	info_lock.release();

	return buffer.buffer;
}
