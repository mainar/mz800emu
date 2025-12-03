/* 
 * File:   iface_sdl_log.h
 * Author: chaky
 *
 * Created on 12. června 2015, 10:44
 * 
 * 
 * ----------------------------- License -------------------------------------
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * ---------------------------------------------------------------------------
 */

#ifndef IFACE_SDL_LOG_H
#define	IFACE_SDL_LOG_H

#ifdef	__cplusplus
extern "C" {
#endif

#define LOG_CATEGORY_APPLICATION    SDL_LOG_CATEGORY_APPLICATION
#define LOG_CATEGORY_ERROR          SDL_LOG_CATEGORY_ERROR
#define LOG_CATEGORY_SYSTEM         SDL_LOG_CATEGORY_SYSTEM
    // SDL_LOG_CATEGORY_AUDIO
    // SDL_LOG_CATEGORY_VIDEO
    // SDL_LOG_CATEGORY_RENDER

    enum {
        LOG_CATEGORY_AWESOME1 = SDL_LOG_CATEGORY_CUSTOM,
        LOG_CATEGORY_AWESOME2,
        LOG_CATEGORY_AWESOME3,
    };

#define LOG_PRIORITY_VERBOSE        SDL_LOG_PRIORITY_VERBOSE
#define LOG_PRIORITY_DEBUG          SDL_LOG_PRIORITY_DEBUG
#define LOG_PRIORITY_INFO           SDL_LOG_PRIORITY_INFO
#define LOG_PRIORITY_WARN           SDL_LOG_PRIORITY_WARN
#define LOG_PRIORITY_ERROR          SDL_LOG_PRIORITY_ERROR
#define LOG_PRIORITY_CRITICAL       SDL_LOG_PRIORITY_CRITICAL
    // SDL_NUM_LOG_PRIORITIES 


#define IFACE_DBG(msg) SDL_Log ( msg )


#ifdef	__cplusplus
}
#endif

#endif	/* IFACE_SDL_LOG_H */

