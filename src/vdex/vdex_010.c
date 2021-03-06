/*

   vdexExtractor
   -----------------------------------------

   Anestis Bechtsoudis <anestis@census-labs.com>
   Copyright 2017 - 2018 by CENSUS S.A. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include "vdex_010.h"
#include "vdex_backend_010.h"
#include "../out_writer.h"
#include "../utils.h"

bool vdex_010_isMagicValid(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return (memcmp(pVdexHeader->magic, kVdexMagic, sizeof(kVdexMagic)) == 0);
}

bool vdex_010_isVersionValid(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return (memcmp(pVdexHeader->version, kVdex010, sizeof(kVdex010)) == 0);
}

bool vdex_010_isValidVdex(const u1 *cursor) {
  return vdex_010_isMagicValid(cursor) && vdex_010_isVersionValid(cursor);
}

bool vdex_010_hasDexSection(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return pVdexHeader->dexSize != 0;
}

u4 vdex_010_GetSizeOfChecksumsSection(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return sizeof(VdexChecksum) * pVdexHeader->numberOfDexFiles;
}

const u1 *vdex_010_DexBegin(const u1 *cursor) {
  return cursor + sizeof(vdexHeader_010) + vdex_010_GetSizeOfChecksumsSection(cursor);
}

u4 vdex_010_DexBeginOffset(const u1 *cursor) {
  return sizeof(vdexHeader_010) + vdex_010_GetSizeOfChecksumsSection(cursor);
}

const u1 *vdex_010_DexEnd(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return vdex_010_DexBegin(cursor) + pVdexHeader->dexSize;
}

u4 vdex_010_DexEndOffset(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return vdex_010_DexBeginOffset(cursor) + pVdexHeader->dexSize;
}

// TODO: Cache embedded Dex file offsets so that we don't have to parse from scratch when we
// want to iterate over all files.
const u1 *vdex_010_GetNextDexFileData(const u1 *cursor, u4 *offset) {
  if (*offset == 0) {
    if (vdex_010_hasDexSection(cursor)) {
      const u1 *dexBuf = vdex_010_DexBegin(cursor);
      *offset = sizeof(vdexHeader_010) + vdex_010_GetSizeOfChecksumsSection(cursor);
      LOGMSG(l_DEBUG, "Processing first Dex file at offset:0x%x", *offset);

      // Adjust offset to point at the end of current Dex file
      dexHeader *pDexHeader = (dexHeader *)(dexBuf);
      *offset += pDexHeader->fileSize;
      return dexBuf;
    } else {
      return NULL;
    }
  } else {
    dexHeader *pDexHeader = (dexHeader *)(cursor + *offset);

    // Check boundaries
    const u1 *dexBuf = cursor + *offset;
    const u1 *dexBufMax = dexBuf + pDexHeader->fileSize;
    if (dexBufMax == vdex_010_DexEnd(cursor)) {
      LOGMSG(l_DEBUG, "Processing last Dex file at offset:0x%x", *offset);
    } else if (dexBufMax <= vdex_010_DexEnd(cursor)) {
      LOGMSG(l_DEBUG, "Processing Dex file at offset:0x%x", *offset);
    } else {
      LOGMSG(l_ERROR, "Invalid cursor offset '0x%x'", *offset);
      return NULL;
    }

    // Adjust offset to point at the end of current Dex file
    *offset += pDexHeader->fileSize;
    return dexBuf;
  }
}

u4 vdex_010_GetLocationChecksum(const u1 *cursor, u4 fileIdx) {
  u4 *checksums = (u4 *)(cursor + sizeof(vdexHeader_010));
  return checksums[fileIdx];
}

void vdex_010_SetLocationChecksum(const u1 *cursor, u4 fileIdx, u4 value) {
  u4 *checksums = (u4 *)(cursor + sizeof(vdexHeader_010));
  checksums[fileIdx] = value;
}

const u1 *vdex_010_GetVerifierDepsData(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return vdex_010_DexBegin(cursor) + pVdexHeader->dexSize;
}

u4 vdex_010_GetVerifierDepsDataOffset(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return vdex_010_DexBeginOffset(cursor) + pVdexHeader->dexSize;
}

u4 vdex_010_GetVerifierDepsDataSize(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return pVdexHeader->verifierDepsSize;
}

const u1 *vdex_010_GetQuickeningInfo(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return vdex_010_GetVerifierDepsData(cursor) + pVdexHeader->verifierDepsSize;
}

u4 vdex_010_GetQuickeningInfoOffset(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return vdex_010_GetVerifierDepsDataOffset(cursor) + pVdexHeader->verifierDepsSize;
}

u4 vdex_010_GetQuickeningInfoSize(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;
  return pVdexHeader->quickeningInfoSize;
}

void vdex_010_dumpHeaderInfo(const u1 *cursor) {
  const vdexHeader_010 *pVdexHeader = (const vdexHeader_010 *)cursor;

  LOGMSG_RAW(l_DEBUG, "------ Vdex Header Info ------\n");
  LOGMSG_RAW(l_DEBUG, "magic header & version      : %.4s-%.4s\n", pVdexHeader->magic,
             pVdexHeader->version);
  LOGMSG_RAW(l_DEBUG, "number of dex files         : %" PRIx32 " (%" PRIu32 ")\n",
             pVdexHeader->numberOfDexFiles, pVdexHeader->numberOfDexFiles);
  LOGMSG_RAW(l_DEBUG, "dex size (overall)          : %" PRIx32 " (%" PRIu32 ")\n",
             pVdexHeader->dexSize, pVdexHeader->dexSize);
  LOGMSG_RAW(l_DEBUG, "verifier dependencies size  : %" PRIx32 " (%" PRIu32 ")\n",
             vdex_010_GetVerifierDepsDataSize(cursor), vdex_010_GetVerifierDepsDataSize(cursor));
  LOGMSG_RAW(l_DEBUG, "verifier dependencies offset: %" PRIx32 " (%" PRIu32 ")\n",
             vdex_010_GetVerifierDepsDataOffset(cursor), vdex_010_GetVerifierDepsDataOffset(cursor));
  LOGMSG_RAW(l_DEBUG, "quickening info size        : %" PRIx32 " (%" PRIu32 ")\n",
             vdex_010_GetQuickeningInfoSize(cursor), vdex_010_GetQuickeningInfoSize(cursor));
  LOGMSG_RAW(l_DEBUG, "quickening info offset      : %" PRIx32 " (%" PRIu32 ")\n",
             vdex_010_GetQuickeningInfoOffset(cursor), vdex_010_GetQuickeningInfoOffset(cursor));
  LOGMSG_RAW(l_DEBUG, "dex files info              :\n");

  for (u4 i = 0; i < pVdexHeader->numberOfDexFiles; ++i) {
    LOGMSG_RAW(l_DEBUG, "  [%" PRIu32 "] location checksum : %" PRIx32 " (%" PRIu32 ")\n", i,
               vdex_010_GetLocationChecksum(cursor, i), vdex_010_GetLocationChecksum(cursor, i));
  }
  LOGMSG_RAW(l_DEBUG, "---- EOF Vdex Header Info ----\n");
}

int vdex_010_process(const char *VdexFileName, const u1 *cursor, const runArgs_t *pRunArgs) {
  // Update Dex disassembler engine status
  dex_setDisassemblerStatus(pRunArgs->enableDisassembler);

  // Measure time spend to process all Dex files of a Vdex file
  struct timespec timer;
  utils_startTimer(&timer);

  // Process Vdex file
  int ret = vdex_backend_010_process(VdexFileName, cursor, pRunArgs);

  // Get elapsed time in ns
  long timeSpend = utils_endTimer(&timer);
  LOGMSG(l_DEBUG, "Took %ld ms to process Vdex file", timeSpend / 1000000);

  return ret;
}

void vdex_010_dumpDepsInfo(const u1 *vdexFileBuf) {
  vdex_backend_010_dumpDepsInfo(vdexFileBuf);
}
