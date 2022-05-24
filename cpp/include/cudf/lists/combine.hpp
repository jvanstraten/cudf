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

namespace cudf {

//! Lists column APIs
namespace lists {
/**
 * @addtogroup lists_combine
 * @{
 * @file
 */

/**
 * @brief Flag to specify whether a null list element will be ignored from concatenation, or the
 * entire concatenation result involving null list elements will be a null element.
 */
enum class concatenate_null_policy { IGNORE, NULLIFY_OUTPUT_ROW };

/**
 * @brief Row-wise concatenating multiple lists columns into a single lists column.
 *
 * The output column is generated by concatenating the elements within each row of the input
 * table. If any row of the input table contains null elements, the concatenation process will
 * either ignore those null elements, or will simply set the entire resulting row to be a null
 * element.
 *
 * @code{.pseudo}
 * s1 = [{0, 1}, {2, 3, 4}, {5}, {}, {6, 7}]
 * s2 = [{8}, {9}, {}, {10, 11, 12}, {13, 14, 15, 16}]
 * r = lists::concatenate_rows(s1, s2)
 * r is now [{0, 1, 8}, {2, 3, 4, 9}, {5}, {10, 11, 12}, {6, 7, 13, 14, 15, 16}]
 * @endcode
 *
 * @throws cudf::logic_error if any column of the input table is not a lists column.
 * @throws cudf::logic_error if all lists columns do not have the same type.
 *
 * @param input Table of lists to be concatenated.
 * @param null_policy The parameter to specify whether a null list element will be ignored from
 *        concatenation, or any concatenation involving a null element will result in a null list.
 * @param mr Device memory resource used to allocate the returned column's device memory.
 * @return A new column in which each row is a list resulted from concatenating all list elements in
 *         the corresponding row of the input table.
 */
std::unique_ptr<column> concatenate_rows(
  table_view const& input,
  concatenate_null_policy null_policy = concatenate_null_policy::IGNORE,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_current_device_resource());

/**
 * @brief Concatenating multiple lists on the same row of a lists column into a single list.
 *
 * Given a lists column where each row in the column is a list of lists of entries, an output lists
 * column is generated by concatenating all the list elements at the same row together. If any row
 * contains null list elements, the concatenation process will either ignore those null elements, or
 * will simply set the entire resulting row to be a null element.
 *
 * @code{.pseudo}
 * l = [ [{1, 2}, {3, 4}, {5}], [{6}, {}, {7, 8, 9}] ]
 * r = lists::concatenate_list_elements(l);
 * r is [ {1, 2, 3, 4, 5}, {6, 7, 8, 9} ]
 * @endcode
 *
 * @throws cudf::logic_error if the input column is not at least two-level depth lists column (i.e.,
 *         each row must be a list of list).
 * @throws cudf::logic_error if the input lists column contains nested typed entries that are not
 *         lists.
 *
 * @param input The lists column containing lists of list elements to concatenate.
 * @param null_policy The parameter to specify whether a null list element will be ignored from
 *        concatenation, or any concatenation involving a null element will result in a null list.
 * @param mr Device memory resource used to allocate the returned column's device memory.
 * @return A new column in which each row is a list resulted from concatenating all list elements in
 *         the corresponding row of the input lists column.
 */
std::unique_ptr<column> concatenate_list_elements(
  column_view const& input,
  concatenate_null_policy null_policy = concatenate_null_policy::IGNORE,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_current_device_resource());

/** @} */  // end of group
}  // namespace lists
}  // namespace cudf
