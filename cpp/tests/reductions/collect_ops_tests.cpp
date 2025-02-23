/*
 * Copyright (c) 2022, NVIDIA CORPORATION.
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

#include <cudf_test/base_fixture.hpp>
#include <cudf_test/column_wrapper.hpp>
#include <cudf_test/iterator_utilities.hpp>
#include <cudf_test/type_lists.hpp>

#include <cudf/aggregation.hpp>
#include <cudf/reduction.hpp>

using namespace cudf::test::iterators;

namespace cudf::test {

template <typename T>
struct CollectTestFixedWidth : public cudf::test::BaseFixture {
};

using CollectFixedWidthTypes =
  Concat<IntegralTypesNotBool, FloatingPointTypes, ChronoTypes, FixedPointTypes>;
TYPED_TEST_SUITE(CollectTestFixedWidth, CollectFixedWidthTypes);

// ------------------------------------------------------------------------
TYPED_TEST(CollectTestFixedWidth, CollectList)
{
  using fw_wrapper = cudf::test::fixed_width_column_wrapper<TypeParam, int32_t>;

  std::vector<int> values({5, 0, -120, -111, 0, 64, 63, 99, 123, -16});
  std::vector<bool> null_mask({1, 1, 0, 1, 1, 1, 0, 1, 0, 1});

  // null_include without nulls
  fw_wrapper col(values.begin(), values.end());
  auto const ret = cudf::reduce(
    col, make_collect_list_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(col, dynamic_cast<list_scalar*>(ret.get())->view());

  // null_include with nulls
  fw_wrapper col_with_null(values.begin(), values.end(), null_mask.begin());
  auto const ret1 = cudf::reduce(
    col_with_null, make_collect_list_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(col_with_null, dynamic_cast<list_scalar*>(ret1.get())->view());

  // null_exclude with nulls
  fw_wrapper col_null_filtered{{5, 0, -111, 0, 64, 99, -16}};
  auto const ret2 =
    cudf::reduce(col_with_null,
                 make_collect_list_aggregation<reduce_aggregation>(null_policy::EXCLUDE),
                 data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(col_null_filtered, dynamic_cast<list_scalar*>(ret2.get())->view());
}

TYPED_TEST(CollectTestFixedWidth, CollectSet)
{
  using fw_wrapper = cudf::test::fixed_width_column_wrapper<TypeParam, int32_t>;

  std::vector<int> values({5, 0, 120, 0, 0, 64, 64, 99, 120, 99});
  std::vector<bool> null_mask({1, 1, 0, 1, 1, 1, 0, 1, 0, 1});

  fw_wrapper col(values.begin(), values.end());
  fw_wrapper col_with_null(values.begin(), values.end(), null_mask.begin());

  auto null_exclude = make_collect_set_aggregation<reduce_aggregation>(
    null_policy::EXCLUDE, null_equality::UNEQUAL, nan_equality::ALL_EQUAL);
  auto null_eq = make_collect_set_aggregation<reduce_aggregation>(
    null_policy::INCLUDE, null_equality::EQUAL, nan_equality::ALL_EQUAL);
  auto null_unequal = make_collect_set_aggregation<reduce_aggregation>(
    null_policy::INCLUDE, null_equality::UNEQUAL, nan_equality::ALL_EQUAL);

  // test without nulls
  auto const ret = cudf::reduce(col, null_eq, data_type{type_id::LIST});
  fw_wrapper expected{{0, 5, 64, 99, 120}};
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, dynamic_cast<list_scalar*>(ret.get())->view());

  // null exclude
  auto const ret1 = cudf::reduce(col_with_null, null_exclude, data_type{type_id::LIST});
  fw_wrapper expected1{{0, 5, 64, 99}};
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected1, dynamic_cast<list_scalar*>(ret1.get())->view());

  // null equal
  auto const ret2 = cudf::reduce(col_with_null, null_eq, data_type{type_id::LIST});
  fw_wrapper expected2{{0, 5, 64, 99, -1}, {1, 1, 1, 1, 0}};
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected2, dynamic_cast<list_scalar*>(ret2.get())->view());

  // null unequal
  auto const ret3 = cudf::reduce(col_with_null, null_unequal, data_type{type_id::LIST});
  fw_wrapper expected3{{0, 5, 64, 99, -1, -1, -1}, {1, 1, 1, 1, 0, 0, 0}};
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected3, dynamic_cast<list_scalar*>(ret3.get())->view());
}

TYPED_TEST(CollectTestFixedWidth, MergeLists)
{
  using fw_wrapper = cudf::test::fixed_width_column_wrapper<TypeParam, int32_t>;
  using lists_col  = cudf::test::lists_column_wrapper<TypeParam, int32_t>;

  // test without nulls
  auto const lists1    = lists_col{{1, 2, 3}, {}, {}, {4}, {5, 6, 7}, {8, 9}, {}};
  auto const expected1 = fw_wrapper{{1, 2, 3, 4, 5, 6, 7, 8, 9}};
  auto const ret1      = cudf::reduce(
    lists1, make_merge_lists_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected1, dynamic_cast<list_scalar*>(ret1.get())->view());

  // test with nulls
  auto const lists2    = lists_col{{
                                  lists_col{1, 2, 3},
                                  lists_col{},
                                  lists_col{{0, 4, 0, 5}, nulls_at({0, 2})},
                                  lists_col{{0, 0, 0}, all_nulls()},
                                  lists_col{6},
                                  lists_col{-1, -1},  // null_list
                                  lists_col{7, 8, 9},
                                },
                                null_at(5)};
  auto const expected2 = fw_wrapper{{1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 7, 8, 9},
                                    {1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1}};
  auto const ret2      = cudf::reduce(
    lists2, make_merge_lists_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected2, dynamic_cast<list_scalar*>(ret2.get())->view());
}

TYPED_TEST(CollectTestFixedWidth, MergeSets)
{
  using fw_wrapper = cudf::test::fixed_width_column_wrapper<TypeParam, int32_t>;
  using lists_col  = cudf::test::lists_column_wrapper<TypeParam, int32_t>;

  // test without nulls
  auto const lists1    = lists_col{{1, 2, 3}, {}, {}, {4}, {1, 3, 4}, {0, 3, 10}, {}};
  auto const expected1 = fw_wrapper{{0, 1, 2, 3, 4, 10}};
  auto const ret1      = cudf::reduce(
    lists1, make_merge_sets_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected1, dynamic_cast<list_scalar*>(ret1.get())->view());

  // test with null_equal
  auto const lists2    = lists_col{{
                                  lists_col{1, 2, 3},
                                  lists_col{},
                                  lists_col{{0, 4, 0, 5}, nulls_at({0, 2})},
                                  lists_col{{0, 0, 0}, all_nulls()},
                                  lists_col{5},
                                  lists_col{-1, -1},  // null_list
                                  lists_col{1, 3, 5},
                                },
                                null_at(5)};
  auto const expected2 = fw_wrapper{{1, 2, 3, 4, 5, 0}, {1, 1, 1, 1, 1, 0}};
  auto const ret2      = cudf::reduce(
    lists2, make_merge_sets_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected2, dynamic_cast<list_scalar*>(ret2.get())->view());

  // test with null_unequal
  auto const& lists3   = lists2;
  auto const expected3 = fw_wrapper{{1, 2, 3, 4, 5, 0, 0, 0, 0, 0}, {1, 1, 1, 1, 1, 0, 0, 0, 0, 0}};
  auto const ret3 =
    cudf::reduce(lists3,
                 make_merge_sets_aggregation<reduce_aggregation>(null_equality::UNEQUAL),
                 data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected3, dynamic_cast<list_scalar*>(ret3.get())->view());
}

struct CollectTest : public cudf::test::BaseFixture {
};

TEST_F(CollectTest, CollectSetWithNaN)
{
  using fp_wrapper = cudf::test::fixed_width_column_wrapper<float>;

  fp_wrapper col{{1.0f, 1.0f, -2.3e-5f, -2.3e-5f, 2.3e5f, 2.3e5f, -NAN, -NAN, NAN, NAN, 0.0f, 0.0f},
                 {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0}};

  // nan unequal with null equal
  fp_wrapper expected1{{-2.3e-5f, 1.0f, 2.3e5f, -NAN, -NAN, NAN, NAN, 0.0f},
                       {1, 1, 1, 1, 1, 1, 1, 0}};
  auto const ret1 =
    cudf::reduce(col, make_collect_set_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected1, dynamic_cast<list_scalar*>(ret1.get())->view());

  // nan unequal with null unequal
  fp_wrapper expected2{{-2.3e-5f, 1.0f, 2.3e5f, -NAN, -NAN, NAN, NAN, 0.0f, 0.0f},
                       {1, 1, 1, 1, 1, 1, 1, 0, 0}};
  auto const ret2 = cudf::reduce(
    col,
    make_collect_set_aggregation<reduce_aggregation>(null_policy::INCLUDE, null_equality::UNEQUAL),
    data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected2, dynamic_cast<list_scalar*>(ret2.get())->view());

  // nan equal with null equal
  fp_wrapper expected3{{-2.3e-5f, 1.0f, 2.3e5f, NAN, 0.0f}, {1, 1, 1, 1, 0}};
  auto const ret3 =
    cudf::reduce(col,
                 make_collect_set_aggregation<reduce_aggregation>(
                   null_policy::INCLUDE, null_equality::EQUAL, nan_equality::ALL_EQUAL),
                 data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected3, dynamic_cast<list_scalar*>(ret3.get())->view());

  // nan equal with null unequal
  fp_wrapper expected4{{-2.3e-5f, 1.0f, 2.3e5f, -NAN, 0.0f, 0.0f}, {1, 1, 1, 1, 0, 0}};
  auto const ret4 =
    cudf::reduce(col,
                 make_collect_set_aggregation<reduce_aggregation>(
                   null_policy::INCLUDE, null_equality::UNEQUAL, nan_equality::ALL_EQUAL),
                 data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected4, dynamic_cast<list_scalar*>(ret4.get())->view());
}

TEST_F(CollectTest, MergeSetsWithNaN)
{
  using fp_wrapper = cudf::test::fixed_width_column_wrapper<float>;
  using lists_col  = cudf::test::lists_column_wrapper<float>;

  auto const col = lists_col{
    lists_col{1.0f, -2.3e-5f, NAN},
    lists_col{},
    lists_col{{-2.3e-5f, 2.3e5f, NAN, 0.0f}, nulls_at({3})},
    lists_col{{0.0f, 0.0f}, all_nulls()},
    lists_col{-NAN},
  };

  // nan unequal with null equal
  fp_wrapper expected1{{-2.3e-5f, 1.0f, 2.3e5f, -NAN, NAN, NAN, 0.0f}, {1, 1, 1, 1, 1, 1, 0}};
  auto const ret1 =
    cudf::reduce(col, make_merge_sets_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected1, dynamic_cast<list_scalar*>(ret1.get())->view());

  // nan unequal with null unequal
  fp_wrapper expected2{{-2.3e-5f, 1.0f, 2.3e5f, -NAN, NAN, NAN, 0.0f, 0.0f, 0.0f},
                       {1, 1, 1, 1, 1, 1, 0, 0, 0}};
  auto const ret2 =
    cudf::reduce(col,
                 make_merge_sets_aggregation<reduce_aggregation>(null_equality::UNEQUAL),
                 data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected2, dynamic_cast<list_scalar*>(ret2.get())->view());

  // nan equal with null equal
  fp_wrapper expected3{{-2.3e-5f, 1.0f, 2.3e5f, -NAN, 0.0f}, {1, 1, 1, 1, 0}};
  auto const ret3 = cudf::reduce(
    col,
    make_merge_sets_aggregation<reduce_aggregation>(null_equality::EQUAL, nan_equality::ALL_EQUAL),
    data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected3, dynamic_cast<list_scalar*>(ret3.get())->view());

  // nan equal with null unequal
  fp_wrapper expected4{{-2.3e-5f, 1.0f, 2.3e5f, -NAN, 0.0f, 0.0f, 0.0f}, {1, 1, 1, 1, 0, 0, 0}};
  auto const ret4 = cudf::reduce(col,
                                 make_merge_sets_aggregation<reduce_aggregation>(
                                   null_equality::UNEQUAL, nan_equality::ALL_EQUAL),
                                 data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected4, dynamic_cast<list_scalar*>(ret4.get())->view());
}

TEST_F(CollectTest, CollectStrings)
{
  using str_col   = cudf::test::strings_column_wrapper;
  using lists_col = cudf::test::lists_column_wrapper<cudf::string_view>;

  auto const s_col =
    str_col{{"a", "a", "b", "b", "b", "c", "c", "d", "e", "e"}, {1, 1, 1, 0, 1, 1, 0, 1, 1, 1}};

  // collect_list including nulls
  auto const ret1 = cudf::reduce(
    s_col, make_collect_list_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(s_col, dynamic_cast<list_scalar*>(ret1.get())->view());

  // collect_list excluding nulls
  auto const expected2 = str_col{"a", "a", "b", "b", "c", "d", "e", "e"};
  auto const ret2 =
    cudf::reduce(s_col,
                 make_collect_list_aggregation<reduce_aggregation>(null_policy::EXCLUDE),
                 data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected2, dynamic_cast<list_scalar*>(ret2.get())->view());

  // collect_set with null_equal
  auto const expected3 = str_col{{"a", "b", "c", "d", "e", ""}, null_at(5)};
  auto const ret3      = cudf::reduce(
    s_col, make_collect_set_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected3, dynamic_cast<list_scalar*>(ret3.get())->view());

  // collect_set with null_unequal
  auto const expected4 = str_col{{"a", "b", "c", "d", "e", "", ""}, {1, 1, 1, 1, 1, 0, 0}};
  auto const ret4      = cudf::reduce(
    s_col,
    make_collect_set_aggregation<reduce_aggregation>(null_policy::INCLUDE, null_equality::UNEQUAL),
    data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected4, dynamic_cast<list_scalar*>(ret4.get())->view());

  lists_col strings{{"a"},
                    {},
                    {"a", "b"},
                    lists_col{{"b", "null", "c"}, null_at(1)},
                    lists_col{{"null", "d"}, null_at(0)},
                    lists_col{{"null"}, null_at(0)},
                    {"e"}};

  // merge_lists
  auto const expected5 = str_col{{"a", "a", "b", "b", "null", "c", "null", "d", "null", "e"},
                                 {1, 1, 1, 1, 0, 1, 0, 1, 0, 1}};
  auto const ret5      = cudf::reduce(
    strings, make_merge_lists_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected5, dynamic_cast<list_scalar*>(ret5.get())->view());

  // merge_sets with null_equal
  auto const expected6 = str_col{{"a", "b", "c", "d", "e", "null"}, {1, 1, 1, 1, 1, 0}};
  auto const ret6      = cudf::reduce(
    strings, make_merge_sets_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected6, dynamic_cast<list_scalar*>(ret6.get())->view());

  // merge_sets with null_unequal
  auto const expected7 =
    str_col{{"a", "b", "c", "d", "e", "null", "null", "null"}, {1, 1, 1, 1, 1, 0, 0, 0}};
  auto const ret7 =
    cudf::reduce(strings,
                 make_merge_sets_aggregation<reduce_aggregation>(null_equality::UNEQUAL),
                 data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected7, dynamic_cast<list_scalar*>(ret7.get())->view());
}

TEST_F(CollectTest, CollectEmptys)
{
  using int_col = cudf::test::fixed_width_column_wrapper<int32_t>;

  // test collect empty columns
  auto empty = int_col{};
  auto ret   = cudf::reduce(
    empty, make_collect_list_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(int_col{}, dynamic_cast<list_scalar*>(ret.get())->view());

  ret = cudf::reduce(
    empty, make_collect_set_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(int_col{}, dynamic_cast<list_scalar*>(ret.get())->view());

  // test collect all null columns
  auto all_nulls = int_col{{1, 2, 3, 4, 5}, {0, 0, 0, 0, 0}};
  ret            = cudf::reduce(
    all_nulls, make_collect_list_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(int_col{}, dynamic_cast<list_scalar*>(ret.get())->view());

  ret = cudf::reduce(
    all_nulls, make_collect_set_aggregation<reduce_aggregation>(), data_type{type_id::LIST});
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(int_col{}, dynamic_cast<list_scalar*>(ret.get())->view());
}

}  // namespace cudf::test
