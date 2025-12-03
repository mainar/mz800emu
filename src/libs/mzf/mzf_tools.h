/* 
 * File:   mzf_tools.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 28. prosince 2017, 16:13
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


#ifndef MZF_TOOLS_H
#define MZF_TOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mzf.h"

    extern void mzf_tools_set_fname ( st_MZF_HEADER *mzfhdr, char *ascii_filename );
    uint8_t mzf_tools_get_fname_length ( st_MZF_HEADER *mzfhdr );
    void mzf_tools_get_fname ( st_MZF_HEADER *mzfhdr, char *ascii_filename );
    extern st_MZF_HEADER* mzf_tools_create_mzfhdr ( uint8_t ftype, uint16_t fsize, uint16_t fstrt, uint16_t fexec, uint8_t *fname, int fname_length, uint8_t *cmnt );

#ifdef __cplusplus
}
#endif

#endif /* MZF_TOOLS_H */

