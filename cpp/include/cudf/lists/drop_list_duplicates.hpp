/*
 * Copyright (c) 2021-2022, NVIDIA CORPORATION.
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

#include <cudf/column/column.hpp>
#include <cudf/lists/lists_column_view.hpp>
#include <cudf/stream_compaction.hpp>

namespace cudf {
namespace lists {
/**
 * @addtogroup lists_drop_duplicates
 * @{
 * @file
 */

/**
 * @brief Copy the elements from the lists in `keys` and associated `values` columns according to
 * the unique elements in `keys`.
 *
 * For each list in `keys` and associated `values`, according to the parameter `keep_option`, copy
 * the unique elements from the list in `keys` and their corresponding elements in `values` to new
 * lists. Order of the output elements within each list are not guaranteed to be preserved as in the
 * input.
 *
 * Behavior is undefined if `count_elements(keys)[i] != count_elements(values)[i]` for all `i` in
 * `[0, keys.size())`.
 *
 * @throw cudf::logic_error If the child column of the input keys column contains nested type other
 *        than STRUCT.
 * @throw cudf::logic_error If `keys.size() != values.size()`.
 *
 * @param keys The input keys lists column to check for uniqueness and copy unique elements.
 * @param values The values lists column in which the elements are mapped to elements in the key
 *        column.
 * @param nulls_equal Flag to specify whether null key elements should be considered as equal.
 * @param nans_equal Flag to specify whether NaN key elements should be considered as equal
 *        (only applicable for floating point keys elements).
 * @param keep_option Flag to specify which elements will be copied from the input to the output.
 * @param mr Device resource used to allocate memory.
 *
 * @code{.pseudo}
 * keys   = { {1,   1,   2,   3},   {4},   NULL, {}, {NULL, NULL, NULL, 5,   6,   6,   6,   5} }
 * values = { {"a", "b", "c", "d"}, {"e"}, NULL, {}, {"N0", "N1", "N2", "f", "g", "h", "i", "j"} }
 *
 * [out_keys, out_values] = drop_list_duplicates(keys, values, duplicate_keep_option::KEEP_FIRST)
 * out_keys   = { {1,   2,   3},   {4},   NULL, {}, {5,   6,   NULL} }
 * out_values = { {"a", "c", "d"}, {"e"}, NULL, {}, {"f", "g", "N0"} }
 *
 * [out_keys, out_values] = drop_list_duplicates(keys, values, duplicate_keep_option::KEEP_LAST)
 * out_keys   = { {1,   2,   3},   {4},   NULL, {}, {5,   6,   NULL} }
 * out_values = { {"b", "c", "d"}, {"e"}, NULL, {}, {"j", "i", "N2"} }
 *
 * [out_keys, out_values] = drop_list_duplicates(keys, values, duplicate_keep_option::KEEP_NONE)
 * out_keys   = { {2,   3},   {4},   NULL, {}, {} }
 * out_values = { {"c", "d"}, {"e"}, NULL, {}, {} }
 * @endcode
 *
 * @return A pair of lists columns storing the results from extracting unique key elements and their
 * corresponding values elements from the input.
 */
std::pair<std::unique_ptr<column>, std::unique_ptr<column>> drop_list_duplicates(
  lists_column_view const& keys,
  lists_column_view const& values,
  duplicate_keep_option keep_option   = duplicate_keep_option::KEEP_FIRST,
  null_equality nulls_equal           = null_equality::EQUAL,
  nan_equality nans_equal             = nan_equality::UNEQUAL,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_current_device_resource());

/**
 * @brief Create a new list column by copying elements from the input lists column ignoring
 * duplicate list elements.
 *
 * Given a lists column, an output lists column is generated by copying elements from the input
 * lists column in a way such that the duplicate elements in each list are ignored, producing only
 * unique list elements.
 *
 * Order of the output elements are not guaranteed to be preserved as in the input.
 *
 * @throw cudf::logic_error If the child column of the input lists column contains nested type other
 *        than STRUCT.
 *
 * @param input The input lists column to check and copy unique elements.
 * @param nulls_equal Flag to specify whether null key elements should be considered as equal.
 * @param nans_equal Flag to specify whether NaN key elements should be considered as equal
 *        (only applicable for floating point keys column).
 * @param mr Device resource used to allocate memory.
 *
 * @code{.pseudo}
 * input  = { {1, 1, 2, 3}, {4}, NULL, {}, {NULL, NULL, NULL, 5, 6, 6, 6, 5} }
 * drop_list_duplicates(input) = { {1, 2, 3}, {4}, NULL, {}, {5, 6, NULL} }
 * @endcode
 *
 * @return A lists column storing the results from extracting unique list elements from the input.
 */
std::unique_ptr<column> drop_list_duplicates(
  lists_column_view const& input,
  null_equality nulls_equal           = null_equality::EQUAL,
  nan_equality nans_equal             = nan_equality::UNEQUAL,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_current_device_resource());

/** @} */  // end of group
}  // namespace lists
}  // namespace cudf
