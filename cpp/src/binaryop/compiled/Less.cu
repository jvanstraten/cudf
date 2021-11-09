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

#include <binaryop/compiled/binary_ops.cuh>
#include <binaryop/compiled/struct_binary_ops.cuh>

namespace cudf::binops::compiled {
template <>
void apply_binary_op<ops::Less>(mutable_column_view& out,
                                column_view const& lhs,
                                column_view const& rhs,
                                bool is_lhs_scalar,
                                bool is_rhs_scalar,
                                rmm::cuda_stream_view stream)
{
  is_struct(lhs.type()) && is_struct(rhs.type())
    ? detail::struct_lexicographic_compare(
        out, lhs, rhs, is_lhs_scalar, is_rhs_scalar, order::ASCENDING, false, stream)
    : detail::apply_unnested_binary_op<ops::Less>(
        out, lhs, rhs, is_lhs_scalar, is_rhs_scalar, stream);
}
}  // namespace cudf::binops::compiled
