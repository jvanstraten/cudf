/*
 * Copyright (c) 2021, NVIDIA CORPORATION.
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

#include <cudf/column/column_device_view.cuh>
#include <cudf/column/column_view.hpp>
#include <cudf/detail/iterator.cuh>

#include <rmm/cuda_stream_view.hpp>
#include <rmm/exec_policy.hpp>

namespace cudf::binops::compiled::detail {
template <typename Comparator>
void struct_compare(mutable_column_view& out,
                    Comparator compare,
                    bool is_lhs_scalar,
                    bool is_rhs_scalar,
                    bool flip_output,
                    rmm::cuda_stream_view stream)
{
  auto d_out = column_device_view::create(out, stream);
  auto optional_iter =
    cudf::detail::make_optional_iterator<bool>(*d_out, contains_nulls::DYNAMIC{}, out.has_nulls());
  thrust::tabulate(
    rmm::exec_policy(stream),
    out.begin<bool>(),
    out.end<bool>(),
    [optional_iter, is_lhs_scalar, is_rhs_scalar, flip_output, compare] __device__(size_type i) {
      auto lhs = is_lhs_scalar ? 0 : i;
      auto rhs = is_rhs_scalar ? 0 : i;
      return optional_iter[i].has_value() and
             (flip_output ? not compare(lhs, rhs) : compare(lhs, rhs));
    });
}

void struct_lexicographic_compare(mutable_column_view& out,
                                  column_view const& lhs,
                                  column_view const& rhs,
                                  bool is_lhs_scalar,
                                  bool is_rhs_scalar,
                                  order op_order,
                                  bool flip_output,
                                  rmm::cuda_stream_view stream);
}  //  namespace cudf::binops::compiled::detail