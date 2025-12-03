/* 
 * File:   cmtspeed.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 23. května 2018, 12:51
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

#include "cmtspeed.h"

// nasobitel rychlosti podle en_CMTSPEED
const double g_cmtspeed_divisor[] = {
                                     0,
                                     ( (double) 1 / 1 ),
                                     ( (double) 2 / 1 ),
                                     ( (double) 2 / 1 ), // cpm
                                     ( (double) 3 / 1 ),
                                     ( (double) 3 / 2 ),
                                     ( (double) 7 / 3 ),
                                     ( (double) 8 / 3 ),
                                     ( (double) 9 / 7 ),
                                     ( (double) 25 / 14 ),
};

// nazev pomeru rychlosti podle en_CMTSPEED
const char *g_cmtspeed_ratio[] = {
                                  "?:?",
                                  "1:1",
                                  "2:1",
                                  "2:1 (cp/m)",
                                  "3:1",
                                  "3:2",
                                  "7:3",
                                  "8:3",
                                  "9:7",
                                  "25:14",
};

