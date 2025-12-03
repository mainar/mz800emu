/* 
 * File:   ui_version_check.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 3. září 2018, 9:34
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


#ifndef UI_VERSION_CHECK_H
#define UI_VERSION_CHECK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "version_check/version_check.h"

    extern void ui_version_check_show_setup_window ( void );
    extern void ui_version_check_show_report_window ( st_VERSION_BRANCH *branch );


#ifdef __cplusplus
}
#endif

#endif /* UI_VERSION_CHECK_H */

