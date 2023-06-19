/* enso.h -- defs specific to our enso exploit implementation
 *
 * Copyright (C) 2017 molecule, 2018-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#define SECOND_PAYLOAD_OFFSET 5
#define SECOND_PAYLOAD_SIZE 0x1000

#define ENSO_CORRUPTED_AREA_START 0x511673A0
#define ENSO_CORRUPTED_AREA_SIZE 0x600 // 3 * SECTOR_SIZE
#define ENSO_SP_AREA_OFFSET 0x51030100
#define ENSO_SP_TOP_CORE0 0x220
#define ENSO_SP_TOP_OLD_CORE0 0x11d

#define ENSO_EMUMBR_OFFSET 1 // enso's emumbr