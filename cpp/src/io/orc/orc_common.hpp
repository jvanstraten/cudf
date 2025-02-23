/*
 * Copyright (c) 2019-2022, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>

namespace cudf {
namespace io {
namespace orc {

static constexpr uint32_t block_header_size = 3;

constexpr uint32_t compressed_block_size(uint32_t compressed_data_size)
{
  return ((compressed_data_size + block_header_size + 0xFF) & ~0xFF);
}

static constexpr uint32_t padded_block_header_size = compressed_block_size(0);

enum CompressionKind : uint8_t {
  NONE   = 0,
  ZLIB   = 1,
  SNAPPY = 2,
  LZO    = 3,
  LZ4    = 4,
  ZSTD   = 5,
};

enum TypeKind : int8_t {
  INVALID_TYPE_KIND = -1,
  BOOLEAN           = 0,
  BYTE              = 1,
  SHORT             = 2,
  INT               = 3,
  LONG              = 4,
  FLOAT             = 5,
  DOUBLE            = 6,
  STRING            = 7,
  BINARY            = 8,
  TIMESTAMP         = 9,
  LIST              = 10,
  MAP               = 11,
  STRUCT            = 12,
  UNION             = 13,
  DECIMAL           = 14,
  DATE              = 15,
  VARCHAR           = 16,
  CHAR              = 17,
};

enum StreamKind : int8_t {
  INVALID_STREAM_KIND = -1,
  PRESENT             = 0,  // boolean stream of whether the next value is non-null
  DATA                = 1,  // the primary data stream
  LENGTH              = 2,  // the length of each value for variable length data
  DICTIONARY_DATA     = 3,  // the dictionary blob
  DICTIONARY_COUNT    = 4,  // deprecated prior to Hive 0.11
  SECONDARY           = 5,  // a secondary data stream
  ROW_INDEX           = 6,  // the index for seeking to particular row groups
  BLOOM_FILTER        = 7,  // original bloom filters used before ORC-101
  BLOOM_FILTER_UTF8   = 8,  // bloom filters that consistently use utf8
};

enum ColumnEncodingKind : int8_t {
  INVALID_ENCODING_KIND = -1,
  DIRECT                = 0,  // the encoding is mapped directly to the stream using RLE v1
  DICTIONARY            = 1,  // the encoding uses a dictionary of unique values using RLE v1
  DIRECT_V2             = 2,  // the encoding is direct using RLE v2
  DICTIONARY_V2         = 3,  // the encoding is dictionary-based using RLE v2
};

enum ProtofType : uint8_t {
  VARINT      = 0,
  FIXED64     = 1,
  FIXEDLEN    = 2,
  START_GROUP = 3,  // deprecated
  END_GROUP   = 4,  // deprecated
  FIXED32     = 5,
  INVALID_6   = 6,
  INVALID_7   = 7,
};

}  // namespace orc
}  // namespace io
}  // namespace cudf
